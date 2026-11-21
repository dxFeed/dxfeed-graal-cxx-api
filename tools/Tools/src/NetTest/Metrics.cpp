// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// The socket headers come first: winsock2.h must precede windows.h, which the headers of the API may include.
#ifdef _WIN32
#    ifndef WIN32_LEAN_AND_MEAN
#        define WIN32_LEAN_AND_MEAN
#    endif
#    ifndef NOMINMAX
#        define NOMINMAX
#    endif
#    include <winsock2.h>
#    include <ws2tcpip.h>
#else
#    include <arpa/inet.h>
#    include <netdb.h>
#    include <netinet/in.h>
#    include <poll.h>
#    include <sys/socket.h>
#    include <sys/time.h>
#    include <unistd.h>
#endif

#include "Metrics.hpp"

#include "Args.hpp"
#include "MetricsRequest.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <exception>
#include <iterator>
#include <thread>
#include <tuple>
#include <utility>

namespace dxfcpp::tools::nettest {

namespace {

#ifdef _WIN32
using NativeSocket = SOCKET;
const NativeSocket NO_SOCKET = INVALID_SOCKET;

void closeSocket(NativeSocket socket) {
    closesocket(socket);
}

int sendBytes(NativeSocket socket, const char *data, std::size_t size) {
    return send(socket, data, static_cast<int>(std::min<std::size_t>(size, std::size_t{1} << 30)), 0);
}

int receiveBytes(NativeSocket socket, char *data, std::size_t size) {
    return recv(socket, data, static_cast<int>(size), 0);
}

int toAddressLength(std::size_t length) {
    return static_cast<int>(length);
}

// Waits until the socket is readable, at most the milliseconds; true when it is.
bool waitReadable(NativeSocket socket, int millis) {
    // The set is filled without FD_SET, whose do-while(0) is a warning of MSVC.
    fd_set set{};

    set.fd_count = 1;
    set.fd_array[0] = socket;

    timeval timeout{};

    timeout.tv_sec = millis / 1000;
    timeout.tv_usec = (millis % 1000) * 1000;

    return select(0, &set, nullptr, nullptr, &timeout) > 0;
}

void setSendTimeout(NativeSocket socket, int millis) {
    const auto timeout = static_cast<DWORD>(millis);

    setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char *>(&timeout), sizeof(timeout));
}
#else
using NativeSocket = int;
const NativeSocket NO_SOCKET = -1;

void closeSocket(NativeSocket socket) {
    close(socket);
}

int sendBytes(NativeSocket socket, const char *data, std::size_t size) {
#    ifdef MSG_NOSIGNAL
    return static_cast<int>(send(socket, data, size, MSG_NOSIGNAL));
#    else
    return static_cast<int>(send(socket, data, size, 0));
#    endif
}

int receiveBytes(NativeSocket socket, char *data, std::size_t size) {
    return static_cast<int>(recv(socket, data, size, 0));
}

socklen_t toAddressLength(socklen_t length) {
    return length;
}

// Waits until the socket is readable (or closed), at most the milliseconds; true when it is.
bool waitReadable(NativeSocket socket, int millis) {
    pollfd descriptor{};

    descriptor.fd = socket;
    descriptor.events = POLLIN;

    return poll(&descriptor, 1, millis) > 0;
}

void setSendTimeout(NativeSocket socket, int millis) {
    timeval timeout{};

    timeout.tv_sec = millis / 1000;
    timeout.tv_usec = (millis % 1000) * 1000;
    setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
}
#endif

NativeSocket nativeOf(std::intptr_t socket) {
    return socket == -1 ? NO_SOCKET : static_cast<NativeSocket>(socket);
}

// How often the accept loop and the request read check the stop.
constexpr int POLL_MILLIS = 100;

// The port of a bound address and whether the address is a loopback one: 127.0.0.0/8 or ::1.
std::pair<std::int32_t, bool> portAndLoopback(const sockaddr_storage &address) {
    if (address.ss_family == AF_INET) {
        sockaddr_in ipv4{};

        std::memcpy(&ipv4, &address, sizeof(ipv4));

        return {static_cast<std::int32_t>(ntohs(ipv4.sin_port)), (ntohl(ipv4.sin_addr.s_addr) >> 24) == 127};
    }

    if (address.ss_family == AF_INET6) {
        sockaddr_in6 ipv6{};

        std::memcpy(&ipv6, &address, sizeof(ipv6));

        // ::1: fifteen zero bytes and 1.
        const auto *bytes = ipv6.sin6_addr.s6_addr;
        const bool loopback = bytes[15] == 1 && std::all_of(bytes, bytes + 15, [](unsigned char value) {
                                  return value == 0;
                              });

        return {static_cast<std::int32_t>(ntohs(ipv6.sin6_port)), loopback};
    }

    return {0, false};
}

std::string response(const std::string &status, const std::string &contentType, const std::string &body) {
    return "HTTP/1.0 " + status + "\r\nContent-Type: " + contentType +
           "\r\nContent-Length: " + std::to_string(body.size()) + "\r\nConnection: close\r\n\r\n" + body;
}

// Reads the request head within one deadline for the whole head, so a slow client holds neither the server nor the
// stop for longer than that, and answers it. A head that is too long, malformed or not complete before the client
// closes gets no response.
void answer(NativeSocket client, const std::function<std::string()> &body, const std::atomic<bool> &stopping) {
    std::string request;
    char buffer[1024];
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(MetricsServer::REQUEST_TIMEOUT_MILLIS);
    auto head = parseRequestHead(request);

    while (head.status == RequestHeadStatus::INCOMPLETE) {
        if (stopping.load() || std::chrono::steady_clock::now() >= deadline) {
            return;
        }

        if (!waitReadable(client, POLL_MILLIS)) {
            continue;
        }

        // INCOMPLETE means fewer than MAX_REQUEST_HEAD bytes, so there is room.
        const int received = receiveBytes(client, buffer, std::min(sizeof(buffer), MAX_REQUEST_HEAD - request.size()));

        if (received <= 0) {
            return;
        }

        request.append(buffer, static_cast<std::size_t>(received));
        head = parseRequestHead(request);
    }

    if (head.status != RequestHeadStatus::COMPLETE) {
        return;
    }

    std::string text;

    if (head.method != "GET" && head.method != "HEAD") {
        text = response("405 Method Not Allowed", "text/plain; charset=utf-8", "GET only\n");
    } else if (head.path != "/metrics") {
        text = response("404 Not Found", "text/plain; charset=utf-8", "the metrics are at /metrics\n");
    } else {
        text = response("200 OK", METRICS_CONTENT_TYPE, body());

        if (head.method == "HEAD") {
            text.erase(text.find("\r\n\r\n") + 4);
        }
    }

    for (std::size_t sent = 0; sent < text.size();) {
        const int written = sendBytes(client, text.data() + sent, text.size() - sent);

        if (written <= 0) {
            break;
        }

        sent += static_cast<std::size_t>(written);
    }
}

// The text of the bound of a bucket of nettest_e2e_lag_seconds, as the kit's sample writes it.
const char *const LAG_BUCKET_TEXTS[] = {"5e-05", "0.0001", "0.00025", "0.0005", "0.001", "0.0025", "0.005",
                                        "0.01",  "0.025",  "0.05",    "0.1",    "0.25",   "1",     "+Inf"};

static_assert(std::size(LAG_BUCKET_TEXTS) == LAG_BUCKET_BOUNDS_NANOS.size() + 1);

class MetricsText {
    std::string text_{};
    std::string labels_;

    public:
    explicit MetricsText(std::string labels) : labels_(std::move(labels)) {
    }

    void head(const std::string &metric, const std::string &type, const std::string &help) {
        text_ += "# HELP " + metric + " " + help + "\n# TYPE " + metric + " " + type + "\n";
    }

    // A series with the common labels and the extra ones (`,key="value"`).
    void series(const std::string &metric, const std::string &value, const std::string &extra = {}) {
        text_ += metric + "{" + labels_ + extra + "} " + value + "\n";
    }

    // A metric of one series; an absent value (negative) leaves the series out and keeps the HELP and TYPE lines.
    void single(const std::string &metric, const std::string &type, const std::string &help, std::int64_t value,
                bool seconds = false) {
        head(metric, type, help);

        if (value >= 0) {
            series(metric, seconds ? formatSeconds(value) : std::to_string(value));
        }
    }

    const std::string &text() const {
        return text_;
    }
};

} // namespace

LiveCounters::LiveCounters(std::int32_t instanceCount)
    : instances(static_cast<std::size_t>(instanceCount)), connections(static_cast<std::size_t>(instanceCount)) {
    for (auto &value : connections) {
        value.store(0, std::memory_order_relaxed);
    }
}

std::int64_t LiveCounters::records() const {
    std::int64_t total = 0;

    for (const auto &instance : instances) {
        total += instance.records.load(std::memory_order_relaxed);
    }

    return total;
}

std::int64_t LiveCounters::openConnections() const {
    std::int64_t total = 0;

    for (const auto &value : connections) {
        total += value.load(std::memory_order_relaxed);
    }

    return total;
}

void LiveCounters::addLags(const LagBuckets &buckets, std::int64_t count, std::int64_t positiveSum,
                           std::int64_t negative) {
    for (std::size_t index = 0; index < buckets.size(); ++index) {
        if (buckets[index] != 0) {
            lagBuckets[index].fetch_add(buckets[index], std::memory_order_relaxed);
        }
    }

    lagSumNanos.fetch_add(positiveSum, std::memory_order_relaxed);
    lagNegative.fetch_add(negative, std::memory_order_relaxed);
    // The count last: a scrape that sees it sees the buckets of the same records or more.
    lagCount.fetch_add(count, std::memory_order_release);
}

void LiveCounters::setRuntime(const RuntimeSample &sample) {
    const auto both = [](std::int64_t first, std::int64_t second) {
        return first >= 0 && second >= 0 ? first + second : RUNTIME_VALUE_ABSENT;
    };

    heapUsedBytes.store(sample.heapUsedBytes, std::memory_order_relaxed);
    heapCommittedBytes.store(sample.heapCommittedBytes, std::memory_order_relaxed);
    gcCount.store(both(sample.youngGcCount, sample.fullGcCount), std::memory_order_relaxed);
    gcTimeMs.store(both(sample.youngGcTimeMs, sample.fullGcTimeMs), std::memory_order_relaxed);
    cpuNanos.store(sample.processCpuTimeNs, std::memory_order_relaxed);
}

void LiveCounters::setConnectors(const ConnectorTotals &totals) {
    connectorReadBytes.store(totals.readBytes, std::memory_order_relaxed);
    connectorWrittenBytes.store(totals.writtenBytes, std::memory_order_relaxed);
    connectorReadRecords.store(totals.readRecords, std::memory_order_relaxed);
    connectorWrittenRecords.store(totals.writtenRecords, std::memory_order_relaxed);
    connectorSubRecords.store(totals.subRecords, std::memory_order_relaxed);
    connectorReadLagMicros.store(totals.readLagMicros, std::memory_order_relaxed);
    connectorRttMicros.store(totals.rttMicros, std::memory_order_relaxed);
}

std::size_t lagBucketIndex(std::int64_t lagNanos) {
    const auto bound = std::lower_bound(LAG_BUCKET_BOUNDS_NANOS.begin(), LAG_BUCKET_BOUNDS_NANOS.end(), lagNanos);

    return static_cast<std::size_t>(bound - LAG_BUCKET_BOUNDS_NANOS.begin());
}

std::string formatSeconds(std::int64_t nanos) {
    const bool negative = nanos < 0;
    const auto magnitude = negative ? 0 - static_cast<std::uint64_t>(nanos) : static_cast<std::uint64_t>(nanos);
    auto fraction = std::to_string(magnitude % 1'000'000'000);

    fraction.insert(0, 9 - fraction.size(), '0');

    while (!fraction.empty() && fraction.back() == '0') {
        fraction.pop_back();
    }

    return (negative ? "-" : "") + std::to_string(magnitude / 1'000'000'000) +
           (fraction.empty() ? std::string() : "." + fraction);
}

std::string labelValue(const std::string &value) {
    std::string escaped;

    for (const char symbol : value) {
        if (symbol == '\\' || symbol == '"') {
            escaped += '\\';
            escaped += symbol;
        } else if (symbol == '\n') {
            escaped += "\\n";
        } else {
            escaped += symbol;
        }
    }

    return escaped;
}

std::string formatMetrics(const MetricsInfo &info, const LiveCounters &counters) {
    const bool producer = info.side == "producer";
    MetricsText text("tool=\"" + std::string(IMPLEMENTATION) + "\",side=\"" + labelValue(info.side) + "\",name=\"" +
                     labelValue(info.name) + "\"");
    const auto load = [](const std::atomic<std::int64_t> &value) {
        return value.load(std::memory_order_relaxed);
    };

    text.head("nettest_info", "gauge", "The configuration of the NetTest run.");
    text.series("nettest_info", "1",
                ",spec_version=\"" + std::string(SPEC_VERSION) + "\",version=\"" + labelValue(info.version) +
                    "\",sdk_version=\"" + labelValue(info.sdkVersion) + "\",role=\"" + labelValue(info.role) +
                    "\",event=\"" + labelValue(info.event) + "\",collector=\"" + labelValue(info.collector) + "\"");

    text.head("nettest_records_total", "counter", producer ? "The published events." : "The received events.");

    for (std::size_t index = 0; index < counters.instances.size(); ++index) {
        text.series("nettest_records_total", std::to_string(load(counters.instances[index].records)),
                    ",connection=\"" + std::to_string(index + 1) + "\"");
    }

    if (producer) {
        text.single("nettest_publish_seconds_total", "counter",
                    "The time of the batch fills and the publishing calls.", counters.publish.fillAndPublishNanos(),
                    true);
        text.single("nettest_publish_calls_total", "counter", "The publishing calls.", load(counters.publish.batches));
        text.single("nettest_schedule_slip_seconds", "gauge", "The schedule slip of the last batch.",
                    info.scheduled ? load(counters.publish.slipLastNanos) : RUNTIME_VALUE_ABSENT, true);
    } else {
        // The count first: the buckets read after it cover at least its records.
        const auto count = counters.lagCount.load(std::memory_order_acquire);
        const auto sum = load(counters.lagSumNanos);
        std::int64_t cumulative = 0;

        text.head("nettest_e2e_lag_seconds", "histogram", "The receive time minus the time of the event.");

        for (std::size_t index = 0; index < counters.lagBuckets.size(); ++index) {
            cumulative += load(counters.lagBuckets[index]);

            // +Inf is the count, so that the buckets never exceed it.
            const auto value = index + 1 == counters.lagBuckets.size() ? count : std::min(cumulative, count);

            text.series("nettest_e2e_lag_seconds_bucket", std::to_string(value),
                        std::string(",le=\"") + LAG_BUCKET_TEXTS[index] + "\"");
        }

        text.series("nettest_e2e_lag_seconds_sum", formatSeconds(sum));
        text.series("nettest_e2e_lag_seconds_count", std::to_string(count));
        text.single("nettest_e2e_lag_negative_total", "counter", "The events received with a negative lag.",
                    load(counters.lagNegative));
    }

    text.head("nettest_connections", "gauge", "The open connections.");

    for (std::size_t index = 0; index < counters.connections.size(); ++index) {
        text.series("nettest_connections", std::to_string(load(counters.connections[index])),
                    ",connection=\"" + std::to_string(index + 1) + "\"");
    }

    text.single("nettest_connector_read_bytes_total", "counter", "The bytes read by the connectors.",
                load(counters.connectorReadBytes));
    text.single("nettest_connector_written_bytes_total", "counter", "The bytes written by the connectors.",
                load(counters.connectorWrittenBytes));
    text.single("nettest_connector_read_records_total", "counter", "The data records read by the connectors.",
                load(counters.connectorReadRecords));
    text.single("nettest_connector_written_records_total", "counter", "The data records written by the connectors.",
                load(counters.connectorWrittenRecords));

    // The connector statistics v2: absent without them; the publisher's lag means nothing (no time marks on its path).
    const auto readLagMicros = load(counters.connectorReadLagMicros);
    const auto rttMicros = load(counters.connectorRttMicros);
    const auto subRecords = load(counters.connectorSubRecords);

    if (!producer && readLagMicros >= 0) {
        text.single("nettest_connector_lag_seconds_total", "counter",
                    "The sum of the connector lags of the data records read.", readLagMicros * 1000, true);
    }

    if (rttMicros >= 0) {
        text.single("nettest_connector_rtt_seconds_total", "counter",
                    "The sum of the round-trip times of the records read and written by the connectors.",
                    rttMicros * 1000, true);
    }

    if (subRecords >= 0) {
        text.single("nettest_connector_sub_records_total", "counter",
                    "The subscription records read and written by the connectors.", subRecords);
    }

    const auto gcTimeMs = load(counters.gcTimeMs);

    text.single("nettest_runtime_heap_used_bytes", "gauge", "The used heap.", load(counters.heapUsedBytes));
    text.single("nettest_runtime_heap_committed_bytes", "gauge", "The committed heap.",
                load(counters.heapCommittedBytes));
    text.single("nettest_runtime_gc_total", "counter", "The collections.", load(counters.gcCount));
    text.single("nettest_runtime_gc_seconds_total", "counter", "The time of the collections.",
                gcTimeMs >= 0 ? gcTimeMs * 1'000'000 : RUNTIME_VALUE_ABSENT, true);
    text.single("nettest_runtime_cpu_seconds_total", "counter", "The CPU time of the process.",
                load(counters.cpuNanos), true);

    return text.text();
}

MetricsServer::MetricsServer(const std::string &host, std::int32_t port) : host_(host) {
#ifdef _WIN32
    WSADATA data;

    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
        error_ = "cannot start Winsock";

        return;
    }

    winsock_ = true;
#endif

    // An IPv6 address comes in brackets.
    auto name = host;

    if (name.size() > 2 && name.front() == '[' && name.back() == ']') {
        name = name.substr(1, name.size() - 2);
    }

    addrinfo hints{};

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

    addrinfo *addresses = nullptr;
    const auto service = std::to_string(port);

    if (getaddrinfo(name.c_str(), service.c_str(), &hints, &addresses) != 0 || addresses == nullptr) {
        error_ = "cannot resolve the host '" + host + "' of --metrics-port";

        return;
    }

    NativeSocket bound = NO_SOCKET;

    for (const auto *address = addresses; address != nullptr && bound == NO_SOCKET; address = address->ai_next) {
        const NativeSocket candidate = ::socket(address->ai_family, address->ai_socktype, address->ai_protocol);

        if (candidate == NO_SOCKET) {
            continue;
        }

#ifndef _WIN32
        // A port of an ended run in TIME_WAIT binds again; on Windows SO_REUSEADDR would also take a port in use.
        const int reuse = 1;

        setsockopt(candidate, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
#endif

        if (::bind(candidate, address->ai_addr, toAddressLength(address->ai_addrlen)) == 0 &&
            ::listen(candidate, 16) == 0) {
            bound = candidate;
        } else {
            closeSocket(candidate);
        }
    }

    freeaddrinfo(addresses);

    if (bound == NO_SOCKET) {
        error_ = "cannot listen on " + host + ":" + std::to_string(port) + " for /metrics (is the port in use?)";

        return;
    }

    sockaddr_storage local{};
    socklen_t length = sizeof(local);

    if (getsockname(bound, reinterpret_cast<sockaddr *>(&local), &length) == 0) {
        std::tie(port_, loopback_) = portAndLoopback(local);
    }

    socket_ = static_cast<std::intptr_t>(bound);
}

MetricsServer::~MetricsServer() {
    stop();

#ifdef _WIN32
    if (winsock_) {
        WSACleanup();
    }
#endif
}

std::string MetricsServer::url() const {
    // An IPv6 address without brackets gets them, so that the port stays apart.
    const auto host = host_.find(':') != std::string::npos && host_.front() != '[' ? "[" + host_ + "]" : host_;

    return "http://" + host + ":" + std::to_string(port_) + "/metrics";
}

void MetricsServer::start(std::function<std::string()> body) {
    if (!ok() || thread_.joinable()) {
        return;
    }

    thread_ = std::thread([this, body = std::move(body)] {
        serve(body);
    });
}

void MetricsServer::serve(const std::function<std::string()> &body) {
    const NativeSocket listening = nativeOf(socket_);

    while (!stopping_.load()) {
        if (!waitReadable(listening, POLL_MILLIS)) {
            continue;
        }

        const NativeSocket client = ::accept(listening, nullptr, nullptr);

        if (client == NO_SOCKET) {
            // The listening socket stays readable when the process runs out of descriptors: not a busy loop.
            std::this_thread::sleep_for(std::chrono::milliseconds(POLL_MILLIS));

            continue;
        }

#ifdef SO_NOSIGPIPE
        // macOS has no MSG_NOSIGNAL: a client that closes early must not raise SIGPIPE.
        const int noSigPipe = 1;

        setsockopt(client, SOL_SOCKET, SO_NOSIGPIPE, &noSigPipe, sizeof(noSigPipe));
#endif

        // A client that does not read the response holds the server for this long at most.
        setSendTimeout(client, static_cast<int>(REQUEST_TIMEOUT_MILLIS));

        // A failed scrape (no memory for the text) must not end the run: the client gets no response.
        try {
            answer(client, body, stopping_);
        } catch (const std::exception &) {
        }

        closeSocket(client);
    }
}

void MetricsServer::stop() {
    stopping_.store(true);

    if (thread_.joinable()) {
        thread_.join();
    }

    if (socket_ != -1) {
        closeSocket(nativeOf(socket_));
        socket_ = -1;
    }
}

} // namespace dxfcpp::tools::nettest

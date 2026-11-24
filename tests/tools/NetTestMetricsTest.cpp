// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// The /metrics of NetTest (section "Metrics" of the NetTest specification): the parsing of the request head, the text
// of each side against the samples of the kit, the HTTP server with its limits, and the runs with --metrics-port.

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
#    include <netinet/in.h>
#    include <poll.h>
#    include <sys/socket.h>
#    include <unistd.h>
#endif

#include <doctest.h>

#include <dxfeed_graal_cpp_api/api.hpp>

#include "../../tools/Tools/src/NetTest/Args.hpp"
#include "../../tools/Tools/src/NetTest/Metrics.hpp"
#include "../../tools/Tools/src/NetTest/MetricsRequest.hpp"
#include "../../tools/Tools/src/NetTest/NetTestTool.hpp"
#include "../../tools/Tools/src/NetTest/Run.hpp"
#include "../../tools/Tools/src/NetTest/StopControl.hpp"
#include "../../tools/Tools/src/NetTest/SymbolPool.hpp"
#include "../../tools/Tools/src/ToolsProperties.hpp"
#include "../support/TestSupport.hpp"
#include "nettest/KitJson.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

using namespace dxfcpp;
using namespace dxfcpp::tools;
using namespace dxfcpp::tools::nettest;
using namespace std::literals;

namespace {

// The process of the tests is the process of Tools: the same system properties before the first endpoint.
struct ToolsProcess {
    ToolsProcess() {
        setToolsSystemProperties();
    }
};

// The directory of the kit, or nullopt when the SDK has none and it is the stub of the SDK: the checks against the kit
// skip then. With the real SDK the missing kit fails the test.
std::optional<std::string> kitDirectory() {
    const std::string directory = DXFCXX_NETTEST_KIT_DIR;

    if (std::filesystem::is_directory(directory)) {
        return directory;
    }

#ifdef DXFCXX_NETTEST_SDK_STUB
    MESSAGE("The kit of the NetTest specification is not in the SDK stub (" << directory << "): the check is skipped");
#else
    FAIL("The kit of the NetTest specification is not found in the archive of the Graal Native SDK: " << directory);
#endif

    return std::nullopt;
}

// The lines of a text without the line ends (the kit files come with CRLF on Windows), the empty ones too.
std::vector<std::string> linesOf(const std::string &text) {
    std::vector<std::string> lines;
    std::istringstream in(text);

    for (std::string line; std::getline(in, line);) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        lines.push_back(line);
    }

    return lines;
}

// The HELP and TYPE lines of an exposition, in their order.
std::vector<std::string> commentLines(const std::string &text) {
    std::vector<std::string> comments;

    for (const auto &line : linesOf(text)) {
        if (line.rfind("# ", 0) == 0) {
            comments.push_back(line);
        }
    }

    return comments;
}

// A series line of the text exposition format: the name, the labels with their escapes, one value.
const std::regex SERIES_LINE(R"(^[a-zA-Z_:][a-zA-Z0-9_:]*\{[a-zA-Z_][a-zA-Z0-9_]*="([^"\\\n]|\\[\\"n])*")"
                             R"((,[a-zA-Z_][a-zA-Z0-9_]*="([^"\\\n]|\\[\\"n])*")*\} [^ ]+$)");

// Checks every line of an exposition: a HELP or TYPE line, or a series line with the labels tool, side and name first.
void checkExposition(const std::string &text, const std::string &side) {
    REQUIRE(!text.empty());
    CHECK(text.back() == '\n');

    for (const auto &line : linesOf(text)) {
        INFO(line);

        if (line.rfind("# HELP ", 0) == 0 || line.rfind("# TYPE ", 0) == 0) {
            continue;
        }

        CHECK(std::regex_match(line, SERIES_LINE));
        CHECK(line.find("{tool=\"cpp-api\",side=\"" + side + "\",name=\"") != std::string::npos);
    }
}

// The value of the first series that starts with the prefix (the name and the opening of the labels), as an integer.
std::optional<std::int64_t> seriesValue(const std::string &text, const std::string &prefix) {
    for (const auto &line : linesOf(text)) {
        if (line.rfind(prefix, 0) == 0) {
            const auto space = line.rfind(' ');

            try {
                return std::stoll(line.substr(space + 1));
            } catch (const std::exception &) {
                return std::nullopt;
            }
        }
    }

    return std::nullopt;
}

// ---- A blocking HTTP client over the loopback.

#ifdef _WIN32
using ClientSocket = SOCKET;
const ClientSocket NO_CLIENT_SOCKET = INVALID_SOCKET;

void closeClientSocket(ClientSocket socket) {
    closesocket(socket);
}

bool clientReadable(ClientSocket socket, int millis) {
    fd_set set{};

    set.fd_count = 1;
    set.fd_array[0] = socket;

    timeval timeout{};

    timeout.tv_sec = millis / 1000;
    timeout.tv_usec = (millis % 1000) * 1000;

    return select(0, &set, nullptr, nullptr, &timeout) > 0;
}

int clientSend(ClientSocket socket, const char *data, std::size_t size) {
    return send(socket, data, static_cast<int>(size), 0);
}

int clientReceive(ClientSocket socket, char *data, std::size_t size) {
    return recv(socket, data, static_cast<int>(size), 0);
}
#else
using ClientSocket = int;
const ClientSocket NO_CLIENT_SOCKET = -1;

void closeClientSocket(ClientSocket socket) {
    close(socket);
}

bool clientReadable(ClientSocket socket, int millis) {
    pollfd descriptor{};

    descriptor.fd = socket;
    descriptor.events = POLLIN;

    return poll(&descriptor, 1, millis) > 0;
}

int clientSend(ClientSocket socket, const char *data, std::size_t size) {
#    ifdef MSG_NOSIGNAL
    return static_cast<int>(send(socket, data, size, MSG_NOSIGNAL));
#    else
    return static_cast<int>(send(socket, data, size, 0));
#    endif
}

int clientReceive(ClientSocket socket, char *data, std::size_t size) {
    return static_cast<int>(recv(socket, data, size, 0));
}
#endif

// A connection to 127.0.0.1:<port>, closed by the destructor.
class Connection {
    ClientSocket socket_ = NO_CLIENT_SOCKET;
    // The server closed or reset the connection.
    bool closed_ = false;

    public:
    explicit Connection(std::int32_t port) : socket_(connectTo(port)) {
        REQUIRE(socket_ != NO_CLIENT_SOCKET);
    }

    // A connected socket, or NO_CLIENT_SOCKET when nothing listens on the port (yet).
    static ClientSocket connectTo(std::int32_t port) {
#ifdef _WIN32
        static const bool started = [] {
            WSADATA data{};

            return WSAStartup(MAKEWORD(2, 2), &data) == 0;
        }();

        REQUIRE(started);
#endif
        const ClientSocket socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        REQUIRE(socket != NO_CLIENT_SOCKET);

#ifdef SO_NOSIGPIPE
        const int noSigPipe = 1;

        setsockopt(socket, SOL_SOCKET, SO_NOSIGPIPE, &noSigPipe, sizeof(noSigPipe));
#endif

        sockaddr_in address{};

        address.sin_family = AF_INET;
        address.sin_port = htons(static_cast<std::uint16_t>(port));
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

        if (::connect(socket, reinterpret_cast<const sockaddr *>(&address), sizeof(address)) != 0) {
            closeClientSocket(socket);

            return NO_CLIENT_SOCKET;
        }

        return socket;
    }

    Connection(const Connection &) = delete;
    Connection &operator=(const Connection &) = delete;

    ~Connection() {
        closeClientSocket(socket_);
    }

    // Sends all bytes; false when the server closed the connection.
    bool sendAll(std::string_view data) {
        for (std::size_t sent = 0; sent < data.size();) {
            const int written = clientSend(socket_, data.data() + sent, data.size() - sent);

            if (written <= 0) {
                closed_ = true;

                return false;
            }

            sent += static_cast<std::size_t>(written);
        }

        return true;
    }

    // Whether the server has closed the connection now: a read gives the end or an error. Bytes are kept out of it.
    bool closedNow(std::string &received) {
        while (!closed_ && clientReadable(socket_, 0)) {
            char buffer[4096];
            const int count = clientReceive(socket_, buffer, sizeof(buffer));

            if (count <= 0) {
                closed_ = true;
            } else {
                received.append(buffer, static_cast<std::size_t>(count));
            }
        }

        return closed_;
    }

    // Reads until the server closes the connection or the timeout passes.
    std::string readAll(std::chrono::milliseconds timeout = 5s) {
        std::string received;
        const auto deadline = std::chrono::steady_clock::now() + timeout;

        while (!closedNow(received) && std::chrono::steady_clock::now() < deadline) {
            clientReadable(socket_, 50);
        }

        return received;
    }
};

struct HttpResponse {
    std::string statusLine{};
    std::map<std::string, std::string> headers{};
    std::string body{};
};

HttpResponse parseResponse(const std::string &text) {
    HttpResponse response;
    const auto end = text.find("\r\n\r\n");

    if (end == std::string::npos) {
        return response;
    }

    const auto lines = linesOf(text.substr(0, end));

    response.statusLine = lines.empty() ? "" : lines.front();

    for (std::size_t index = 1; index < lines.size(); ++index) {
        const auto colon = lines[index].find(": ");

        if (colon != std::string::npos) {
            response.headers[lines[index].substr(0, colon)] = lines[index].substr(colon + 2);
        }
    }

    response.body = text.substr(end + 4);

    return response;
}

// Sends the request and reads the whole response.
HttpResponse request(std::int32_t port, const std::string &text) {
    Connection connection(port);

    REQUIRE(connection.sendAll(text));

    return parseResponse(connection.readAll());
}

HttpResponse scrape(std::int32_t port) {
    return request(port, "GET /metrics HTTP/1.0\r\nHost: 127.0.0.1\r\n\r\n");
}

// A 200 response with the content type of /metrics.
void checkMetricsResponse(const HttpResponse &response) {
    CHECK(response.statusLine == "HTTP/1.0 200 OK");
    CHECK(response.headers.count("Content-Type") == 1);
    CHECK(response.headers.at("Content-Type") == "text/plain; version=0.0.4; charset=utf-8");
    CHECK(response.headers.at("Content-Length") == std::to_string(response.body.size()));
    CHECK(response.headers.at("Connection") == "close");
}

// ---- The runs.

Settings settingsOf(const std::vector<std::string> &args) {
    const auto parsed = parseArguments(args);

    REQUIRE_MESSAGE(parsed.ok, parsed.error);

    return parsed.settings;
}

Symbols symbolsOf(const Settings &settings) {
    const auto pool = buildSymbolPool(settings);

    REQUIRE_MESSAGE(pool.ok, pool.error);

    return pool.symbols;
}

std::string listenAddress(std::uint16_t port) {
    return ":" + std::to_string(port) + "[bindAddr=127.0.0.1]";
}

// Waits until a producer listens on the port: a feed refused before that reconnects only after 10-20 s, so the feed is
// created again until it is connected.
bool waitListening(std::uint16_t port) {
    for (int attempt = 0; attempt < 20; ++attempt) {
        const auto endpoint = DXEndpoint::newBuilder()
                                  ->withRole(DXEndpoint::Role::STREAM_FEED)
                                  ->withName("NetTestMetricsProbe")
                                  ->build();

        endpoint->connect("127.0.0.1:" + std::to_string(port));

        const bool connected = test::waitUntil(
            [&] {
                return endpoint->getState() == DXEndpoint::State::CONNECTED;
            },
            1000ms);

        endpoint->closeAndAwaitTermination();

        if (connected) {
            return true;
        }
    }

    return false;
}

// The producer of the tool on a thread of the test, stopped and joined on every path.
struct BackgroundProducer {
    Settings settings;
    Symbols symbols;
    StopControl stop{};
    std::ostringstream out{};
    std::thread thread{};

    explicit BackgroundProducer(const std::vector<std::string> &args)
        : settings(settingsOf(args)), symbols(symbolsOf(settings)) {
        thread = std::thread([this] {
            runProducer(settings, symbols, stop, out);
        });
    }

    BackgroundProducer(const BackgroundProducer &) = delete;
    BackgroundProducer &operator=(const BackgroundProducer &) = delete;

    ~BackgroundProducer() {
        stop.request();

        if (thread.joinable()) {
            thread.join();
        }
    }
};

struct ProcessResult {
    int exitCode{};
    std::string out{};
    std::string err{};
};

ProcessResult runTool(const std::vector<std::string> &args) {
    std::ostringstream out;
    std::ostringstream err;
    const auto exitCode = NetTestTool::runProcess(args, out, err);

    return {exitCode, out.str(), err.str()};
}

// The tool on a thread of the test; the result is read after join().
struct BackgroundTool {
    std::vector<std::string> args;
    ProcessResult result{};
    std::thread thread{};

    explicit BackgroundTool(std::vector<std::string> arguments) : args(std::move(arguments)) {
        thread = std::thread([this] {
            result = runTool(args);
        });
    }

    BackgroundTool(const BackgroundTool &) = delete;
    BackgroundTool &operator=(const BackgroundTool &) = delete;

    const ProcessResult &join() {
        if (thread.joinable()) {
            thread.join();
        }

        return result;
    }

    ~BackgroundTool() {
        join();
    }
};

// The time for the tool to start the metrics server: the process, the arguments and the isolate, slow in Debug builds
// on slow runners. The timeout of a scrape counts from the server's first answer, so the start does not consume it.
constexpr auto METRICS_SERVER_START_TIMEOUT = 30s;

// The time from the server's first answer until the predicate of a scrape holds. The endpoints start after the server,
// and on macOS runners the first endpoint after a pause may take 5 s more (the start of the JVM self-monitoring). The
// predicate ends a scrape early, so the timeout only bounds a failure. The duration of a run counts from the start of
// its endpoints, so a slow start does not shorten the run either.
constexpr auto SCRAPE_TIMEOUT = 30s;

// Whether the server accepts a connection. A connection closed without a request gets no response, and the server moves
// on.
bool serverAccepts(std::int32_t port) {
    const auto socket = Connection::connectTo(port);

    if (socket == NO_CLIENT_SOCKET) {
        return false;
    }

    closeClientSocket(socket);

    return true;
}

// Scrapes until the predicate holds for the text, the timeout passes or the run stops its server; returns the last
// text, so that a failed predicate shows what the scrape had.
template <typename Predicate> std::string scrapeUntil(std::int32_t port, Predicate &&predicate, std::chrono::seconds timeout) {
    std::string text;

    // The tool binds the port in its own thread, after the arguments and the isolate, so the first connect may come
    // too early.
    REQUIRE(test::waitUntil(
        [port] {
            return serverAccepts(port);
        },
        METRICS_SERVER_START_TIMEOUT, 50ms));

    const auto deadline = std::chrono::steady_clock::now() + timeout;

    while (true) {
        // The timeout may outlast the run: its server stops at the end of the duration.
        if (!serverAccepts(port)) {
            return text;
        }

        const auto response = scrape(port);

        checkMetricsResponse(response);
        text = response.body;

        if (predicate(text) || std::chrono::steady_clock::now() >= deadline) {
            return text;
        }

        std::this_thread::sleep_for(200ms);
    }
}

// The metrics line of the output: the name, the host and the port of the expression `metrics` of regexes.json.
void checkMetricsLine(const std::string &out, const std::string &host, std::optional<std::int32_t> port) {
    const auto lines = linesOf(out);

    REQUIRE(!lines.empty());
    INFO(lines.front());

    if (const auto directory = kitDirectory(); directory.has_value()) {
        const auto expressions = kit::readJson(*directory + "/regexes.json");
        const std::regex expression(expressions["regexes"]["metrics"].asString(), std::regex::ECMAScript);
        std::smatch match;

        REQUIRE(std::regex_search(lines.front(), match, expression));
        CHECK(match[2].str() == host);

        if (port.has_value()) {
            CHECK(match[3].str() == std::to_string(*port));
        } else {
            CHECK(match[3].str() != "0");
        }
    }

    CHECK(lines.front().rfind("{*NetTestMetrics-", 0) == 0);
}

// The comment lines of the text equal those of the sample of the kit, in the same order: every family of the side with
// its HELP and TYPE lines, and no other.
void checkFamiliesOfSample(const std::string &text, const std::string &sampleName) {
    if (const auto directory = kitDirectory(); directory.has_value()) {
        const auto sample = kit::readText(*directory + "/" + sampleName);

        CHECK(commentLines(text) == commentLines(sample));
    }
}

std::string occupyingAddress(std::int32_t port) {
    return "127.0.0.1:" + std::to_string(port);
}

} // namespace

TEST_CASE("NetTest metrics: the request head") {
    const auto parse = [](std::string_view bytes) {
        return parseRequestHead(bytes);
    };

    SUBCASE("GET and HEAD with CRLF or LF, the query left out") {
        auto head = parse("GET /metrics HTTP/1.1\r\nHost: x\r\n\r\n");

        CHECK(head.status == RequestHeadStatus::COMPLETE);
        CHECK(head.method == "GET");
        CHECK(head.path == "/metrics");

        head = parse("HEAD /metrics?x=1 HTTP/1.0\n\n");
        CHECK(head.status == RequestHeadStatus::COMPLETE);
        CHECK(head.method == "HEAD");
        CHECK(head.path == "/metrics");

        head = parse("GET /metrics\r\n\r\n");
        CHECK(head.status == RequestHeadStatus::COMPLETE);
        CHECK(head.path == "/metrics");
    }

    SUBCASE("another method and another path are complete heads") {
        auto head = parse("POST /metrics HTTP/1.0\r\nContent-Length: 0\r\n\r\nbody");

        CHECK(head.status == RequestHeadStatus::COMPLETE);
        CHECK(head.method == "POST");

        head = parse("GET /other HTTP/1.0\r\n\r\n");
        CHECK(head.status == RequestHeadStatus::COMPLETE);
        CHECK(head.path == "/other");
    }

    SUBCASE("an unfinished head needs more bytes") {
        CHECK(parse("").status == RequestHeadStatus::INCOMPLETE);
        CHECK(parse("GET /metrics HTTP/1.0\r\n").status == RequestHeadStatus::INCOMPLETE);
        CHECK(parse("GET /metrics HTTP/1.0\r\nHost: x\r\n\r").status == RequestHeadStatus::INCOMPLETE);
    }

    SUBCASE("garbage, control and NUL bytes in the request line are bad") {
        CHECK(parse("\r\n\r\n").status == RequestHeadStatus::BAD);
        CHECK(parse("hello\r\n\r\n").status == RequestHeadStatus::BAD);
        CHECK(parse(" /metrics HTTP/1.0\r\n\r\n").status == RequestHeadStatus::BAD);
        CHECK(parse("GET  HTTP/1.0\r\n\r\n").status == RequestHeadStatus::BAD);
        CHECK(parse("GET /metrics HTTP/1.0 extra\r\n\r\n").status == RequestHeadStatus::BAD);
        CHECK(parse("GET /metrics \r\n\r\n").status == RequestHeadStatus::BAD);
        CHECK(parse("G(T /metrics HTTP/1.0\r\n\r\n").status == RequestHeadStatus::BAD);
        CHECK(parse("\x01\x02\x03 \x04\n\n").status == RequestHeadStatus::BAD);
        CHECK(parse("GET /met\x7Frics HTTP/1.0\r\n\r\n").status == RequestHeadStatus::BAD);
        CHECK(parse("GET /met\rrics HTTP/1.0\r\n\r\n").status == RequestHeadStatus::BAD);
        CHECK(parse("GET /met\0rics HTTP/1.0\r\n\r\n"sv).status == RequestHeadStatus::BAD);
        CHECK(parse("\0\0\0\0\n\n"sv).status == RequestHeadStatus::BAD);
        // NUL bytes after the head are not looked at.
        CHECK(parse("GET /metrics HTTP/1.0\r\n\r\n\0\0"sv).status == RequestHeadStatus::COMPLETE);
    }

    SUBCASE("the limit of the head") {
        const std::string start = "GET /metrics HTTP/1.0\r\nX: ";
        const std::string end = "\r\n\r\n";
        const auto headOf = [&](std::size_t size) {
            return start + std::string(size - start.size() - end.size(), 'a') + end;
        };

        CHECK(parse(headOf(MAX_REQUEST_HEAD)).status == RequestHeadStatus::COMPLETE);
        CHECK(parse(headOf(MAX_REQUEST_HEAD + 1)).status == RequestHeadStatus::BAD);
        CHECK(parse(start + std::string(MAX_REQUEST_HEAD - start.size() - 1, 'a')).status ==
              RequestHeadStatus::INCOMPLETE);
        CHECK(parse(start + std::string(MAX_REQUEST_HEAD - start.size(), 'a')).status == RequestHeadStatus::BAD);
    }
}

TEST_CASE("NetTest metrics: the seconds and the buckets of the lag") {
    CHECK(formatSeconds(86'100'000'000) == "86.1");
    CHECK(formatSeconds(12'000) == "0.000012");
    CHECK(formatSeconds(0) == "0");
    CHECK(formatSeconds(1'000'000'000) == "1");
    CHECK(formatSeconds(-1'500'000'000) == "-1.5");

    CHECK(lagBucketIndex(-5) == 0);
    CHECK(lagBucketIndex(50'000) == 0);
    CHECK(lagBucketIndex(50'001) == 1);
    CHECK(lagBucketIndex(1'000'000'000) == 12);
    CHECK(lagBucketIndex(1'000'000'001) == 13);
}

// The text of each side has the families of the sample of the kit in its order, with the same HELP and TYPE lines.
TEST_CASE("NetTest metrics: the text of each side has the families of the kit sample") {
    LiveCounters counters(2);

    counters.instances[0].records.store(105);
    counters.instances[1].records.store(104);
    counters.connections[0].store(1);
    counters.connections[1].store(1);
    counters.publish.batches.store(21);
    counters.publish.fillNanos.store(1'000);
    counters.publish.publishNanos.store(86'100'000'000 - 1'000);
    counters.publish.slipLastNanos.store(12'000);

    LagBuckets buckets{};

    buckets[1] = 3;
    buckets[13] = 2;
    counters.addLags(buckets, 5, 2'000'000'500, 1);

    ConnectorTotals totals;

    totals.readBytes = 1200;
    totals.writtenBytes = 6'825'000'000;
    totals.readRecords = 0;
    totals.writtenRecords = 210;
    totals.subRecords = 100'000;
    totals.readLagMicros = 31'499'850'000;
    totals.rttMicros = 42'020'000'000;
    counters.setConnectors(totals);

    RuntimeSample runtime;

    runtime.heapUsedBytes = 125'829'120;
    runtime.heapCommittedBytes = 268'435'456;
    runtime.youngGcCount = 300;
    runtime.fullGcCount = 12;
    runtime.youngGcTimeMs = 1'500;
    runtime.fullGcTimeMs = 20;
    runtime.processCpuTimeNs = 402'700'000'000;
    counters.setRuntime(runtime);

    MetricsInfo info;

    info.name = "bench";
    info.version = "9.0.0";
    info.role = "STREAM_PUBLISHER";
    info.event = "Trade";
    info.collector = "stream";
    info.scheduled = true;

    SUBCASE("the producer") {
        info.side = "producer";

        const auto text = formatMetrics(info, counters);

        checkExposition(text, "producer");
        checkFamiliesOfSample(text, "metrics-sample-producer.txt");
        CHECK(text.find("nettest_info{tool=\"cpp-api\",side=\"producer\",name=\"bench\",spec_version=\"nettest/1\","
                        "version=\"9.0.0\",sdk_version=\"\",role=\"STREAM_PUBLISHER\",event=\"Trade\","
                        "collector=\"stream\"} 1\n") != std::string::npos);
        CHECK(text.find("nettest_records_total{tool=\"cpp-api\",side=\"producer\",name=\"bench\",connection=\"2\"} "
                        "104\n") != std::string::npos);
        CHECK(text.find("nettest_publish_seconds_total{tool=\"cpp-api\",side=\"producer\",name=\"bench\"} 86.1\n") !=
              std::string::npos);
        CHECK(text.find("nettest_publish_calls_total{tool=\"cpp-api\",side=\"producer\",name=\"bench\"} 21\n") !=
              std::string::npos);
        CHECK(text.find("nettest_schedule_slip_seconds{tool=\"cpp-api\",side=\"producer\",name=\"bench\"} 0.000012\n") !=
              std::string::npos);
        CHECK(text.find("nettest_runtime_gc_total{tool=\"cpp-api\",side=\"producer\",name=\"bench\"} 312\n") !=
              std::string::npos);
        CHECK(text.find("nettest_runtime_gc_seconds_total{tool=\"cpp-api\",side=\"producer\",name=\"bench\"} 1.52\n") !=
              std::string::npos);
        // The publisher has no time marks on its path: no connector lag on `p`.
        CHECK(text.find("nettest_connector_lag_seconds_total") == std::string::npos);
        CHECK(text.find("nettest_e2e_lag") == std::string::npos);

        // Without -r the slip has its HELP and TYPE lines and no series.
        info.scheduled = false;

        const auto flat = formatMetrics(info, counters);

        CHECK(flat.find("# TYPE nettest_schedule_slip_seconds gauge\n") != std::string::npos);
        CHECK(flat.find("nettest_schedule_slip_seconds{") == std::string::npos);
    }

    SUBCASE("the consumer") {
        info.side = "consumer";
        info.role = "STREAM_FEED";

        const auto text = formatMetrics(info, counters);
        const std::string labels = "{tool=\"cpp-api\",side=\"consumer\",name=\"bench\"";

        checkExposition(text, "consumer");
        checkFamiliesOfSample(text, "metrics-sample-consumer.txt");
        CHECK(text.find("nettest_e2e_lag_seconds_bucket" + labels + ",le=\"5e-05\"} 0\n") != std::string::npos);
        CHECK(text.find("nettest_e2e_lag_seconds_bucket" + labels + ",le=\"0.0001\"} 3\n") != std::string::npos);
        CHECK(text.find("nettest_e2e_lag_seconds_bucket" + labels + ",le=\"1\"} 3\n") != std::string::npos);
        CHECK(text.find("nettest_e2e_lag_seconds_bucket" + labels + ",le=\"+Inf\"} 5\n") != std::string::npos);
        CHECK(text.find("nettest_e2e_lag_seconds_sum" + labels + "} 2.0000005\n") != std::string::npos);
        CHECK(text.find("nettest_e2e_lag_seconds_count" + labels + "} 5\n") != std::string::npos);
        CHECK(text.find("nettest_e2e_lag_negative_total" + labels + "} 1\n") != std::string::npos);
        CHECK(text.find("nettest_connector_lag_seconds_total" + labels + "} 31499.85\n") != std::string::npos);
        CHECK(text.find("nettest_connector_rtt_seconds_total" + labels + "} 42020\n") != std::string::npos);
        CHECK(text.find("nettest_connections" + labels + ",connection=\"2\"} 1\n") != std::string::npos);
    }

    SUBCASE("without the connector statistics v2 and the runtime their series are absent") {
        info.side = "consumer";

        LiveCounters empty(1);
        const auto text = formatMetrics(info, empty);

        checkExposition(text, "consumer");
        CHECK(text.find("nettest_connector_lag_seconds_total") == std::string::npos);
        CHECK(text.find("nettest_connector_rtt_seconds_total") == std::string::npos);
        CHECK(text.find("nettest_connector_sub_records_total") == std::string::npos);
        CHECK(text.find("# TYPE nettest_runtime_heap_used_bytes gauge\n") != std::string::npos);
        CHECK(text.find("nettest_runtime_heap_used_bytes{") == std::string::npos);
        CHECK(text.find("nettest_e2e_lag_seconds_bucket{tool=\"cpp-api\",side=\"consumer\",name=\"bench\","
                        "le=\"+Inf\"} 0\n") != std::string::npos);
    }
}

// The name of -n goes into the labels with `\`, `"` and the line break escaped: no line of the text breaks.
TEST_CASE("NetTest metrics: the labels escape the name") {
    const std::string name = "a\"b\\c\nd";
    const auto settings = settingsOf({"p", ":7001", "-S", "10", "-n", name});
    LiveCounters counters(1);
    MetricsInfo info;

    REQUIRE(settings.name == name);
    info.name = "plain";

    const auto plain = formatMetrics(info, counters);

    info.name = settings.name;

    const auto text = formatMetrics(info, counters);

    checkExposition(text, "producer");
    CHECK(linesOf(text).size() == linesOf(plain).size());
    CHECK(text.find("name=\"a\\\"b\\\\c\\nd\"") != std::string::npos);
}

// The server on its own: GET and HEAD of /metrics, 404, 405, and the limits of a request.
TEST_CASE("NetTest metrics: the server answers /metrics only") {
    LiveCounters counters(1);
    MetricsInfo info;

    counters.instances[0].records.store(42);

    MetricsServer server("127.0.0.1", 0);

    REQUIRE_MESSAGE(server.ok(), server.error());
    CHECK(server.port() > 0);
    CHECK(server.loopback());
    CHECK(server.url() == "http://127.0.0.1:" + std::to_string(server.port()) + "/metrics");
    server.start([info, &counters] {
        return formatMetrics(info, counters);
    });

    SUBCASE("GET gives the text, HEAD only the head") {
        const auto get = scrape(server.port());

        checkMetricsResponse(get);
        CHECK(get.body == formatMetrics(info, counters));

        const auto head = request(server.port(), "HEAD /metrics HTTP/1.0\r\n\r\n");

        CHECK(head.statusLine == "HTTP/1.0 200 OK");
        CHECK(head.headers.at("Content-Type") == METRICS_CONTENT_TYPE);
        CHECK(head.headers.at("Content-Length") == std::to_string(get.body.size()));
        CHECK(head.body.empty());
    }

    SUBCASE("another path gives 404, another method 405") {
        CHECK(request(server.port(), "GET / HTTP/1.0\r\n\r\n").statusLine == "HTTP/1.0 404 Not Found");
        CHECK(request(server.port(), "GET /metrics/x HTTP/1.0\r\n\r\n").statusLine == "HTTP/1.0 404 Not Found");
        CHECK(request(server.port(), "POST /metrics HTTP/1.0\r\nContent-Length: 0\r\n\r\n").statusLine ==
              "HTTP/1.0 405 Method Not Allowed");
        CHECK(request(server.port(), "DELETE /other HTTP/1.0\r\n\r\n").statusLine == "HTTP/1.0 405 Method Not Allowed");
    }

    SUBCASE("a head of 8 KB is answered, a longer one is not") {
        const std::string start = "GET /metrics HTTP/1.0\r\nX: ";
        const std::string end = "\r\n\r\n";
        const auto headOf = [&](std::size_t size) {
            return start + std::string(size - start.size() - end.size(), 'a') + end;
        };

        CHECK(request(server.port(), headOf(MAX_REQUEST_HEAD)).statusLine == "HTTP/1.0 200 OK");

        Connection connection(server.port());

        // The server may reset the connection before all bytes are sent.
        connection.sendAll(headOf(MAX_REQUEST_HEAD + 1000));

        const auto received = connection.readAll();

        CHECK(received.find("200 OK") == std::string::npos);
        CHECK(received.empty());
        // The next request is answered.
        checkMetricsResponse(scrape(server.port()));
    }

    SUBCASE("malformed and unfinished heads get no response") {
        {
            Connection connection(server.port());

            connection.sendAll("garbage\r\n\r\n");
            CHECK(connection.readAll().empty());
        }
        {
            Connection connection(server.port());

            // The client closes before the end of its head.
            connection.sendAll("GET /metrics HTTP/1.0\r\n");
        }

        checkMetricsResponse(scrape(server.port()));
    }

    SUBCASE("a client that sends its head slowly is cut off, and the next one is answered at once") {
        Connection slow(server.port());
        const std::string head = "GET /metrics HTTP/1.0\r\nX-Slow: " + std::string(100, 'a');
        const auto start = std::chrono::steady_clock::now();
        std::string received;
        bool closed = false;

        // A byte every 100 ms for up to 4 s, never the end of the head.
        for (std::size_t index = 0; index < head.size() && std::chrono::steady_clock::now() - start < 4s; ++index) {
            if (slow.closedNow(received) || !slow.sendAll(head.substr(index, 1))) {
                closed = true;

                break;
            }

            std::this_thread::sleep_for(100ms);
        }

        const auto cutAfter = std::chrono::steady_clock::now() - start;

        CHECK(closed);
        CHECK(received.empty());
        CHECK(cutAfter >= 1500ms);
        CHECK(cutAfter < 3500ms);

        const auto scrapeStart = std::chrono::steady_clock::now();

        checkMetricsResponse(scrape(server.port()));
        CHECK(std::chrono::steady_clock::now() - scrapeStart < 1s);
    }

    server.stop();
    CHECK_FALSE(server.ok());
}

// A port in use ends the run with 1 before the symbols and the first endpoint: the endpoint of the same port would fail
// with its own message after SERVER_START_TIMEOUT.
TEST_CASE_FIXTURE(ToolsProcess, "NetTest metrics: a metrics port in use exits with 1 before the endpoints") {
    MetricsServer occupied("127.0.0.1", 0);

    REQUIRE(occupied.ok());

    const auto port = occupied.port();
    const auto summary = (std::filesystem::path(DXFCXX_TEST_TEMP_DIR) / "nettest-metrics-busy.json").string();

    std::filesystem::remove(summary);

    const auto start = std::chrono::steady_clock::now();
    const auto result =
        runTool({"p", listenAddress(static_cast<std::uint16_t>(port)), "-S", "10", "--summary", summary,
                 "--metrics-port", occupyingAddress(port)});

    CHECK(result.exitCode == 1);
    CHECK(result.out.empty());
    CHECK(linesOf(result.err).front() ==
          "NetTest: cannot listen on 127.0.0.1:" + std::to_string(port) + " for /metrics (is the port in use?)");
    CHECK(std::chrono::steady_clock::now() - start < 2500ms);

    const auto json = kit::readJson(summary);

    CHECK(json["status"].asString() == "error");
    CHECK(json["exit_code"].asInteger() == 1);
}

// The consumer of a run with --metrics-port: the scrape has every family of the consumer sample of the kit, the lag
// buckets, the records and the connection of the run, and no address of a connection.
TEST_CASE_FIXTURE(ToolsProcess, "NetTest metrics: a scrape of a consumer run") {
    const auto port = test::findFreeLoopbackPort();
    BackgroundProducer producer({"p", listenAddress(port), "-S", "10", "-r", "*/1000"});

    REQUIRE(waitListening(port));

    const auto metricsPort = static_cast<std::int32_t>(test::findFreeLoopbackPort());
    BackgroundTool tool({"c", "127.0.0.1:" + std::to_string(port), "-S", "10", "-n", "scraped", "-s", "1s",
                         "--duration", "6s", "--metrics-port", "127.0.0.1:" + std::to_string(metricsPort)});
    const std::string labels = "{tool=\"cpp-api\",side=\"consumer\",name=\"scraped\"";
    const auto text = scrapeUntil(
        metricsPort,
        [&](const std::string &body) {
            return seriesValue(body, "nettest_records_total" + labels + ",connection=\"1\"}").value_or(0) > 0 &&
                   seriesValue(body, "nettest_connections" + labels + ",connection=\"1\"}").value_or(0) == 1 &&
                   seriesValue(body, "nettest_connector_read_records_total" + labels).value_or(0) > 0 &&
                   seriesValue(body, "nettest_runtime_heap_used_bytes" + labels).value_or(0) > 0 &&
                   body.find("nettest_connector_rtt_seconds_total" + labels) != std::string::npos;
        },
        SCRAPE_TIMEOUT);

    INFO(text);
    checkExposition(text, "consumer");
    checkFamiliesOfSample(text, "metrics-sample-consumer.txt");
    CHECK(seriesValue(text, "nettest_connections" + labels + ",connection=\"1\"}") == 1);
    CHECK(seriesValue(text, "nettest_connector_read_records_total" + labels).value_or(0) > 0);

    const auto count = seriesValue(text, "nettest_e2e_lag_seconds_count" + labels);

    REQUIRE(count.has_value());
    CHECK(*count > 0);
    CHECK(seriesValue(text, "nettest_e2e_lag_seconds_bucket" + labels + ",le=\"+Inf\"}") == count);
    CHECK(text.find("nettest_connector_lag_seconds_total" + labels) != std::string::npos);
    CHECK(text.find("nettest_info" + labels + ",spec_version=\"nettest/1\"") != std::string::npos);
    // The versions of the tool and of the SDK are numbers, without the "v" of the tags.
    CHECK(text.find(",version=\"v") == std::string::npos);
    CHECK(text.find(",sdk_version=\"v") == std::string::npos);
    // The address of a connection may carry a token: it is never a label.
    CHECK(text.find("127.0.0.1:" + std::to_string(port)) == std::string::npos);

    const auto &result = tool.join();

    CHECK_MESSAGE(result.exitCode == 0, result.err);
    CHECK(result.err.empty());
    checkMetricsLine(result.out, "127.0.0.1", metricsPort);
}

// The producer of a run with --metrics-port and -r: the scrape has every family of the producer sample of the kit.
TEST_CASE_FIXTURE(ToolsProcess, "NetTest metrics: a scrape of a producer run") {
    const auto port = test::findFreeLoopbackPort();
    const auto metricsPort = static_cast<std::int32_t>(test::findFreeLoopbackPort());
    BackgroundTool tool({"p", listenAddress(port), "-S", "10", "-r", "*/1000", "-s", "1s", "--duration", "6s",
                         "--metrics-port", std::to_string(metricsPort)});
    const std::string labels = "{tool=\"cpp-api\",side=\"producer\",name=\"NetTest\"";
    const auto text = scrapeUntil(
        metricsPort,
        [&](const std::string &body) {
            return seriesValue(body, "nettest_records_total" + labels + ",connection=\"1\"}").value_or(0) > 0 &&
                   seriesValue(body, "nettest_publish_calls_total" + labels).value_or(0) > 0 &&
                   seriesValue(body, "nettest_runtime_heap_used_bytes" + labels).value_or(0) > 0 &&
                   body.find("nettest_connector_rtt_seconds_total" + labels) != std::string::npos;
        },
        SCRAPE_TIMEOUT);

    INFO(text);
    checkExposition(text, "producer");
    checkFamiliesOfSample(text, "metrics-sample-producer.txt");
    CHECK(text.find("nettest_schedule_slip_seconds" + labels + "} ") != std::string::npos);
    CHECK(text.find("nettest_connector_lag_seconds_total") == std::string::npos);
    CHECK(text.find("bindAddr") == std::string::npos);
    CHECK(text.find(":" + std::to_string(port)) == std::string::npos);

    const auto &result = tool.join();

    CHECK_MESSAGE(result.exitCode == 0, result.err);
    CHECK(result.err.empty());
    // Without a host the server listens on 127.0.0.1 only.
    checkMetricsLine(result.out, "127.0.0.1", metricsPort);
}

// The vectors of the section `metrics` of cli-vectors.json: the command lines of exit 0 run (the ports of the vectors
// replaced by free ones, the server sockets on the loopback, a short duration) and print the metrics line; those of
// exit 2 give their error.
TEST_CASE_FIXTURE(ToolsProcess, "NetTest metrics: the vectors of the section metrics run") {
    const auto directory = kitDirectory();

    if (!directory.has_value()) {
        return;
    }

    const auto vectors = kit::readJson(*directory + "/cli-vectors.json");
    int checked = 0;

    for (const auto &entry : vectors["vectors"].array) {
        if (!entry.has("section") || entry["section"].asString() != "metrics") {
            continue;
        }

        std::vector<std::string> args;
        std::optional<std::int32_t> metricsPort{};

        for (const auto &arg : entry["args"].array) {
            args.push_back(arg.asString());
        }

        const std::string original = [&] {
            std::string text;

            for (const auto &arg : args) {
                text += arg + " ";
            }

            return text;
        }();

        INFO(original);

        if (entry["exit"].asInteger() == 2) {
            const auto result = runTool(args);

            CHECK(result.exitCode == 2);
            CHECK(linesOf(result.err).front() == "NetTest: " + entry["error"].asString());
            ++checked;

            continue;
        }

        for (auto &arg : args) {
            if (arg.size() > 1 && arg[0] == ':') {
                arg = listenAddress(test::findFreeLoopbackPort());
            } else if (const auto position = arg.rfind("127.0.0.1:"); arg.rfind("--metrics-port=", 0) == 0 &&
                                                                       position != std::string::npos &&
                                                                       arg.substr(position + 10) != "0") {
                metricsPort = static_cast<std::int32_t>(test::findFreeLoopbackPort());
                arg = arg.substr(0, position) + "127.0.0.1:" + std::to_string(*metricsPort);
            }
        }

        args.insert(args.end(), {"--duration", "500ms"});

        const auto result = runTool(args);

        CHECK_MESSAGE(result.exitCode == 0, result.err);
        CHECK(result.err.empty());
        checkMetricsLine(result.out, "127.0.0.1", metricsPort);
        ++checked;
    }

    CHECK(checked >= 4);
}

// A host that is not a loopback one prints one warning on stderr: /metrics has no authentication and no TLS. The
// loopback hosts print none; the output and the exit code are the same.
TEST_CASE_FIXTURE(ToolsProcess, "NetTest metrics: the warning for a host that is not a loopback one") {
    const auto feed = "127.0.0.1:" + std::to_string(test::findFreeLoopbackPort());
    const auto runWith = [&](const std::string &metrics) {
        return runTool({"c", feed, "-S", "10", "--duration", "500ms", "--metrics-port", metrics});
    };

    for (const auto &[metrics, host] :
         std::vector<std::pair<std::string, std::string>>{{"0", "127.0.0.1"}, {"127.0.0.1:0", "127.0.0.1"},
                                                          {"localhost:0", "localhost"}}) {
        INFO(metrics);

        const auto result = runWith(metrics);

        CHECK_MESSAGE(result.exitCode == 0, result.err);
        CHECK(result.err.find("warning") == std::string::npos);
        CHECK(linesOf(result.out).size() == 1);
        checkMetricsLine(result.out, host, std::nullopt);
    }

#ifdef _WIN32
    // A server on all the interfaces asks the firewall of Windows to allow the test program: the case runs elsewhere.
    MESSAGE("The warning for 0.0.0.0 is checked on Linux and macOS only (the firewall prompt of Windows)");
#else
    const auto result = runWith("0.0.0.0:0");
    const std::string warning = "NetTest: warning: the metrics on 0.0.0.0:";
    const auto first = result.err.find(warning);

    CHECK_MESSAGE(result.exitCode == 0, result.err);
    REQUIRE(first != std::string::npos);
    CHECK(result.err.find(warning, first + 1) == std::string::npos);
    CHECK(linesOf(result.err).size() == 1);
    CHECK(linesOf(result.err).front().find(" have no authentication and no TLS") != std::string::npos);
    CHECK(linesOf(result.out).size() == 1);
    checkMetricsLine(result.out, "0.0.0.0", std::nullopt);
#endif
}

// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// The process ends when main() returns while the Graal threads are still running: an endpoint delivers events, or a
// live instrument profile connection holds its stream open; or after dxfcpp::shutdown(). Each scenario is a separate
// process, and the test fails by its timeout when the process hangs at exit. Usage: <executable> <scenario>.

#include <dxfeed_graal_cpp_api/api.hpp>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#ifdef _WIN32
#    ifndef WIN32_LEAN_AND_MEAN
#        define WIN32_LEAN_AND_MEAN
#    endif
#    include <winsock2.h>
#    include <ws2tcpip.h>
#else
#    include <arpa/inet.h>
#    include <netinet/in.h>
#    include <sys/socket.h>
#    include <unistd.h>
#endif

using namespace dxfcpp;
using namespace std::literals;

namespace {

#ifdef _WIN32
using NativeSocket = SOCKET;
using IoSize = int;
constexpr NativeSocket INVALID_NATIVE_SOCKET = INVALID_SOCKET;
#else
using NativeSocket = int;
using IoSize = std::size_t;
constexpr NativeSocket INVALID_NATIVE_SOCKET = -1;
#endif

/**
 * An HTTP server on a free loopback port that serves a live instrument profile stream: it answers each request with
 * `X-Live: provided`, sends the profiles and `##COMPLETE`, then an update every 10 ms, and keeps the connection open,
 * as a live IPF service does. It is never stopped: the process ends with it.
 */
class LiveIpfServer final {
    NativeSocket listenSocket_{INVALID_NATIVE_SOCKET};
    std::uint16_t port_{};

    static void serve(NativeSocket client) {
        // Read the request headers, then answer and leave the connection open.
        std::string request{};
        char buffer[1024];

        while (request.find("\r\n\r\n") == std::string::npos) {
            const auto length = recv(client, buffer, static_cast<IoSize>(sizeof(buffer)), 0);

            if (length <= 0) {
                return;
            }

            request.append(buffer, static_cast<std::size_t>(length));
        }

        const std::string response = "HTTP/1.1 200 OK\r\n"
                                     "Content-Type: text/plain\r\n"
                                     "X-Live: provided\r\n"
                                     "\r\n"
                                     "#STOCK::=TYPE,SYMBOL,DESCRIPTION\n"
                                     "STOCK,EXIT_TEST,Exit test stock\n"
                                     "##COMPLETE\n";

        if (send(client, response.data(), static_cast<IoSize>(response.size()), 0) <= 0) {
            return;
        }

        // The updates: a changed profile and a flush (`##`).
        for (int i = 0;; i++) {
            std::this_thread::sleep_for(10ms);

            const auto update = "STOCK,EXIT_TEST,Exit test stock " + std::to_string(i) + "\n##\n";

            if (send(client, update.data(), static_cast<IoSize>(update.size()), 0) <= 0) {
                return;
            }
        }
    }

  public:
    LiveIpfServer() {
#ifdef _WIN32
        WSADATA wsaData{};

        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            throw std::runtime_error("WSAStartup failed");
        }
#endif

        listenSocket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        if (listenSocket_ == INVALID_NATIVE_SOCKET) {
            throw std::runtime_error("LiveIpfServer: socket() failed");
        }

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        address.sin_port = 0; // the OS picks a free port

        socklen_t length = sizeof(address);

        if (bind(listenSocket_, reinterpret_cast<sockaddr *>(&address), sizeof(address)) != 0 ||
            listen(listenSocket_, 16) != 0 ||
            getsockname(listenSocket_, reinterpret_cast<sockaddr *>(&address), &length) != 0) {
            throw std::runtime_error("LiveIpfServer: bind() or listen() failed");
        }

        port_ = ntohs(address.sin_port);

        std::thread([listenSocket = listenSocket_] {
            while (true) {
                const NativeSocket client = accept(listenSocket, nullptr, nullptr);

                if (client != INVALID_NATIVE_SOCKET) {
                    std::thread(serve, client).detach(); // the client socket stays open
                } else {
                    std::this_thread::sleep_for(10ms);
                }
            }
        }).detach();
    }

    std::string getUrl() const {
        return "http://127.0.0.1:" + std::to_string(port_) + "/ipf";
    }
};

// Not destroyed at exit: the listener may still be called then.
constinit std::atomic<std::size_t> received{};
constinit std::atomic<std::size_t> listenerCalls{};
constinit std::atomic<bool> slowListener{};

/**
 * Created before main(), so destroyed after the exit handler of the library (registered when the library is first
 * used). When enabled, the destructor fails the process if the listener is called after the handler has run.
 */
struct LateCallbackCheck {
    bool enabled{};

    ~LateCallbackCheck() {
        if (!enabled) {
            return;
        }

        std::this_thread::sleep_for(200ms); // the listener calls in progress end
        const auto calls = listenerCalls.load();
        std::this_thread::sleep_for(500ms);

        if (listenerCalls.load() != calls) {
            std::fputs("The listener was called after the exit handler of the library\n", stderr);
            std::fflush(stderr);
            std::_Exit(5);
        }
    }
} lateCallbackCheck{};

/// Creates an endpoint that delivers a stream of events to a listener; returns once the listener has been called.
std::shared_ptr<DXEndpoint> startEvents() {
    auto endpoint = DXEndpoint::create(DXEndpoint::Role::LOCAL_HUB);
    auto subscription = endpoint->getFeed()->createSubscription(Quote::TYPE);

    subscription->addEventListener([](const auto &events) {
        listenerCalls++;
        received += events.size();
        std::this_thread::sleep_for(1ms);
    });
    subscription->addSymbols("EXIT_TEST"s);

    std::vector<std::shared_ptr<EventType>> events{};

    for (int i = 0; i < 1000; i++) {
        auto quote = std::make_shared<Quote>("EXIT_TEST");
        quote->setBidPrice(i);
        events.push_back(quote);
    }

    for (int i = 0; i < 20; i++) {
        endpoint->getPublisher()->publishEvents(events);
    }

    const auto deadline = std::chrono::steady_clock::now() + 10s;

    while (received == 0 && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(10ms);
    }

    if (received == 0) {
        throw std::runtime_error("No events were received");
    }

    return endpoint;
}

/// Returns from main() while the feed delivers a stream of events to a slow listener, without endpoint->close().
int endpointWithEvents() {
    startEvents();

    return 0;
}

/// Starts a live instrument profile connection; returns once it has received the profiles and an update.
std::shared_ptr<InstrumentProfileConnection> startLiveIpf() {
    static const auto *server = new LiveIpfServer{};

    auto collector = InstrumentProfileCollector::create();
    auto connection = InstrumentProfileConnection::createConnection(server->getUrl(), collector);

    collector->addUpdateListener([](const auto &) {
        listenerCalls++;
        std::this_thread::sleep_for(slowListener ? 50ms : 1ms);
    });
    connection->start();

    if (!connection->waitUntilCompleted(10s)) {
        throw std::runtime_error("The live instrument profile connection did not complete");
    }

    // The stream goes on.
    const auto calls = listenerCalls.load();
    const auto deadline = std::chrono::steady_clock::now() + 10s;

    while (listenerCalls == calls && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(10ms);
    }

    if (listenerCalls == calls) {
        throw std::runtime_error("The live instrument profile connection receives no updates");
    }

    return connection;
}

/// Keeps the connection open until the process ends: its destructor would close it.
void keepOpen(std::shared_ptr<InstrumentProfileConnection> connection) {
    static auto &connections = *new std::vector<std::shared_ptr<InstrumentProfileConnection>>{};

    connections.push_back(std::move(connection));
}

/// Returns from main() while the listener is called; it must not be called after the library's exit handler.
int lateCallback() {
    slowListener = true;
    lateCallbackCheck.enabled = true;

    keepOpen(startLiveIpf()); // the server sends updates until the process ends

    return 0;
}

/// Returns from main() with a live instrument profile connection, closed or open.
int liveIpf(bool close) {
    auto connection = startLiveIpf();

    if (close) {
        connection->close();
    } else {
        keepOpen(connection);
    }

    return 0;
}

/// Calls dxfcpp::shutdown() with open endpoints and a live instrument profile connection, then uses the library.
int shutdownWithOpenEntities() {
    auto endpoint = startEvents();
    auto connection = startLiveIpf();
    auto feed = DXFeed::getInstance(); // the default endpoint

    shutdown();

    const auto calls = listenerCalls.load();
    std::this_thread::sleep_for(200ms);

    if (listenerCalls.load() != calls) {
        std::fputs("The listener was called after shutdown()\n", stderr);

        return 3;
    }

    try {
        DXFeed::getInstance();
        std::fputs("DXFeed::getInstance() did not throw after shutdown()\n", stderr);

        return 3;
    } catch (const GraalException &) {
    }

    shutdown(); // does nothing

    return 0; // the endpoint and the connection are destroyed after shutdown()
}

} // namespace

int main(int argc, char *argv[]) {
#ifndef _WIN32
    // The server writes to the connections that the library closes.
    std::signal(SIGPIPE, SIG_IGN);
#endif

    if (argc != 2) {
        std::fputs("Usage: <executable> endpoint | late-callback | ipf-live | ipf-live-close | shutdown\n", stderr);

        return 1;
    }

    const std::string_view scenario = argv[1];

    try {
        if (scenario == "endpoint") {
            return endpointWithEvents();
        }

        if (scenario == "late-callback") {
            return lateCallback();
        }

        if (scenario == "ipf-live") {
            return liveIpf(false);
        }

        if (scenario == "ipf-live-close") {
            return liveIpf(true);
        }

        if (scenario == "shutdown") {
            return shutdownWithOpenEntities();
        }
    } catch (const std::exception &e) {
        std::fprintf(stderr, "Exception: %s\n", e.what());

        return 1;
    }

    std::fprintf(stderr, "Unknown scenario: %s\n", argv[1]);

    return 1;
}

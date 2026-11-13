// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// The process ends when main() returns while the Graal threads are still running: an endpoint delivers events, or a
// live instrument profile connection holds its stream open; or after dxfcpp::shutdown(). Each scenario is a separate
// process, and the test fails by its timeout when the process hangs at exit. The error-manager scenario reports errors
// at exit. Usage: <executable> <scenario>.

#include <dxfeed_graal_cpp_api/api.hpp>
#include "../../src/internal/CallbackBarrier.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
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
#    ifndef NOMINMAX
#        define NOMINMAX
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
constinit std::atomic<bool> blockListener{};
constinit std::atomic<bool> listenerBlocked{};
// How long the blocked listener of the instrument profiles blocks.
constinit std::atomic<std::int64_t> blockMilliseconds{500};
constinit std::atomic<bool> blockEventListener{};
constinit std::atomic<bool> eventListenerBlocked{};

// The result of dxfcpp::shutdown() called from the event listener.
enum class ShutdownFromListener : int { NOT_CALLED, REFUSED, RETURNED, OTHER_EXCEPTION };

constinit std::atomic<bool> callShutdownFromListener{};
constinit std::atomic<ShutdownFromListener> shutdownFromListenerResult{ShutdownFromListener::NOT_CALLED};

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

/**
 * Created before main(), so destroyed after the exit handler of the library. When enabled, the destructor fails the
 * process if a listener call is still in progress: the handler must wait for it.
 */
struct CallbackInProgressCheck {
    bool enabled{};

    ~CallbackInProgressCheck() {
        if (enabled && listenerBlocked.load()) {
            std::fputs("A listener call was in progress after the exit handler of the library\n", stderr);
            std::fflush(stderr);
            std::_Exit(6);
        }
    }
} callbackInProgressCheck{};

/// Creates an endpoint that delivers a stream of events to a listener; returns once the listener has been called.
std::shared_ptr<DXEndpoint> startEvents() {
    auto endpoint = DXEndpoint::create(DXEndpoint::Role::LOCAL_HUB);
    auto subscription = endpoint->getFeed()->createSubscription(Quote::TYPE);

    subscription->addEventListener([](const auto &events) {
        listenerCalls++;

        if (callShutdownFromListener.exchange(false)) {
            try {
                shutdown();
                shutdownFromListenerResult = ShutdownFromListener::RETURNED;
            } catch (const RuntimeException &) {
                shutdownFromListenerResult = ShutdownFromListener::REFUSED;
            } catch (...) {
                shutdownFromListenerResult = ShutdownFromListener::OTHER_EXCEPTION;
            }
        }

        received += events.size();

        if (blockEventListener.exchange(false)) {
            eventListenerBlocked = true;
            std::this_thread::sleep_for(3s); // longer than the library waits for its own callbacks
            eventListenerBlocked = false;
        }

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

        if (blockListener.exchange(false)) {
            // The process exits or the library shuts down meanwhile.
            listenerBlocked = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(blockMilliseconds.load()));
            listenerBlocked = false;
        }

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

/// Returns from main() while a listener call is in progress; the exit handler of the library must wait for it.
int callbackInProgress() {
    callbackInProgressCheck.enabled = true;
    keepOpen(startLiveIpf());
    blockListener = true;

    const auto deadline = std::chrono::steady_clock::now() + 10s;

    while (!listenerBlocked && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(1ms);
    }

    if (!listenerBlocked) {
        throw std::runtime_error("The listener was not called");
    }

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

/// Reports errors to ErrorManager after dxfcpp::shutdown(), from the destructor of a static object and from a thread
/// that keeps reporting while the process ends: ErrorManager is never destroyed and does not use the isolate.
int errorManagerAtExit() {
    static auto &handled = *new std::atomic<std::uint64_t>{};

    ErrorManager::setHandler([](const ErrorInfo &) {
        handled.fetch_add(1);
    });

    shutdown();

    ErrorManager::report("ExitTest", "after shutdown()");
    ErrorManager::reportException("ExitTest", std::make_exception_ptr(std::runtime_error("after shutdown()")));

    const auto last = ErrorManager::getLastError();

    if (handled.load() != 2 || !last || last->message != "after shutdown()") {
        std::fputs("ErrorManager did not handle the errors after shutdown()\n", stderr);

        return 3;
    }

    struct ReportAtExit {
        ~ReportAtExit() {
            ErrorManager::report("ExitTest", "static destructor");
        }
    };

    static const ReportAtExit reportAtExit{};

    std::thread{[] {
        for (;;) {
            ErrorManager::report("ExitTest", "exit thread");
            std::this_thread::sleep_for(1ms);
        }
    }}.detach();

    std::this_thread::sleep_for(20ms);

    return 0;
}

/// Releases after dxfcpp::shutdown() (a model closed by its destructor, runRelease, DXFCPP_FINALLY) report nothing,
/// even when they fail: the library is shutting down.
int releaseAfterShutdown() {
    static auto &handled = *new std::atomic<std::uint64_t>{};

    ErrorManager::setHandler([](const ErrorInfo &) {
        handled.fetch_add(1);
    });

    auto endpoint = DXEndpoint::create(DXEndpoint::Role::LOCAL_HUB);
    auto model = IndexedTxModel<Order>::newBuilder()->withSymbol("EXIT_TEST")->withFeed(endpoint->getFeed())->build();

    shutdown();

    model.reset();
    runRelease("ExitTest", [] {
        throw std::runtime_error("release after shutdown()");
    });

    {
        DXFCPP_FINALLY([] {
            throw std::runtime_error("finally after shutdown()");
        });
    }

    if (handled.load() != 0 || ErrorManager::getLastError()) {
        std::fputs("A release after shutdown() was reported to ErrorManager\n", stderr);

        return 3;
    }

    return 0;
}

/// After dxfcpp::shutdown() attaching and detaching a subscription do nothing and do not throw.
int detachAfterShutdown() {
    auto endpoint = DXEndpoint::create(DXEndpoint::Role::LOCAL_HUB);
    auto feed = endpoint->getFeed();
    auto subscription = feed->createSubscription(Quote::TYPE);

    shutdown();

    try {
        feed->detachSubscription(subscription);
        feed->attachSubscription(subscription);
        feed->detachSubscriptionAndClear(subscription);
    } catch (const std::exception &e) {
        std::fprintf(stderr, "Attach or detach threw after shutdown(): %s\n", e.what());

        return 3;
    }

    return 0;
}

/// Publishes quotes until the condition holds or 10 seconds pass; returns whether it holds.
template <typename Condition> bool publishUntil(const std::shared_ptr<DXEndpoint> &endpoint, Condition condition) {
    const auto deadline = std::chrono::steady_clock::now() + 10s;
    const std::vector<std::shared_ptr<EventType>> events{std::make_shared<Quote>("EXIT_TEST")};

    while (!condition() && std::chrono::steady_clock::now() < deadline) {
        endpoint->getPublisher()->publishEvents(events);
        std::this_thread::sleep_for(10ms);
    }

    return condition();
}

/// Calls dxfcpp::shutdown() while the listeners of a subscription and of the instrument profiles are blocked for longer
/// than the library waits for its own callbacks: shutdown() returns after their calls complete, and they are not
/// called afterwards.
int shutdownWithCallbacksInProgress() {
    auto endpoint = startEvents();
    auto connection = startLiveIpf();

    blockMilliseconds = 3000;
    blockListener = true;
    blockEventListener = true;

    if (!publishUntil(endpoint, [] {
            return listenerBlocked.load() && eventListenerBlocked.load();
        })) {
        throw std::runtime_error("The listeners were not blocked");
    }

    shutdown();

    if (listenerBlocked.load() || eventListenerBlocked.load()) {
        std::fputs("A listener call was in progress after shutdown() returned\n", stderr);

        return 3;
    }

    const auto calls = listenerCalls.load();
    std::this_thread::sleep_for(200ms);

    if (listenerCalls.load() != calls) {
        std::fputs("The listener was called after shutdown()\n", stderr);

        return 3;
    }

    return 0;
}

/// Calls dxfcpp::shutdown() from a listener: the call is refused, and the library goes on working until shutdown() is
/// called outside of the listeners.
int shutdownFromListener() {
    callShutdownFromListener = true;

    auto endpoint = startEvents();

    if (!publishUntil(endpoint, [] {
            return shutdownFromListenerResult.load() != ShutdownFromListener::NOT_CALLED;
        })) {
        throw std::runtime_error("The listener did not call shutdown()");
    }

    if (shutdownFromListenerResult.load() != ShutdownFromListener::REFUSED) {
        std::fprintf(stderr, "shutdown() from a listener was not refused with RuntimeException: %d\n",
                     static_cast<int>(shutdownFromListenerResult.load()));

        return 3;
    }

    const auto calls = listenerCalls.load();

    if (!publishUntil(endpoint, [calls] {
            return listenerCalls.load() > calls;
        })) {
        std::fputs("The listener is not called after the refused shutdown()\n", stderr);

        return 3;
    }

    shutdown();

    try {
        DXFeed::getInstance();
        std::fputs("DXFeed::getInstance() did not throw after shutdown()\n", stderr);

        return 3;
    } catch (const GraalException &) {
    }

    return 0;
}

} // namespace

int main(int argc, char *argv[]) {
#ifndef _WIN32
    // The server writes to the connections that the library closes.
    std::signal(SIGPIPE, SIG_IGN);
#endif

    if (argc != 2) {
        std::fputs("Usage: <executable> endpoint | late-callback | callback-in-progress | ipf-live | ipf-live-close | "
                   "shutdown | shutdown-callbacks-in-progress | shutdown-from-listener | error-manager | "
                   "release-after-shutdown | detach-after-shutdown\n",
                   stderr);

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

        if (scenario == "callback-in-progress") {
            return callbackInProgress();
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

        if (scenario == "shutdown-callbacks-in-progress") {
            return shutdownWithCallbacksInProgress();
        }

        if (scenario == "shutdown-from-listener") {
            return shutdownFromListener();
        }

        if (scenario == "error-manager") {
            return errorManagerAtExit();
        }

        if (scenario == "release-after-shutdown") {
            return releaseAfterShutdown();
        }

        if (scenario == "detach-after-shutdown") {
            return detachAfterShutdown();
        }
    } catch (const std::exception &e) {
        std::fprintf(stderr, "Exception: %s\n", e.what());

        return 1;
    }

    std::fprintf(stderr, "Unknown scenario: %s\n", argv[1]);

    return 1;
}

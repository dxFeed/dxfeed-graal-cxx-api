// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// The process ends when main() returns while the Graal threads are still running: an endpoint delivers events, or a
// live instrument profile connection holds its stream open; or after dxfcpp::shutdown(). Each scenario is a separate
// process, and the test fails by its timeout when the process hangs at exit. The error-manager scenario reports errors
// at exit. Usage: <executable> <scenario>.

#include <dxfeed_graal_cpp_api/api.hpp>
#include "../../src/internal/CallbackBarrier.hpp"
#include "../../src/internal/Isolate.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <memory>
#include <mutex>
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

// The result of Isolate::shutdownAndTearDown() called from the event listener.
constinit std::atomic<bool> callTearDownFromListener{};
constinit std::atomic<bool> tearDownFromListenerCalled{};
constinit std::atomic<Isolate::TearDownResult::Status> tearDownFromListenerStatus{
    Isolate::TearDownResult::Status::NOTHING_TO_TEAR_DOWN};

// The event listener leaks memory once: the allocation has the frames of the SDK library in its stack.
constinit std::atomic<bool> leakFromListener{};
constinit std::atomic<bool> leakedFromListenerDone{};
constinit int *volatile leakedFromListener{};

// The event listener closes a subscription: the release thread starts.
constinit std::atomic<bool> closeFromListener{};
constinit std::atomic<bool> closedFromListenerDone{};

// The subscription that the event listener closes. Never destroyed: the listener may still be called at exit.
std::shared_ptr<DXFeedSubscription> &closedFromListener() {
    static auto &subscription = *new std::shared_ptr<DXFeedSubscription>{};

    return subscription;
}

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

        if (leakFromListener.exchange(false)) {
            leakedFromListener = new int[16]{};
            leakedFromListenerDone = true;
        }

        if (callTearDownFromListener.exchange(false)) {
            tearDownFromListenerStatus = Isolate::shutdownAndTearDown().status;
            tearDownFromListenerCalled = true;
        }

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

    // The INFO line of the versions at the creation of the endpoint is not a report of a release.
    ErrorManager::setHandler([](const ErrorInfo &error) {
        if (error.level != ErrorInfo::Level::INFO) {
            handled.fetch_add(1);
        }
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

    const auto last = ErrorManager::getLastError();

    if (handled.load() != 0 || (last && last->level != ErrorInfo::Level::INFO)) {
        std::fputs("A release after shutdown() was reported to ErrorManager\n", stderr);

        return 3;
    }

    return 0;
}

// The name of the OS in the line of the versions, empty for an OS the check does not know.
constexpr std::string_view expectedOsName() {
#if defined(_WIN32)
    return "Windows";
#elif defined(__linux__) && !defined(__ANDROID__)
    return "Linux";
#elif defined(__APPLE__)
    return "Mac";
#else
    return "";
#endif
}

// Checks the text of the line of the versions; prints the reason and returns false if it is wrong.
bool checkVersionsLine(const std::string &message) {
    const auto expectedApi = std::string("dxFeed Graal C++ API ") + DXFC_TEST_CXX_API_VERSION + ", ";
    constexpr std::string_view sdkPrefix = "dxFeed Graal Native SDK ";
    const auto sdk = message.find(sdkPrefix);

    if (message.rfind(expectedApi, 0) != 0 || sdk == std::string::npos || sdk + sdkPrefix.size() >= message.size() ||
        message[sdk + sdkPrefix.size()] < '0' || message[sdk + sdkPrefix.size()] > '9' ||
        message.find(expectedOsName()) == std::string::npos) {
        std::fprintf(stderr, "Wrong line of the versions: %s\n", message.c_str());

        return false;
    }

    return true;
}

/// The first endpoint of the process reports one INFO line with the versions and the platform; the next endpoints
/// (created by DXEndpoint::create() and by a Builder) do not repeat it.
int versionsOnce() {
    static auto &infos = *new std::vector<ErrorInfo>{};
    static auto &mutex = *new std::mutex{};

    ErrorManager::setHandler([](const ErrorInfo &error) {
        if (error.level == ErrorInfo::Level::INFO) {
            std::lock_guard lock{mutex};

            infos.push_back(error);
        }
    });

    auto feed = DXEndpoint::create();
    auto publisher = DXEndpoint::newBuilder()->withRole(DXEndpoint::Role::PUBLISHER)->build();
    auto hub = DXEndpoint::create(DXEndpoint::Role::LOCAL_HUB);

    std::lock_guard lock{mutex};

    if (infos.size() != 1) {
        std::fprintf(stderr, "Expected one INFO line of the versions, got %llu\n",
                     static_cast<unsigned long long>(infos.size()));

        return 3;
    }

    if (infos[0].component != "DXEndpoint" || !checkVersionsLine(infos[0].message)) {
        return 3;
    }

    return 0;
}

/// The first endpoint comes from DXEndpoint::getInstance(), and the handler of the INFO line calls
/// DXFeed::getInstance(): the line is reported out of the lock of the default endpoints, so there is no deadlock, and
/// the line is reported once.
int versionsGetInstance() {
    static auto &infos = *new std::atomic<std::uint64_t>{};
    static auto &feedFromHandler = *new std::atomic<bool>{};

    ErrorManager::setHandler([](const ErrorInfo &error) {
        if (error.level == ErrorInfo::Level::INFO) {
            infos.fetch_add(1);
            feedFromHandler.store(DXFeed::getInstance() != nullptr);
        }
    });

    auto endpoint = DXEndpoint::getInstance();

    if (infos.load() != 1 || !feedFromHandler.load() || DXFeed::getInstance() != endpoint->getFeed()) {
        std::fprintf(stderr, "Expected one INFO line and the default feed from the handler, got %llu line(s)\n",
                     static_cast<unsigned long long>(infos.load()));

        return 3;
    }

    return 0;
}

/// The default handler writes the line of the versions to stderr: the test checks the output of the process.
int versionsDefaultHandler() {
    auto endpoint = DXEndpoint::create();

    return 0;
}

/// A creation of an endpoint that fails (after dxfcpp::shutdown()) reports no line of the versions.
int versionsFailedCreation() {
    static auto &infos = *new std::atomic<std::uint64_t>{};

    ErrorManager::setHandler([](const ErrorInfo &error) {
        if (error.level == ErrorInfo::Level::INFO) {
            infos.fetch_add(1);
        }
    });

    shutdown();

    try {
        auto endpoint = DXEndpoint::create();

        std::fputs("DXEndpoint::create() after shutdown() did not throw\n", stderr);

        return 3;
    } catch (const std::exception &) {
    }

    if (infos.load() != 0) {
        std::fputs("A failed creation of an endpoint reported the line of the versions\n", stderr);

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

using TearDownStatus = Isolate::TearDownResult::Status;

const char *toText(TearDownStatus status) {
    switch (status) {
    case TearDownStatus::TORN_DOWN:
        return "TORN_DOWN";
    case TearDownStatus::NOTHING_TO_TEAR_DOWN:
        return "NOTHING_TO_TEAR_DOWN";
    case TearDownStatus::REFUSED:
        return "REFUSED";
    case TearDownStatus::FAILED:
        return "FAILED";
    }

    return "unknown";
}

/// Tears the isolate down; prints the result and returns false if the status is not the expected one.
bool tearDownExpecting(TearDownStatus expected) {
    const auto start = std::chrono::steady_clock::now();
    const auto result = Isolate::shutdownAndTearDown();
    const auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();

    std::fprintf(stderr, "Isolate::shutdownAndTearDown(): %s, error %u, %lld ms\n", toText(result.status),
                 static_cast<unsigned>(result.error), static_cast<long long>(milliseconds));

    if (result.status != expected) {
        std::fprintf(stderr, "Expected %s\n", toText(expected));

        return false;
    }

    return true;
}

/// The library is not entered after the tear-down: the calls throw, and the objects are released without the isolate.
bool checkAfterTearDown() {
    try {
        DXFeed::getInstance();
        std::fputs("DXFeed::getInstance() did not throw after the tear-down\n", stderr);

        return false;
    } catch (const GraalException &) {
    }

    return true;
}

/// Tears the isolate down while an endpoint delivers events; the endpoint is destroyed after the tear-down.
int tearDown() {
    auto endpoint = startEvents();

    if (!tearDownExpecting(TearDownStatus::TORN_DOWN) || !checkAfterTearDown()) {
        return 3;
    }

    const auto calls = listenerCalls.load();
    std::this_thread::sleep_for(200ms);

    if (listenerCalls.load() != calls) {
        std::fputs("The listener was called after the tear-down\n", stderr);

        return 3;
    }

    return 0;
}

/// The second tear-down does nothing.
int tearDownTwice() {
    auto endpoint = startEvents();

    return tearDownExpecting(TearDownStatus::TORN_DOWN) && tearDownExpecting(TearDownStatus::NOTHING_TO_TEAR_DOWN) &&
                   checkAfterTearDown()
               ? 0
               : 3;
}

/// The tear-down does not create the isolate.
int tearDownNotCreated() {
    if (!tearDownExpecting(TearDownStatus::NOTHING_TO_TEAR_DOWN)) {
        return 3;
    }

    if (Isolate::isCreated()) {
        std::fputs("The tear-down created the isolate\n", stderr);

        return 3;
    }

    return 0;
}

/// The tear-down after dxfcpp::shutdown() tears the isolate down.
int tearDownAfterShutdown() {
    auto endpoint = startEvents();

    shutdown();

    return tearDownExpecting(TearDownStatus::TORN_DOWN) && checkAfterTearDown() ? 0 : 3;
}

/// The isolate is created on a thread that has ended; the main thread has never called into it.
int tearDownOtherThread() {
    std::shared_ptr<DXEndpoint> endpoint{};
    std::exception_ptr error{};

    std::thread{[&endpoint, &error] {
        try {
            endpoint = startEvents();
        } catch (...) {
            error = std::current_exception();
        }
    }}.join();

    if (error) {
        std::rethrow_exception(error);
    }

    return tearDownExpecting(TearDownStatus::TORN_DOWN) && checkAfterTearDown() ? 0 : 3;
}

/// The release thread of the library is attached to the isolate and waits forever.
int tearDownReleaseThread() {
    auto endpoint = DXEndpoint::create(DXEndpoint::Role::LOCAL_HUB);

    closedFromListener() = endpoint->getFeed()->createSubscription(Quote::TYPE);
    closedFromListener()->addEventListener([](const auto &) {
    });
    closedFromListener()->addSymbols("EXIT_TEST"s);
    closeFromListener = true;

    auto subscription = endpoint->getFeed()->createSubscription(Quote::TYPE);

    subscription->addEventListener([](const auto &) {
        if (closeFromListener.exchange(false)) {
            // Passes the native listener of the subscription to the release thread, which stays attached.
            closedFromListener()->close();
            closedFromListenerDone = true;
        }
    });
    subscription->addSymbols("EXIT_TEST"s);

    if (!publishUntil(endpoint, [] {
            return closedFromListenerDone.load();
        })) {
        throw std::runtime_error("The listener did not close the subscription");
    }

    return tearDownExpecting(TearDownStatus::TORN_DOWN) && checkAfterTearDown() ? 0 : 3;
}

/// The tear-down from a listener is refused; the tear-down after it, outside the listeners, tears the isolate down.
int tearDownFromListener() {
    callTearDownFromListener = true;

    auto endpoint = startEvents();

    if (!publishUntil(endpoint, [] {
            return tearDownFromListenerCalled.load();
        })) {
        throw std::runtime_error("The listener did not call the tear-down");
    }

    if (tearDownFromListenerStatus.load() != TearDownStatus::REFUSED) {
        std::fprintf(stderr, "The tear-down from a listener was not refused: %s\n",
                     toText(tearDownFromListenerStatus.load()));

        return 3;
    }

    return tearDownExpecting(TearDownStatus::TORN_DOWN) && checkAfterTearDown() ? 0 : 3;
}

/// Tears the isolate down after a live instrument profile connection is closed.
int tearDownLiveIpf() {
    auto endpoint = startEvents();
    auto connection = startLiveIpf();

    connection->close();

    return tearDownExpecting(TearDownStatus::TORN_DOWN) && checkAfterTearDown() ? 0 : 3;
}

/**
 * Leaks memory from a listener and tears the isolate down: LeakSanitizer must report the leak, which the suppression of
 * the SDK library would hide (a canary of the teardown scenarios).
 */
int tearDownLeakCanary() {
    leakFromListener = true;

    auto endpoint = startEvents();

    if (!publishUntil(endpoint, [] {
            return leakedFromListenerDone.load();
        })) {
        throw std::runtime_error("The listener did not leak");
    }

    if (!tearDownExpecting(TearDownStatus::TORN_DOWN)) {
        return 3;
    }

    leakedFromListener = nullptr;

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
                   "release-after-shutdown | detach-after-shutdown | versions-once | versions-get-instance | "
                   "versions-default-handler | versions-failed-creation | teardown | teardown-twice | "
                   "teardown-not-created | teardown-after-shutdown | teardown-other-thread | "
                   "teardown-release-thread | teardown-from-listener | teardown-live-ipf | teardown-leak-canary\n",
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

        if (scenario == "versions-once") {
            return versionsOnce();
        }

        if (scenario == "versions-get-instance") {
            return versionsGetInstance();
        }

        if (scenario == "versions-default-handler") {
            return versionsDefaultHandler();
        }

        if (scenario == "versions-failed-creation") {
            return versionsFailedCreation();
        }

        if (scenario == "teardown") {
            return tearDown();
        }

        if (scenario == "teardown-twice") {
            return tearDownTwice();
        }

        if (scenario == "teardown-not-created") {
            return tearDownNotCreated();
        }

        if (scenario == "teardown-after-shutdown") {
            return tearDownAfterShutdown();
        }

        if (scenario == "teardown-other-thread") {
            return tearDownOtherThread();
        }

        if (scenario == "teardown-release-thread") {
            return tearDownReleaseThread();
        }

        if (scenario == "teardown-from-listener") {
            return tearDownFromListener();
        }

        if (scenario == "teardown-live-ipf") {
            return tearDownLiveIpf();
        }

        if (scenario == "teardown-leak-canary") {
            return tearDownLeakCanary();
        }
    } catch (const std::exception &e) {
        std::fprintf(stderr, "Exception: %s\n", e.what());

        return 1;
    }

    std::fprintf(stderr, "Unknown scenario: %s\n", argv[1]);

    return 1;
}

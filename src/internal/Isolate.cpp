// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/internal/Isolate.hpp"
#include "../../include/dxfeed_graal_cpp_api/internal/CallbackBarrier.hpp"
#include "../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../include/dxfeed_graal_cpp_api/internal/Handler.hpp"
#include "../../include/dxfeed_graal_cpp_api/internal/JavaObjectHandle.hpp"
#include "../../include/dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <config/config.hpp>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <dxfg_api.h>
#include <exception>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
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
#    if defined(_MSC_VER) && defined(_DLL)
#        include <corecrt_startup.h>
#    endif
#    include <windows.h>
#endif

#ifdef NO_ERROR
#    undef NO_ERROR
#endif

#ifdef _WIN32
std::optional<std::string> getEnvironmentVariable(const std::string &name) {
    std::size_t requiredSize = 0;

    getenv_s(&requiredSize, nullptr, 0, name.c_str());

    if (requiredSize == 0) {
        return std::nullopt;
    }

    std::string buffer(requiredSize, '\0');

    if (getenv_s(&requiredSize, buffer.data(), requiredSize, name.c_str()) != 0) {
        return std::nullopt;
    }

    buffer.resize(requiredSize - 1);

    return buffer;
}
#else
std::optional<std::string> getEnvironmentVariable(const std::string &name) {
    const char *value = std::getenv(name.c_str());

    return value ? std::optional<std::string>(value) : std::nullopt;
}
#endif

DXFCPP_BEGIN_NAMESPACE

Isolate::IsolateThread::IsolateThread(GraalIsolateThreadHandle threadHandle) noexcept
    : handle{threadHandle}, tid{std::this_thread::get_id()} {
    static std::atomic<std::size_t> lastIdx{0};

    this->idx = ++lastIdx;

    if constexpr (Debugger::traceIsolates) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::trace("IsolateThread{" + dxfcpp::toString(handle) + ", tid = " + dxfcpp::toString(tid) +
                        ", idx = " + std::to_string(idx) + "}()");
    }
}
Isolate::IsolateThread::IsolateThread(IsolateThread &&) noexcept = default;

Isolate::IsolateThread &Isolate::IsolateThread::operator=(IsolateThread &&) noexcept = default;

CEntryPointErrorsEnum Isolate::IsolateThread::detach() noexcept {
    if constexpr (Debugger::traceIsolates) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::trace(toString() + "::detach()");
    }

    // OK if nothing is attached.
    if (!handle) {
        if constexpr (Debugger::traceIsolates) {
            // ReSharper disable once CppDFAUnreachableCode
            Debugger::trace(toString() + "::detach(): !handle => Not attached");
        }

        return CEntryPointErrorsEnum::NO_ERROR;
    }

    const auto result =
        static_cast<CEntryPointErrorsEnum>(graal_detach_thread(static_cast<graal_isolatethread_t *>(handle)));

    if (result == CEntryPointErrorsEnum::NO_ERROR) {
        if constexpr (Debugger::traceIsolates) {
            // ReSharper disable once CppDFAUnreachableCode
            Debugger::trace(toString() + "::detach(): result == CEntryPointErrorsEnum::NO_ERROR => Detached");
        }

        handle = nullptr;
        detachOnDestruction = false;
    }

    return result;
}

Isolate::IsolateThread::~IsolateThread() noexcept {
    if constexpr (Debugger::traceIsolates) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::trace(toString() + "::~()");
    }

    // At the process exit the thread is not detached: the other threads may already be ended, and the isolate
    // ends with the process.
    if (detachOnDestruction && !Isolate::isShutDown()) {
        ignoreUnused(detach());
    }
}

std::string Isolate::IsolateThread::toString() const {
    // Without fmt (its formatting is not noexcept): the traces of the noexcept functions call it.
    return "IsolateThread{" + dxfcpp::toString(handle) + ", tid = " + dxfcpp::toString(tid) +
           ", idx = " + std::to_string(idx) + "}";
}

Isolate::IsolateThread &Isolate::currentIsolateThread() noexcept {
    thread_local IsolateThread current{};

    return current;
}

namespace {
// Constant-initialized, so they can be read before the dynamic initialization of the library and after the
// destruction of its statics.
constinit std::atomic<bool> isolateCreated{false};

// The arguments of the isolate and the mutex that orders the setter against the creation. Never destroyed: the setter may
// be called from a static initializer, and the isolate is created at any moment up to the exit of the process.
struct IsolateArguments {
    std::mutex mutex{};
    std::vector<std::string> arguments{};
};

IsolateArguments &isolateArguments() noexcept {
    static auto &result = *new IsolateArguments{};

    return result;
}

// The Graal callbacks return at once (Isolate::isShuttingDown()).
constinit std::atomic<bool> shuttingDown{false};

// The isolate is not entered anymore (Isolate::isShutDown()).
constinit std::atomic<bool> shutDown{false};

// The Graal callbacks of the library in progress (Isolate::CallbackScope), on all threads and on this one.
constinit std::atomic<std::size_t> callbacksInProgress{0};
constinit thread_local std::size_t threadCallbacksInProgress{0};

// How long the exit handler waits for the callbacks in progress: a listener may block, for example, on a lock that
// the exiting thread holds. With the UCRT the handler runs under the exit lock, so a callback that registers an exit
// function meanwhile (for example, initializes a function-local static with a destructor) blocks until the timeout.
constexpr auto CALLBACKS_TIMEOUT = std::chrono::seconds(2);

// The exit handler does not wait when it runs at DLL_PROCESS_DETACH: the other threads are already ended, and a
// thread ended in a callback never leaves it.
constinit std::atomic<bool> waitAtExit{true};

// Waits for the callbacks in progress on the other threads, not for those of this thread: the process may exit from
// a listener. Call it after setting shuttingDown.
void waitForCallbacks() noexcept {
    const auto deadline = std::chrono::steady_clock::now() + CALLBACKS_TIMEOUT;

    while (callbacksInProgress.load() > threadCallbacksInProgress && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

// Registered when the isolate is created (registerExitHandler()), so it runs before the destructors of the statics
// created earlier, and after those of the statics created later (they must not depend on the Graal threads).
void onExit() noexcept {
    shuttingDown = true;

    if (waitAtExit) {
        waitForCallbacks();
    }

    shutDown = true;
}

#if defined(_MSC_VER) && defined(_DLL)
// Sets the flags at DLL_PROCESS_DETACH if onExit() has not run: the exit table of the process is not run when the
// executable has its own C runtime or the process ends by ExitProcess().
void onDetach() noexcept {
    shuttingDown = true;
    shutDown = true;
}
#endif

void registerExitHandler() noexcept {
#if defined(_MSC_VER) && defined(_DLL)
    // std::atexit of a DLL runs the handler at DLL_PROCESS_DETACH, after the statics of the executable are destroyed
    // and the other threads are ended. So the handler goes to the exit table of the process, as std::atexit of an
    // executable does, and the module is pinned: it must not be unloaded before the process exits. onDetach() is
    // registered first: in an executable both go to the same table, and onExit() runs first.
    HMODULE module{};

    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                       reinterpret_cast<LPCWSTR>(&isolateCreated), &module);
    std::atexit(onDetach);
    _crt_atexit(onExit);
#else
#    ifdef _WIN32
    // In a DLL (MinGW, the static C runtime) std::atexit runs the handler at DLL_PROCESS_DETACH.
    HMODULE module{};

    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCWSTR>(&isolateCreated), &module) &&
        module != GetModuleHandleW(nullptr)) {
        waitAtExit = false;
    }
#    endif
    std::atexit(onExit);
#endif
}
} // namespace

Isolate::CallbackScope::CallbackScope() noexcept {
    // Counted before the check: the exit handler sets the flag before it reads the counter.
    callbacksInProgress.fetch_add(1);
    threadCallbacksInProgress++;
    active_ = !shuttingDown.load();
}

Isolate::CallbackScope::~CallbackScope() noexcept {
    threadCallbacksInProgress--;
    callbacksInProgress.fetch_sub(1);
}

Isolate::CallbackScope::operator bool() const noexcept {
    return active_;
}

bool Isolate::isInCallback() noexcept {
    return threadCallbacksInProgress > 0;
}

Isolate::Isolate() noexcept {
    if constexpr (Debugger::traceIsolates) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::trace("Isolate::Isolate()");
    }

    std::vector<std::string> arguments{};

    {
        auto &state = isolateArguments();
        std::lock_guard lock(state.mutex);

        // From now on the setter returns false: the arguments are final.
        isolateCreated.store(true);
        arguments = state.arguments;
    }

    if constexpr (Debugger::traceIsolates) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::trace("Isolate::Isolate(): " + std::to_string(arguments.size()) + " argument(s)");
    }

    graal_isolate_t *graalIsolateHandle{};
    graal_isolatethread_t *graalIsolateThreadHandle{};

    // The parameters only when there are arguments: otherwise the defaults of the SDK, as before. The strings live until
    // the call returns.
    std::string programName{"dxfcpp"};
    std::vector<char *> argv{};
    graal_create_isolate_params_t params;
    graal_create_isolate_params_t *paramsPointer = nullptr;

    if (!arguments.empty()) {
        argv.reserve(arguments.size() + 1);
        // The SDK skips the first one: it is the name of the program.
        argv.push_back(programName.data());

        for (auto &argument : arguments) {
            argv.push_back(argument.data());
        }

        std::memset(&params, 0, sizeof(params));
        params.version = __graal_create_isolate_params_version;
        params.argc = static_cast<int>(argv.size());
        params.argv = argv.data();
        // An unknown option fails the creation instead of being silently dropped.
        params.ignore_unrecognized_args = 0;
        paramsPointer = &params;
    }

    if (const auto result = static_cast<CEntryPointErrorsEnum>(
            graal_create_isolate(paramsPointer, &graalIsolateHandle, &graalIsolateThreadHandle));
        result == CEntryPointErrorsEnum::NO_ERROR) {

        handle_ = graalIsolateHandle;
        currentIsolateThread().handle = graalIsolateThreadHandle;
    }

    if constexpr (Debugger::traceIsolates) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::trace("Isolate::Isolate() -> " + std::string("Isolate{") + dxfcpp::toString(handle_) +
                        ", current = " + currentIsolateThread().toString() + "}");
    }
}

CEntryPointErrorsEnum Isolate::attach() const noexcept {
    if constexpr (Debugger::traceIsolates) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::trace(toString() + "::attach()");
    }

    // We will not re-attach.
    if (!currentIsolateThread().handle) {
        if constexpr (Debugger::traceIsolates) {
            // ReSharper disable once CppDFAUnreachableCode
            Debugger::trace(toString() + "::attach(): !currentIsolateThread().handle => Needs to be attached.");
        }

        graal_isolatethread_t *newIsolateThreadHandle{};

        if (auto result = static_cast<CEntryPointErrorsEnum>(
                graal_attach_thread(dxfcpp::bit_cast<graal_isolate_t *>(handle_), &newIsolateThreadHandle));
            result != CEntryPointErrorsEnum::NO_ERROR) {

            if constexpr (Debugger::traceIsolates) {
                // ReSharper disable once CppDFAUnreachableCode
                Debugger::trace(toString() + "::attach(): result != CEntryPointErrorsEnum::NO_ERROR [" +
                                std::to_string(static_cast<std::underlying_type_t<CEntryPointErrorsEnum>>(result)) +
                                "] " + CEntryPointErrorsEnumToStr(result));
            }

            return result;
        }

        currentIsolateThread().handle = newIsolateThreadHandle;
        currentIsolateThread().detachOnDestruction = true;

        if constexpr (Debugger::traceIsolates) {
            // ReSharper disable once CppDFAUnreachableCode
            Debugger::trace(toString() + "::attach(): Attached: " + currentIsolateThread().toString());
        }
    } else {
        if constexpr (Debugger::traceIsolates) {
            // ReSharper disable once CppDFAUnreachableCode
            Debugger::trace(toString() + "::attach(): Cached: " + currentIsolateThread().toString());
        }
    }

    return CEntryPointErrorsEnum::NO_ERROR;
}

GraalIsolateThreadHandle Isolate::get() const noexcept {
    if constexpr (Debugger::traceIsolates) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::trace(toString() + "::get()");
    }

    // ReSharper disable once CppRedundantCastExpression
    return static_cast<void *>(graal_get_current_thread(dxfcpp::bit_cast<graal_isolate_t *>(handle_)));
}

void Isolate::init(Isolate &isolate) noexcept {
    namespace otc = org::ttldtor::config;
    static auto ENV_PREFIX = "DXFEED_";
    static auto DXFEED_SYSTEM_PROPERTIES = "dxfeed.system.properties";

    if constexpr (Debugger::traceIsolates) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::trace("init()");
    }

    const auto propsFile = [] {
        const auto fileNameEnv = std::string(ENV_PREFIX) + DXFEED_SYSTEM_PROPERTIES;

        return getEnvironmentVariable(fileNameEnv).value_or(std::string(DXFEED_SYSTEM_PROPERTIES));
    }();

    otc::Config config{};

    config.addSource(otc::IniSource(propsFile));
    config.addSource(otc::EnvSource(ENV_PREFIX));

    for (auto &&[k, v] : config) {
        isolate.runIsolated([key = k, value = v](auto threadHandle) {
            return isolated::setSystemProperty(static_cast<graal_isolatethread_t *>(threadHandle), key.c_str(),
                                               value.c_str());
        });
    }
}

Isolate &Isolate::getInstance() noexcept {
    if constexpr (Debugger::traceIsolates) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::trace("Isolate::getInstance()");
    }

    // Never deleted: see the description of the function.
    static Isolate &instance = *new Isolate{};
    static std::once_flag flag{};

    std::call_once(flag, []() {
        init(instance);
        registerExitHandler();
    });

    if constexpr (Debugger::traceIsolates) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::trace("Isolate::getInstance() -> " + instance.toString());
    }

    return instance;
}

bool Isolate::isCreated() noexcept {
    return isolateCreated.load();
}

bool Isolate::setArguments(std::vector<std::string> arguments) {
    // A string with a NUL would be cut by the C string of argv without a word. The value is not in the message.
    for (std::size_t i = 0; i < arguments.size(); i++) {
        if (arguments[i].find('\0') != std::string::npos) {
            throw InvalidArgumentException("The isolate argument #" + std::to_string(i) +
                                           " contains a NUL character.");
        }
    }

    auto &state = isolateArguments();
    std::lock_guard lock(state.mutex);

    if (isolateCreated.load()) {
        return false;
    }

    state.arguments = std::move(arguments);

    return true;
}

bool Isolate::isShuttingDown() noexcept {
    return shuttingDown.load(std::memory_order_acquire);
}

bool Isolate::isShutDown() noexcept {
    return shutDown.load(std::memory_order_acquire);
}

namespace {

// The objects that releaseOutsideCallbacks() passes to the release thread. Never destroyed: the thread waits on it
// until the process ends.
struct ReleaseQueue {
    std::mutex mutex{};
    std::condition_variable changed{};
    std::deque<std::shared_ptr<void>> objects{};
    bool started{};
    bool releasing{};
    bool stopped{};
};

ReleaseQueue &getReleaseQueue() noexcept {
    static auto &queue = *new ReleaseQueue{};

    return queue;
}

// The release thread is not joined: after stopReleaseThread() it waits forever, so it does not leave the isolate while
// the library is shutting down.
void runReleaseThread() noexcept {
    auto &queue = getReleaseQueue();
    std::unique_lock lock{queue.mutex};

    while (true) {
        queue.changed.wait(lock, [&queue] {
            return !queue.objects.empty();
        });

        auto object = std::move(queue.objects.front());

        queue.objects.pop_front();
        queue.releasing = true;
        lock.unlock();
        object.reset();
        lock.lock();
        queue.releasing = false;
        queue.changed.notify_all();
    }
}

// Releases the queued objects (they hold native handles) before the handles are released by the shutdown. The objects
// passed later are released at once. Waits no longer than for the callbacks: the release waits for the calls of the
// listener in progress, and the shutdown may be called from a listener.
void stopReleaseThread() noexcept {
    auto &queue = getReleaseQueue();
    std::unique_lock lock{queue.mutex};

    queue.stopped = true;
    queue.changed.wait_for(lock, CALLBACKS_TIMEOUT, [&queue] {
        return queue.objects.empty() && !queue.releasing;
    });
}

} // namespace

void releaseOutsideCallbacks(std::shared_ptr<void> object) noexcept {
    if (!object) {
        return;
    }

    std::exception_ptr error{};

    if (ListenerCallScope::isActive()) {
        auto &queue = getReleaseQueue();
        std::lock_guard lock{queue.mutex};

        if (!queue.stopped) {
            try {
                if (!queue.started) {
                    std::thread{runReleaseThread}.detach();
                    queue.started = true;
                }

                queue.objects.push_back(std::move(object));
                queue.changed.notify_all();

                return;
            } catch (...) {
                // The thread could not be started or the queue could not grow: the object is released at once.
                error = std::current_exception();
            }
        }
    }

    if (error) {
        ErrorManager::reportException("Isolate", error);
    }

    object.reset();
}

void Isolate::shutdown() {
    // A concurrent call waits for the first one. Never destroyed: shutdown() may be called from a static destructor.
    static auto &mutex = *new std::mutex{};
    std::lock_guard lock(mutex);

    // A second call, or the exit handler has run.
    if (isShutDown()) {
        return;
    }

    // The new callbacks return at once.
    shuttingDown = true;
    waitForCallbacks();
    stopReleaseThread();

    if (isCreated()) {
        // Releasing the last handle of a native listener deactivates it: the release waits for its callbacks in
        // progress, and the listener is not called afterwards.
        JavaObjectHandleRegistry::getInstance().releaseAll();
        ignoreUnused(getInstance().runIsolated([](auto threadHandle) {
            auto *thread = static_cast<graal_isolatethread_t *>(threadHandle);

            if (dxfg_system_close_all_and_await_termination(thread) != 0) {
                isolated::clearThreadException(thread);
            }

            return 0;
        }));
        // The handles created while the endpoints were closing.
        JavaObjectHandleRegistry::getInstance().releaseAll();
    }

    shutDown = true;
}

std::string Isolate::toString() const {
    // Without fmt, as IsolateThread::toString().
    return "Isolate{" + dxfcpp::toString(handle_) + ", current = " + currentIsolateThread().toString() + "}";
}

DXFCPP_END_NAMESPACE

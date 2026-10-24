// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/internal/Isolate.hpp"
#include "../../include/dxfeed_graal_cpp_api/internal/JavaObjectHandle.hpp"

#include <atomic>
#include <config/config.hpp>
#include <cstdlib>
#include <dxfg_api.h>
#include <mutex>
#include <optional>
#include <string>
#include <utility>

#include <fmt/format.h>

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
    this->idx = Id<IsolateThread>::getNext().getValue();

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
    return fmt::format("IsolateThread{{{}, tid = {}, idx = {}}}", dxfcpp::toString(handle), dxfcpp::toString(tid), idx);
}

Isolate::IsolateThread &Isolate::currentIsolateThread() noexcept {
    thread_local IsolateThread current{};

    return current;
}

namespace {
// Constant-initialized, so they can be read before the dynamic initialization of the library and after the
// destruction of its statics.
constinit std::atomic<bool> isolateCreated{false};

// The Graal callbacks return at once (Isolate::isShuttingDown()).
constinit std::atomic<bool> shuttingDown{false};

// The isolate is not entered anymore (Isolate::isShutDown()).
constinit std::atomic<bool> shutDown{false};

// Registered with std::atexit when the isolate is created, so it runs before the destructors of the statics
// created earlier, and after those of the statics created later (they must not depend on the Graal threads).
// In a Windows DLL it runs at DLL_PROCESS_DETACH, when the other threads are already ended.
void onExit() noexcept {
    shuttingDown = true;
    shutDown = true;
}
} // namespace

Isolate::Isolate() noexcept {
    if constexpr (Debugger::traceIsolates) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::trace("Isolate::Isolate()");
    }

    isolateCreated.store(true);

    graal_isolate_t *graalIsolateHandle{};
    graal_isolatethread_t *graalIsolateThreadHandle{};

    if (const auto result = static_cast<CEntryPointErrorsEnum>(
            graal_create_isolate(nullptr, &graalIsolateHandle, &graalIsolateThreadHandle));
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
            return static_cast<CEntryPointErrorsEnum>(dxfg_system_set_property(
                       static_cast<graal_isolatethread_t *>(threadHandle), key.c_str(), value.c_str())) ==
                   CEntryPointErrorsEnum::NO_ERROR;
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
        std::atexit(onExit);
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

bool Isolate::isShuttingDown() noexcept {
    return shuttingDown.load(std::memory_order_acquire);
}

bool Isolate::isShutDown() noexcept {
    return shutDown.load(std::memory_order_acquire);
}

void Isolate::shutdown() {
    // A concurrent call waits for the first one. Never destroyed: shutdown() may be called from a static destructor.
    static auto &mutex = *new std::mutex{};
    std::lock_guard lock(mutex);

    // A second call, or the exit handler has run.
    if (isShutDown()) {
        return;
    }

    // The callbacks in progress end, the new ones return at once.
    shuttingDown = true;

    if (isCreated()) {
        // Releasing the last handle of a native listener deactivates it: the release waits for its callbacks in
        // progress, and the listener is not called afterwards.
        JavaObjectHandleRegistry::getInstance().releaseAll();
        ignoreUnused(getInstance().runIsolated([](auto threadHandle) {
            return dxfg_system_close_all_and_await_termination(static_cast<graal_isolatethread_t *>(threadHandle));
        }));
        // The handles created while the endpoints were closing.
        JavaObjectHandleRegistry::getInstance().releaseAll();
    }

    shutDown = true;
}

std::string Isolate::toString() const {
    return fmt::format("Isolate{{{}, current = {}}}", dxfcpp::toString(handle_), currentIsolateThread().toString());
}

DXFCPP_END_NAMESPACE

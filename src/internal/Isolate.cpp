// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/api.hpp>

#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>

#include <config/config.hpp>

#include <cstdlib>

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
    }

    return result;
}

CEntryPointErrorsEnum Isolate::IsolateThread::detachAllThreadsAndTearDownIsolate() noexcept {
    if constexpr (Debugger::traceIsolates) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::trace(toString() + "::detachAllThreadsAndTearDownIsolate()");
    }

    if (!handle) {
        if constexpr (Debugger::traceIsolates) {
            // ReSharper disable once CppDFAUnreachableCode
            Debugger::trace(toString() + "::detachAllThreadsAndTearDownIsolate(): !handle => Not attached");
        }

        return CEntryPointErrorsEnum::NO_ERROR;
    }

    const auto result = static_cast<CEntryPointErrorsEnum>(
        graal_detach_all_threads_and_tear_down_isolate(static_cast<graal_isolatethread_t *>(handle)));

    if (result == CEntryPointErrorsEnum::NO_ERROR) {
        if constexpr (Debugger::traceIsolates) {
            // ReSharper disable once CppDFAUnreachableCode
            Debugger::trace(toString() +
                            "::detachAllThreadsAndTearDownIsolate(): CEntryPointErrorsEnum::NO_ERROR => All "
                            "threads have been detached. The isolate has been teared down.");
        }

        handle = nullptr;
    }

    return result;
}

Isolate::Isolate() noexcept : mtx_{} {
    if constexpr (Debugger::traceIsolates) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::trace("Isolate::Isolate()");
    }

    graal_isolate_t *graalIsolateHandle{};
    graal_isolatethread_t *graalIsolateThreadHandle{};

    if (const auto result = static_cast<CEntryPointErrorsEnum>(
            graal_create_isolate(nullptr, &graalIsolateHandle, &graalIsolateThreadHandle));
        result == CEntryPointErrorsEnum::NO_ERROR) {

        handle_ = graalIsolateHandle;
        currentIsolateThread_.handle = graalIsolateThreadHandle;
    }

    if constexpr (Debugger::traceIsolates) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::trace("Isolate::Isolate() -> " + std::string("Isolate{") + dxfcpp::toString(handle_) +
                        ", current = " + currentIsolateThread_.toString() + "}");
    }
}

CEntryPointErrorsEnum Isolate::attach() const noexcept {
    if constexpr (Debugger::traceIsolates) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::trace(toString() + "::attach()");
    }

    // We will not re-attach.
    if (!currentIsolateThread_.handle) {
        if constexpr (Debugger::traceIsolates) {
            // ReSharper disable once CppDFAUnreachableCode
            Debugger::trace(toString() + "::attach(): !currentIsolateThread_.handle => Needs to be attached.");
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

        currentIsolateThread_.handle = newIsolateThreadHandle;

        if constexpr (Debugger::traceIsolates) {
            // ReSharper disable once CppDFAUnreachableCode
            Debugger::trace(toString() + "::attach(): Attached: " + currentIsolateThread_.toString());
        }
    } else {
        if constexpr (Debugger::traceIsolates) {
            // ReSharper disable once CppDFAUnreachableCode
            Debugger::trace(toString() + "::attach(): Cached: " + currentIsolateThread_.toString());
        }
    }

    return CEntryPointErrorsEnum::NO_ERROR;
}

GraalIsolateThreadHandle Isolate::get() noexcept {
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

DXFCPP_END_NAMESPACE
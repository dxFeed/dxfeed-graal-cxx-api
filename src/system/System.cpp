// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/system/System.hpp"

#include "../../include/dxfeed_graal_c_api/api.h"
#include "../../include/dxfeed_graal_cpp_api/exceptions/RuntimeException.hpp"
#include "../internal/Isolate.hpp"
#include "../internal/utils/debug/Debug.hpp"
#include "../isolated/IsolatedCommon.hpp"
#include "../isolated/internal/IsolatedString.hpp"

#include <dxfg_v4_api.h>
#include <fmt/format.h>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utf8.h>

DXFCPP_BEGIN_NAMESPACE

namespace isolated {
bool setSystemProperty(graal_isolatethread_t *thread, const char *key, const char *value) noexcept {
    if (dxfg_v4_system_set_property(thread, key, value) == DXFG_EXECUTE_SUCCESSFULLY) {
        return true;
    }

    clearThreadException(thread);

    return false;
}
} // namespace isolated

namespace {

// The longest valid UTF-8 prefix of a property value from the SDK.
std::string validUtf8Prefix(std::string_view s) {
    return std::string{s.begin(), utf8::find_invalid(s.begin(), s.end())};
}

// The creation of the isolate failed: GraalException with the code and the reason, instead of the fallback value that
// hides the failure.
void throwIfIsolateFailed() {
    ignoreUnused(Isolate::getInstance());
    Isolate::throwCreationError();
}

} // namespace

// System::getRuntimeStatistics() maps the 18 values one to one.
static_assert(sizeof(dxfg_v4_runtime_stats_t) == 18 * sizeof(std::int64_t),
              "dxfg_v4_runtime_stats_t changed: update RuntimeStatistics");

RuntimeStatistics System::getRuntimeStatistics() {
    throwIfIsolateFailed();

    // Defensive: the minimum version of the SDK supports the feature.
    if (isolated::runGraalV4FunctionWithResult(dxfg_v4_system_is_feature_supported,
                                               DXFG_V4_FEATURE_RUNTIME_STATISTICS) == 0) {
        throw RuntimeException(fmt::format("The runtime statistics (the feature '{}') are not supported by the "
                                           "Graal Native SDK library {}",
                                           DXFG_V4_FEATURE_RUNTIME_STATISTICS, Isolate::getGraalNativeSdkVersion()));
    }

    dxfg_v4_runtime_stats_t stats{};

    isolated::runGraalV4Function(dxfg_v4_system_get_runtime_statistics, &stats);

    return RuntimeStatistics{
        .heapUsedBytes = stats.heap_used_bytes,
        .heapCommittedBytes = stats.heap_committed_bytes,
        .heapMaxBytes = stats.heap_max_bytes,
        .youngGcCount = stats.young_gc_count,
        .youngGcTimeMs = stats.young_gc_time_ms,
        .fullGcCount = stats.full_gc_count,
        .fullGcTimeMs = stats.full_gc_time_ms,
        .threadCount = stats.thread_count,
        .peakThreadCount = stats.peak_thread_count,
        .processCpuTimeNs = stats.process_cpu_time_ns,
        .uptimeMs = stats.uptime_ms,
        .allocatedBytes = stats.allocated_bytes,
        .edenUsedBytes = stats.eden_used_bytes,
        .survivorUsedBytes = stats.survivor_used_bytes,
        .oldUsedBytes = stats.old_used_bytes,
        .daemonThreadCount = stats.daemon_thread_count,
        .totalStartedThreadCount = stats.total_started_thread_count,
        .availableProcessors = stats.available_processors,
    };
}

std::optional<std::string> System::getBuildInfo(StringParam key) {
    throwIfIsolateFailed();

    const isolated::SdkString graalKey{key.view(), "System::getBuildInfo(key)"};

    // Defensive: the minimum version of the SDK supports the feature.
    if (isolated::runGraalV4FunctionWithResult(dxfg_v4_system_is_feature_supported, DXFG_V4_FEATURE_BUILD_INFO) == 0) {
        throw RuntimeException(fmt::format("The build info (the feature '{}') is not supported by the Graal Native SDK "
                                           "library {}",
                                           DXFG_V4_FEATURE_BUILD_INFO, Isolate::getGraalNativeSdkVersion()));
    }

    // NULL: an unknown key or a value that the library does not know.
    const auto value = isolated::internal::IsolatedString::toUnique(
        isolated::runGraalV4FunctionWithResult(dxfg_v4_system_get_build_info, graalKey.c_str()));

    if (!value) {
        return std::nullopt;
    }

    return validUtf8Prefix(value.get());
}

bool System::setProperty(StringParam key, StringParam value) {
    throwIfIsolateFailed();

    const std::string_view keyView = key.view();
    const std::string_view valueView = value.view();

    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("System::setProperty(key = '" + std::string{keyView} + "', value = '" + std::string{valueView} +
                        "')");
    }

    auto result = runIsolatedOrElse(
        [keyView, valueView](auto threadHandle) {
            return isolated::setSystemProperty(
                static_cast<graal_isolatethread_t *>(threadHandle),
                isolated::SdkString{keyView, "System::setProperty(key)"}.c_str(),
                isolated::SdkString{valueView, StringLabel::ofPropertyValue("System::setProperty(value)", keyView)}
                    .c_str());
        },
        false);

    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("System::setProperty(key = '" + std::string{keyView} + "', value = '" + std::string{valueView} +
                        "') -> " + toString(result));
    }

    return result;
}

std::string System::getProperty(StringParam key) {
    throwIfIsolateFailed();

    const std::string_view keyView = key.view();

    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("System::getProperty(key = " + std::string{keyView} + ")");
    }

    auto result = runIsolatedOrElse(
        [keyView](auto threadHandle) {
            auto *thread = static_cast<graal_isolatethread_t *>(threadHandle);
            std::string resultString{};
            const char *property = nullptr;

            // A failure is an empty result, as a property that is not set (NULL).
            if (dxfg_v4_system_get_property(thread, isolated::SdkString{keyView, "System::getProperty(key)"}.c_str(),
                                            &property) != DXFG_EXECUTE_SUCCESSFULLY) {
                isolated::clearThreadException(thread);
            } else if (property != nullptr) {
                resultString = validUtf8Prefix(property);

                if (dxfg_v4_system_release_property(thread, property) != DXFG_EXECUTE_SUCCESSFULLY) {
                    isolated::clearThreadException(thread);
                }
            }

            return resultString;
        },
        std::string{});

    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("System::getProperty(key = '" + std::string{keyView} + "') -> '" + result + "'");
    }

    return result;
}

bool System::setIsolateArguments(std::vector<std::string> arguments) {
    return Isolate::setArguments(std::move(arguments));
}

std::string RuntimeStatistics::toString() const {
    return fmt::format(
        "RuntimeStatistics{{heapUsedBytes = {}, heapCommittedBytes = {}, heapMaxBytes = {}, youngGcCount = {}, "
        "youngGcTimeMs = {}, fullGcCount = {}, fullGcTimeMs = {}, threadCount = {}, peakThreadCount = {}, "
        "processCpuTimeNs = {}, uptimeMs = {}, allocatedBytes = {}, edenUsedBytes = {}, survivorUsedBytes = {}, "
        "oldUsedBytes = {}, daemonThreadCount = {}, totalStartedThreadCount = {}, availableProcessors = {}}}",
        heapUsedBytes, heapCommittedBytes, heapMaxBytes, youngGcCount, youngGcTimeMs, fullGcCount, fullGcTimeMs,
        threadCount, peakThreadCount, processCpuTimeNs, uptimeMs, allocatedBytes, edenUsedBytes, survivorUsedBytes,
        oldUsedBytes, daemonThreadCount, totalStartedThreadCount, availableProcessors);
}

void shutdown() {
    Isolate::shutdown();
}

DXFCPP_END_NAMESPACE

dxfc_error_code_t dxfc_system_set_property(const char *key, const char *value) {
    try {
        return dxfcpp::System::setProperty(key, value) ? DXFC_EC_SUCCESS : DXFC_EC_ERROR;
    } catch (...) {
        // For example, the creation of the isolate failed: an exception does not leave the C function.
        return DXFC_EC_ERROR;
    }
}

// ReSharper disable once CppDFAConstantFunctionResult
dxfc_error_code_t dxfc_system_get_property(const char *key, char *buffer, size_t bufferSize) {
    std::string value;

    try {
        value = dxfcpp::System::getProperty(key);
    } catch (...) {
        // For example, the creation of the isolate failed: an exception does not leave the C function.
        if (buffer != nullptr && bufferSize > 0ULL) {
            buffer[0] = '\0';
        }

        return DXFC_EC_ERROR;
    }

    if (buffer != nullptr && bufferSize > 0ULL) {
        auto size = std::min(value.size(), bufferSize - 1);

        buffer[size] = '\0';

        if (size == 0ULL) {
            return DXFC_EC_SUCCESS;
        }

        memcpy(buffer, value.c_str(), size);

        const auto begin = buffer;
        const auto end = buffer + size;

        // Looks for the first invalid UTF-8 character.
        // An invalid character position was found. Trim the result string buffer to this invalid character.
        if (const auto found = utf8::find_invalid(begin, end); found != end) {
            *found = '\0';
        }
    }

    return DXFC_EC_SUCCESS;
}
// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/system/System.hpp"

#include "../../include/dxfeed_graal_c_api/api.h"
#include "../internal/Isolate.hpp"
#include "../internal/utils/debug/Debug.hpp"
#include "../isolated/IsolatedCommon.hpp"

#include <dxfg_api.h>
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
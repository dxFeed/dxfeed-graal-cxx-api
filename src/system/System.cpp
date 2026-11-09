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

#ifdef NO_ERROR
#    undef NO_ERROR
#endif

DXFCPP_BEGIN_NAMESPACE

namespace isolated {
bool setSystemProperty(graal_isolatethread_t *thread, const char *key, const char *value) noexcept {
    if (static_cast<CEntryPointErrorsEnum>(dxfg_system_set_property(thread, key, value)) ==
        CEntryPointErrorsEnum::NO_ERROR) {
        return true;
    }

    clearThreadException(thread);

    return false;
}
} // namespace isolated

namespace {

// The longest valid UTF-8 prefix of the string: the SDK would replace the invalid bytes with U+FFFD.
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
        [key = validUtf8Prefix(keyView), value = validUtf8Prefix(valueView)](auto threadHandle) {
            return isolated::setSystemProperty(static_cast<graal_isolatethread_t *>(threadHandle),
                                               isolated::SdkString{key}.c_str(), isolated::SdkString{value}.c_str());
        },
        false);

    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("System::setProperty(key = '" + std::string{keyView} + "', value = '" + std::string{valueView} +
                        "') -> " + toString(result));
    }

    return result;
}

std::string System::getProperty(StringParam key, bool) {
    throwIfIsolateFailed();

    const std::string_view keyView = key.view();

    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("System::getProperty(key = " + std::string{keyView} + ")");
    }

    auto result = runIsolatedOrElse(
        [key = validUtf8Prefix(keyView)](auto threadHandle) {
            auto *thread = static_cast<graal_isolatethread_t *>(threadHandle);
            std::string resultString{};

            // NULL is also the result for a property that is not set.
            if (auto property = dxfg_system_get_property(thread, isolated::SdkString{key}.c_str());
                property != nullptr) {
                resultString = validUtf8Prefix(property);

                if (dxfg_system_release_property(thread, property) != 0) {
                    isolated::clearThreadException(thread);
                }
            } else {
                isolated::clearThreadException(thread);
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

void temporaryShutdown() {
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
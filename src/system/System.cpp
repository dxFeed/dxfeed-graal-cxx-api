// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/system/System.hpp"

#include "../../include/dxfeed_graal_c_api/api.h"
#include "../../include/dxfeed_graal_cpp_api/internal/Isolate.hpp"

#include <dxfg_api.h>
#include <string>
#include <utf8.h>

#ifdef NO_ERROR
#    undef NO_ERROR
#endif

DXFCPP_BEGIN_NAMESPACE

bool System::setProperty(const StringLike &key, const StringLike &value) {
    // TODO: check invalid utf-8 [EN-8233]
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("System::setProperty(key = '" + key + "', value = '" + value + "')");
    }

    auto result = runIsolatedOrElse(
        [key = key, value = value](auto threadHandle) {
            return static_cast<CEntryPointErrorsEnum>(dxfg_system_set_property(
                       static_cast<graal_isolatethread_t *>(threadHandle), key.c_str(), value.c_str())) ==
                   CEntryPointErrorsEnum::NO_ERROR;
        },
        false);

    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("System::setProperty(key = '" + key + "', value = '" + value + "') -> " + toString(result));
    }

    return result;
}

std::string System::getProperty(const StringLike &key) {
    // TODO: check invalid utf-8 [EN-8233]
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("System::getProperty(key = " + key + ")");
    }

    auto result = runIsolatedOrElse(
        [key = key](auto threadHandle) {
            std::string resultString{};

            if (auto result = dxfg_system_get_property(static_cast<graal_isolatethread_t *>(threadHandle), key.c_str());
                result != nullptr) {
                resultString = result;
                dxfg_system_release_property(static_cast<graal_isolatethread_t *>(threadHandle), result);
            }

            return resultString;
        },
        std::string{});

    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("System::getProperty(key = '" + key + "') -> '" + result + "'");
    }

    return result;
}

DXFCPP_END_NAMESPACE

dxfc_error_code_t dxfc_system_set_property(const char *key, const char *value) {
    // TODO: check invalid utf-8 [EN-8233]

    return dxfcpp::System::setProperty(key, value) ? DXFC_EC_SUCCESS : DXFC_EC_ERROR;
}

// ReSharper disable once CppDFAConstantFunctionResult
dxfc_error_code_t dxfc_system_get_property(const char *key, char *buffer, size_t bufferSize) {
    auto value = dxfcpp::System::getProperty(key);

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
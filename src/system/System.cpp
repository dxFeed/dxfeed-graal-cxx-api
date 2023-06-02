// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <string>
#include <utf8.h>

#ifdef NO_ERROR
#    undef NO_ERROR
#endif

namespace dxfcpp {

bool System::setProperty(const std::string &key, const std::string &value) {
    // TODO: check invalid utf-8
    if constexpr (Debugger::isDebug) {
        Debugger::debug("System::setProperty(key = '" + key + "', value = '" + value + "')");
    }

    auto result = runIsolatedOrElse(
        [key = key, value = value](auto threadHandle) {
            return CEntryPointErrors::valueOf(dxfg_system_set_property(
                       dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), key.c_str(), value.c_str())) ==
                   CEntryPointErrors::NO_ERROR;
        },
        false);

    if constexpr (Debugger::isDebug) {
        Debugger::debug("System::setProperty(key = '" + key + "', value = '" + value + "') -> " + toString(result));
    }

    return result;
}

std::string System::getProperty(const std::string &key) {
    // TODO: check invalid utf-8
    if constexpr (Debugger::isDebug) {
        Debugger::debug("System::getProperty(key = " + key + ")");
    }

    auto result = runIsolatedOrElse(
        [key = key](auto threadHandle) {
            std::string resultString{};

            if (auto result =
                    dxfg_system_get_property(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), key.c_str());
                result != nullptr) {
                resultString = result;
                dxfg_system_release_property(dxfcpp::bit_cast<graal_isolatethread_t *>(threadHandle), result);
            }

            return resultString;
        },
        std::string{});

    if constexpr (Debugger::isDebug) {
        Debugger::debug("System::getProperty(key = '" + key + "') -> '" + result + "'");
    }

    return result;
}

} // namespace dxfcpp

dxfc_error_code_t dxfc_system_set_property(const char *key, const char *value) {
    // TODO: check invalid utf-8

    return dxfcpp::System::setProperty(key, value) ? DXFC_EC_SUCCESS : DXFC_EC_ERROR;
}

dxfc_error_code_t dxfc_system_get_property(const char *key, char *buffer, size_t bufferSize) {
    auto value = dxfcpp::System::getProperty(key);

    if (buffer != nullptr && bufferSize > 0ULL) {
        auto size = std::min(value.size(), bufferSize - 1);

        buffer[size] = '\0';

        if (size == 0ULL) {
            return DXFC_EC_SUCCESS;
        }

        memcpy(buffer, value.c_str(), size);

        auto begin = buffer;
        auto end = buffer + size;

        // Looks for the first invalid UTF-8 character.
        auto found = utf8::find_invalid(begin, end);

        // An invalid character position was found. Trim the result string buffer to this invalid character.
        if (found != end) {
            *found = '\0';
        }
    }

    return DXFC_EC_SUCCESS;
}
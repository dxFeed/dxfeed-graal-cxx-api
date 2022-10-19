// SPDX-License-Identifier: MPL-2.0

#include <dxFeedNativeAPI.h>
#include <dxFeedNativeCppAPI.hpp>

#include <utf8cpp/utf8.h>

dxfc_error_code_t dxfc_system_set_property(const char *key, const char *value) {
    dxfcpp::System::setProperty(key, value);

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_system_get_property(const char *key, char *buffer, size_t bufferSize) {
    auto value = dxfcpp::System::getProperty(key);

    if (buffer != nullptr && bufferSize > 0ULL) {
        auto size = std::min(value.size(), bufferSize - 1);

        if (size == 0ULL) {
            return DXFC_EC_SUCCESS;
        }

        memcpy(buffer, value.c_str(), size);
        buffer[size] = '\0';

        auto begin = buffer;
        auto end = buffer + size;

        auto found = utf8::find_invalid(begin, end);

        if (found != end) {
            *found = '\0';
        }

    }

    return DXFC_EC_SUCCESS;
}
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <dxf_graal_system.h>
#include <dxf_graal_utils.h>

#include <string>

namespace dxfcpp {
struct System {
    static bool setProperty(const std::string &key, const std::string &value) {
        graal_isolate_t *isolate;
        graal_isolatethread_t *graalvmThread;
        graal_create_isolate(nullptr, &isolate, &graalvmThread);

        return dxf_graal_system_set_property(graalvmThread, key.c_str(), value.c_str()) == DX_EC_SUCCESS;
    }

    static std::string getProperty(const std::string &key) {
        graal_isolate_t *isolate;
        graal_isolatethread_t *graalvmThread;
        graal_create_isolate(nullptr, &isolate, &graalvmThread);

        std::string resultString{};

        if (auto result = dxf_graal_system_get_property(graalvmThread, key.c_str())) {
            resultString = result;
            dxf_graal_utils_free(graalvmThread, static_cast<void *>(const_cast<char *>(result)));
        }

        return resultString;
    }
};

} // namespace dxfcpp

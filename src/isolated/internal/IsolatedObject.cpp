// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "IsolatedObject.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../internal/CallbackBarrier.hpp"
#include "../IsolatedCommon.hpp"
#include "IsolatedString.hpp"

#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::internal {

void freeGraal(void *pointer) {
    if (!pointer) {
        return;
    }

    runGraalV4Function(dxfg_v4_free, pointer);
}

namespace IsolatedObject {

/// dxfg_v4_Object_toString
/* const char* */ std::string toString(/* dxfg_v4_java_object_handler* */ void *object) {
    if (!object) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_Object_toString`. The `object` is null");
    }

    const auto value = IsolatedString::toUnique(runGraalV4FunctionWithNonNullResult(
        dxfg_v4_Object_toString, static_cast<dxfg_v4_java_object_handler *>(object)));
    return dxfcpp::toString(value.get());
}

/// dxfg_v4_Object_equals
std::int32_t equals(/* dxfg_v4_java_object_handler* */ void *object, /* dxfg_v4_java_object_handler* */ void *other) {
    if (!object) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_Object_equals`. The `object` is null");
    }

    if (!other) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_Object_equals`. The `other` is null");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_Object_equals, static_cast<dxfg_v4_java_object_handler *>(object),
                                        static_cast<dxfg_v4_java_object_handler *>(other));
}

/// dxfg_v4_Object_hashCode
/* int32_t */ std::size_t hashCode(/* dxfg_v4_java_object_handler* */ void *object) {
    if (!object) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_Object_hashCode`. The `object` is null");
    }

    return static_cast<std::size_t>(
        runGraalV4FunctionWithResult(dxfg_v4_Object_hashCode, static_cast<dxfg_v4_java_object_handler *>(object)));
}

bool release(void *handle) noexcept {
    if (!handle) {
        return false;
    }

    bool released = false;

    runRelease("JavaObjectHandle", [handle, &released] {
        released = runIsolatedOrElse(
            [handle](auto threadHandle) {
                auto *thread = static_cast<graal_isolatethread_t *>(threadHandle);

                throwIfFailed(thread, dxfg_v4_JavaObjectHandler_release(
                                          thread, static_cast<dxfg_v4_java_object_handler *>(handle)));

                return true;
            },
            false);
    });

    return released;
}

bool List::release(void *handle) noexcept {
    if (!handle) {
        return false;
    }

    bool released = false;

    runRelease("JavaObjectHandle", [handle, &released] {
        released = runIsolatedOrElse(
            [handle](auto threadHandle) {
                auto *thread = static_cast<graal_isolatethread_t *>(threadHandle);

                throwIfFailed(thread, dxfg_v4_java_object_handler_list_release(
                                          thread, static_cast<dxfg_v4_java_object_handler_list *>(handle)));

                return true;
            },
            false);
    });

    return released;
}

} // namespace IsolatedObject
} // namespace isolated::internal

DXFCPP_END_NAMESPACE
// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>
#include <dxfeed_graal_cpp_api/isolated/internal/IsolatedObject.hpp>
#include <dxfeed_graal_cpp_api/isolated/internal/IsolatedString.hpp>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::internal {
namespace IsolatedObject {

/// dxfg_Object_toString
/* const char* */ std::string toString(/* dxfg_java_object_handler* */ void *object) {
    if (!object) {
        throw InvalidArgumentException("Unable to execute function `dxfg_Object_toString`. The `object` is null");
    }

    auto value =
        runGraalFunctionAndThrowIfNullptr(dxfg_Object_toString, static_cast<dxfg_java_object_handler *>(object));
    auto result = dxfcpp::toString(value);

    IsolatedString::release(value);

    return result;
}

/// dxfg_Object_equals
std::int32_t equals(/* dxfg_java_object_handler* */ void *object, /* dxfg_java_object_handler* */ void *other) {
    if (!object) {
        throw InvalidArgumentException("Unable to execute function `dxfg_Object_equals`. The `object` is null");
    }

    if (!other) {
        throw InvalidArgumentException("Unable to execute function `dxfg_Object_equals`. The `other` is null");
    }

    return runGraalFunctionAndThrowIfMinusMin(dxfg_Object_equals, static_cast<dxfg_java_object_handler *>(object),
                                              static_cast<dxfg_java_object_handler *>(other));
}

/// dxfg_Object_hashCode
/* int32_t */ std::size_t hashCode(/* dxfg_java_object_handler* */ void *object) {
    if (!object) {
        throw InvalidArgumentException("Unable to execute function `dxfg_Object_hashCode`. The `object` is null");
    }

    return static_cast<std::size_t>(
        runGraalFunctionAndThrowIfMinusMin(dxfg_Object_hashCode, static_cast<dxfg_java_object_handler *>(object)));
}

} // namespace IsolatedObject
} // namespace isolated::internal

DXFCPP_END_NAMESPACE
// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../../include/dxfeed_graal_cpp_api/detail/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <cstddef>
#include <cstdint>
#include <string>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::internal {

/// dxfg_v4_free; `NULL` is skipped.
void freeGraal(void *pointer);

namespace IsolatedObject {

/// dxfg_v4_Object_toString
/* const char* */ std::string toString(/* dxfg_v4_java_object_handler* */ void *object);

/// dxfg_v4_Object_equals
std::int32_t equals(/* dxfg_v4_java_object_handler* */ void *object,
                    /* dxfg_v4_java_object_handler* */ void *other);

/// dxfg_v4_Object_hashCode
/* int32_t */ std::size_t hashCode(/* dxfg_v4_java_object_handler* */ void *object);

/// dxfg_v4_JavaObjectHandler_release
bool release(/* dxfg_v4_java_object_handler* */ void *handle) noexcept;

namespace List {
/// dxfg_v4_java_object_handler_list_release
bool release(/* dxfg_v4_java_object_handler_list* */ void *handle) noexcept;
}

} // namespace IsolatedObject
} // namespace isolated::internal

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
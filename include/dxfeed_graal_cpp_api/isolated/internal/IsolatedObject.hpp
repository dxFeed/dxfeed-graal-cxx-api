// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <string>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::internal {

void freeGraal(void *pointer);

namespace IsolatedObject {

// dxfg_Object_toString
/* const char* */ std::string toString(/* dxfg_java_object_handler* */ void *object);

// dxfg_Object_equals
std::int32_t equals(/* dxfg_java_object_handler* */ void *object, /* dxfg_java_object_handler* */ void *other);

// dxfg_Object_hashCode
/* int32_t */ std::size_t hashCode(/* dxfg_java_object_handler* */ void *object);

// int32_t dxfg_JavaObjectHandler_release(graal_isolatethread_t *thread, dxfg_java_object_handler *handler);
bool release(void *handle);

namespace List {
// int32_t dxfg_CList_JavaObjectHandler_release(graal_isolatethread_t *thread, dxfg_java_object_handler_list *list);
bool release(void *handle);
}

} // namespace IsolatedObject
} // namespace isolated::internal

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
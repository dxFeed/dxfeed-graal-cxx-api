// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../include/dxfeed_graal_cpp_api/detail/Conf.hpp"

// DXFCPP_INTERNAL_EXPORT marks the internal classes and functions of src/ that the tests and the Tools use through the
// shared library. They are exported only when the library is built with DXFCXX_EXPORT_INTERNALS (CMake option of the
// same name, ON with DXFCXX_DYNAMICALLY_LINK_UNIT_TESTS or DXFCXX_DYNAMICALLY_LINK_TOOLS). The CMake target passes the
// definition to the targets of the build tree that link the shared library, so they get dllimport on Windows; the
// installed package has neither the definition nor these symbols.
#ifndef DXFCPP_INTERNAL_EXPORT
#    if defined(DXFCXX_EXPORT_INTERNALS)
#        define DXFCPP_INTERNAL_EXPORT DXFCPP_EXPORT
#    else
#        define DXFCPP_INTERNAL_EXPORT
#    endif
#endif

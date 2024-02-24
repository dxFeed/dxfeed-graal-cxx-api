// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <utility>

#ifdef NO_ERROR
#    undef NO_ERROR
#endif

namespace dxfcpp {

thread_local Isolate::IsolateThread Isolate::currentIsolateThread_{};

auto IS = Isolate::getInstance().toString();

} // namespace dxfcpp

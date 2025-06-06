// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251 4996)

#include <dxfg_api.h>

#ifdef __cpp_lib_bit_cast
#    include <bit>
#endif

#include <atomic>
#include <cassert>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <locale>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

#include <utf8.h>

DXFCXX_DISABLE_GCC_WARNINGS_PUSH("-Wunused-variable")
DXFCXX_DISABLE_GCC_WARNINGS("-Wmaybe-uninitialized")
DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4702)
#include <range/v3/all.hpp>
DXFCXX_DISABLE_MSC_WARNINGS_POP()
DXFCXX_DISABLE_GCC_WARNINGS_POP()

#include "Common.hpp"
#include "Handler.hpp"
#include "Id.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_POP()
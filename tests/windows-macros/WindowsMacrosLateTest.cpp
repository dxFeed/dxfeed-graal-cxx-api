// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// windows.h included after the API headers, without NOMINMAX and WIN32_LEAN_AND_MEAN, defines its macros as usual.

#include <dxfeed_graal_cpp_api/api.hpp>

#include <windows.h>

#include <cstdio>

#if !defined(min) || !defined(max) || !defined(NO_ERROR) || !defined(DELETE) || !defined(ERROR)
#    error windows.h included after the API headers must define its macros
#endif

int checkWindowsHeaderAfterApi() {
    static_assert(max(1, 2) == 2 && min(1, 2) == 1);

    if (dxfcpp::OrderAction::valueOf(4).getName() != "DELETE") {
        std::puts("OrderAction::valueOf(4) is wrong");

        return 1;
    }

    return 0;
}

// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// The API headers compile after windows.h included without NOMINMAX and WIN32_LEAN_AND_MEAN, and keep the windows.h
// macros of the user defined.

#include <windows.h>

#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstdio>

#if !defined(min) || !defined(max) || !defined(NO_ERROR) || !defined(DELETE) || !defined(ERROR) || !defined(TRUE) ||   \
    !defined(FALSE)
#    error The API headers must keep the windows.h macros defined
#endif

using namespace dxfcpp;

// Defined in WindowsMacrosLateTest.cpp, which includes windows.h after the API headers.
int checkWindowsHeaderAfterApi();

int main() {
    // The user's macros still work.
    static_assert(max(1, 2) == 2 && min(1, 2) == 1 && TRUE == 1 && FALSE == 0);

    int failures = checkWindowsHeaderAfterApi();

    // The names hidden by the macros are reachable when the user hides the macros the same way.
#pragma push_macro("DELETE")
#pragma push_macro("ERROR")
#pragma push_macro("TRUE")
#pragma push_macro("FALSE")
#undef DELETE
#undef ERROR
#undef TRUE
#undef FALSE
    if (OrderAction::DELETE.getCode() != 4 || OrderAction::DELETE.getName() != "DELETE") {
        std::puts("OrderAction::DELETE is wrong");
        failures++;
    }

    static_assert(static_cast<int>(Logging::Level::ERROR) == static_cast<int>(Logging::Level::WARN) + 1);
    static_assert(static_cast<int>(Tristate::TRUE) == 1 && static_cast<int>(Tristate::FALSE) == 0);
#pragma pop_macro("FALSE")
#pragma pop_macro("TRUE")
#pragma pop_macro("ERROR")
#pragma pop_macro("DELETE")

    if (failures == 0) {
        std::puts("OK");
    }

    return failures == 0 ? 0 : 1;
}

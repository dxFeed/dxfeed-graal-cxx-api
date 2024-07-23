// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>
#include <dxfeed_graal_cpp_api/isolated/internal/IsolatedString.hpp>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::internal {

namespace IsolatedString {

bool release(const char *string) {
    if (!string) {
        throw std::invalid_argument("Unable to execute function `dxfg_String_release`. The `string` is nullptr");
    }

    return runGraalFunctionAndThrowIfLessThanZero(dxfg_String_release, string) == 0;
}

std::unique_ptr<const char, decltype(&release)> toUnique(const char *string) {
    return {string, release};
}

} // namespace IsolatedString

namespace IsolatedStringList {

bool release(/* dxfg_string_list* */ void *stringList) {
    if (!stringList) {
        throw std::invalid_argument(
            "Unable to execute function `dxfg_CList_String_release`. The `stringList` is nullptr");
    }

    return runGraalFunctionAndThrowIfLessThanZero(dxfg_CList_String_release,
                                                  static_cast<dxfg_string_list *>(stringList));
}

std::unique_ptr<void, decltype(&release)> toUnique(void *stringList) {
    return {stringList, release};
}

} // namespace IsolatedStringList

} // namespace isolated::internal

DXFCPP_END_NAMESPACE
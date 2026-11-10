// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "IsolatedString.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../IsolatedCommon.hpp"

#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::internal {

namespace IsolatedString {

bool release(const char *string) {
    if (!string) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_String_release`. The `string` is nullptr");
    }

    runGraalV4Function(dxfg_v4_String_release, string);

    return true;
}

std::unique_ptr<const char, decltype(&release)> toUnique(const char *string) {
    return {string, release};
}

} // namespace IsolatedString

namespace IsolatedStringList {

bool release(/* dxfg_v4_string_list* */ void *stringList) {
    if (!stringList) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_string_list_release`. The `stringList` is nullptr");
    }

    runGraalV4Function(dxfg_v4_string_list_release, static_cast<dxfg_v4_string_list *>(stringList));

    return true;
}

std::unique_ptr<void, decltype(&release)> toUnique(void *stringList) {
    return {stringList, release};
}

} // namespace IsolatedStringList

} // namespace isolated::internal

DXFCPP_END_NAMESPACE
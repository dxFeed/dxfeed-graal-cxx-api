// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include "dxfeed_graal_c_api/api.h"
#include "dxfeed_graal_cpp_api/api.hpp"

namespace dxfcpp {

const WildcardSymbol WildcardSymbol::ALL{"*"};

void *WildcardSymbol::toGraal() const noexcept {
    static const dxfg_wildcard_symbol_t wildcardGraalSymbol{{WILDCARD}};

    return bit_cast<void *>(&wildcardGraalSymbol);
}

} // namespace dxfcpp
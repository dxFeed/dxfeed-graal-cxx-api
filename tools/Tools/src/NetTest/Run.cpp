// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "Run.hpp"

#include <ostream>

namespace dxfcpp::tools::nettest {

// TODO: MDAPI-600: the producer run; until then the tool stops after the command line is checked.
int run(const Settings &, std::ostream &, std::ostream &err) {
    err << "NetTest: the run is not implemented yet\n";

    return 1;
}

} // namespace dxfcpp::tools::nettest

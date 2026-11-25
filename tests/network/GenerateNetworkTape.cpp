// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// Writes tests/data/tapes/network.txt from the events of NetworkTapeData.hpp:
//   TZ=UTC ./dxFeedGraalCxxApi_GenerateNetworkTape <path of the tape>

#include "NetworkTapeData.hpp"

#include <exception>
#include <iostream>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <path of the tape>\n";

        return 2;
    }

    try {
        dxfcpp::test::network_tape::writeTape(argv[1]);
    } catch (const std::exception &e) {
        std::cerr << e.what() << "\n";

        return 1;
    }

    return 0;
}

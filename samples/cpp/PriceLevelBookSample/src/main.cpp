// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfeed_graal_cpp_api/api.hpp>

#include <chrono>
#include <mutex>
#include <string>

#include "PriceLevel.hpp"
#include "PriceLevelBook.hpp"

using namespace dxfcpp;
using namespace dxfcpp::literals;
using namespace std::literals;

int main(int argc, char *argv[]) {
    try {
        const auto address = "demo.dxfeed.com:7300";

        // Disable QD logging.
        // Logging::init();

        std::cin.get();
    } catch (const RuntimeException &e) {
        std::cerr << e << '\n';

        return 1;
    }

    return 0;
}
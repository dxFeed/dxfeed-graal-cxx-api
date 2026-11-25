// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// Fuzz the parser of the request head of the /metrics server of Tools NetTest (parseRequestHead): any bytes from the
// network give INCOMPLETE, COMPLETE or BAD without a crash; a complete head has a method and a path without the query,
// and the bytes after it do not change it.
#include "../tools/Tools/src/NetTest/MetricsRequest.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <string_view>

using dxfcpp::tools::nettest::MAX_REQUEST_HEAD;
using dxfcpp::tools::nettest::parseRequestHead;
using dxfcpp::tools::nettest::RequestHeadStatus;

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size) {
    const std::string_view bytes(reinterpret_cast<const char *>(data), size);
    const auto head = parseRequestHead(bytes);

    if (head.status == RequestHeadStatus::COMPLETE) {
        if (head.method.empty() || head.path.size() > MAX_REQUEST_HEAD || head.path.find('?') != std::string::npos ||
            head.method.find(' ') != std::string::npos) {
            std::abort();
        }

        const auto longer = parseRequestHead(std::string(bytes) + "\r\n\r\nx");

        if (longer.status != RequestHeadStatus::COMPLETE || longer.method != head.method || longer.path != head.path) {
            std::abort();
        }
    } else if (!head.method.empty() || !head.path.empty()) {
        std::abort();
    }

    if (head.status == RequestHeadStatus::INCOMPLETE && size >= MAX_REQUEST_HEAD) {
        std::abort();
    }

    // The inputs of libFuzzer are much shorter than the limit, so the input is stretched to it. Without the end of the
    // head, a stretched copy is too long.
    std::string stretched(bytes);

    stretched.append(MAX_REQUEST_HEAD, 'x');

    if (head.status != RequestHeadStatus::COMPLETE && parseRequestHead(stretched).status != RequestHeadStatus::BAD) {
        std::abort();
    }

    // A head that ends exactly at the limit is complete; one byte longer is too long. The input fills a header line.
    std::string filler(bytes);

    std::replace(filler.begin(), filler.end(), '\n', 'x');

    const std::string start = "GET /metrics HTTP/1.0\r\nX: ";
    const std::string end = "\r\n\r\n";

    filler.resize(MAX_REQUEST_HEAD - start.size() - end.size(), 'x');

    const auto atLimit = parseRequestHead(start + filler + end);
    const auto overLimit = parseRequestHead(start + filler + "x" + end);

    if (atLimit.status != RequestHeadStatus::COMPLETE || atLimit.method != "GET" || atLimit.path != "/metrics" ||
        overLimit.status != RequestHeadStatus::BAD) {
        std::abort();
    }

    return 0;
}

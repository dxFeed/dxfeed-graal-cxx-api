// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "MetricsRequest.hpp"

#include <algorithm>
#include <cstring>

namespace dxfcpp::tools::nettest {

namespace {

// The end of the head (after its empty line), or npos while there is none.
std::size_t headEnd(std::string_view bytes) {
    const auto crlf = bytes.find("\r\n\r\n");
    const auto lf = bytes.find("\n\n");
    const auto crlfEnd = crlf == std::string_view::npos ? std::string_view::npos : crlf + 4;
    const auto lfEnd = lf == std::string_view::npos ? std::string_view::npos : lf + 2;

    return std::min(crlfEnd, lfEnd);
}

bool isTokenChar(char symbol) {
    const auto code = static_cast<unsigned char>(symbol);

    return (code >= '0' && code <= '9') || (code >= 'A' && code <= 'Z') || (code >= 'a' && code <= 'z') ||
           (code != 0 && std::strchr("!#$%&'*+-.^_`|~", static_cast<int>(code)) != nullptr);
}

bool hasControlBytes(std::string_view text) {
    return std::any_of(text.begin(), text.end(), [](char symbol) {
        const auto code = static_cast<unsigned char>(symbol);

        return code < 0x20 || code == 0x7F;
    });
}

RequestHead bad() {
    return {RequestHeadStatus::BAD, {}, {}};
}

} // namespace

RequestHead parseRequestHead(std::string_view bytes) {
    const auto end = headEnd(bytes);

    if (end == std::string_view::npos) {
        return bytes.size() >= MAX_REQUEST_HEAD ? bad() : RequestHead{};
    }

    if (end > MAX_REQUEST_HEAD) {
        return bad();
    }

    auto line = bytes.substr(0, bytes.find('\n'));

    if (!line.empty() && line.back() == '\r') {
        line.remove_suffix(1);
    }

    if (hasControlBytes(line)) {
        return bad();
    }

    const auto firstSpace = line.find(' ');

    if (firstSpace == std::string_view::npos) {
        return bad();
    }

    const auto method = line.substr(0, firstSpace);
    const auto rest = line.substr(firstSpace + 1);
    const auto secondSpace = rest.find(' ');
    const auto target = rest.substr(0, secondSpace);
    const auto version = secondSpace == std::string_view::npos ? std::string_view{} : rest.substr(secondSpace + 1);

    if (method.empty() || !std::all_of(method.begin(), method.end(), isTokenChar) || target.empty() ||
        (secondSpace != std::string_view::npos && (version.empty() || version.find(' ') != std::string_view::npos))) {
        return bad();
    }

    return {RequestHeadStatus::COMPLETE, std::string(method), std::string(target.substr(0, target.find('?')))};
}

} // namespace dxfcpp::tools::nettest

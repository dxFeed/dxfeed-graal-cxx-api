// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// The head of a request to /metrics: the bytes a client sent so far, read without sockets, so that the parsing can be
// tested and fuzzed alone.

#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace dxfcpp::tools::nettest {

/// The largest request head that the server of /metrics reads, the empty line at its end included.
inline constexpr std::size_t MAX_REQUEST_HEAD = 8192;

/// What the bytes received so far make of a request head.
enum class RequestHeadStatus {
    /// No empty line yet and fewer than MAX_REQUEST_HEAD bytes: more must come.
    INCOMPLETE,
    /// The head ends within MAX_REQUEST_HEAD bytes and its request line is valid.
    COMPLETE,
    /// The head is longer than MAX_REQUEST_HEAD bytes, or its request line is not `<method> <target>[ <version>]`.
    BAD,
};

/// The request line of a complete head.
struct RequestHead {
    RequestHeadStatus status = RequestHeadStatus::INCOMPLETE;
    /// The method, for example `GET`; empty unless COMPLETE.
    std::string method{};
    /// The path of the target without its query, for example `/metrics`; empty unless COMPLETE.
    std::string path{};
};

/// Reads the bytes received so far. The head ends at the first empty line (CRLF CRLF or LF LF); the bytes after it are
/// not looked at. The method is a token of RFC 9110; the target and the version have no control bytes, NUL included.
/// Never throws on bad input.
RequestHead parseRequestHead(std::string_view bytes);

} // namespace dxfcpp::tools::nettest

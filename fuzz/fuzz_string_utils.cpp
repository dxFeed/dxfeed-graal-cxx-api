// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// Fuzz internal string / time utilities (StringUtils.hpp). Most are declared noexcept.
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstdint>
#include <cstring>
#include <exception>
#include <string>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size) {
    using namespace dxfcpp;
    const std::string s(reinterpret_cast<const char *>(data), size);

    (void)trimStr(s);
    (void)splitStr(s);
    (void)splitStr(s, ';');
    (void)toBool(s);
    (void)iEquals(s, "true");
    (void)toString(s.c_str());

    std::vector<std::int16_t> u16;
    for (std::size_t i = 0; i + 1 < size; i += 2) {
        std::int16_t v;
        std::memcpy(&v, data + i, 2);
        u16.push_back(v);
        (void)utf16to8(v);
        (void)utf16toUtf8String(v);
        (void)encodeChar(v);
    }
    (void)utf16toUtf8String(u16);
    for (char c : s) {
        (void)utf8to16(c);
    }

    // Timestamps: arbitrary int64 millis (events may carry any value coming from the wire).
    if (size >= 8) {
        std::int64_t ts;
        std::memcpy(&ts, data, 8);
        try {
            (void)formatTimeStamp(ts);
            (void)formatTimeStampFast(ts);
            (void)formatTimeStampWithMillis(ts);
            (void)formatTimeStampWithTimeZone(ts);
            (void)formatTimeStampWithMillisWithTimeZone(ts);
        } catch (const std::exception &) {
        }
    }

    try {
        (void)static_cast<double>(StringLike(s));
    } catch (const std::exception &) {
    }

    return 0;
}

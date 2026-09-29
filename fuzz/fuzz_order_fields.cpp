// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// Fuzz packed fields of Order (index/source, exchange code, time/sequence) and day_util.
// Many setters are noexcept -> any internal throw = std::terminate.
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstdint>
#include <cstring>
#include <exception>

template <typename T> static T take(const std::uint8_t *&p, std::size_t &n) {
    T v{};
    if (n >= sizeof(T)) {
        std::memcpy(&v, p, sizeof(T));
        p += sizeof(T);
        n -= sizeof(T);
    }
    return v;
}

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size) {
    using namespace dxfcpp;
    const auto index = take<std::int64_t>(data, size);
    const auto time = take<std::int64_t>(data, size);
    const auto seq = take<std::int32_t>(data, size);
    const auto ex16 = take<std::int16_t>(data, size);
    const auto dayId = take<std::int32_t>(data, size);

    Order o("AAPL");

    // setIndex() throws for a negative index (as in Java); the noexcept functions below must not throw.
    try {
        o.setIndex(index);
    } catch (const std::exception &) {
    }

    (void)o.getIndex();
    (void)o.getSource(); // noexcept, decodes source id from index
    o.setTime(time);
    try {
        o.setSequence(seq);
    } catch (const std::exception &) {
    }
    o.setExchangeCode(ex16); // noexcept
    (void)o.getExchangeCode();
    (void)o.getExchangeCodeString();
    (void)o.withIndex(index); // noexcept
    (void)o.toString();

    (void)day_util::getYearMonthDayByDayId(dayId);

    return 0;
}

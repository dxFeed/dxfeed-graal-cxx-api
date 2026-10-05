// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// Fuzz packed fields of Order (index/source, exchange code, time/sequence) and day_util.
// Invalid values throw InvalidArgumentException, as Java throws IllegalArgumentException; any other exception, or a
// throw from a noexcept function (std::terminate), is a defect.
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstdint>
#include <cstring>
#include <exception>

// Calls f(); an InvalidArgumentException is the documented reaction to an invalid value.
template <typename F> static void allowInvalidArgument(F &&f) {
    try {
        f();
    } catch (const dxfcpp::InvalidArgumentException &) {
    }
}

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

    allowInvalidArgument([&] {
        o.setIndex(index);
    });

    (void)o.getIndex();
    allowInvalidArgument([&] {
        (void)o.getSource(); // decodes the source id from the index
    });
    o.setTime(time);
    try {
        o.setSequence(seq);
    } catch (const std::exception &) {
    }
    allowInvalidArgument([&] {
        o.setExchangeCode(ex16);
    });
    (void)o.getExchangeCode();
    (void)o.getExchangeCodeString();
    allowInvalidArgument([&] {
        (void)o.withIndex(index);
    });
    allowInvalidArgument([&] {
        (void)o.toString(); // prints the source
    });

    (void)day_util::getYearMonthDayByDayId(dayId);

    return 0;
}

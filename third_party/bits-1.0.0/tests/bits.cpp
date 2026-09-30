// Copyright (c) 2025 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#include <doctest/doctest.h>

#include <bits/bits.hpp>
#include <cstddef>
#include <cstdint>
#include <limits>

using namespace org::ttldtor::bits;

template <typename T>
constexpr std::size_t bitWidth() {
  using U = std::make_unsigned_t<T>;

  return static_cast<std::size_t>(std::numeric_limits<U>::digits);
}

TEST_CASE("sal - basic invariants") {
  SUBCASE("shift == 0 returns value") {
    CHECK_EQ(sal<std::uint32_t, int>(0x12345678u, 0), 0x12345678u);
    CHECK_EQ(sal<std::int32_t, int>(123, 0), 123);
    CHECK_EQ(sal<std::int32_t, int>(-456, 0), -456);
  }

  SUBCASE("value == 0 returns 0 for any shift") {
    CHECK_EQ(sal<std::uint32_t, int>(0u, 0), 0u);
    CHECK_EQ(sal<std::uint32_t, int>(0u, 5), 0u);
    CHECK_EQ(sal<std::uint32_t, int>(0u, -5), 0u);
  }
}

TEST_CASE("sal - non-negative shifts within bit width") {
  SUBCASE("unsigned V, safe positive shifts") {
    CHECK_EQ(sal<std::uint8_t, int>(0x12u, 1), static_cast<std::uint8_t>(0x24u));
    CHECK_EQ(sal<std::uint16_t, int>(0x0003u, 2), static_cast<std::uint16_t>(0x000Cu));
    CHECK_EQ(sal<std::uint32_t, int>(1u, 5), 32u);
    CHECK_EQ(sal<std::uint64_t, int>(7u, 3), 56u);
  }

  SUBCASE("signed V, use small shifts that do not lead to overflow") {
    CHECK_EQ(sal<std::int8_t, int>(3, 1), 6);
    CHECK_EQ(sal<std::int16_t, int>(5, 2), 20);
    CHECK_EQ(sal<std::int32_t, int>(7, 3), 56);
    CHECK_EQ(sal<std::int64_t, int>(9, 4), 144);
  }
}

TEST_CASE("sal - shift >= bit width returns 0") {
  SUBCASE("unsigned V") {
    constexpr auto BIT_WIDTH_8 = bitWidth<std::uint8_t>();
    constexpr auto BIT_WIDTH_32 = bitWidth<std::uint32_t>();
    constexpr auto BIT_WIDTH_64 = bitWidth<std::uint64_t>();

    CHECK_EQ(sal<std::uint8_t, int>(0xFFu, static_cast<int>(BIT_WIDTH_8)), 0u);
    CHECK_EQ(sal<std::uint8_t, int>(0xFFu, 100), 0u);

    CHECK_EQ(sal<std::uint32_t, int>(0xFFFFFFFFu, static_cast<int>(BIT_WIDTH_32)), 0u);
    CHECK_EQ(sal<std::uint64_t, int>(0xFFFFFFFFFFFFFFFFull, static_cast<int>(BIT_WIDTH_64)), 0ull);
  }

  SUBCASE("signed V (do not rely on the behavior of negative left shift)") {
    constexpr auto BIT_WIDTH_16 = bitWidth<std::int16_t>();

    CHECK_EQ(sal<std::int16_t, int>(123, static_cast<int>(BIT_WIDTH_16)), 0);
    CHECK_EQ(sal<std::int32_t, int>(456, 1000), 0);
  }
}

TEST_CASE("sal - negative shift delegates to sar (right arithmetic)") {
  SUBCASE("signed V, negative value: sign-extend on right shift") {
    // value = -8; shifting left by -1 => arithmetic right shift by 1 => -4
    CHECK_EQ(sal<std::int32_t, int>(-8, -1), sar<std::int32_t, unsigned>(-8, 1));
    CHECK_EQ(sal<std::int32_t, int>(-8, -1), -4);

    // larger magnitude
    CHECK_EQ(sal<std::int32_t, int>(-1024, -5), sar<std::int32_t, unsigned>(-1024, 5));
    CHECK_EQ(sal<std::int32_t, int>(-1024, -5), -32);
  }

  SUBCASE("signed V, non-negative value: behaves like logical right via sar") {
    CHECK_EQ(sal<std::int32_t, int>(1024, -5), sar<std::int32_t, unsigned>(1024, 5));
    CHECK_EQ(sal<std::int32_t, int>(1024, -5), 32);
  }

  SUBCASE("unsigned V: delegating negative shift performs right shift filling zeros") {
    CHECK_EQ(sal<std::uint32_t, int>(0xF0000000u, -4), sar<std::uint32_t, unsigned>(0xF0000000u, 4));
    CHECK_EQ(sal<std::uint32_t, int>(0xF0000000u, -4), 0x0F000000u);
  }

  SUBCASE("extreme negative shift: shift == min(S)") {
    // use the minimum possible shift value for the signed S
    constexpr int MIN_SHIFT = std::numeric_limits<int>::min();
    // magnitude is correctly calculated in unsigned representation, resulting right shift >= bit width => 0 for
    // unsigned, -1/0 for signed inside sar
    CHECK_EQ(
      sal<std::uint32_t, int>(0xDEADBEEFu, MIN_SHIFT),
      sar(0xDEADBEEFu, static_cast<std::make_unsigned_t<int>>(0) - static_cast<std::make_unsigned_t<int>>(MIN_SHIFT)));
    CHECK_EQ(sal(-1, MIN_SHIFT),
             sar(-1, static_cast<std::make_unsigned_t<int>>(0) - static_cast<std::make_unsigned_t<int>>(MIN_SHIFT)));
  }
}

TEST_CASE("sal - cross-size S and V") {
  SUBCASE("S already than V (std::int8_t -> std::uint64_t)") {
    CHECK_EQ(sal<std::uint64_t, std::int8_t>(1ull, std::int8_t{5}), 32ull);
    CHECK_EQ(sal<std::uint64_t, std::int8_t>(0xFFFFFFFFFFFFFFFFull, std::int8_t{64}), 0ull);
  }

  SUBCASE("S wider than V (std::int64_t -> std::uint8_t)") {
    CHECK_EQ(sal<std::uint8_t, std::int64_t>(0x01u, std::int64_t{3}), static_cast<std::uint8_t>(0x08u));
    CHECK_EQ(sal<std::uint8_t, std::int64_t>(0xFFu, std::int64_t{100}), static_cast<std::uint8_t>(0));
  }

  SUBCASE("Unsigned S: no negative shifts, big shifts are zeroed out") {
    CHECK_EQ(sal<std::uint16_t, std::uint8_t>(0x0001u, static_cast<std::uint8_t>(4)),
             static_cast<std::uint16_t>(0x0010u));
    CHECK_EQ(sal<std::uint16_t, std::uint8_t>(0xFFFFu, static_cast<std::uint8_t>(255)), static_cast<std::uint16_t>(0));
  }
}

TEST_CASE("sar - basic invariants") {
  SUBCASE("shift == 0 returns value") {
    CHECK_EQ(sar<std::uint32_t, int>(0x12345678u, 0), 0x12345678u);
    CHECK_EQ(sar<std::int32_t, int>(123, 0), 123);
    CHECK_EQ(sar<std::int32_t, int>(-456, 0), -456);
  }

  SUBCASE("value == 0 returns 0 for any shift") {
    CHECK_EQ(sar<std::uint32_t, int>(0u, 0), 0u);
    CHECK_EQ(sar<std::uint32_t, int>(0u, 5), 0u);
    CHECK_EQ(sar<std::uint32_t, int>(0u, -5), 0u);
  }
}

TEST_CASE("sar - non-negative shifts within bit width") {
  SUBCASE("unsigned V behaves like logical right shift") {
    CHECK_EQ(sar<std::uint8_t, int>(0x80u, 1), static_cast<std::uint8_t>(0x40u));
    CHECK_EQ(sar<std::uint16_t, int>(0x00F0u, 4), static_cast<std::uint16_t>(0x000Fu));
    CHECK_EQ(sar<std::uint32_t, int>(0x80000000u, 31), 1u);
    CHECK_EQ(sar<std::uint64_t, int>(0xF000000000000000ull, 4), 0x0F00000000000000ull);
  }

  SUBCASE("signed V sign-extends on right shift") {
    CHECK_EQ(sar<std::int8_t, int>(-2, 1), static_cast<std::int8_t>(-1));
    CHECK_EQ(sar<std::int16_t, int>(-8, 1), static_cast<std::int16_t>(-4));
    CHECK_EQ(sar<std::int32_t, int>(-1024, 5), -32);
    CHECK_EQ(sar<std::int64_t, int>(-9, 3), -2);
  }
}

TEST_CASE("sar - shift >= bit width returns 0 for non-negative and -1 for negative") {
  SUBCASE("unsigned V") {
    constexpr auto BIT_WIDTH_8 = bitWidth<std::uint8_t>();
    constexpr auto BIT_WIDTH_32 = bitWidth<std::uint32_t>();
    constexpr auto BIT_WIDTH_64 = bitWidth<std::uint64_t>();

    CHECK_EQ(sar<std::uint8_t, int>(0xFFu, static_cast<int>(BIT_WIDTH_8)), 0u);
    CHECK_EQ(sar<std::uint8_t, int>(0xFFu, 100), 0u);

    CHECK_EQ(sar<std::uint32_t, int>(0xFFFFFFFFu, static_cast<int>(BIT_WIDTH_32)), 0u);
    CHECK_EQ(sar<std::uint64_t, int>(0xFFFFFFFFFFFFFFFFull, static_cast<int>(BIT_WIDTH_64)), 0ull);
  }

  SUBCASE("signed V") {
    constexpr auto BIT_WIDTH_16 = bitWidth<std::int16_t>();

    CHECK_EQ(sar<std::int16_t, int>(123, static_cast<int>(BIT_WIDTH_16)), 0);
    CHECK_EQ(sar<std::int16_t, int>(-123, static_cast<int>(BIT_WIDTH_16)), static_cast<std::int16_t>(-1));

    CHECK_EQ(sar<std::int32_t, int>(456, 1000), 0);
    CHECK_EQ(sar<std::int32_t, int>(-456, 1000), static_cast<std::int32_t>(-1));
  }
}

TEST_CASE("sar - negative shift delegates to sal (left arithmetic)") {
  SUBCASE("unsigned V: negative shift becomes left logical via sal delegation") {
    CHECK_EQ(sar<std::uint32_t, int>(0x0000000Fu, -4), sal<std::uint32_t, unsigned>(0x0000000Fu, 4));
    CHECK_EQ(sar<std::uint32_t, int>(0x0000000Fu, -4), 0x000000F0u);
  }

  SUBCASE("signed V, negative value") {
    // right arithmetic by -(-shift) means left shift by magnitude
    CHECK_EQ(sar<std::int32_t, int>(-8, -1), sal<std::int32_t, unsigned>(-8, 1));
    CHECK_EQ(sar<std::int32_t, int>(-8, -1), -16);
  }

  SUBCASE("signed V, non-negative value") {
    CHECK_EQ(sar<std::int32_t, int>(1024, -5), sal<std::int32_t, unsigned>(1024, 5));
    CHECK_EQ(sar<std::int32_t, int>(1024, -5), 32768);
  }

  SUBCASE("extreme negative shift: shift == min(S)") {
    constexpr int MIN_SHIFT = std::numeric_limits<int>::min();
    using U = std::make_unsigned_t<int>;
    constexpr auto magnitude = U{} - static_cast<U>(MIN_SHIFT);

    CHECK_EQ(sar<std::uint32_t, int>(0x00000001u, MIN_SHIFT), sal<std::uint32_t, U>(0x00000001u, magnitude));
    CHECK_EQ(sar<std::int32_t, int>(1, MIN_SHIFT), sal<std::int32_t, U>(1, magnitude));
  }
}

TEST_CASE("sar - cross-size S and V") {
  SUBCASE("S narrower than V (std::int8_t -> std::uint64_t)") {
    CHECK_EQ(sar<std::uint64_t, std::int8_t>(0x8000000000000000ull, std::int8_t{63}), 1ull);
    CHECK_EQ(sar<std::uint64_t, std::int8_t>(0x00000000000000FFull, std::int8_t{7}), 1ull);
  }

  SUBCASE("S wider than V (std::int64_t -> std::uint8_t)") {
    CHECK_EQ(sar<std::uint8_t, std::int64_t>(0x80u, std::int64_t{7}), static_cast<std::uint8_t>(0x01u));
    CHECK_EQ(sar<std::uint8_t, std::int64_t>(0xFFu, std::int64_t{100}), static_cast<std::uint8_t>(0));
  }

  SUBCASE("Unsigned S: no negative shifts, big shifts are zeroed out or -1 per sign") {
    CHECK_EQ(sar<std::uint16_t, std::uint8_t>(0x00F0u, static_cast<std::uint8_t>(4)),
             static_cast<std::uint16_t>(0x000Fu));
    CHECK_EQ(sar<std::uint16_t, std::uint8_t>(0xFFFFu, static_cast<std::uint8_t>(255)), static_cast<std::uint16_t>(0));
    CHECK_EQ(sar<std::int16_t, std::uint8_t>(-1, static_cast<std::uint8_t>(255)), static_cast<std::int16_t>(-1));
  }
}

TEST_CASE("shl - basic invariants") {
  SUBCASE("shift == 0 returns value") {
    CHECK_EQ(shl<std::uint32_t, int>(0x12345678u, 0), 0x12345678u);
    CHECK_EQ(shl<std::int32_t, int>(123, 0), 123);
    CHECK_EQ(shl<std::int32_t, int>(0, 0), 0);
  }

  SUBCASE("value == 0 returns 0 for any shift") {
    CHECK_EQ(shl<std::uint32_t, int>(0u, 0), 0u);
    CHECK_EQ(shl<std::uint32_t, int>(0u, 5), 0u);
    CHECK_EQ(shl<std::uint32_t, int>(0u, -5), 0u);
  }
}

TEST_CASE("shl - non-negative shifts within bit width") {
  SUBCASE("unsigned V, safe positive shifts") {
    CHECK_EQ(shl<std::uint8_t, int>(0x12u, 1), static_cast<std::uint8_t>(0x24u));
    CHECK_EQ(shl<std::uint16_t, int>(0x0003u, 2), static_cast<std::uint16_t>(0x000Cu));
    CHECK_EQ(shl<std::uint32_t, int>(1u, 5), 32u);
    CHECK_EQ(shl<std::uint64_t, int>(7u, 3), 56u);
  }

  SUBCASE("signed V, use small shifts and non-negative values") {
    CHECK_EQ(shl<std::int8_t, int>(3, 1), 6);
    CHECK_EQ(shl<std::int16_t, int>(5, 2), 20);
    CHECK_EQ(shl<std::int32_t, int>(7, 3), 56);
    CHECK_EQ(shl<std::int64_t, int>(9, 4), 144);
  }
}

TEST_CASE("shl - shift >= bit width returns 0") {
  SUBCASE("unsigned V") {
    constexpr auto BIT_WIDTH_8 = bitWidth<std::uint8_t>();
    constexpr auto BIT_WIDTH_32 = bitWidth<std::uint32_t>();
    constexpr auto BIT_WIDTH_64 = bitWidth<std::uint64_t>();

    CHECK_EQ(shl<std::uint8_t, int>(0xFFu, static_cast<int>(BIT_WIDTH_8)), 0u);
    CHECK_EQ(shl<std::uint8_t, int>(0xFFu, 100), 0u);

    CHECK_EQ(shl<std::uint32_t, int>(0xFFFFFFFFu, static_cast<int>(BIT_WIDTH_32)), 0u);
    CHECK_EQ(shl<std::uint64_t, int>(0xFFFFFFFFFFFFFFFFull, static_cast<int>(BIT_WIDTH_64)), 0ull);
  }

  SUBCASE("signed V") {
    constexpr auto BIT_WIDTH_16 = bitWidth<std::int16_t>();

    CHECK_EQ(shl<std::int16_t, int>(123, static_cast<int>(BIT_WIDTH_16)), 0);
    CHECK_EQ(shl<std::int32_t, int>(456, 1000), 0);
  }
}

TEST_CASE("shl - negative shift delegates to shr (right logical)") {
  SUBCASE("unsigned V: negative shift becomes right logical via shr") {
    CHECK_EQ(shl<std::uint32_t, int>(0xF0000000u, -4), shr<std::uint32_t, unsigned>(0xF0000000u, 4));
    CHECK_EQ(shl<std::uint32_t, int>(0xF0000000u, -4), 0x0F000000u);
  }

  SUBCASE("signed V, non-negative value: right logical via shr") {
    CHECK_EQ(shl<std::int32_t, int>(1024, -5), shr<std::int32_t, unsigned>(1024, 5));
    CHECK_EQ(shl<std::int32_t, int>(1024, -5), 32);
  }

  SUBCASE("extreme negative shift: shift == min(S)") {
    constexpr int MIN_SHIFT = std::numeric_limits<int>::min();
    using U = std::make_unsigned_t<int>;
    constexpr auto MAGNITUDE = U{} - static_cast<U>(MIN_SHIFT);

    CHECK_EQ(shl<std::uint32_t, int>(0xDEADBEEFu, MIN_SHIFT), shr<std::uint32_t, U>(0xDEADBEEFu, MAGNITUDE));
    CHECK_EQ(shl<std::int32_t, int>(1, MIN_SHIFT), shr<std::int32_t, U>(1, MAGNITUDE));
  }
}

TEST_CASE("shl - cross-size S and V") {
  SUBCASE("S narrower than V (std::int8_t -> std::uint64_t)") {
    CHECK_EQ(shl<std::uint64_t, std::int8_t>(1ull, std::int8_t{5}), 32ull);
    CHECK_EQ(shl<std::uint64_t, std::int8_t>(0xFFFFFFFFFFFFFFFFull, std::int8_t{64}), 0ull);
  }

  SUBCASE("S wider than V (std::int64_t -> std::uint8_t)") {
    CHECK_EQ(shl<std::uint8_t, std::int64_t>(0x01u, std::int64_t{3}), static_cast<std::uint8_t>(0x08u));
    CHECK_EQ(shl<std::uint8_t, std::int64_t>(0xFFu, std::int64_t{100}), static_cast<std::uint8_t>(0));
  }

  SUBCASE("Unsigned S: no negative shifts, big shifts are zeroed out") {
    CHECK_EQ(shl<std::uint16_t, std::uint8_t>(0x0001u, static_cast<std::uint8_t>(4)),
             static_cast<std::uint16_t>(0x0010u));
    CHECK_EQ(shl<std::uint16_t, std::uint8_t>(0xFFFFu, static_cast<std::uint8_t>(255)), static_cast<std::uint16_t>(0));
  }
}

TEST_CASE("shr - basic invariants") {
  SUBCASE("shift == 0 returns value") {
    CHECK_EQ(shr<std::uint32_t, int>(0x12345678u, 0), 0x12345678u);
    CHECK_EQ(shr<std::int32_t, int>(123, 0), 123);
    CHECK_EQ(shr<std::int32_t, int>(-456, 0), -456);
  }

  SUBCASE("value == 0 returns 0 for any shift") {
    CHECK_EQ(shr<std::uint32_t, int>(0u, 0), 0u);
    CHECK_EQ(shr<std::uint32_t, int>(0u, 5), 0u);
    CHECK_EQ(shr<std::uint32_t, int>(0u, -5), 0u);
  }
}

TEST_CASE("shr - non-negative shifts within bit width") {
  SUBCASE("unsigned V behaves like built-in >> with zero-fill") {
    CHECK_EQ(shr<std::uint8_t, int>(0x80u, 1), static_cast<std::uint8_t>(0x40u));
    CHECK_EQ(shr<std::uint16_t, int>(0x00F0u, 4), static_cast<std::uint16_t>(0x000Fu));
    CHECK_EQ(shr<std::uint32_t, int>(0x80000000u, 31), 1u);
    CHECK_EQ(shr<std::uint64_t, int>(0xF000000000000000ull, 4), 0x0F00000000000000ull);
  }

  SUBCASE("signed V also zero-fills (logical right)") {
    CHECK_EQ(shr<std::int8_t, int>(static_cast<std::int8_t>(-128), 1), static_cast<std::int8_t>(0x40));
    CHECK_EQ(shr<std::int16_t, int>(static_cast<std::int16_t>(-1), 1), static_cast<std::int16_t>(0x7FFF));
    CHECK_EQ(shr<std::int32_t, int>(static_cast<std::int32_t>(-1024), 5), static_cast<std::int32_t>(0x07FFFFE0));
    CHECK_EQ(shr<std::int64_t, int>(static_cast<std::int64_t>(-9), 3), static_cast<std::int64_t>(0x1FFFFFFFFFFFFFFE));
  }
}

TEST_CASE("shr - shift >= bit width returns 0") {
  SUBCASE("unsigned V") {
    constexpr auto BIT_WIDTH_8 = bitWidth<std::uint8_t>();
    constexpr auto BIT_WIDTH_32 = bitWidth<std::uint32_t>();
    constexpr auto BIT_WIDTH_64 = bitWidth<std::uint64_t>();

    CHECK_EQ(shr<std::uint8_t, int>(0xFFu, static_cast<int>(BIT_WIDTH_8)), 0u);
    CHECK_EQ(shr<std::uint8_t, int>(0xFFu, 100), 0u);

    CHECK_EQ(shr<std::uint32_t, int>(0xFFFFFFFFu, static_cast<int>(BIT_WIDTH_32)), 0u);
    CHECK_EQ(shr<std::uint64_t, int>(0xFFFFFFFFFFFFFFFFull, static_cast<int>(BIT_WIDTH_64)), 0ull);
  }

  SUBCASE("signed V") {
    constexpr auto BIT_WIDTH_16 = bitWidth<std::int16_t>();

    CHECK_EQ(shr<std::int16_t, int>(-1, static_cast<int>(BIT_WIDTH_16)), 0);
    CHECK_EQ(shr<std::int32_t, int>(-456, 1000), 0);
    CHECK_EQ(shr<std::int32_t, int>(456, 1000), 0);
  }
}

TEST_CASE("shr - negative shift delegates to shl (left logical)") {
  SUBCASE("unsigned V: negative shift becomes left logical via shl") {
    CHECK_EQ(shr<std::uint32_t, int>(0x0000000Fu, -4), shl<std::uint32_t, unsigned>(0x0000000Fu, 4));
    CHECK_EQ(shr<std::uint32_t, int>(0x0000000Fu, -4), 0x000000F0u);
  }

  SUBCASE("signed V, non-negative value") {
    CHECK_EQ(shr<std::int32_t, int>(1024, -5), shl<std::int32_t, unsigned>(1024, 5));
    CHECK_EQ(shr<std::int32_t, int>(1024, -5), 32768);
  }

  SUBCASE("extreme negative shift: shift == min(S)") {
    constexpr int MIN_SHIFT = std::numeric_limits<int>::min();
    using U = std::make_unsigned_t<int>;
    constexpr auto MAGNITUDE = U{} - static_cast<U>(MIN_SHIFT);

    CHECK_EQ(shr<std::uint32_t, int>(0x00000001u, MIN_SHIFT), shl<std::uint32_t, U>(0x00000001u, MAGNITUDE));
    CHECK_EQ(shr<std::int32_t, int>(1, MIN_SHIFT), shl<std::int32_t, U>(1, MAGNITUDE));
  }
}

TEST_CASE("shr - cross-size S and V") {
  SUBCASE("S narrower than V (std::int8_t -> std::uint64_t)") {
    CHECK_EQ(shr<std::uint64_t, std::int8_t>(0x8000000000000000ull, std::int8_t{63}), 1ull);
    CHECK_EQ(shr<std::uint64_t, std::int8_t>(0x00000000000000FFull, std::int8_t{8}), 0ull);
  }

  SUBCASE("S wider than V (std::int64_t -> std::uint8_t)") {
    CHECK_EQ(shr<std::uint8_t, std::int64_t>(0x80u, std::int64_t{7}), static_cast<std::uint8_t>(0x01u));
    CHECK_EQ(shr<std::uint8_t, std::int64_t>(0xFFu, std::int64_t{100}), static_cast<std::uint8_t>(0));
  }

  SUBCASE("Unsigned S: no negative shifts, big shifts are zeroed out") {
    CHECK_EQ(shr<std::uint16_t, std::uint8_t>(0x00F0u, static_cast<std::uint8_t>(4)),
             static_cast<std::uint16_t>(0x000Fu));
    CHECK_EQ(shr<std::uint16_t, std::uint8_t>(0xFFFFu, static_cast<std::uint8_t>(255)), static_cast<std::uint16_t>(0));
    CHECK_EQ(shr<std::int16_t, std::uint8_t>(-1, static_cast<std::uint8_t>(15)), static_cast<std::int16_t>(0x0001));
  }
}

TEST_CASE("andOp - basic invariants") {
  SUBCASE("identity with all bits set") {
    CHECK_EQ(andOp<std::uint32_t, std::uint32_t>(0x12345678u, 0xFFFFFFFFu), 0x12345678u);
    CHECK_EQ(andOp<std::int32_t, std::int32_t>(123, -1), 123);
  }

  SUBCASE("zero result with 0") {
    CHECK_EQ(andOp<std::uint32_t, std::uint32_t>(0x12345678u, 0u), 0u);
    CHECK_EQ(andOp<std::int32_t, std::int32_t>(-456, 0), 0);
  }

  SUBCASE("same value with itself") {
    CHECK_EQ(andOp<std::uint32_t, std::uint32_t>(0xDEADBEEFu, 0xDEADBEEFu), 0xDEADBEEFu);
    CHECK_EQ(andOp<std::int16_t, std::int16_t>(-123, -123), -123);
  }
}

TEST_CASE("andOp - same types") {
  SUBCASE("unsigned types - uint8_t") {
    CHECK_EQ(andOp<std::uint8_t, std::uint8_t>(0b11110000u, 0b10101010u), static_cast<std::uint8_t>(0b10100000u));
    CHECK_EQ(andOp<std::uint8_t, std::uint8_t>(0xFFu, 0x0Fu), static_cast<std::uint8_t>(0x0Fu));
  }

  SUBCASE("unsigned types - uint16_t") {
    CHECK_EQ(andOp<std::uint16_t, std::uint16_t>(0xF0F0u, 0xFF00u), static_cast<std::uint16_t>(0xF000u));
    CHECK_EQ(andOp<std::uint16_t, std::uint16_t>(0xABCDu, 0x00FFu), static_cast<std::uint16_t>(0x00CDu));
  }

  SUBCASE("unsigned types - uint32_t") {
    CHECK_EQ(andOp<std::uint32_t, std::uint32_t>(0xFFFF0000u, 0x0000FFFFu), 0u);
    CHECK_EQ(andOp<std::uint32_t, std::uint32_t>(0x12345678u, 0x0F0F0F0Fu), 0x02040608u);
  }

  SUBCASE("unsigned types - uint64_t") {
    CHECK_EQ(andOp<std::uint64_t, std::uint64_t>(0xFFFFFFFF00000000ull, 0x00000000FFFFFFFFull), 0ull);
    CHECK_EQ(andOp<std::uint64_t, std::uint64_t>(0xDEADBEEFCAFEBABEull, 0xFFFFFFFF00000000ull), 0xDEADBEEF00000000ull);
  }

  SUBCASE("signed types - int8_t") {
    CHECK_EQ(andOp<std::int8_t, std::int8_t>(0x7F, 0x0F), static_cast<std::int8_t>(0x0F));
    CHECK_EQ(andOp<std::int8_t, std::int8_t>(-1, 0x55), static_cast<std::int8_t>(0x55));
  }

  SUBCASE("signed types - int16_t") {
    CHECK_EQ(andOp<std::int16_t, std::int16_t>(0x7FFF, 0x00FF), static_cast<std::int16_t>(0x00FF));
    CHECK_EQ(andOp<std::int16_t, std::int16_t>(-1, 0x0F0F), static_cast<std::int16_t>(0x0F0F));
  }

  SUBCASE("signed types - int32_t") {
    CHECK_EQ(andOp<std::int32_t, std::int32_t>(0x7FFFFFFF, 0x0000FFFF), 0x0000FFFF);
    CHECK_EQ(andOp<std::int32_t, std::int32_t>(-1, 0x12345678), 0x12345678);
  }

  SUBCASE("signed types - int64_t") {
    CHECK_EQ(andOp<std::int64_t, std::int64_t>(0x7FFFFFFFFFFFFFFFll, 0x00000000FFFFFFFFll), 0x00000000FFFFFFFFll);
    CHECK_EQ(andOp<std::int64_t, std::int64_t>(-1, 0x123456789ABCDEFll), 0x123456789ABCDEFll);
  }
}

TEST_CASE("andOp - different sizes, same signedness") {
  SUBCASE("unsigned: uint8_t with uint16_t") {
    CHECK_EQ(andOp<std::uint8_t, std::uint16_t>(0xFFu, 0x00FFu), static_cast<std::uint8_t>(0xFFu));
    CHECK_EQ(andOp<std::uint16_t, std::uint8_t>(0xABCDu, 0xCDu), static_cast<std::uint16_t>(0x00CDu));
  }

  SUBCASE("unsigned: uint8_t with uint32_t") {
    CHECK_EQ(andOp<std::uint8_t, std::uint32_t>(0xF0u, 0x0Fu), static_cast<std::uint8_t>(0u));
    CHECK_EQ(andOp<std::uint32_t, std::uint8_t>(0x12345678u, 0xFFu), 0x00000078u);
  }

  SUBCASE("unsigned: uint16_t with uint64_t") {
    CHECK_EQ(andOp<std::uint16_t, std::uint64_t>(0xFFFFu, 0x0000000000000FFFull), static_cast<std::uint16_t>(0x0FFFu));
    CHECK_EQ(andOp<std::uint64_t, std::uint16_t>(0xDEADBEEFCAFEBABEull, 0xBABEu), 0x000000000000BABEull);
  }

  SUBCASE("signed: int8_t with int32_t") {
    CHECK_EQ(andOp<std::int8_t, std::int32_t>(-1, 0x0F), static_cast<std::int8_t>(0x0F));
    CHECK_EQ(andOp<std::int32_t, std::int8_t>(0x12345678, 0x0F), 0x00000008);
  }

  SUBCASE("signed: int16_t with int64_t") {
    CHECK_EQ(andOp<std::int16_t, std::int64_t>(-1, 0xFFll), static_cast<std::int16_t>(0xFFll));
    CHECK_EQ(andOp<std::int64_t, std::int16_t>(0x123456789ABCDEFll, 0x0FFF), 0x0000000000000DEFll);
  }
}

TEST_CASE("andOp - mixed signedness") {
  SUBCASE("unsigned with signed: uint8_t with int8_t") {
    CHECK_EQ(andOp<std::uint8_t, std::int8_t>(0b11111111u, 0b01010101), static_cast<std::uint8_t>(0b01010101u));
    CHECK_EQ(andOp<std::uint8_t, std::int8_t>(0xFFu, -1), static_cast<std::uint8_t>(0xFFu));
  }

  SUBCASE("signed with unsigned: int16_t with uint8_t") {
    CHECK_EQ(andOp<std::int16_t, std::uint8_t>(-1, 0x0Fu), static_cast<std::int16_t>(0x0Fu));
    CHECK_EQ(andOp<std::int16_t, std::uint8_t>(0x1234, 0xFFu), static_cast<std::int16_t>(0x0034));
  }

  SUBCASE("unsigned with signed: uint32_t with int16_t") {
    CHECK_EQ(andOp<std::uint32_t, std::int16_t>(0xFFFFFFFFu, -1), 0xFFFFFFFFu);
    CHECK_EQ(andOp<std::uint32_t, std::int16_t>(0x12345678u, 0x00FF), 0x00000078u);
  }

  SUBCASE("signed with unsigned: int32_t with uint16_t") {
    CHECK_EQ(andOp<std::int32_t, std::uint16_t>(-1, 0xFFFFu), 0x0000FFFF);
    CHECK_EQ(andOp<std::int32_t, std::uint16_t>(0x12345678, 0xF0F0u), 0x00005070);
  }
}

TEST_CASE("orOp - basic invariants") {
  SUBCASE("identity with 0") {
    CHECK_EQ(orOp<std::uint32_t, std::uint32_t>(0x12345678u, 0u), 0x12345678u);
    CHECK_EQ(orOp<std::int32_t, std::int32_t>(123, 0), 123);
  }

  SUBCASE("all bits set with -1") {
    CHECK_EQ(orOp<std::uint8_t, std::uint8_t>(0x00u, 0xFFu), static_cast<std::uint8_t>(0xFFu));
    CHECK_EQ(orOp<std::int32_t, std::int32_t>(0, -1), -1);
  }

  SUBCASE("same value with itself") {
    CHECK_EQ(orOp<std::uint32_t, std::uint32_t>(0xDEADBEEFu, 0xDEADBEEFu), 0xDEADBEEFu);
    CHECK_EQ(orOp<std::int16_t, std::int16_t>(-123, -123), -123);
  }
}

TEST_CASE("orOp - same types") {
  SUBCASE("unsigned types - uint8_t") {
    CHECK_EQ(orOp<std::uint8_t, std::uint8_t>(0b11110000u, 0b00001111u), static_cast<std::uint8_t>(0b11111111u));
    CHECK_EQ(orOp<std::uint8_t, std::uint8_t>(0xF0u, 0x0Fu), static_cast<std::uint8_t>(0xFFu));
  }

  SUBCASE("unsigned types - uint16_t") {
    CHECK_EQ(orOp<std::uint16_t, std::uint16_t>(0xF0F0u, 0x0F0Fu), static_cast<std::uint16_t>(0xFFFFu));
    CHECK_EQ(orOp<std::uint16_t, std::uint16_t>(0xAB00u, 0x00CDu), static_cast<std::uint16_t>(0xABCDu));
  }

  SUBCASE("unsigned types - uint32_t") {
    CHECK_EQ(orOp<std::uint32_t, std::uint32_t>(0xFFFF0000u, 0x0000FFFFu), 0xFFFFFFFFu);
    CHECK_EQ(orOp<std::uint32_t, std::uint32_t>(0x12340000u, 0x00005678u), 0x12345678u);
  }

  SUBCASE("unsigned types - uint64_t") {
    CHECK_EQ(orOp<std::uint64_t, std::uint64_t>(0xFFFFFFFF00000000ull, 0x00000000FFFFFFFFull), 0xFFFFFFFFFFFFFFFFull);
    CHECK_EQ(orOp<std::uint64_t, std::uint64_t>(0xDEADBEEF00000000ull, 0x00000000CAFEBABEull), 0xDEADBEEFCAFEBABEull);
  }

  SUBCASE("signed types - int8_t") {
    CHECK_EQ(orOp<std::int8_t, std::int8_t>(0x70, 0x0F), static_cast<std::int8_t>(0x7F));
    CHECK_EQ(orOp<std::int8_t, std::int8_t>(0x10, 0x01), static_cast<std::int8_t>(0x11));
  }

  SUBCASE("signed types - int16_t") {
    CHECK_EQ(orOp<std::int16_t, std::int16_t>(0x7F00, 0x00FF), static_cast<std::int16_t>(0x7FFF));
    CHECK_EQ(orOp<std::int16_t, std::int16_t>(0x00F0, 0x0F00), static_cast<std::int16_t>(0x0FF0));
  }

  SUBCASE("signed types - int32_t") {
    CHECK_EQ(orOp<std::int32_t, std::int32_t>(0x7FFF0000, 0x0000FFFF), 0x7FFFFFFF);
    CHECK_EQ(orOp<std::int32_t, std::int32_t>(0x12340000, 0x00005678), 0x12345678);
  }

  SUBCASE("signed types - int64_t") {
    CHECK_EQ(orOp<std::int64_t, std::int64_t>(0x7FFFFFFF00000000ll, 0x00000000FFFFFFFFll), 0x7FFFFFFFFFFFFFFFll);
    CHECK_EQ(orOp<std::int64_t, std::int64_t>(0x0123456700000000ll, 0x0000000089ABCDEFll), 0x0123456789ABCDEFll);
  }
}

TEST_CASE("orOp - different sizes, same signedness") {
  SUBCASE("unsigned: uint8_t with uint16_t") {
    CHECK_EQ(orOp<std::uint8_t, std::uint16_t>(0xF0u, 0x000Fu), static_cast<std::uint8_t>(0xFFu));
    CHECK_EQ(orOp<std::uint16_t, std::uint8_t>(0xAB00u, 0xCDu), static_cast<std::uint16_t>(0xABCDu));
  }

  SUBCASE("unsigned: uint8_t with uint32_t") {
    CHECK_EQ(orOp<std::uint8_t, std::uint32_t>(0xF0u, 0x0Fu), static_cast<std::uint8_t>(0xFFu));
    CHECK_EQ(orOp<std::uint32_t, std::uint8_t>(0x12345600u, 0x78u), 0x12345678u);
  }

  SUBCASE("unsigned: uint16_t with uint64_t") {
    CHECK_EQ(orOp<std::uint16_t, std::uint64_t>(0xF000u, 0x0FFFull), static_cast<std::uint16_t>(0xFFFFu));
    CHECK_EQ(orOp<std::uint64_t, std::uint16_t>(0xFFFFFFFF00000000ull, 0x1234u), 0xFFFFFFFF00001234ull);
  }

  SUBCASE("signed: int8_t with int32_t") {
    CHECK_EQ(orOp<std::int8_t, std::int32_t>(0x10, 0x01), static_cast<std::int8_t>(0x11));
    CHECK_EQ(orOp<std::int32_t, std::int8_t>(0x12345670, 0x08), 0x12345678);
  }

  SUBCASE("signed: int16_t with int64_t") {
    CHECK_EQ(orOp<std::int16_t, std::int64_t>(0x00F0, 0x0F00ll), static_cast<std::int16_t>(0x0FF0));
    CHECK_EQ(orOp<std::int64_t, std::int16_t>(0x123456789ABC0000ll, 0x0DEF), 0x123456789ABC0DEFll);
  }
}

TEST_CASE("orOp - mixed signedness") {
  SUBCASE("unsigned with signed: uint8_t with int8_t") {
    CHECK_EQ(orOp<std::uint8_t, std::int8_t>(0b11110000u, 0b00001111), static_cast<std::uint8_t>(0b11111111u));
    CHECK_EQ(orOp<std::uint8_t, std::int8_t>(0x00u, 0x0F), static_cast<std::uint8_t>(0x0Fu));
  }

  SUBCASE("signed with unsigned: int16_t with uint8_t") {
    CHECK_EQ(orOp<std::int16_t, std::uint8_t>(0x00F0, 0x0Fu), static_cast<std::int16_t>(0x00FF));
    CHECK_EQ(orOp<std::int16_t, std::uint8_t>(0x1200, 0x34u), static_cast<std::int16_t>(0x1234));
  }

  SUBCASE("unsigned with signed: uint32_t with int16_t") {
    CHECK_EQ(orOp<std::uint32_t, std::int16_t>(0xFFFF0000u, 0x00FF), 0xFFFF00FFu);
    CHECK_EQ(orOp<std::uint32_t, std::int16_t>(0x12340000u, 0x5678), 0x12345678u);
  }

  SUBCASE("signed with unsigned: int32_t with uint16_t") {
    CHECK_EQ(orOp<std::int32_t, std::uint16_t>(0x12340000, 0x5678u), 0x12345678);
    CHECK_EQ(orOp<std::int32_t, std::uint16_t>(0x00F00000, 0x0F0Fu), 0x00F00F0F);
  }
}

TEST_CASE("xorOp - basic invariants") {
  SUBCASE("identity with 0") {
    CHECK_EQ(xorOp<std::uint32_t, std::uint32_t>(0x12345678u, 0u), 0x12345678u);
    CHECK_EQ(xorOp<std::int32_t, std::int32_t>(123, 0), 123);
  }

  SUBCASE("zero with itself") {
    CHECK_EQ(xorOp<std::uint32_t, std::uint32_t>(0xDEADBEEFu, 0xDEADBEEFu), 0u);
    CHECK_EQ(xorOp<std::int16_t, std::int16_t>(-123, -123), 0);
  }

  SUBCASE("inversion with all bits set") {
    CHECK_EQ(xorOp<std::uint8_t, std::uint8_t>(0xAAu, 0xFFu), static_cast<std::uint8_t>(0x55u));
    CHECK_EQ(xorOp<std::int8_t, std::int8_t>(0x55, -1), static_cast<std::int8_t>(0xAA));
  }
}

TEST_CASE("xorOp - same types") {
  SUBCASE("unsigned types - uint8_t") {
    CHECK_EQ(xorOp<std::uint8_t, std::uint8_t>(0b11110000u, 0b10101010u), static_cast<std::uint8_t>(0b01011010u));
    CHECK_EQ(xorOp<std::uint8_t, std::uint8_t>(0xFFu, 0xAAu), static_cast<std::uint8_t>(0x55u));
  }

  SUBCASE("unsigned types - uint16_t") {
    CHECK_EQ(xorOp<std::uint16_t, std::uint16_t>(0xF0F0u, 0xFF00u), static_cast<std::uint16_t>(0x0FF0u));
    CHECK_EQ(xorOp<std::uint16_t, std::uint16_t>(0xABCDu, 0xFFFFu), static_cast<std::uint16_t>(0x5432u));
  }

  SUBCASE("unsigned types - uint32_t") {
    CHECK_EQ(xorOp<std::uint32_t, std::uint32_t>(0xFFFF0000u, 0x0000FFFFu), 0xFFFFFFFFu);
    CHECK_EQ(xorOp<std::uint32_t, std::uint32_t>(0x12345678u, 0x0F0F0F0Fu), 0x1D3B5977u);
  }

  SUBCASE("unsigned types - uint64_t") {
    CHECK_EQ(xorOp<std::uint64_t, std::uint64_t>(0xFFFFFFFFFFFFFFFFull, 0x12345678u), 0xFFFFFFFFEDCBA987ull);
    CHECK_EQ(xorOp<std::uint64_t, std::uint64_t>(0xDEADBEEFCAFEBABEull, 0xDEADBEEFCAFEBABEull), 0ull);
  }

  SUBCASE("signed types - int8_t") {
    CHECK_EQ(xorOp<std::int8_t, std::int8_t>(0x7F, 0x55), static_cast<std::int8_t>(0x2A));
    CHECK_EQ(xorOp<std::int8_t, std::int8_t>(-1, 0x55), static_cast<std::int8_t>(0xAA));
  }

  SUBCASE("signed types - int16_t") {
    CHECK_EQ(xorOp<std::int16_t, std::int16_t>(0x7FFF, 0x00FF), static_cast<std::int16_t>(0x7F00));
    CHECK_EQ(xorOp<std::int16_t, std::int16_t>(0x1234, 0x4321), static_cast<std::int16_t>(0x5115));
  }

  SUBCASE("signed types - int32_t") {
    CHECK_EQ(xorOp<std::int32_t, std::int32_t>(0x7FFFFFFF, 0x0000FFFF), 0x7FFF0000);
    CHECK_EQ(xorOp<std::int32_t, std::int32_t>(0x12345678, 0), 0x12345678);
  }

  SUBCASE("signed types - int64_t") {
    CHECK_EQ(xorOp<std::int64_t, std::int64_t>(0x7FFFFFFFFFFFFFFFll, 0x00000000FFFFFFFFll), 0x7FFFFFFF00000000ll);
    CHECK_EQ(xorOp<std::int64_t, std::int64_t>(0x0123456789ABCDEFll, 0x0123456789ABCDEFll), 0ll);
  }
}

TEST_CASE("xorOp - different sizes, same signedness") {
  SUBCASE("unsigned: uint8_t with uint16_t") {
    CHECK_EQ(xorOp<std::uint8_t, std::uint16_t>(0xFFu, 0x00AAu), static_cast<std::uint8_t>(0x55u));
    CHECK_EQ(xorOp<std::uint16_t, std::uint8_t>(0xABCDu, 0xFFu), static_cast<std::uint16_t>(0xAB32u));
  }

  SUBCASE("unsigned: uint8_t with uint32_t") {
    CHECK_EQ(xorOp<std::uint8_t, std::uint32_t>(0xF0u, 0x0Fu), static_cast<std::uint8_t>(0xFFu));
    CHECK_EQ(xorOp<std::uint32_t, std::uint8_t>(0x12345678u, 0xFFu), 0x12345687u);
  }

  SUBCASE("unsigned: uint16_t with uint64_t") {
    CHECK_EQ(xorOp<std::uint16_t, std::uint64_t>(0xFFFFu, 0x0000000000000FFFull), static_cast<std::uint16_t>(0xF000u));
    CHECK_EQ(xorOp<std::uint64_t, std::uint16_t>(0xDEADBEEFCAFEBABEull, 0xFFFFu), 0xDEADBEEFCAFE4541ull);
  }

  SUBCASE("signed: int8_t with int32_t") {
    CHECK_EQ(xorOp<std::int8_t, std::int32_t>(0x0F, 0x05), static_cast<std::int8_t>(0x0A));
    CHECK_EQ(xorOp<std::int32_t, std::int8_t>(0x12345678, 0x0F), 0x12345677);
  }

  SUBCASE("signed: int16_t with int64_t") {
    CHECK_EQ(xorOp<std::int16_t, std::int64_t>(0x00FF, 0xF000ll), static_cast<std::int16_t>(0xF0FF));
    CHECK_EQ(xorOp<std::int64_t, std::int16_t>(0x123456789ABC0DEFll, 0x0FFF), 0x123456789ABC0210ll);
  }
}

TEST_CASE("xorOp - mixed signedness") {
  SUBCASE("unsigned with signed: uint8_t with int8_t") {
    CHECK_EQ(xorOp<std::uint8_t, std::int8_t>(0b11111111u, 0b01010101), static_cast<std::uint8_t>(0b10101010u));
    CHECK_EQ(xorOp<std::uint8_t, std::int8_t>(0xFFu, -1), static_cast<std::uint8_t>(0x00u));
  }

  SUBCASE("signed with unsigned: int16_t with uint8_t") {
    CHECK_EQ(xorOp<std::int16_t, std::uint8_t>(0x00FF, 0xF0u), static_cast<std::int16_t>(0x000F));
    CHECK_EQ(xorOp<std::int16_t, std::uint8_t>(0x1234, 0xFFu), static_cast<std::int16_t>(0x12CB));
  }

  SUBCASE("unsigned with signed: uint32_t with int16_t") {
    CHECK_EQ(xorOp<std::uint32_t, std::int16_t>(0xFFFF0000u, 0x00FF), 0xFFFF00FFu);
    CHECK_EQ(xorOp<std::uint32_t, std::int16_t>(0x12345678u, -1), 0xEDCBA987u);
  }

  SUBCASE("signed with unsigned: int32_t with uint16_t") {
    CHECK_EQ(xorOp<std::int32_t, std::uint16_t>(0x12345678, 0xFFFFu), 0x1234A987);
    CHECK_EQ(xorOp<std::int32_t, std::uint16_t>(0x00F00F00, 0x0F0Fu), 0x00F0000F);
  }
}

TEST_CASE("bitsAreSet - basic invariants") {
  SUBCASE("no bits set returns false") {
    CHECK_FALSE(bitsAreSet<std::uint32_t, std::uint32_t>(0x12345678u, 0u));
    CHECK_FALSE(bitsAreSet<std::uint8_t, std::uint8_t>(0xFFu, 0u));
  }

  SUBCASE("checking against zero source returns false") {
    CHECK_FALSE(bitsAreSet<std::uint32_t, std::uint32_t>(0u, 0xFFu));
    CHECK_FALSE(bitsAreSet<std::int32_t, std::int32_t>(0, -1));
  }

  SUBCASE("all bits match returns true") {
    CHECK(bitsAreSet<std::uint32_t, std::uint32_t>(0xFFFFFFFFu, 0xFFFFFFFFu));
    CHECK(bitsAreSet<std::uint8_t, std::uint8_t>(0xFFu, 0xFFu));
  }

  SUBCASE("partial match returns true") {
    CHECK(bitsAreSet<std::uint32_t, std::uint32_t>(0x12345678u, 0x00000008u));
    CHECK(bitsAreSet<std::uint8_t, std::uint8_t>(0b11110000u, 0b00010000u));
  }
}

TEST_CASE("bitsAreSet - same types unsigned") {
  SUBCASE("uint8_t checks") {
    CHECK(bitsAreSet<std::uint8_t, std::uint8_t>(0b11110000u, 0b10000000u));
    CHECK_FALSE(bitsAreSet<std::uint8_t, std::uint8_t>(0b11110000u, 0b00001111u));
    CHECK(bitsAreSet<std::uint8_t, std::uint8_t>(0xFFu, 0x01u));
    CHECK(bitsAreSet<std::uint8_t, std::uint8_t>(0xAAu, 0x80u));
  }

  SUBCASE("uint16_t checks") {
    CHECK(bitsAreSet<std::uint16_t, std::uint16_t>(0xF0F0u, 0x8000u));
    CHECK_FALSE(bitsAreSet<std::uint16_t, std::uint16_t>(0xF0F0u, 0x0F0Fu));
    CHECK(bitsAreSet<std::uint16_t, std::uint16_t>(0xABCDu, 0x0001u));
    CHECK(bitsAreSet<std::uint16_t, std::uint16_t>(0xFFFFu, 0x1234u));
  }

  SUBCASE("uint32_t checks") {
    CHECK(bitsAreSet<std::uint32_t, std::uint32_t>(0x12345678u, 0x00000008u));
    CHECK_FALSE(bitsAreSet<std::uint32_t, std::uint32_t>(0x12345678u, 0x80000000u));
    CHECK(bitsAreSet<std::uint32_t, std::uint32_t>(0xFFFFFFFFu, 0x00000001u));
    CHECK(bitsAreSet<std::uint32_t, std::uint32_t>(0xF0F0F0F0u, 0x10101010u));
  }

  SUBCASE("uint64_t checks") {
    CHECK(bitsAreSet<std::uint64_t, std::uint64_t>(0xDEADBEEFCAFEBABEull, 0x0000000000000002ull));
    CHECK_FALSE(bitsAreSet<std::uint64_t, std::uint64_t>(0xDEADBEEFCAFEBABEull, 0x0000000000000001ull));
    CHECK(bitsAreSet<std::uint64_t, std::uint64_t>(0xFFFFFFFFFFFFFFFFull, 0x8000000000000000ull));
  }
}

TEST_CASE("bitsAreSet - same types signed") {
  SUBCASE("int8_t checks") {
    CHECK(bitsAreSet<std::int8_t, std::int8_t>(-1, 0x01));
    CHECK(bitsAreSet<std::int8_t, std::int8_t>(0x7F, 0x40));
    CHECK_FALSE(bitsAreSet<std::int8_t, std::int8_t>(0x0F, 0x80));
  }

  SUBCASE("int16_t checks") {
    CHECK(bitsAreSet<std::int16_t, std::int16_t>(-1, 0x00FF));
    CHECK(bitsAreSet<std::int16_t, std::int16_t>(0x7FFF, 0x0001));
    CHECK_FALSE(bitsAreSet<std::int16_t, std::int16_t>(0x00FF, 0xFF00));
  }

  SUBCASE("int32_t checks") {
    CHECK(bitsAreSet<std::int32_t, std::int32_t>(-1, 0x12345678));
    CHECK(bitsAreSet<std::int32_t, std::int32_t>(0x12345678, 0x00000008));
    CHECK_FALSE(bitsAreSet<std::int32_t, std::int32_t>(0x0000FFFF, 0xFFFF0000));
  }

  SUBCASE("int64_t checks") {
    CHECK(bitsAreSet<std::int64_t, std::int64_t>(-1, 0x123456789ABCDEFll));
    CHECK(bitsAreSet<std::int64_t, std::int64_t>(0x7FFFFFFFFFFFFFFFll, 0x4000000000000000ll));
    CHECK_FALSE(bitsAreSet<std::int64_t, std::int64_t>(0x00000000FFFFFFFFll, 0xFFFFFFFF00000000ll));
  }
}

TEST_CASE("bitsAreSet - different sizes, same signedness") {
  SUBCASE("unsigned: uint8_t with uint32_t") {
    CHECK(bitsAreSet<std::uint8_t, std::uint32_t>(0xFFu, 0x00000001u));
    CHECK_FALSE(bitsAreSet<std::uint8_t, std::uint32_t>(0x0Fu, 0x00000080u));
    CHECK(bitsAreSet<std::uint32_t, std::uint8_t>(0x12345678u, 0x08u));
  }

  SUBCASE("unsigned: uint16_t with uint64_t") {
    CHECK(bitsAreSet<std::uint16_t, std::uint64_t>(0xFFFFu, 0x0000000000000001ull));
    CHECK_FALSE(bitsAreSet<std::uint16_t, std::uint64_t>(0x00FFu, 0x0000000000008000ull));
    CHECK(bitsAreSet<std::uint64_t, std::uint16_t>(0xDEADBEEFCAFEBABEull, 0x0002u));
  }

  SUBCASE("signed: int8_t with int32_t") {
    CHECK(bitsAreSet<std::int8_t, std::int32_t>(-1, 0x00000001));
    CHECK_FALSE(bitsAreSet<std::int8_t, std::int32_t>(0x0F, 0x00000080));
    CHECK(bitsAreSet<std::int32_t, std::int8_t>(0x12345678, 0x08));
  }

  SUBCASE("signed: int16_t with int64_t") {
    CHECK(bitsAreSet<std::int16_t, std::int64_t>(-1, 0x0001ll));
    CHECK_FALSE(bitsAreSet<std::int16_t, std::int64_t>(0x00FF, 0x8000ll));
    CHECK(bitsAreSet<std::int64_t, std::int16_t>(0x123456789ABCDEFll, 0x000F));
  }
}

TEST_CASE("bitsAreSet - mixed signedness") {
  SUBCASE("unsigned with signed: uint8_t with int8_t") {
    CHECK(bitsAreSet<std::uint8_t, std::int8_t>(0xFFu, -1));
    CHECK(bitsAreSet<std::uint8_t, std::int8_t>(0b11110000u, 0b01010000));
    CHECK_FALSE(bitsAreSet<std::uint8_t, std::int8_t>(0b11110000u, 0b00001111));
  }

  SUBCASE("signed with unsigned: int16_t with uint8_t") {
    CHECK(bitsAreSet<std::int16_t, std::uint8_t>(-1, 0xFFu));
    CHECK(bitsAreSet<std::int16_t, std::uint8_t>(0x1234, 0x10u));
    CHECK_FALSE(bitsAreSet<std::int16_t, std::uint8_t>(0x007F, 0x80u));
  }

  SUBCASE("unsigned with signed: uint32_t with int16_t") {
    CHECK(bitsAreSet<std::uint32_t, std::int16_t>(0xFFFFFFFFu, -1));
    CHECK(bitsAreSet<std::uint32_t, std::int16_t>(0x12345678u, 0x0008));
    CHECK_FALSE(bitsAreSet<std::uint32_t, std::int16_t>(0x00007FFFu, 0x8000));
  }

  SUBCASE("signed with unsigned: int32_t with uint16_t") {
    CHECK(bitsAreSet<std::int32_t, std::uint16_t>(-1, 0xFFFFu));
    CHECK(bitsAreSet<std::int32_t, std::uint16_t>(0x12345678, 0x0070u));
    CHECK_FALSE(bitsAreSet<std::int32_t, std::uint16_t>(0x00007FFF, 0x8000u));
  }
}

TEST_CASE("setBits - basic invariants") {
  SUBCASE("setting no bits returns original") {
    CHECK_EQ(setBits<std::uint32_t, std::uint32_t>(0x12345678u, 0u), 0x12345678u);
    CHECK_EQ(setBits<std::int32_t, std::int32_t>(123, 0), 123);
  }

  SUBCASE("setting on zero returns mask") {
    CHECK_EQ(setBits<std::uint32_t, std::uint32_t>(0u, 0x12345678u), 0x12345678u);
    CHECK_EQ(setBits<std::uint8_t, std::uint8_t>(0u, 0xFFu), static_cast<std::uint8_t>(0xFFu));
  }

  SUBCASE("setting already set bits is idempotent") {
    CHECK_EQ(setBits<std::uint32_t, std::uint32_t>(0xFFFFFFFFu, 0x12345678u), 0xFFFFFFFFu);
    CHECK_EQ(setBits<std::int16_t, std::int16_t>(-1, 0x00FF), -1);
  }
}

TEST_CASE("setBits - same types unsigned") {
  SUBCASE("uint8_t operations") {
    CHECK_EQ(setBits<std::uint8_t, std::uint8_t>(0b11110000u, 0b00001111u), static_cast<std::uint8_t>(0b11111111u));
    CHECK_EQ(setBits<std::uint8_t, std::uint8_t>(0x00u, 0xFFu), static_cast<std::uint8_t>(0xFFu));
    CHECK_EQ(setBits<std::uint8_t, std::uint8_t>(0xF0u, 0x0Fu), static_cast<std::uint8_t>(0xFFu));
    CHECK_EQ(setBits<std::uint8_t, std::uint8_t>(0xAAu, 0x55u), static_cast<std::uint8_t>(0xFFu));
  }

  SUBCASE("uint16_t operations") {
    CHECK_EQ(setBits<std::uint16_t, std::uint16_t>(0xF0F0u, 0x0F0Fu), static_cast<std::uint16_t>(0xFFFFu));
    CHECK_EQ(setBits<std::uint16_t, std::uint16_t>(0xAB00u, 0x00CDu), static_cast<std::uint16_t>(0xABCDu));
    CHECK_EQ(setBits<std::uint16_t, std::uint16_t>(0x0000u, 0x1234u), static_cast<std::uint16_t>(0x1234u));
  }

  SUBCASE("uint32_t operations") {
    CHECK_EQ(setBits<std::uint32_t, std::uint32_t>(0xFFFF0000u, 0x0000FFFFu), 0xFFFFFFFFu);
    CHECK_EQ(setBits<std::uint32_t, std::uint32_t>(0x12340000u, 0x00005678u), 0x12345678u);
    CHECK_EQ(setBits<std::uint32_t, std::uint32_t>(0x00000000u, 0xDEADBEEFu), 0xDEADBEEFu);
  }

  SUBCASE("uint64_t operations") {
    CHECK_EQ(setBits<std::uint64_t, std::uint64_t>(0xFFFFFFFF00000000ull, 0x00000000FFFFFFFFull),
             0xFFFFFFFFFFFFFFFFull);
    CHECK_EQ(setBits<std::uint64_t, std::uint64_t>(0xDEADBEEF00000000ull, 0x00000000CAFEBABEull),
             0xDEADBEEFCAFEBABEull);
  }
}

TEST_CASE("setBits - same types signed") {
  SUBCASE("int8_t operations") {
    CHECK_EQ(setBits<std::int8_t, std::int8_t>(0x70, 0x0F), static_cast<std::int8_t>(0x7F));
    CHECK_EQ(setBits<std::int8_t, std::int8_t>(0x00, -1), static_cast<std::int8_t>(-1));
    CHECK_EQ(setBits<std::int8_t, std::int8_t>(0x10, 0x01), static_cast<std::int8_t>(0x11));
  }

  SUBCASE("int16_t operations") {
    CHECK_EQ(setBits<std::int16_t, std::int16_t>(0x7F00, 0x00FF), static_cast<std::int16_t>(0x7FFF));
    CHECK_EQ(setBits<std::int16_t, std::int16_t>(0x0000, -1), static_cast<std::int16_t>(-1));
    CHECK_EQ(setBits<std::int16_t, std::int16_t>(0x1200, 0x0034), static_cast<std::int16_t>(0x1234));
  }

  SUBCASE("int32_t operations") {
    CHECK_EQ(setBits<std::int32_t, std::int32_t>(0x7FFF0000, 0x0000FFFF), 0x7FFFFFFF);
    CHECK_EQ(setBits<std::int32_t, std::int32_t>(0x12340000, 0x00005678), 0x12345678);
    CHECK_EQ(setBits<std::int32_t, std::int32_t>(0x00000000, -1), -1);
  }

  SUBCASE("int64_t operations") {
    CHECK_EQ(setBits<std::int64_t, std::int64_t>(0x7FFFFFFF00000000ll, 0x00000000FFFFFFFFll), 0x7FFFFFFFFFFFFFFFll);
    CHECK_EQ(setBits<std::int64_t, std::int64_t>(0x0123456700000000ll, 0x0000000089ABCDEFll), 0x0123456789ABCDEFll);
  }
}

TEST_CASE("setBits - different sizes, same signedness") {
  SUBCASE("unsigned: uint8_t with uint32_t") {
    CHECK_EQ(setBits<std::uint8_t, std::uint32_t>(0xF0u, 0x0000000Fu), static_cast<std::uint8_t>(0xFFu));
    CHECK_EQ(setBits<std::uint32_t, std::uint8_t>(0x12345600u, 0x78u), 0x12345678u);
  }

  SUBCASE("unsigned: uint16_t with uint64_t") {
    CHECK_EQ(setBits<std::uint16_t, std::uint64_t>(0xF000u, 0x0FFFull), static_cast<std::uint16_t>(0xFFFFu));
    CHECK_EQ(setBits<std::uint64_t, std::uint16_t>(0xFFFFFFFF00000000ull, 0x1234u), 0xFFFFFFFF00001234ull);
  }

  SUBCASE("signed: int8_t with int32_t") {
    CHECK_EQ(setBits<std::int8_t, std::int32_t>(0x10, 0x01), static_cast<std::int8_t>(0x11));
    CHECK_EQ(setBits<std::int32_t, std::int8_t>(0x12345670, 0x08), 0x12345678);
  }

  SUBCASE("signed: int16_t with int64_t") {
    CHECK_EQ(setBits<std::int16_t, std::int64_t>(0x00F0, 0x0F00ll), static_cast<std::int16_t>(0x0FF0));
    CHECK_EQ(setBits<std::int64_t, std::int16_t>(0x123456789ABC0000ll, 0x0DEF), 0x123456789ABC0DEFll);
  }
}

TEST_CASE("setBits - mixed signedness") {
  SUBCASE("unsigned with signed: uint8_t with int8_t") {
    CHECK_EQ(setBits<std::uint8_t, std::int8_t>(0b11110000u, 0b00001111), static_cast<std::uint8_t>(0b11111111u));
    CHECK_EQ(setBits<std::uint8_t, std::int8_t>(0x00u, -1), static_cast<std::uint8_t>(0xFFu));
  }

  SUBCASE("signed with unsigned: int16_t with uint8_t") {
    CHECK_EQ(setBits<std::int16_t, std::uint8_t>(0x00F0, 0x0Fu), static_cast<std::int16_t>(0x00FF));
    CHECK_EQ(setBits<std::int16_t, std::uint8_t>(0x1200, 0x34u), static_cast<std::int16_t>(0x1234));
  }

  SUBCASE("unsigned with signed: uint32_t with int16_t") {
    CHECK_EQ(setBits<std::uint32_t, std::int16_t>(0xFFFF0000u, 0x00FF), 0xFFFF00FFu);
    CHECK_EQ(setBits<std::uint32_t, std::int16_t>(0x12340000u, 0x5678), 0x12345678u);
  }

  SUBCASE("signed with unsigned: int32_t with uint16_t") {
    CHECK_EQ(setBits<std::int32_t, std::uint16_t>(0x12340000, 0x5678u), 0x12345678);
    CHECK_EQ(setBits<std::int32_t, std::uint16_t>(0x00F00000, 0x0F0Fu), 0x00F00F0F);
  }
}

TEST_CASE("resetBits - basic invariants") {
  SUBCASE("resetting no bits returns original") {
    CHECK_EQ(resetBits<std::uint32_t, std::uint32_t>(0x12345678u, 0u), 0x12345678u);
    CHECK_EQ(resetBits<std::int32_t, std::int32_t>(123, 0), 123);
  }

  SUBCASE("resetting all bits returns zero") {
    CHECK_EQ(resetBits<std::uint32_t, std::uint32_t>(0x12345678u, 0xFFFFFFFFu), 0u);
    CHECK_EQ(resetBits<std::uint8_t, std::uint8_t>(0xFFu, 0xFFu), static_cast<std::uint8_t>(0u));
  }

  SUBCASE("resetting from zero returns zero") {
    CHECK_EQ(resetBits<std::uint32_t, std::uint32_t>(0u, 0x12345678u), 0u);
    CHECK_EQ(resetBits<std::int32_t, std::int32_t>(0, -1), 0);
  }
}

TEST_CASE("resetBits - same types unsigned") {
  SUBCASE("uint8_t operations") {
    CHECK_EQ(resetBits<std::uint8_t, std::uint8_t>(0b11111111u, 0b00001111u), static_cast<std::uint8_t>(0b11110000u));
    CHECK_EQ(resetBits<std::uint8_t, std::uint8_t>(0xFFu, 0x0Fu), static_cast<std::uint8_t>(0xF0u));
    CHECK_EQ(resetBits<std::uint8_t, std::uint8_t>(0xFFu, 0xFFu), static_cast<std::uint8_t>(0x00u));
    CHECK_EQ(resetBits<std::uint8_t, std::uint8_t>(0xAAu, 0x55u), static_cast<std::uint8_t>(0xAAu));
  }

  SUBCASE("uint16_t operations") {
    CHECK_EQ(resetBits<std::uint16_t, std::uint16_t>(0xFFFFu, 0x0F0Fu), static_cast<std::uint16_t>(0xF0F0u));
    CHECK_EQ(resetBits<std::uint16_t, std::uint16_t>(0xABCDu, 0x00CDu), static_cast<std::uint16_t>(0xAB00u));
    CHECK_EQ(resetBits<std::uint16_t, std::uint16_t>(0x1234u, 0x0034u), static_cast<std::uint16_t>(0x1200u));
  }

  SUBCASE("uint32_t operations") {
    CHECK_EQ(resetBits<std::uint32_t, std::uint32_t>(0xFFFFFFFFu, 0x0000FFFFu), 0xFFFF0000u);
    CHECK_EQ(resetBits<std::uint32_t, std::uint32_t>(0x12345678u, 0x00005678u), 0x12340000u);
    CHECK_EQ(resetBits<std::uint32_t, std::uint32_t>(0xDEADBEEFu, 0xFFFFFFFFu), 0x00000000u);
  }

  SUBCASE("uint64_t operations") {
    CHECK_EQ(resetBits<std::uint64_t, std::uint64_t>(0xFFFFFFFFFFFFFFFFull, 0x00000000FFFFFFFFull),
             0xFFFFFFFF00000000ull);
    CHECK_EQ(resetBits<std::uint64_t, std::uint64_t>(0xDEADBEEFCAFEBABEull, 0x00000000CAFEBABEull),
             0xDEADBEEF00000000ull);
  }
}

TEST_CASE("resetBits - same types signed") {
  SUBCASE("int8_t operations") {
    CHECK_EQ(resetBits<std::int8_t, std::int8_t>(0x7F, 0x0F), static_cast<std::int8_t>(0x70));
    CHECK_EQ(resetBits<std::int8_t, std::int8_t>(-1, 0x0F), static_cast<std::int8_t>(0xF0));
    CHECK_EQ(resetBits<std::int8_t, std::int8_t>(0x11, 0x01), static_cast<std::int8_t>(0x10));
  }

  SUBCASE("int16_t operations") {
    CHECK_EQ(resetBits<std::int16_t, std::int16_t>(0x7FFF, 0x00FF), static_cast<std::int16_t>(0x7F00));
    CHECK_EQ(resetBits<std::int16_t, std::int16_t>(-1, 0x0F0F), static_cast<std::int16_t>(0xF0F0));
    CHECK_EQ(resetBits<std::int16_t, std::int16_t>(0x1234, 0x0034), static_cast<std::int16_t>(0x1200));
  }

  SUBCASE("int32_t operations") {
    CHECK_EQ(resetBits<std::int32_t, std::int32_t>(0x7FFFFFFF, 0x0000FFFF), 0x7FFF0000);
    CHECK_EQ(resetBits<std::int32_t, std::int32_t>(0x12345678, 0x00005678), 0x12340000);
    CHECK_EQ(resetBits<std::int32_t, std::int32_t>(-1, 0x0000FFFF), static_cast<std::int32_t>(0xFFFF0000));
  }

  SUBCASE("int64_t operations") {
    CHECK_EQ(resetBits<std::int64_t, std::int64_t>(0x7FFFFFFFFFFFFFFFll, 0x00000000FFFFFFFFll), 0x7FFFFFFF00000000ll);
    CHECK_EQ(resetBits<std::int64_t, std::int64_t>(0x0123456789ABCDEFll, 0x0000000089ABCDEFll), 0x0123456700000000ll);
  }
}

TEST_CASE("resetBits - different sizes, same signedness") {
  SUBCASE("unsigned: uint8_t with uint32_t") {
    CHECK_EQ(resetBits<std::uint8_t, std::uint32_t>(0xFFu, 0x0000000Fu), static_cast<std::uint8_t>(0xF0u));
    CHECK_EQ(resetBits<std::uint32_t, std::uint8_t>(0x12345678u, 0x78u), 0x12345600u);
  }

  SUBCASE("unsigned: uint16_t with uint64_t") {
    CHECK_EQ(resetBits<std::uint16_t, std::uint64_t>(0xFFFFu, 0x0FFFull), static_cast<std::uint16_t>(0xF000u));
    CHECK_EQ(resetBits<std::uint64_t, std::uint16_t>(0xFFFFFFFF00001234ull, 0x1234u), 0xFFFFFFFF00000000ull);
  }

  SUBCASE("signed: int8_t with int32_t") {
    CHECK_EQ(resetBits<std::int8_t, std::int32_t>(0x11, 0x01), static_cast<std::int8_t>(0x10));
    CHECK_EQ(resetBits<std::int32_t, std::int8_t>(0x12345678, 0x08), 0x12345670);
  }

  SUBCASE("signed: int16_t with int64_t") {
    CHECK_EQ(resetBits<std::int16_t, std::int64_t>(0x0FF0, 0x0F00ll), static_cast<std::int16_t>(0x00F0));
    CHECK_EQ(resetBits<std::int64_t, std::int16_t>(0x123456789ABC0DEFll, 0x0DEF), 0x123456789ABC0000ll);
  }
}

TEST_CASE("resetBits - mixed signedness") {
  SUBCASE("unsigned with signed: uint8_t with int8_t") {
    CHECK_EQ(resetBits<std::uint8_t, std::int8_t>(0b11111111u, 0b00001111), static_cast<std::uint8_t>(0b11110000u));
    CHECK_EQ(resetBits<std::uint8_t, std::int8_t>(0xFFu, -1), static_cast<std::uint8_t>(0x00u));
  }

  SUBCASE("signed with unsigned: int16_t with uint8_t") {
    CHECK_EQ(resetBits<std::int16_t, std::uint8_t>(0x00FF, 0xF0u), static_cast<std::int16_t>(0x000F));
    CHECK_EQ(resetBits<std::int16_t, std::uint8_t>(0x1234, 0x34u), static_cast<std::int16_t>(0x1200));
  }

  SUBCASE("unsigned with signed: uint32_t with int16_t") {
    CHECK_EQ(resetBits<std::uint32_t, std::int16_t>(0xFFFF00FFu, 0x00FF), 0xFFFF0000u);
    CHECK_EQ(resetBits<std::uint32_t, std::int16_t>(0x12345678u, 0x5678), 0x12340000u);
  }

  SUBCASE("signed with unsigned: int32_t with uint16_t") {
    CHECK_EQ(resetBits<std::int32_t, std::uint16_t>(0x1234A987, 0xA987u), 0x12340000);
    CHECK_EQ(resetBits<std::int32_t, std::uint16_t>(0x00F00F0F, 0x0F0Fu), 0x00F00000);
  }
}

TEST_CASE("resetBits - clearing specific bit patterns") {
  SUBCASE("clear individual bits") {
    CHECK_EQ(resetBits<std::uint8_t, std::uint8_t>(0xFFu, 0x01u), static_cast<std::uint8_t>(0xFEu));
    CHECK_EQ(resetBits<std::uint8_t, std::uint8_t>(0xFFu, 0x80u), static_cast<std::uint8_t>(0x7Fu));
    CHECK_EQ(resetBits<std::uint16_t, std::uint16_t>(0xFFFFu, 0x0001u), static_cast<std::uint16_t>(0xFFFEu));
  }

  SUBCASE("clear bit ranges") {
    CHECK_EQ(resetBits<std::uint32_t, std::uint32_t>(0xFFFFFFFFu, 0x00FF00FFu), 0xFF00FF00u);
    CHECK_EQ(resetBits<std::uint32_t, std::uint32_t>(0x12345678u, 0x0000FF00u), 0x12340078u);
  }

  SUBCASE("alternating patterns") {
    CHECK_EQ(resetBits<std::uint8_t, std::uint8_t>(0xFFu, 0xAAu), static_cast<std::uint8_t>(0x55u));
    CHECK_EQ(resetBits<std::uint8_t, std::uint8_t>(0xFFu, 0x55u), static_cast<std::uint8_t>(0xAAu));
  }
}
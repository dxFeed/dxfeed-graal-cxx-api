// Copyright (c) 2025 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#include <doctest/doctest.h>

#define ANKERL_NANOBENCH_IMPLEMENT
#include <nanobench.h>

#include <bits/bits.hpp>

using namespace org::ttldtor::bits;
using namespace std::literals;

TEST_CASE("bench_sal_sar_vs_builtin") {
  constexpr size_t N = 1u << 15;

  std::mt19937_64 rng{0xB00B00};
  // For comparison with builtin << we use unsigned types and non-negative shifts
  std::uniform_int_distribution u32dist{0u, 0x7FFF'FFFFu};
  std::uniform_int_distribution<uint64_t> u64dist{0ull, (1ull << 50)};
  std::uniform_int_distribution shldist{0u, 63u};

  // For comparison with builtin >> we use signed types and non-negative shifts
  std::uniform_int_distribution s32dist{-(1 << 29), (1 << 29)};
  std::uniform_int_distribution<int64_t> s64dist{-(1ll << 50), (1ll << 50)};
  std::uniform_int_distribution shrdist{0u, 63u};

  std::vector<uint32_t> u32(N);
  std::vector<unsigned> shl32(N);
  std::vector<uint64_t> u64(N);
  std::vector<unsigned> shl64(N);

  std::vector<int32_t> s32(N);
  std::vector<unsigned> shr32(N);
  std::vector<int64_t> s64(N);
  std::vector<unsigned> shr64(N);

  for (size_t i = 0; i < N; ++i) {
    u32[i] = u32dist(rng);
    shl32[i] = shldist(rng) % 32;
    u64[i] = u64dist(rng);
    shl64[i] = shldist(rng) % 64;

    s32[i] = s32dist(rng);
    shr32[i] = shrdist(rng) % 32;
    s64[i] = s64dist(rng);
    shr64[i] = shrdist(rng) % 64;
  }

  auto createBench = [&](auto title) {
    ankerl::nanobench::Bench bench;

    bench.title(title)
      .unit("op")
      .batch(N)
      .warmup(100)
      .minEpochTime(150ms)
      .minEpochIterations(60000)
      .relative(true)
      .performanceCounters(true);

    return bench;
  };

  auto runSalBench = [&](auto& bench, const auto& typeName, const auto& values, const auto& shifts) {
    bench.run("builtin << ("s + typeName + ")", [&] {
      std::decay_t<decltype(values[0])> acc = 0;

      for (size_t i = 0; i < N; ++i) {
        acc ^= values[i] << shifts[i];
      }

      ankerl::nanobench::doNotOptimizeAway(acc);
    });

    bench.run("sal<"s + typeName + ">", [&] {
      std::decay_t<decltype(values[0])> acc = 0;

      for (size_t i = 0; i < N; ++i) {
        acc ^= sal(values[i], shifts[i]);
      }

      ankerl::nanobench::doNotOptimizeAway(acc);
    });
  };

  auto runSarBench = [&](auto& bench, const auto& typeName, const auto& values, const auto& shifts) {
    bench.run("builtin >> ("s + typeName + ")", [&] {
      std::decay_t<decltype(values[0])> acc = 0;

      for (size_t i = 0; i < N; ++i) {
        acc ^= values[i] >> shifts[i];
      }

      ankerl::nanobench::doNotOptimizeAway(acc);
    });

    bench.run("sar<"s + typeName + ">", [&] {
      std::decay_t<decltype(values[0])> acc = 0;

      for (size_t i = 0; i < N; ++i) {
        acc ^= sar(values[i], shifts[i]);
      }

      ankerl::nanobench::doNotOptimizeAway(acc);
    });
  };

  auto salBench1 = createBench("Compare sal vs builtin << (uint32_t)");
  runSalBench(salBench1, "uint32_t", u32, shl32);

  auto salBench2 = createBench("Compare sal vs builtin << (uint64_t)");
  runSalBench(salBench2, "uint64_t", u64, shl64);

  auto sarBench1 = createBench("Compare sar vs builtin >> (uint32_t)");
  runSarBench(sarBench1, "uint32_t", u32, shl32);

  auto sarBench2 = createBench("Compare sar vs builtin >> (uint64_t)");
  runSarBench(sarBench2, "uint64_t", u64, shl64);

  auto sarBench3 = createBench("Compare sar vs builtin >> (int32_t)");
  runSarBench(sarBench3, "int32_t", s32, shr32);

  auto sarBench4 = createBench("Compare sar vs builtin >> (int64_t)");
  runSarBench(sarBench4, "int64_t", s64, shr64);
}

TEST_CASE("bench_shl_shr_vs_builtin") {
  constexpr size_t N = 1u << 15;

  std::mt19937_64 rng{0x5EEDFEED};
  // For shl and shr vs builtin << and >>: use unsigned values and non-negative shifts
  std::uniform_int_distribution<uint32_t> u32dist{0u, 0x7FFF'FFFFu};
  std::uniform_int_distribution<uint64_t> u64dist{0ull, (1ull << 50)};
  std::uniform_int_distribution<unsigned> shdist{0u, 63u};

  std::vector<uint32_t> u32(N);
  std::vector<unsigned> sh32(N);
  std::vector<uint64_t> u64(N);
  std::vector<unsigned> sh64(N);

  for (size_t i = 0; i < N; ++i) {
    u32[i] = u32dist(rng);
    sh32[i] = shdist(rng) % 32;
    u64[i] = u64dist(rng);
    sh64[i] = shdist(rng) % 64;
  }

  auto createBench = [&](auto title) {
    ankerl::nanobench::Bench bench;

    bench.title(title)
      .unit("op")
      .batch(N)
      .warmup(100)
      .minEpochTime(150ms)
      .minEpochIterations(60000)
      .relative(true)
      .performanceCounters(true);

    return bench;
  };

  auto runShlBench = [&](auto& bench, const auto& typeName, const auto& values, const auto& shifts) {
    bench.run("builtin << ("s + typeName + ")", [&] {
      std::decay_t<decltype(values[0])> acc = 0;

      for (size_t i = 0; i < N; ++i) {
        acc ^= values[i] << shifts[i];
      }

      ankerl::nanobench::doNotOptimizeAway(acc);
    });

    bench.run("shl<"s + typeName + ">", [&] {
      std::decay_t<decltype(values[0])> acc = 0;

      for (size_t i = 0; i < N; ++i) {
        acc ^= shl(values[i], shifts[i]);
      }

      ankerl::nanobench::doNotOptimizeAway(acc);
    });
  };

  auto runShrBench = [&](auto& bench, const auto& typeName, const auto& values, const auto& shifts) {
    bench.run("builtin >> ("s + typeName + ")", [&] {
      std::decay_t<decltype(values[0])> acc = 0;

      for (size_t i = 0; i < N; ++i) {
        acc ^= values[i] >> shifts[i];
      }

      ankerl::nanobench::doNotOptimizeAway(acc);
    });

    bench.run("shr<"s + typeName + ">", [&] {
      std::decay_t<decltype(values[0])> acc = 0;

      for (size_t i = 0; i < N; ++i) {
        acc ^= shr(values[i], shifts[i]);
      }

      ankerl::nanobench::doNotOptimizeAway(acc);
    });
  };

  auto shlBench1 = createBench("Compare shl vs builtin << (uint32_t)");
  runShlBench(shlBench1, "uint32_t", u32, sh32);

  auto shlBench2 = createBench("Compare shl vs builtin << (uint64_t)");
  runShlBench(shlBench2, "uint64_t", u64, sh64);

  auto shrBench1 = createBench("Compare shr vs builtin >> (uint32_t)");
  runShrBench(shrBench1, "uint32_t", u32, sh32);

  auto shrBench2 = createBench("Compare shr vs builtin >> (uint64_t)");
  runShrBench(shrBench2, "uint64_t", u64, sh64);
}
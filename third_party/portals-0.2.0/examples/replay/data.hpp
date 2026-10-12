// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once
#include <array>

namespace example {
// A small recorded tape ships as source and compiled data; replay never depends on cwd.
inline constexpr std::array<double, 16> REPLAY_PRICES{100.0, 104.0, 97.0, 102.0, 108.0, 95.0,  103.0, 101.0,
                                                      110.0, 99.0,  96.0, 106.0, 98.0,  107.0, 94.0,  105.0};
} // namespace example

// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstring>
#include <memory>
#include <utf8.h>
#include <utility>

namespace dxfcpp {

const Direction Direction::UNDEFINED{0, "UNDEFINED"};
const Direction Direction::DOWN{1, "DOWN"};
const Direction Direction::ZERO_DOWN{2, "ZERO_DOWN"};
const Direction Direction::ZERO{3, "ZERO"};
const Direction Direction::ZERO_UP{4, "ZERO_UP"};
const Direction Direction::UP{5, "UP"};

template <>
const std::unordered_map<Direction::CodeType, std::reference_wrapper<const Direction>> Direction::ParentType::ALL{
    {Direction::UNDEFINED.getCode(), std::cref(Direction::UNDEFINED)},
    {Direction::DOWN.getCode(), std::cref(Direction::DOWN)},
    {Direction::ZERO_DOWN.getCode(), std::cref(Direction::ZERO_DOWN)},
    {Direction::ZERO.getCode(), std::cref(Direction::ZERO)},
    {Direction::ZERO_UP.getCode(), std::cref(Direction::ZERO_UP)},
    {Direction::UP.getCode(), std::cref(Direction::UP)},
};

} // namespace dxfcpp
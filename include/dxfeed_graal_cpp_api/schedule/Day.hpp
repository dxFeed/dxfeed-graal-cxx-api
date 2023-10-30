// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

namespace dxfcpp {

struct DXFCPP_EXPORT Day {
    friend struct Schedule;

    /// The alias to a type of shared pointer to the Day object
    using Ptr = std::shared_ptr<Day>;

    /// The alias to a type of unique pointer to the Day object
    using Unique = std::unique_ptr<Day>;

  private:
    JavaObjectHandle<Day> handle_;

    Day() noexcept;

  public:
};

}
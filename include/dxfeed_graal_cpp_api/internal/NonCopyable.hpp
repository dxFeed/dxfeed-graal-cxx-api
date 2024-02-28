// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

namespace dxfcpp {

template <class T> class NonCopyable {
  public:
    NonCopyable(const NonCopyable &) = delete;
    T &operator=(const T &) = delete;

  protected:
    NonCopyable() = default;
    ~NonCopyable() = default;
};

} // namespace dxfcpp

DXFCXX_DISABLE_MSC_WARNINGS_POP()
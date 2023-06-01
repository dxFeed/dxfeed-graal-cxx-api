// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Conf.hpp"

namespace dxfcpp {

template <class T> class NonCopyable {
  public:
    NonCopyable(const NonCopyable &) = delete;
    T &operator=(const T &) = delete;

  protected:
    NonCopyable() = default;
    ~NonCopyable() = default;
};

}
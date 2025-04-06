// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <memory>

DXFCPP_BEGIN_NAMESPACE

class DXFCPP_EXPORT ApiContext {
    ApiContext() noexcept = default;
    ~ApiContext() noexcept = default;

    public:
    static std::shared_ptr<ApiContext> getInstance() noexcept {
        static std::shared_ptr<ApiContext> instance = std::shared_ptr<ApiContext>(new ApiContext{});

        return instance;
    }

    template <typename Manager> std::shared_ptr<Manager> getManager() const noexcept {
        static std::shared_ptr<Manager> instance = std::shared_ptr<Manager>(new Manager{});

        return instance;
    }
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
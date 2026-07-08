// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <memory>

DXFCPP_BEGIN_NAMESPACE

class DXFCPP_EXPORT ApiContext {
    ApiContext() noexcept;

    public:
    ~ApiContext() noexcept;

    static std::shared_ptr<ApiContext> getInstance() noexcept;

    template <typename Manager> std::shared_ptr<Manager> getManager() const noexcept {
        // ReSharper disable once CppSmartPointerVsMakeFunction
        static std::shared_ptr<Manager> instance = std::shared_ptr<Manager>(new Manager{});

        return instance;
    }
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
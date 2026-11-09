// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <memory>
#include <typeinfo>

DXFCPP_BEGIN_NAMESPACE

class DXFCPP_EXPORT ApiContext {
    ApiContext() noexcept;

    public:
    ~ApiContext() noexcept;

    static std::shared_ptr<ApiContext> getInstance() noexcept;

    template <typename Manager> std::shared_ptr<Manager> getManager() const noexcept {
        // One manager of each type for all the modules: the library registers it (getManagerImpl), and this static
        // owns it. A static of this template alone would have a copy in every module that instantiates it (on Windows:
        // the library and each executable), and the code inlined from the headers would register the entities in other
        // managers than the library looks them up in.
        static const std::shared_ptr<Manager> instance =
            std::static_pointer_cast<Manager>(getManagerImpl(typeid(Manager).name(), [] {
                // ReSharper disable once CppSmartPointerVsMakeFunction
                return std::static_pointer_cast<void>(std::shared_ptr<Manager>(new Manager{}));
            }));

        return instance;
    }

    private:
    std::shared_ptr<void> getManagerImpl(const char *typeName, std::shared_ptr<void> (*create)()) const noexcept;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
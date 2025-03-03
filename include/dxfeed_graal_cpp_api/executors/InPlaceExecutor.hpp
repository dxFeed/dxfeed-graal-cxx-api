// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../internal/Common.hpp"
#include "../internal/Id.hpp"
#include "../internal/JavaObjectHandle.hpp"

#include "../entity/SharedEntity.hpp"

DXFCPP_BEGIN_NAMESPACE

struct ExecutorTag {};

struct DXFCPP_EXPORT InPlaceExecutor final : RequireMakeShared<InPlaceExecutor> {

    private:
    mutable std::recursive_mutex mutex_{};
    JavaObjectHandle<ExecutorTag> handle_{};

    public:
    InPlaceExecutor(LockExternalConstructionTag);

    ~InPlaceExecutor() override;

    static std::shared_ptr<InPlaceExecutor> create();

    void processAllPendingTasks() const;

    const JavaObjectHandle<ExecutorTag> &getHandle() const;
};

DXFCPP_END_NAMESPACE

// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"
#include "../internal/Handler.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../entity/SharedEntity.hpp"
#include "../event/IndexedEvent.hpp"
#include "../event/IndexedEventSource.hpp"
#include "../internal/JavaObjectHandle.hpp"

#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <vector>

DXFCPP_BEGIN_NAMESPACE

class IndexedEventSource;

struct TxModelListenerTag {};

struct DXFCPP_EXPORT TxModelListener : RequireMakeShared<TxModelListener> {
    explicit TxModelListener(LockExternalConstructionTag);
    ~TxModelListener() noexcept override;

    static std::shared_ptr<TxModelListener>
    create(std::function<void(const IndexedEventSource & /* source */,
                              const std::vector<std::shared_ptr<EventType>> & /* events */, bool /* isSnapshot */)>
               onEventsReceived);

    const JavaObjectHandle<TxModelListenerTag> &getHandle() const;

  private:
    mutable std::recursive_mutex mutex_{};
    JavaObjectHandle<TxModelListenerTag> handle_;
    SimpleHandler<void(const IndexedEventSource & /* source */,
                       const std::vector<std::shared_ptr<EventType>> & /* events */, bool /* isSnapshot */)>
        onEventsReceived_{};

    struct Impl;

    std::unique_ptr<Impl> impl_;
};

DXFCPP_END_NAMESPACE
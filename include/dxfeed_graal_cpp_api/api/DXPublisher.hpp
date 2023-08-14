// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

#include "../internal/CEntryPointErrors.hpp"
#include "../internal/Common.hpp"
#include "../internal/Handler.hpp"
#include "../internal/Isolate.hpp"
#include "../internal/JavaObjectHandler.hpp"

#include "DXFeedSubscription.hpp"

#include <memory>
#include <mutex>
#include <unordered_set>

namespace dxfcpp {

struct DXEndpoint;
class EventTypeEnum;

/**
 * Main entry class for dxFeed API (<b>read it first</b>).
 */
struct DXFCPP_EXPORT DXPublisher : SharedEntity {
    friend struct DXEndpoint;

  private:
    JavaObjectHandler<DXPublisher> handler_;

    static std::shared_ptr<DXPublisher> create(void *feedHandle) noexcept;

  protected:
    DXPublisher() noexcept : handler_{} {
        if constexpr (Debugger::isDebug) {
            Debugger::debug("DXPublisher()");
        }
    }

  public:

    ~DXPublisher() noexcept override {
        if constexpr (Debugger::isDebug) {
            Debugger::debug("DXPublisher{" + handler_.toString() + "}::~DXPublisher()");
        }
    }

    /**
     * Returns a default application-wide singleton instance of DXPublisher. Most applications use only a single
     * data-sink and should rely on this method to get one. This is a shortcut to
     * @ref DXEndpoint "DXEndpoint"::@ref DXEndpoint::getInstance() "getInstance"(DXEndpoint::Role::PUBLISHER)->@ref DXEndpoint::getPublisher() "getPublisher"().
     */
    static std::shared_ptr<DXPublisher> getInstance() noexcept;


    virtual void publishEvents() noexcept;

    std::string toString() const noexcept override;
};

}
// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include <cstdint>
#include <memory>
#include <string>

#include "../EventType.hpp"

namespace dxfcpp {

/**
 * Base class for all market events. All market events are POCO that
 * extend this class. Market event classes are simple beans with setter and getter methods for their
 * properties and minimal business logic. All market events have `eventSymbol` property that is
 * defined by this class.
 */
struct DXFCPP_EXPORT MarketEvent : public EventTypeWithSymbol<std::string> {
    /// The alias to a type of shared pointer to the MarketEvent object
    using Ptr = std::shared_ptr<MarketEvent>;

  private:
    std::string eventSymbol_{};
    std::int64_t eventTime_{};

  protected:
    MarketEvent() noexcept = default;

    /**
     * Protected constructor for concrete implementation classes that initializes `eventSymbol` property.
     *
     * @param eventSymbol The event symbol.
     */
    explicit MarketEvent(std::string eventSymbol) noexcept : eventSymbol_{std::move(eventSymbol)} {}

  public:
    /**
     * Returns symbol of this event.
     *
     * @return symbol of this event.
     */
    const std::string &getEventSymbol() const override { return eventSymbol_; }

    /**
     * Changes symbol of this event.
     *
     * @param eventSymbol The symbol of this event.
     */
    void setEventSymbol(const std::string &eventSymbol) override {
        //TODO: check invalid utf-8
        eventSymbol_ = eventSymbol;
    }

    ///
    std::int64_t getEventTime() const override { return eventTime_; }

    ///
    void setEventTime(std::int64_t eventTime) override { eventTime_ = eventTime; }
};

} // namespace dxfcpp
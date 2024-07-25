// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include "../EventType.hpp"

DXFCPP_BEGIN_NAMESPACE

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
    std::optional<std::string> eventSymbol_{};
    std::int64_t eventTime_{};

  protected:
    MarketEvent() noexcept = default;

    /**
     * Protected constructor for concrete implementation classes that initializes `eventSymbol` property.
     *
     * @param eventSymbol The event symbol.
     */
    explicit MarketEvent(std::string eventSymbol) noexcept : eventSymbol_{std::move(eventSymbol)} {
    }

    virtual void fillData(void *graalNative) noexcept;
    virtual void fillGraalData(void *graalNative) const noexcept;
    static void freeGraalData(void *graalNative) noexcept;

  public:
    ///
    void assign(std::shared_ptr<EventType> event) override {
        if (const auto other = event->sharedAs<MarketEvent>(); other) {
            eventSymbol_ = other->eventSymbol_;
            eventTime_ = other->eventTime_;
        }
    }

    /**
     * Returns symbol of this event.
     *
     * @return symbol of this event or dxfcpp::String::NUL (`std::string{"<null>"}`)
     */
    const std::string &getEventSymbol() const & noexcept override {
        if (!eventSymbol_) {
            return dxfcpp::String::NUL;
        }

        return eventSymbol_.value();
    }

    /**
     * Returns symbol of this event.
     *
     * @return symbol of this event or `std::nullopt`.
     */
    const std::optional<std::string> &getEventSymbolOpt() const & noexcept override {
        return eventSymbol_;
    }

    /**
     * Changes symbol of this event.
     *
     * @param eventSymbol The symbol of this event.
     */
    void setEventSymbol(const std::string &eventSymbol) noexcept override {
        // TODO: check invalid utf-8 [EN-8233]
        eventSymbol_ = eventSymbol;
    }

    ///
    std::int64_t getEventTime() const noexcept override {
        return eventTime_;
    }

    ///
    void setEventTime(std::int64_t eventTime) noexcept override {
        eventTime_ = eventTime;
    }
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
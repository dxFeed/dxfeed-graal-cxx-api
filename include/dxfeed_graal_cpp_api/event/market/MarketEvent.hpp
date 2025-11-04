// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../EventType.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

DXFCPP_BEGIN_NAMESPACE

/**
 * Base class for all market events. All market events are POCO that
 * extend this class. Market event classes are simple beans with setter and getter methods for their
 * properties and minimal business logic. All market events have `eventSymbol` property defined by this class.
 */
struct DXFCPP_EXPORT MarketEvent : EventTypeWithSymbol<std::string> {
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
    explicit MarketEvent(const StringLike &eventSymbol) noexcept;

    virtual void fillData(void *graalNative) noexcept;
    virtual void fillGraalData(void *graalNative) const noexcept;
    static void freeGraalData(void *graalNative) noexcept;

    public:
    ///
    void assign(std::shared_ptr<EventType> event) override;

    /**
     * Returns a symbol of this event.
     *
     * @return symbol of this event or dxfcpp::String::NUL (`std::string{"<null>"}`)
     */
    const std::string &getEventSymbol() const & noexcept override;

    /**
     * Returns a symbol of this event.
     *
     * @return symbol of this event or `std::nullopt`.
     */
    const std::optional<std::string> &getEventSymbolOpt() const & noexcept override;

    /**
     * Changes symbol of this event.
     *
     * @param eventSymbol The symbol of this event.
     */
    void setEventSymbol(const StringLike &eventSymbol) noexcept override;

    ///
    std::int64_t getEventTime() const noexcept override;

    ///
    void setEventTime(std::int64_t eventTime) noexcept override;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
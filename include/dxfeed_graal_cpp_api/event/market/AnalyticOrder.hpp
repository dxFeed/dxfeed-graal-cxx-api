// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <cassert>
#include <cstdint>
#include <memory>
#include <string>

#include "../../internal/Common.hpp"
#include "../EventTypeEnum.hpp"

#include "IcebergType.hpp"
#include "Order.hpp"

DXFCPP_BEGIN_NAMESPACE

struct EventMapper;

/**
 * Represents an extension of Order introducing analytic information, e.g. adding to this order iceberg related
 * information (@ref AnalyticOrder::getIcebergPeakSize() "icebergPeakSize", @ref AnalyticOrder::getIcebergHiddenSize()
 * "icebergHiddenSize",
 * @ref AnalyticOrder::getIcebergExecutedSize() "icebergExecutedSize").
 * The collection of analytic order events of a symbol represents the most recent analytic information
 * that is available about orders on the market at any given moment of time.
 *
 * <p> Analytic order is similar to a regular Order. In addition this event has few additional properties:
 * <ul>
 *     <li>@ref AnalyticOrder::getIcebergPeakSize() "icebergPeakSize" - the size of the peak, i.e. the visible part of
 * the iceberg, that is being continually refilled until the order is fully traded or cancelled; <li>@ref
 * AnalyticOrder::getIcebergHiddenSize() "icebergHiddenSize" - the prediction of current hidden size of the iceberg, as
 * inferred by the model; <li>@ref AnalyticOrder::getIcebergExecutedSize() "icebergExecutedSize" - the executed size of
 * the iceberg order. For IcebergType::SYNTHETIC type represents total executed size of all orders composing current
 * iceberg; <li>AnalyticOrder::getIcebergType() - type of the iceberg, either native (exchange-managed) or synthetic
 * (managed outside of the exchange).
 * </ul>
 *
 * <h3>Implementation details</h3>
 * <p>
 * This event is implemented on top of QDS records `AnalyticOrder#<source-id>`,
 * where `<source-id>` is up to 4 ASCII characters with a mnemonic for the source like "GLBX".
 */
class DXFCPP_EXPORT AnalyticOrder final : public Order {
    friend struct EventMapper;

    /*
     * Analytic flags property has several significant bits that are packed into an integer in the following way:
     *      31...2       1        0
     * +--------------+-------+-------+
     * |              |   IcebergType |
     * +--------------+-------+-------+
     */

    // TYPE values are taken from Type enum.
    static constexpr std::uint32_t ICEBERG_TYPE_MASK = 3U;
    static constexpr std::uint32_t ICEBERG_TYPE_SHIFT = 0U;

  protected:
    struct AnalyticOrderData {
        double icebergPeakSize = math::NaN;
        double icebergHiddenSize = math::NaN;
        double icebergExecutedSize = math::NaN;
        std::int32_t icebergFlags{};
    };

    AnalyticOrderData analyticOrderData_{};

    void fillData(void *graalNative) noexcept override;
    void fillGraalData(void *graalNative) const noexcept override;

  public:
    /// The alias to a type of shared pointer to the AnalyticOrder object
    using Ptr = std::shared_ptr<AnalyticOrder>;

    /// The alias to a type of unique pointer to the AnalyticOrder object
    using Unique = std::unique_ptr<AnalyticOrder>;

    /// Type identifier and additional information about the current event class.
    static const EventTypeEnum &TYPE;

    /**
     * Creates an object of the current type and fills it with data from the the dxFeed Graal SDK structure.
     *
     * @param graalNative The pointer to the dxFeed Graal SDK structure.
     * @return The object of current type.
     * @throws InvalidArgumentException
     */
    static Ptr fromGraal(void *graalNative);

    /**
     * Allocates memory for the dxFeed Graal SDK structure (recursively if necessary).
     * Fills the dxFeed Graal SDK structure's fields by the data of the current entity (recursively if necessary).
     * Returns the pointer to the filled structure.
     *
     * @return The pointer to the filled dxFeed Graal SDK structure
     */
    void *toGraal() const override;

    /**
     * Releases the memory occupied by the dxFeed Graal SDK structure (recursively if necessary).
     *
     * @param graalNative The pointer to the dxFeed Graal SDK structure.
     * @throws InvalidArgumentException
     */
    static void freeGraal(void *graalNative);

    ///
    void assign(std::shared_ptr<EventType> event) override;

    /// Creates new analytic order event with default values.
    AnalyticOrder() noexcept = default;

    /**
     * Creates new analytic order event with the specified event symbol.
     *
     * @param eventSymbol The event symbol.
     */
    explicit AnalyticOrder(std::string eventSymbol) noexcept : Order(std::move(eventSymbol)) {
    }

    // MarketEvent methods

    /**
     * Changes event's symbol and returns the current analytic order.
     *
     * @param eventSymbol The symbol of this event.
     * @return The current analytic order.
     */
    AnalyticOrder &withEventSymbol(const std::string &eventSymbol) noexcept override {
        return dynamic_cast<AnalyticOrder &>(Order::withEventSymbol(eventSymbol));
    }

    /**
     * Changes event's creation time and returns the current analytic order.
     *
     * @param eventTime the difference, measured in milliseconds, between the event creation time and
     * midnight, January 1, 1970 UTC.
     * @return The current analytic order.
     */
    AnalyticOrder &withEventTime(std::int64_t eventTime) noexcept {
        return dynamic_cast<AnalyticOrder &>(Order::withEventTime(eventTime));
    }

    // OrderBase methods

    /**
     * Changes event's source and returns the current analytic order.
     * This method changes highest bits of the @ref OrderBase::getIndex() "index" of this event.
     *
     * @param source source of this event.
     * @return The current analytic order.
     */
    AnalyticOrder &withSource(const OrderSource &source) noexcept {
        return dynamic_cast<AnalyticOrder &>(Order::withSource(source));
    }

    /**
     * Changes transactional event flags and returns the current analytic order.
     * See EventFlag "Event Flags" section.
     *
     * @param eventFlags transactional event flags.
     * @return The current analytic order.
     */
    AnalyticOrder &withEventFlags(std::int32_t eventFlags) noexcept {
        return dynamic_cast<AnalyticOrder &>(Order::withEventFlags(eventFlags));
    }

    /**
     * Changes transactional event flags and returns the current analytic order.
     * See EventFlag "Event Flags" section.
     *
     * @param eventFlags transactional event flags' mask.
     * @return The current analytic order.
     */
    AnalyticOrder &withEventFlags(const EventFlagsMask &eventFlags) noexcept {
        return dynamic_cast<AnalyticOrder &>(Order::withEventFlags(eventFlags));
    }

    /**
     * Changes unique per-symbol index of this analytic order and returns it. Note, that this method also changes
     * @ref OrderBase::getSource() "source", whose id occupies highest bits of index.
     * Use OrderBase::setSource() after invocation of this method to set the desired value of source.
     *
     * @param index unique per-symbol index of this analytic order.
     * @return The current analytic order.
     */
    AnalyticOrder &withIndex(std::int64_t index) noexcept {
        return dynamic_cast<AnalyticOrder &>(Order::withIndex(index));
    }

    /**
     * Changes time of this analytic order and returns it.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @param time time of this analytic order.
     * @return The current analytic order.
     */
    AnalyticOrder &withTime(std::int64_t time) noexcept {
        return dynamic_cast<AnalyticOrder &>(Order::withTime(time));
    }

    /**
     * Changes microseconds and nanoseconds time part of this analytic order.
     * Returns the current analytic order.
     *
     * @param timeNanoPart microseconds and nanoseconds time part of this analytic order.
     * @return The current analytic order.
     */
    AnalyticOrder &withTimeNanoPart(std::int32_t timeNanoPart) noexcept {
        return dynamic_cast<AnalyticOrder &>(Order::withTimeNanoPart(timeNanoPart));
    }

    /**
     * Changes @ref OrderBase::getSequence() "sequence number" of this analytic order.
     * Returns the current analytic order.
     *
     * @param sequence the sequence.
     * @return The current analytic order.
     * @see OrderBase::getSequence()
     */
    AnalyticOrder &withSequence(std::int32_t sequence) noexcept {
        return dynamic_cast<AnalyticOrder &>(Order::withSequence(sequence));
    }

    /**
     * Changes time of this analytic order and returns it.
     * Time is measured in nanoseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @param timeNanos The time of this analytic order in nanoseconds.
     * @return The current analytic order.
     */
    AnalyticOrder &withTimeNanos(std::int64_t timeNanos) noexcept {
        return dynamic_cast<AnalyticOrder &>(Order::withTimeNanos(timeNanos));
    }

    /**
     * Changes action of this analytic order and returns it.
     *
     * @param action The side of this analytic order.
     * @return The current analytic order.
     */
    AnalyticOrder &withAction(const OrderAction &action) noexcept {
        return dynamic_cast<AnalyticOrder &>(Order::withAction(action));
    }

    /**
     * Changes time of the last action and returns current analytic order.
     *
     * @param actionTime The last order action time.
     * @return The current analytic order.
     */
    AnalyticOrder &withActionTime(std::int64_t actionTime) noexcept {
        return dynamic_cast<AnalyticOrder &>(Order::withActionTime(actionTime));
    }

    /**
     * Changes order ID.
     * Returns the current analytic order.
     *
     * @param orderId The order ID.
     * @return The current analytic order.
     */
    AnalyticOrder &withOrderId(std::int64_t orderId) noexcept {
        return dynamic_cast<AnalyticOrder &>(Order::withOrderId(orderId));
    }

    /**
     * Changes auxiliary order ID.
     * Returns the current analytic order.
     *
     * @param auxOrderId The auxiliary order ID.
     * @return The current analytic order.
     */
    AnalyticOrder &withAuxOrderId(std::int64_t auxOrderId) noexcept {
        return dynamic_cast<AnalyticOrder &>(Order::withAuxOrderId(auxOrderId));
    }

    /**
     * Changes price of this analytic order.
     * Returns the current analytic order.
     *
     * @param price The price of this analytic order.
     * @return The current analytic order.
     */
    AnalyticOrder &withPrice(double price) noexcept {
        return dynamic_cast<AnalyticOrder &>(Order::withPrice(price));
    }

    /**
     * Changes size of this analytic order.
     * Returns the current analytic order.
     *
     * @param size The size of this analytic order.
     * @return The current analytic order.
     */
    AnalyticOrder &withSize(double size) noexcept {
        return dynamic_cast<AnalyticOrder &>(Order::withSize(size));
    }

    /**
     * Changes executed size of this analytic order.
     * Returns the current analytic order.
     *
     * @param executedSize The executed size of this analytic order.
     * @return The current analytic order.
     */
    AnalyticOrder &withExecutedSize(double executedSize) noexcept {
        return dynamic_cast<AnalyticOrder &>(Order::withExecutedSize(executedSize));
    }

    /**
     * Changes number of individual orders in this aggregate order.
     * Returns the current analytic order.
     *
     * @param count The number of individual orders in this aggregate order.
     * @return The current analytic order.
     */
    AnalyticOrder &withCount(std::int64_t count) noexcept {
        return dynamic_cast<AnalyticOrder &>(Order::withCount(count));
    }

    /**
     * Changes trade ID.
     * Returns the current analytic order.
     *
     * @param tradeId The trade ID.
     * @return The current analytic order.
     */
    AnalyticOrder &withTradeId(std::int64_t tradeId) noexcept {
        return dynamic_cast<AnalyticOrder &>(Order::withTradeId(tradeId));
    }

    /**
     * Changes trade price.
     * Returns the current analytic order.
     *
     * @param tradePrice The trade price.
     * @return The current analytic order.
     */
    AnalyticOrder &withTradePrice(double tradePrice) noexcept {
        return dynamic_cast<AnalyticOrder &>(Order::withTradePrice(tradePrice));
    }

    /**
     * Changes trade size.
     * Returns the current analytic order.
     *
     * @param tradeSize The trade size.
     * @return The current analytic order.
     */
    AnalyticOrder &withTradeSize(double tradeSize) noexcept {
        return dynamic_cast<AnalyticOrder &>(Order::withTradeSize(tradeSize));
    }

    /**
     * Changes exchange code of this analytic order.
     * Returns the current analytic order.
     *
     * @param exchangeCode The exchange code of this analytic order.
     * @return The current analytic order.
     */
    AnalyticOrder &withExchangeCode(char exchangeCode) noexcept {
        return dynamic_cast<AnalyticOrder &>(Order::withExchangeCode(exchangeCode));
    }

    /**
     * Changes exchange code of this analytic order.
     * Returns the current analytic order.
     *
     * @param exchangeCode The exchange code of this analytic order.
     * @return The current analytic order.
     */
    AnalyticOrder &withExchangeCode(std::int16_t exchangeCode) noexcept {
        return dynamic_cast<AnalyticOrder &>(Order::withExchangeCode(exchangeCode));
    }

    /**
     * Changes side of this analytic order.
     * Returns the current analytic order.
     *
     * @param side The side of this analytic order.
     * @return The current analytic order.
     */
    AnalyticOrder &withOrderSide(const Side &side) noexcept {
        return dynamic_cast<AnalyticOrder &>(Order::withOrderSide(side));
    }

    /**
     * Changes scope of this analytic order.
     * Returns the current analytic order.
     *
     * @param scope The scope of this analytic order.
     * @return The current analytic order.
     */
    AnalyticOrder &withScope(const Scope &scope) noexcept {
        return dynamic_cast<AnalyticOrder &>(Order::withScope(scope));
    }

    // Order methods

    /**
     * Changes market maker or other aggregate identifier of this analytic order.
     * Returns the current analytic order.
     *
     * @param marketMaker The market maker or other aggregate identifier of this analytic order.
     * @return The current analytic order.
     */
    AnalyticOrder &withMarketMaker(std::string marketMaker) noexcept {
        return dynamic_cast<AnalyticOrder &>(Order::withMarketMaker(std::move(marketMaker)));
    }

    // AnalyticOrder methods

    /**
     * Returns iceberg peak size of this analytic order.
     *
     * @return iceberg peak size of this analytic order.
     */
    double getIcebergPeakSize() const noexcept {
        return analyticOrderData_.icebergPeakSize;
    }

    /**
     * Changes iceberg peak size of this analytic order.
     *
     * @param icebergPeakSize iceberg peak size of this analytic order.
     */
    void setIcebergPeakSize(double icebergPeakSize) noexcept {
        analyticOrderData_.icebergPeakSize = icebergPeakSize;
    }

    /**
     * Changes iceberg peak size and returns the current analytic order.
     *
     * @param icebergPeakSize iceberg peak size of this analytic order.
     * @return The current analytic order.
     */
    AnalyticOrder &withIcebergPeakSize(double icebergPeakSize) noexcept {
        setIcebergPeakSize(icebergPeakSize);

        return *this;
    }

    /**
     * Returns iceberg hidden size of this analytic order.
     *
     * @return iceberg hidden size of this analytic order.
     */
    double getIcebergHiddenSize() const noexcept {
        return analyticOrderData_.icebergHiddenSize;
    }

    /**
     * Changes iceberg hidden size of this analytic order.
     *
     * @param icebergHiddenSize iceberg hidden size of this analytic order.
     */
    void setIcebergHiddenSize(double icebergHiddenSize) noexcept {
        analyticOrderData_.icebergHiddenSize = icebergHiddenSize;
    }

    /**
     * Changes iceberg hidden size and returns the current analytic order.
     *
     * @param icebergHiddenSize iceberg hidden size of this analytic order.
     * @return The current analytic order.
     */
    AnalyticOrder &withIcebergHiddenSize(double icebergHiddenSize) noexcept {
        setIcebergHiddenSize(icebergHiddenSize);

        return *this;
    }

    /**
     * Returns iceberg executed size of this analytic order.
     *
     * @return iceberg executed size of this analytic order.
     */

    double getIcebergExecutedSize() const noexcept {
        return analyticOrderData_.icebergExecutedSize;
    }

    /**
     * Changes iceberg executed size of this analytic order.
     *
     * @param icebergExecutedSize iceberg executed size of this analytic order.
     */
    void setIcebergExecutedSize(double icebergExecutedSize) noexcept {
        analyticOrderData_.icebergExecutedSize = icebergExecutedSize;
    }

    /**
     * Changes iceberg executed size and returns the current analytic order.
     *
     * @param icebergExecutedSize iceberg executed size of this analytic order.
     * @return The current analytic order.
     */
    AnalyticOrder &withIcebergExecutedSize(double icebergExecutedSize) noexcept {
        setIcebergExecutedSize(icebergExecutedSize);

        return *this;
    }

    /**
     * Returns iceberg type of this analytic order.
     *
     * @return iceberg type of this analytic order.
     */
    const IcebergType &getIcebergType() const & noexcept {
        return IcebergType::valueOf(getBits(analyticOrderData_.icebergFlags, ICEBERG_TYPE_MASK, ICEBERG_TYPE_SHIFT));
    }

    /**
     * Changes iceberg type of this analytic order.
     *
     * @param icebergType iceberg type of this analytic order.
     */
    void setIcebergType(const IcebergType &icebergType) noexcept {
        analyticOrderData_.icebergFlags =
            setBits(analyticOrderData_.icebergFlags, ICEBERG_TYPE_MASK, ICEBERG_TYPE_SHIFT, icebergType.getCode());
    }

    /**
     * Changes iceberg type and returns the current analytic order.
     *
     * @param icebergType iceberg type of this analytic order.
     * @return The current analytic order.
     */
    AnalyticOrder &withIcebergType(const IcebergType &icebergType) noexcept {
        setIcebergType(icebergType);

        return *this;
    }

    /**
     * Returns a string representation of the current object.
     *
     * @return a string representation
     */
    std::string toString() const override;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
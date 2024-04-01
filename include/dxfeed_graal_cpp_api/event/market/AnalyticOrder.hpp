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
    static std::shared_ptr<AnalyticOrder> fromGraal(void *graalNative);

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
     */
    static void freeGraal(void *graalNative);

  public:
    /// The alias to a type of shared pointer to the AnalyticOrder object
    using Ptr = std::shared_ptr<AnalyticOrder>;

    /// The alias to a type of unique pointer to the AnalyticOrder object
    using Unique = std::unique_ptr<AnalyticOrder>;

    /// Type identifier and additional information about the current event class.
    static const EventTypeEnum &TYPE;

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
     * Changes event's symbol and returns a shared pointer to the current analytic order.
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<AnalyticOrder>(new
     * AnalyticOrder(...))` or `std::make_shared<AnalyticOrder>(...)`
     *
     * @param eventSymbol The symbol of this event.
     * @return A shared pointer to the current analytic order.
     */
    AnalyticOrder::Ptr withEventSymbolShared(const std::string &eventSymbol) noexcept {
        return Order::withEventSymbolShared(eventSymbol)->sharedAs<AnalyticOrder>();
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

    /**
     * Changes event's creation time and returns a shared pointer to the current analytic order.
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<AnalyticOrder>(new
     * AnalyticOrder(...))` or `std::make_shared<AnalyticOrder>(...)`
     *
     * @param eventTime the difference, measured in milliseconds, between the event creation time and
     * midnight, January 1, 1970 UTC.
     * @return A shared pointer to the current analytic order.
     */
    AnalyticOrder::Ptr withEventTimeShared(std::int64_t eventTime) noexcept {
        return Order::withEventTimeShared(eventTime)->sharedAs<AnalyticOrder>();
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
     * Changes event's source and returns a shared pointer to the current analytic order.
     * This method changes highest bits of the @ref OrderBase::getIndex() "index" of this event.
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<AnalyticOrder>(new
     * AnalyticOrder(...))` or `std::make_shared<AnalyticOrder>(...)`
     *
     * @param source source of this event.
     * @return A shared pointer to the current analytic order.
     */
    AnalyticOrder::Ptr withSourceShared(const OrderSource &source) noexcept {
        return Order::withSourceShared(source)->sharedAs<AnalyticOrder>();
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
     * Changes transactional event flags and returns a shared pointer to the current analytic order.
     * See EventFlag "Event Flags" section.
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<AnalyticOrder>(new
     * AnalyticOrder(...))` or `std::make_shared<AnalyticOrder>(...)`
     *
     * @param eventFlags transactional event flags.
     * @return A shared pointer to the current analytic order.
     */
    AnalyticOrder::Ptr withEventFlagsShared(std::int32_t eventFlags) noexcept {
        return Order::withEventFlagsShared(eventFlags)->sharedAs<AnalyticOrder>();
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
     * Changes transactional event flags and returns a shared pointer to the current analytic order.
     * See EventFlag "Event Flags" section.
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<AnalyticOrder>(new
     * AnalyticOrder(...))` or `std::make_shared<AnalyticOrder>(...)`
     *
     * @param eventFlags transactional event flags' mask.
     * @return A shared pointer to the current analytic order.
     */
    AnalyticOrder::Ptr withEventFlagsShared(const EventFlagsMask &eventFlags) noexcept {
        return Order::withEventFlagsShared(eventFlags)->sharedAs<AnalyticOrder>();
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
     * Changes unique per-symbol index of this analytic order and returns a shared pointer to it. Note, that this
     * method also changes
     * @ref OrderBase::getSource() "source", whose id occupies highest bits of index.
     * Use OrderBase::setSource() after invocation of this method to set the desired value of source.
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<AnalyticOrder>(new
     * AnalyticOrder(...))` or `std::make_shared<AnalyticOrder>(...)`
     *
     * @param index unique per-symbol index of this analytic order.
     * @return A shared pointer to the current analytic order.
     */
    AnalyticOrder::Ptr withIndexShared(std::int64_t index) noexcept {
        return Order::withIndexShared(index)->sharedAs<AnalyticOrder>();
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
     * Changes time of this analytic order and returns a shared pointer to it.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<AnalyticOrder>(new
     * AnalyticOrder(...))` or `std::make_shared<AnalyticOrder>(...)`
     *
     * @param time time of this analytic order.
     * @return A shared pointer to the current analytic order.
     */
    AnalyticOrder::Ptr withTimeShared(std::int64_t time) noexcept {
        return Order::withTimeShared(time)->sharedAs<AnalyticOrder>();
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
     * Changes microseconds and nanoseconds time part of this analytic order.
     * Returns a shared pointer to the current analytic order.
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<AnalyticOrder>(new
     * AnalyticOrder(...))` or `std::make_shared<AnalyticOrder>(...)`
     *
     * @param timeNanoPart microseconds and nanoseconds time part of this analytic order.
     * @return A shared pointer to the current analytic order.
     */
    AnalyticOrder::Ptr withTimeNanoPartShared(std::int32_t timeNanoPart) noexcept {
        return Order::withTimeNanoPartShared(timeNanoPart)->sharedAs<AnalyticOrder>();
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
     * Changes @ref OrderBase::getSequence() "sequence number" of this analytic order.
     * Returns a shared pointer to the current analytic order.
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<AnalyticOrder>(new
     * AnalyticOrder(...))` or `std::make_shared<AnalyticOrder>(...)`
     *
     * @param sequence the sequence.
     * @return A shared pointer to the current analytic order.
     * @see OrderBase::getSequence()
     */
    AnalyticOrder::Ptr withSequenceShared(std::int32_t sequence) noexcept {
        return Order::withSequenceShared(sequence)->sharedAs<AnalyticOrder>();
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
     * Changes time of this analytic order.
     * Returns a shared pointer to the current analytic order.
     * Time is measured in nanoseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<AnalyticOrder>(new
     * AnalyticOrder(...))` or `std::make_shared<AnalyticOrder>(...)`
     *
     * @param timeNanos time of this analytic order in nanoseconds.
     * @return A shared pointer to the current analytic order.
     */
    AnalyticOrder::Ptr withTimeNanosShared(std::int64_t timeNanos) noexcept {
        return Order::withTimeNanosShared(timeNanos)->sharedAs<AnalyticOrder>();
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
     * Changes action of this analytic order.
     * Returns a shared pointer to the current analytic order.
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<AnalyticOrder>(new
     * AnalyticOrder(...))` or `std::make_shared<AnalyticOrder>(...)`
     *
     * @param action The action of this analytic order.
     * @return A shared pointer to the current analytic order.
     */
    AnalyticOrder::Ptr withActionShared(const OrderAction &action) noexcept {
        return Order::withActionShared(action)->sharedAs<AnalyticOrder>();
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
     * Changes time of the last action and returns a shared pointer to the current analytic order.
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<AnalyticOrder>(new
     * AnalyticOrder(...))` or `std::make_shared<AnalyticOrder>(...)`
     *
     * @param actionTime The last order action time.
     * @return A shared pointer to the current analytic order.
     */
    AnalyticOrder::Ptr withActionTimeShared(std::int64_t actionTime) noexcept {
        return Order::withActionTimeShared(actionTime)->sharedAs<AnalyticOrder>();
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
     * Changes order ID.
     * Returns a shared pointer to the current analytic order.
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<AnalyticOrder>(new
     * AnalyticOrder(...))` or `std::make_shared<AnalyticOrder>(...)`
     *
     * @param orderId order ID.
     * @return A shared pointer to the current analytic order.
     */
    AnalyticOrder::Ptr withOrderIdShared(std::int64_t orderId) noexcept {
        return Order::withOrderIdShared(orderId)->sharedAs<AnalyticOrder>();
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
     * Changes auxiliary order ID.
     * Returns a shared pointer to the current analytic order.
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<AnalyticOrder>(new
     * AnalyticOrder(...))` or `std::make_shared<AnalyticOrder>(...)`
     *
     * @param auxOrderId The auxiliary order ID.
     * @return A shared pointer to the current analytic order.
     */
    AnalyticOrder::Ptr withAuxOrderIdShared(std::int64_t auxOrderId) noexcept {
        return Order::withAuxOrderIdShared(auxOrderId)->sharedAs<AnalyticOrder>();
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
     * Changes price of this analytic order.
     * Returns a shared pointer to the current analytic order.
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<AnalyticOrder>(new
     * AnalyticOrder(...))` or `std::make_shared<AnalyticOrder>(...)`
     *
     * @param price The price of this analytic order.
     * @return A shared pointer to the current analytic order.
     */
    AnalyticOrder::Ptr withPriceShared(double price) noexcept {
        return Order::withPriceShared(price)->sharedAs<AnalyticOrder>();
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
     * Changes size of this analytic order.
     * Returns a shared pointer to the current analytic order.
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<AnalyticOrder>(new
     * AnalyticOrder(...))` or `std::make_shared<AnalyticOrder>(...)`
     *
     * @param size The size of this analytic order.
     * @return A shared pointer to the current analytic order.
     */
    AnalyticOrder::Ptr withSizeShared(double size) noexcept {
        return Order::withSizeShared(size)->sharedAs<AnalyticOrder>();
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
     * Changes executed size of this analytic order.
     * Returns a shared pointer to the current analytic order.
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<AnalyticOrder>(new
     * AnalyticOrder(...))` or `std::make_shared<AnalyticOrder>(...)`
     *
     * @param executedSize The executed size of this analytic order.
     * @return A shared pointer to the current analytic order.
     */
    AnalyticOrder::Ptr withExecutedSizeShared(double executedSize) noexcept {
        return Order::withExecutedSizeShared(executedSize)->sharedAs<AnalyticOrder>();
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
     * Changes number of individual orders in this aggregate order.
     * Returns a shared pointer to the current analytic order.
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<AnalyticOrder>(new
     * AnalyticOrder(...))` or `std::make_shared<AnalyticOrder>(...)`
     *
     * @param count The number of individual orders in this aggregate order.
     * @return A shared pointer to the current analytic order.
     */
    AnalyticOrder::Ptr withCountShared(std::int64_t count) noexcept {
        return Order::withCountShared(count)->sharedAs<AnalyticOrder>();
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
     * Changes trade ID.
     * Returns a shared pointer to the current analytic order.
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<AnalyticOrder>(new
     * AnalyticOrder(...))` or `std::make_shared<AnalyticOrder>(...)`
     *
     * @param tradeId The trade ID.
     * @return A shared pointer to the current analytic order.
     */
    AnalyticOrder::Ptr withTradeIdShared(std::int64_t tradeId) noexcept {
        return Order::withTradeIdShared(tradeId)->sharedAs<AnalyticOrder>();
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
     * Changes trade price.
     * Returns a shared pointer to the current analytic order.
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<AnalyticOrder>(new
     * AnalyticOrder(...))` or `std::make_shared<AnalyticOrder>(...)`
     *
     * @param tradePrice The trade price.
     * @return A shared pointer to the current analytic order.
     */
    AnalyticOrder::Ptr withTradePriceShared(double tradePrice) noexcept {
        return Order::withTradePriceShared(tradePrice)->sharedAs<AnalyticOrder>();
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
     * Changes trade size.
     * Returns a shared pointer to the current analytic order.
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<AnalyticOrder>(new
     * AnalyticOrder(...))` or `std::make_shared<AnalyticOrder>(...)`
     *
     * @param tradeSize The trade size.
     * @return A shared pointer to the current analytic order.
     */
    AnalyticOrder::Ptr withTradeSizeShared(double tradeSize) noexcept {
        return Order::withTradeSizeShared(tradeSize)->sharedAs<AnalyticOrder>();
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
     * Returns a shared pointer to the current analytic order.
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<AnalyticOrder>(new
     * AnalyticOrder(...))` or `std::make_shared<AnalyticOrder>(...)`
     *
     * @param exchangeCode The exchange code of this analytic order.
     * @return A shared pointer to the current analytic order.
     */
    AnalyticOrder::Ptr withExchangeCodeShared(char exchangeCode) noexcept {
        return Order::withExchangeCodeShared(exchangeCode)->sharedAs<AnalyticOrder>();
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
     * Changes exchange code of this analytic order.
     * Returns a shared pointer to the current analytic order.
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<AnalyticOrder>(new
     * AnalyticOrder(...))` or `std::make_shared<AnalyticOrder>(...)`
     *
     * @param exchangeCode The exchange code of this analytic order.
     * @return A shared pointer to the current analytic order.
     */
    AnalyticOrder::Ptr withExchangeCodeShared(std::int16_t exchangeCode) noexcept {
        return Order::withExchangeCodeShared(exchangeCode)->sharedAs<AnalyticOrder>();
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
     * Changes side of this analytic order.
     * Returns a shared pointer to the current analytic order.
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<AnalyticOrder>(new
     * AnalyticOrder(...))` or `std::make_shared<AnalyticOrder>(...)`
     *
     * @param side The side of this analytic order.
     * @return A shared pointer to the current analytic order.
     */
    AnalyticOrder::Ptr withOrderSideShared(const Side &side) noexcept {
        return Order::withOrderSideShared(side)->sharedAs<AnalyticOrder>();
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

    /**
     * Changes scope of this analytic order.
     * Returns a shared pointer to the current analytic order.
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<AnalyticOrder>(new
     * AnalyticOrder(...))` or `std::make_shared<AnalyticOrder>(...)`
     *
     * @param scope The scope of this analytic order.
     * @return A shared pointer to the current analytic order.
     */
    AnalyticOrder::Ptr withScopeShared(const Scope &scope) noexcept {
        return Order::withScopeShared(scope)->sharedAs<AnalyticOrder>();
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

    /**
     * Changes market maker or other aggregate identifier of this analytic order.
     * Returns a shared pointer to the current analytic order.
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<AnalyticOrder>(new
     * AnalyticOrder(...))` or `std::make_shared<AnalyticOrder>(...)`
     *
     * @param marketMaker The market maker or other aggregate identifier of this analytic order.
     * @return A shared pointer to the current analytic order.
     */
    AnalyticOrder::Ptr withMarketMakerShared(std::string marketMaker) noexcept {
        return Order::withMarketMakerShared(std::move(marketMaker))->sharedAs<AnalyticOrder>();
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
     * Changes iceberg peak size and returns a shared pointer to the current analytic order.
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<AnalyticOrder>(new
     * AnalyticOrder(...))` or `std::make_shared<AnalyticOrder>(...)`
     *
     * @param icebergPeakSize iceberg peak size of this analytic order.
     * @return A shared pointer to the current analytic order.
     */
    AnalyticOrder::Ptr withIcebergPeakSizeShared(double icebergPeakSize) noexcept {
        setIcebergPeakSize(icebergPeakSize);

        return sharedAs<AnalyticOrder>();
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
     * Changes iceberg hidden size and returns a shared pointer to the current analytic order.
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<AnalyticOrder>(new
     * AnalyticOrder(...))` or `std::make_shared<AnalyticOrder>(...)`
     *
     * @param icebergHiddenSize iceberg hidden size of this analytic order.
     * @return A shared pointer to the current analytic order.
     */
    AnalyticOrder::Ptr withIcebergHiddenSizeShared(double icebergHiddenSize) noexcept {
        setIcebergHiddenSize(icebergHiddenSize);

        return sharedAs<AnalyticOrder>();
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
     * Changes iceberg executed size and returns a shared pointer to the current analytic order.
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<AnalyticOrder>(new
     * AnalyticOrder(...))` or `std::make_shared<AnalyticOrder>(...)`
     *
     * @param icebergExecutedSize iceberg executed size of this analytic order.
     * @return A shared pointer to the current analytic order.
     */
    AnalyticOrder::Ptr withIcebergExecutedSizeShared(double icebergExecutedSize) noexcept {
        setIcebergExecutedSize(icebergExecutedSize);

        return sharedAs<AnalyticOrder>();
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
     * Changes iceberg type and returns a shared pointer to the current analytic order.
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<AnalyticOrder>(new
     * AnalyticOrder(...))` or `std::make_shared<AnalyticOrder>(...)`
     *
     * @param icebergType iceberg type of this analytic order.
     * @return A shared pointer to the current analytic order.
     */
    AnalyticOrder::Ptr withIcebergTypeShared(const IcebergType &icebergType) noexcept {
        setIcebergType(icebergType);

        return sharedAs<AnalyticOrder>();
    }

    /**
     * Returns a string representation of the current object.
     *
     * @return a string representation
     */
    std::string toString() const noexcept override;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
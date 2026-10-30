// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../internal/Common.hpp"
#include "../EventTypeEnum.hpp"
#include "../market/Order.hpp"
#include "./NuamOrderType.hpp"
#include "./NuamTimeInForceType.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

/**
 * \addtogroup dxfcpp_custom
 * @{
 */

DXFCPP_BEGIN_NAMESPACE

struct EventMapper;

/**
 * Represents an extension of Order for the symbols traded on the Nuam Exchange.
 * It is the new Regional Holding that integrates the Santiago, Lima, and Colombia Stock Exchanges into a single market.
 *
 * It includes the Nuam Exchange internal data, obtained directly from the matching engine, such as additional internal
 * ids, customer additional info, extended matching options, info about triggers, the structure of quantity, etc.,
 * see the properties for details.
 *
 * This is an experimental API, it may change in future versions.
 *
 * Java: <a href="https://docs.dxfeed.com/dxfeed/api/com/dxfeed/event/custom/NuamOrder.html">NuamOrder</a>
 *
 * <h3>Nuam Flags</h3>
 *
 * Nuam Flags from the Nuam Exchange are mapped to the access methods of this class as follows:
 * <ul>
 * <li>OrderType - @ref NuamOrder::getOrderType() "orderType"
 * <li>TimeInForceType - @ref NuamOrder::getTimeInForce() "timeInForce"
 * </ul>
 *
 * <h3>Properties</h3>
 *
 * NuamOrder event has all the properties of Order and the following ones:
 *
 * <ul>
 * <li>@ref NuamOrder::getActorId() "actorId" - actor identifier for the user sending the request;
 * <li>@ref NuamOrder::getParticipantId() "participantId" - the ID of the participant the actor belongs to;
 * <li>@ref NuamOrder::getSubmitterId() "submitterId" - the actor submitting the message that changed the order;
 * <li>@ref NuamOrder::getOnBehalfOfSubmitterId() "onBehalfOfSubmitterId" - actor submitting an order message on behalf
 * of someone else;
 * <li>@ref NuamOrder::getClientOrderId() "clientOrderId" - a client generated order identity;
 * <li>@ref NuamOrder::getCustomerAccount() "customerAccount" - the account to use for this order;
 * <li>@ref NuamOrder::getCustomerInfo() "customerInfo" - a free text field filled in by the client entering the
 * transaction;
 * <li>@ref NuamOrder::getExchangeInfo() "exchangeInfo" - exchange-specific information that can be included with an
 * order, note that it may contain unprintable characters;
 * <li>@ref NuamOrder::getTimeInForce() "timeInForce" - the duration an order remains active in the order book if it
 * isn't fully executed;
 * <li>@ref NuamOrder::getTimeInForceData() "timeInForceData" - additional information for the time validities that
 * require it;
 * <li>@ref NuamOrder::getTriggerOrderBookId() "triggerOrderBookId" - the order book the order will trigger on;
 * <li>@ref NuamOrder::getTriggerPrice() "triggerPrice" - the trigger price of the order;
 * <li>@ref NuamOrder::getTriggerSessionType() "triggerSessionType" - the session type to trigger on;
 * <li>@ref NuamOrder::getOrderType() "orderType" - order type of the order: limit, market, best order, etc.;
 * <li>@ref NuamOrder::getOrderQuantity() "orderQuantity" - the initial quantity of the order when it was entered, it
 * can be changed later if the order is amended;
 * <li>@ref NuamOrder::getDisplayQuantity() "displayQuantity" - the actual display quantity of a reserve order at the
 * end of the transaction;
 * <li>@ref NuamOrder::getRefreshQuantity() "refreshQuantity" - the initial display quantity of a reserve order;
 * <li>@ref NuamOrder::getLeavesQuantity() "leavesQuantity" - the remaining (open) quantity of the order;
 * <li>@ref NuamOrder::getMatchedQuantity() "matchedQuantity" - the matched quantity within the transaction that
 * updated the order.
 * </ul>
 *
 * <h3><a name="nuamOrderEventFlagsSection">Event flags, transactions and snapshots</a></h3>
 *
 * Nuam order event sources provide a consistent view of the order book. Their updates
 * may incorporate multiple changes to individual orders that have to be processed at the same time.
 * The corresponding information is carried in @ref getEventFlags() "eventFlags" property.
 *
 * <p> See the "Event Flags" section of OrderBase class documentation for details.
 *
 * <h3>Publishing order books</h3>
 *
 * When publishing an order event with DXPublisher::publishEvents() method, least significant 32 bits of
 * order @ref getIndex() "index" must be in a range of from 0 to `std::numeric_limits<std::int32_t>::max()`
 * inclusive. Use ::setSource() method after ::setIndex() to properly include source identifier into the
 * index.
 *
 * A snapshot has to be published in the <em>descending</em> order of @ref getIndex() "index", starting with
 * an event with the largest index and marking it with EventFlag::SNAPSHOT_BEGIN bit in @ref getEventFlags()
 * "eventFlags", and finishing the snapshot with an event that has zero 32 least significant bits of index.
 * EventFlag::SNAPSHOT_END bit in @ref getEventFlags() "eventFlags" is optional during publishing.
 * It will be properly set on receiving end anyway.
 *
 * <h3>Limitations</h3>
 *
 * This event type cannot be used with DXFeed::getLastEvent() method.
 *
 * <h3>Implementation details</h3>
 *
 * This event is implemented on top of QDS records `NuamOrder#<source-id>`,
 * where `<source-id>` is up to 4 ASCII characters with a mnemonic for the source like "NUAM".
 */
class DXFCPP_EXPORT NuamOrder final : public Order {
    friend struct EventMapper;

    /*
     * Nuam flags property has several significant bits that are packed into an integer in the following way:
     *   31..8       7..4            3..0
     * +--------+---------------+------------+
     * |        | Time in Force | Order Type |
     * +--------+---------------+------------+
     */

    // NUAM_ORDER_TYPE values are taken from the NuamOrderType enum.
    static constexpr std::uint32_t NUAM_ORDER_TYPE_MASK = 15;
    static constexpr std::uint32_t NUAM_ORDER_TYPE_SHIFT = 0;

    // NUAM_TIME_IN_FORCE_TYPE values are taken from the NuamTimeInForceType enum.
    static constexpr std::uint32_t NUAM_TIME_IN_FORCE_TYPE_MASK = 15;
    static constexpr std::uint32_t NUAM_TIME_IN_FORCE_TYPE_SHIFT = 4;

    struct NuamOrderData {
        std::int32_t actorId{};
        std::int32_t participantId{};
        std::int32_t submitterId{};
        std::int32_t onBehalfOfSubmitterId{};
        std::optional<std::string> clientOrderId{};
        std::optional<std::string> customerAccount{};
        std::optional<std::string> customerInfo{};
        std::optional<std::string> exchangeInfo{};
        std::int32_t timeInForceData{};
        std::int32_t triggerOrderBookId{};
        double triggerPrice = math::NaN;
        std::int32_t triggerSessionType{};
        double orderQuantity = math::NaN;
        double displayQuantity = math::NaN;
        double refreshQuantity = math::NaN;
        double leavesQuantity = math::NaN;
        double matchedQuantity = math::NaN;
        std::int32_t nuamFlags{};
    };

    NuamOrderData nuamOrderData_{};

    void fillData(void *graalNative) noexcept override;
    void fillGraalData(void *graalNative) const noexcept override;
    static void freeGraalData(void *graalNative) noexcept;

    public:
    /// The alias to a type of shared pointer to the NuamOrder object
    using Ptr = std::shared_ptr<NuamOrder>;

    /// The alias to a type of unique pointer to the NuamOrder object
    using Unique = std::unique_ptr<NuamOrder>;

    /// Type identifier and additional information about the current event class.
    static const EventTypeEnum &TYPE;

    /**
     * Creates an object of the current type and fills it with data from the dxFeed Graal SDK structure.
     *
     * @param graalNative The pointer to the dxFeed Graal SDK structure.
     * @return The object of the current type.
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

    /// Creates new Nuam order event with default values.
    NuamOrder() noexcept;

    /**
     * Creates new Nuam order event with the specified event symbol.
     *
     * @param eventSymbol The event symbol.
     */
    explicit NuamOrder(const StringLike &eventSymbol) noexcept;

    // MarketEvent methods

    /**
     * Changes the event's symbol and returns the current Nuam order.
     *
     * @param eventSymbol The symbol of this event.
     * @return The current Nuam order.
     */
    NuamOrder &withEventSymbol(const StringLike &eventSymbol) noexcept override;

    /**
     * Changes event's creation time and returns the current Nuam order.
     *
     * @param eventTime the difference, measured in milliseconds, between the event creation time and
     * midnight, January 1, 1970 UTC.
     * @return The current Nuam order.
     */
    NuamOrder &withEventTime(std::int64_t eventTime) noexcept override;

    // OrderBase methods

    /**
     * Changes event's source and returns the current Nuam order.
     * This method changes highest bits of the @ref OrderBase::getIndex() "index" of this event.
     *
     * @param source source of this event.
     * @return The current Nuam order.
     */
    NuamOrder &withSource(const OrderSource &source) noexcept override;

    /**
     * Changes transactional event flags and returns the current Nuam order.
     * See EventFlag "Event Flags" section.
     *
     * @param eventFlags transactional event flags.
     * @return The current Nuam order.
     */
    NuamOrder &withEventFlags(std::int32_t eventFlags) noexcept override;

    /**
     * Changes transactional event flags and returns the current Nuam order.
     * See EventFlag "Event Flags" section.
     *
     * @param eventFlags transactional event flags' mask.
     * @return The current Nuam order.
     */
    NuamOrder &withEventFlags(const EventFlagsMask &eventFlags) noexcept override;

    /**
     * Changes unique per-symbol index of this Nuam order and returns it. Note, that this method also changes
     * @ref OrderBase::getSource() "source", whose id occupies highest bits of index.
     * Use OrderBase::setSource() after invocation of this method to set the desired value of source.
     *
     * @param index unique per-symbol index of this Nuam order.
     * @return The current Nuam order.
     * @throws InvalidArgumentException if the index is negative.
     */
    NuamOrder &withIndex(std::int64_t index) override;

    /**
     * Changes time of this Nuam order and returns it.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @param time time of this Nuam order.
     * @return The current Nuam order.
     */
    NuamOrder &withTime(std::int64_t time) noexcept override;

    /**
     * Changes microseconds and nanoseconds time part of this Nuam order.
     * Returns the current Nuam order.
     *
     * @param timeNanoPart microseconds and nanoseconds time part of this Nuam order.
     * @return The current Nuam order.
     */
    NuamOrder &withTimeNanoPart(std::int32_t timeNanoPart) noexcept override;

    /**
     * Changes @ref OrderBase::getSequence() "sequence number" of this Nuam order.
     * Returns the current Nuam order.
     *
     * @param sequence the sequence.
     * @return The current Nuam order.
     * @see OrderBase::getSequence()
     */
    NuamOrder &withSequence(std::int32_t sequence) override;

    /**
     * Changes time of this Nuam order and returns it.
     * Time is measured in nanoseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @param timeNanos The time of this Nuam order in nanoseconds.
     * @return The current Nuam order.
     */
    NuamOrder &withTimeNanos(std::int64_t timeNanos) noexcept override;

    /**
     * Changes an action of this Nuam order and returns it.
     *
     * @param action The side of this Nuam order.
     * @return The current Nuam order.
     */
    NuamOrder &withAction(const OrderAction &action) noexcept override;

    /**
     * Changes time of the last action and returns current Nuam order.
     *
     * @param actionTime The last order action time.
     * @return The current Nuam order.
     */
    NuamOrder &withActionTime(std::int64_t actionTime) noexcept override;

    /**
     * Changes order ID.
     * Returns the current Nuam order.
     *
     * @param orderId The order ID.
     * @return The current Nuam order.
     */
    NuamOrder &withOrderId(std::int64_t orderId) noexcept override;

    /**
     * Changes auxiliary order ID.
     * Returns the current Nuam order.
     *
     * @param auxOrderId The auxiliary order ID.
     * @return The current Nuam order.
     */
    NuamOrder &withAuxOrderId(std::int64_t auxOrderId) noexcept override;

    /**
     * Changes price of this Nuam order.
     * Returns the current Nuam order.
     *
     * @param price The price of this Nuam order.
     * @return The current Nuam order.
     */
    NuamOrder &withPrice(double price) noexcept override;

    /**
     * Changes size of this Nuam order.
     * Returns the current Nuam order.
     *
     * @param size The size of this Nuam order.
     * @return The current Nuam order.
     */
    NuamOrder &withSize(double size) noexcept override;

    /**
     * Changes executed size of this Nuam order.
     * Returns the current Nuam order.
     *
     * @param executedSize The executed size of this Nuam order.
     * @return The current Nuam order.
     */
    NuamOrder &withExecutedSize(double executedSize) noexcept override;

    /**
     * Changes a number of individual orders in this aggregate order.
     * Returns the current Nuam order.
     *
     * @param count The number of individual orders in this aggregate order.
     * @return The current Nuam order.
     */
    NuamOrder &withCount(std::int64_t count) noexcept override;

    /**
     * Changes trade ID.
     * Returns the current Nuam order.
     *
     * @param tradeId The trade ID.
     * @return The current Nuam order.
     */
    NuamOrder &withTradeId(std::int64_t tradeId) noexcept override;

    /**
     * Changes trade price.
     * Returns the current Nuam order.
     *
     * @param tradePrice The trade price.
     * @return The current Nuam order.
     */
    NuamOrder &withTradePrice(double tradePrice) noexcept override;

    /**
     * Changes trade size.
     * Returns the current Nuam order.
     *
     * @param tradeSize The trade size.
     * @return The current Nuam order.
     */
    NuamOrder &withTradeSize(double tradeSize) noexcept override;

    /**
     * Changes exchange code of this Nuam order.
     * Returns the current Nuam order.
     *
     * @param exchangeCode The exchange code of this Nuam order.
     * @return The current Nuam order.
     * @throws InvalidArgumentException if the exchange code is not a 7-bit character.
     */
    NuamOrder &withExchangeCode(char exchangeCode) override;

    /**
     * Changes exchange code of this Nuam order.
     * Returns the current Nuam order.
     *
     * @param exchangeCode The exchange code of this Nuam order.
     * @return The current Nuam order.
     * @throws InvalidArgumentException if the exchange code is not a 7-bit character.
     */
    NuamOrder &withExchangeCode(std::int16_t exchangeCode) override;

    /**
     * Changes side of this Nuam order.
     * Returns the current Nuam order.
     *
     * @param side The side of this Nuam order.
     * @return The current Nuam order.
     */
    NuamOrder &withOrderSide(const Side &side) noexcept override;

    /**
     * Changes scope of this Nuam order.
     * Returns the current Nuam order.
     *
     * @param scope The scope of this Nuam order.
     * @return The current Nuam order.
     */
    NuamOrder &withScope(const Scope &scope) noexcept override;

    // Order methods

    /**
     * Changes market maker or other aggregate identifier of this Nuam order.
     * Returns the current Nuam order.
     *
     * @param marketMaker The market maker or another aggregate identifier of this Nuam order.
     * @return The current Nuam order.
     */
    NuamOrder &withMarketMaker(const StringLike &marketMaker) noexcept override;

    /**
     * Changes the market maker or the other aggregate identifier of this order, or makes it null with `std::nullopt`.
     * Returns the current order.
     *
     * @param marketMaker the market maker or the other aggregate identifier of this order or `std::nullopt`.
     * @return The current order.
     */
    template <OptionalString S> NuamOrder &withMarketMaker(S &&marketMaker) noexcept {
        Order::setMarketMaker(std::forward<S>(marketMaker));

        return *this;
    }

    // NuamOrder methods

    /**
     * Returns the actor identifier for the user sending the request.
     *
     * @return the actor identifier for the user sending the request.
     */
    std::int32_t getActorId() const noexcept;

    /**
     * Changes the actor identifier for the user sending the request.
     *
     * @param actorId the actor identifier for the user sending the request.
     */
    void setActorId(std::int32_t actorId) noexcept;

    /**
     * Changes the actor identifier for the user sending the request.
     * Returns the current Nuam order.
     *
     * @param actorId the actor identifier for the user sending the request.
     * @return The current Nuam order.
     */
    NuamOrder &withActorId(std::int32_t actorId) noexcept;

    /**
     * Returns the ID of the participant the actor belongs to.
     *
     * @return the ID of the participant the actor belongs to.
     */
    std::int32_t getParticipantId() const noexcept;

    /**
     * Changes the ID of the participant the actor belongs to.
     *
     * @param participantId the ID of the participant the actor belongs to.
     */
    void setParticipantId(std::int32_t participantId) noexcept;

    /**
     * Changes the ID of the participant the actor belongs to.
     * Returns the current Nuam order.
     *
     * @param participantId the ID of the participant the actor belongs to.
     * @return The current Nuam order.
     */
    NuamOrder &withParticipantId(std::int32_t participantId) noexcept;

    /**
     * Returns the ID of the actor that submitted the message that changed the order.
     *
     * @return the ID of the actor that submitted the message that changed the order.
     */
    std::int32_t getSubmitterId() const noexcept;

    /**
     * Changes the ID of the actor that submitted the message that changed the order.
     *
     * @param submitterId the ID of the actor that submitted the message that changed the order.
     */
    void setSubmitterId(std::int32_t submitterId) noexcept;

    /**
     * Changes the ID of the actor that submitted the message that changed the order.
     * Returns the current Nuam order.
     *
     * @param submitterId the ID of the actor that submitted the message that changed the order.
     * @return The current Nuam order.
     */
    NuamOrder &withSubmitterId(std::int32_t submitterId) noexcept;

    /**
     * Returns the ID of the actor that submitted the order message on behalf of someone else.
     *
     * @return the ID of the actor that submitted the order message on behalf of someone else.
     */
    std::int32_t getOnBehalfOfSubmitterId() const noexcept;

    /**
     * Changes the ID of the actor that submitted the order message on behalf of someone else.
     *
     * @param onBehalfOfSubmitterId the ID of the actor that submitted the order message on behalf of someone else.
     */
    void setOnBehalfOfSubmitterId(std::int32_t onBehalfOfSubmitterId) noexcept;

    /**
     * Changes the ID of the actor that submitted the order message on behalf of someone else.
     * Returns the current Nuam order.
     *
     * @param onBehalfOfSubmitterId the ID of the actor that submitted the order message on behalf of someone else.
     * @return The current Nuam order.
     */
    NuamOrder &withOnBehalfOfSubmitterId(std::int32_t onBehalfOfSubmitterId) noexcept;

    /**
     * Returns the client-generated order identity.
     *
     * @return the client-generated order identity or dxfcpp::String::NUL (`std::string{"<null>"}`).
     */
    const std::string &getClientOrderId() const & noexcept;

    /**
     * Returns the client-generated order identity.
     *
     * @return the client-generated order identity or `std::nullopt`.
     */
    const std::optional<std::string> &getClientOrderIdOpt() const & noexcept;

    /**
     * Changes the client-generated order identity.
     *
     * @param clientOrderId the client-generated order identity.
     */
    void setClientOrderId(const StringLike &clientOrderId) noexcept;

    /**
     * Changes the client-generated order identity, or makes it null with `std::nullopt`.
     * `setClientOrderId(other.getClientOrderIdOpt())` copies it from another event, null included.
     *
     * @param clientOrderId the client-generated order identity or `std::nullopt`.
     */
    template <OptionalString S> void setClientOrderId(S &&clientOrderId) noexcept {
        nuamOrderData_.clientOrderId = std::forward<S>(clientOrderId);
    }

    /**
     * Changes the client-generated order identity.
     * Returns the current Nuam order.
     *
     * @param clientOrderId the client-generated order identity.
     * @return The current Nuam order.
     */
    NuamOrder &withClientOrderId(const StringLike &clientOrderId) noexcept;

    /**
     * Changes the client-generated order identity, or makes it null with `std::nullopt`.
     * Returns the current order.
     *
     * @param clientOrderId the client-generated order identity or `std::nullopt`.
     * @return The current order.
     */
    template <OptionalString S> NuamOrder &withClientOrderId(S &&clientOrderId) noexcept {
        setClientOrderId(std::forward<S>(clientOrderId));

        return *this;
    }

    /**
     * Returns the account to use for this order.
     *
     * @return the account to use for this order or dxfcpp::String::NUL (`std::string{"<null>"}`).
     */
    const std::string &getCustomerAccount() const & noexcept;

    /**
     * Returns the account to use for this order.
     *
     * @return the account to use for this order or `std::nullopt`.
     */
    const std::optional<std::string> &getCustomerAccountOpt() const & noexcept;

    /**
     * Changes the account to use for this order.
     *
     * @param customerAccount the account to use for this order.
     */
    void setCustomerAccount(const StringLike &customerAccount) noexcept;

    /**
     * Changes the account to use for this order, or makes it null with `std::nullopt`.
     * `setCustomerAccount(other.getCustomerAccountOpt())` copies it from another event, null included.
     *
     * @param customerAccount the account to use for this order or `std::nullopt`.
     */
    template <OptionalString S> void setCustomerAccount(S &&customerAccount) noexcept {
        nuamOrderData_.customerAccount = std::forward<S>(customerAccount);
    }

    /**
     * Changes the account to use for this order.
     * Returns the current Nuam order.
     *
     * @param customerAccount the account to use for this order.
     * @return The current Nuam order.
     */
    NuamOrder &withCustomerAccount(const StringLike &customerAccount) noexcept;

    /**
     * Changes the account to use for this order, or makes it null with `std::nullopt`.
     * Returns the current order.
     *
     * @param customerAccount the account to use for this order or `std::nullopt`.
     * @return The current order.
     */
    template <OptionalString S> NuamOrder &withCustomerAccount(S &&customerAccount) noexcept {
        setCustomerAccount(std::forward<S>(customerAccount));

        return *this;
    }

    /**
     * Returns the free-text information filled in by the client entering the transaction.
     *
     * @return the free-text information filled in by the client entering the transaction or dxfcpp::String::NUL
     * (`std::string{"<null>"}`).
     */
    const std::string &getCustomerInfo() const & noexcept;

    /**
     * Returns the free-text information filled in by the client entering the transaction.
     *
     * @return the free-text information filled in by the client entering the transaction or `std::nullopt`.
     */
    const std::optional<std::string> &getCustomerInfoOpt() const & noexcept;

    /**
     * Changes the free-text information filled in by the client entering the transaction.
     *
     * @param customerInfo the free-text information filled in by the client entering the transaction.
     */
    void setCustomerInfo(const StringLike &customerInfo) noexcept;

    /**
     * Changes the free-text information filled in by the client, or makes it null with `std::nullopt`.
     * `setCustomerInfo(other.getCustomerInfoOpt())` copies it from another event, null included.
     *
     * @param customerInfo the free-text information filled in by the client or `std::nullopt`.
     */
    template <OptionalString S> void setCustomerInfo(S &&customerInfo) noexcept {
        nuamOrderData_.customerInfo = std::forward<S>(customerInfo);
    }

    /**
     * Changes the free-text information filled in by the client entering the transaction.
     * Returns the current Nuam order.
     *
     * @param customerInfo the free-text information filled in by the client entering the transaction.
     * @return The current Nuam order.
     */
    NuamOrder &withCustomerInfo(const StringLike &customerInfo) noexcept;

    /**
     * Changes the free-text information filled in by the client, or makes it null with `std::nullopt`.
     * Returns the current order.
     *
     * @param customerInfo the free-text information filled in by the client or `std::nullopt`.
     * @return The current order.
     */
    template <OptionalString S> NuamOrder &withCustomerInfo(S &&customerInfo) noexcept {
        setCustomerInfo(std::forward<S>(customerInfo));

        return *this;
    }

    /**
     * Returns the exchange-specific information that can be included with an order (it may contain unprintable
     * characters).
     *
     * @return the exchange-specific information that can be included with an order (it may contain unprintable
     * characters) or dxfcpp::String::NUL (`std::string{"<null>"}`).
     */
    const std::string &getExchangeInfo() const & noexcept;

    /**
     * Returns the exchange-specific information that can be included with an order (it may contain unprintable
     * characters).
     *
     * @return the exchange-specific information that can be included with an order (it may contain unprintable
     * characters) or `std::nullopt`.
     */
    const std::optional<std::string> &getExchangeInfoOpt() const & noexcept;

    /**
     * Changes the exchange-specific information that can be included with an order (it may contain unprintable
     * characters).
     *
     * @param exchangeInfo the exchange-specific information that can be included with an order (it may contain
     * unprintable characters).
     */
    void setExchangeInfo(const StringLike &exchangeInfo) noexcept;

    /**
     * Changes the exchange-specific information of the order, or makes it null with `std::nullopt`.
     * `setExchangeInfo(other.getExchangeInfoOpt())` copies it from another event, null included.
     *
     * @param exchangeInfo the exchange-specific information of the order or `std::nullopt`.
     */
    template <OptionalString S> void setExchangeInfo(S &&exchangeInfo) noexcept {
        nuamOrderData_.exchangeInfo = std::forward<S>(exchangeInfo);
    }

    /**
     * Changes the exchange-specific information that can be included with an order (it may contain unprintable
     * characters). Returns the current Nuam order.
     *
     * @param exchangeInfo the exchange-specific information that can be included with an order (it may contain
     * unprintable characters).
     * @return The current Nuam order.
     */
    NuamOrder &withExchangeInfo(const StringLike &exchangeInfo) noexcept;

    /**
     * Changes the exchange-specific information of the order, or makes it null with `std::nullopt`.
     * Returns the current order.
     *
     * @param exchangeInfo the exchange-specific information of the order or `std::nullopt`.
     * @return The current order.
     */
    template <OptionalString S> NuamOrder &withExchangeInfo(S &&exchangeInfo) noexcept {
        setExchangeInfo(std::forward<S>(exchangeInfo));

        return *this;
    }

    /**
     * Returns the time in force of this order, that is how long the order remains active in the order book if it is not
     * fully executed.
     *
     * @return the time in force of this order, that is how long the order remains active in the order book if it is not
     * fully executed.
     */
    const NuamTimeInForceType &getTimeInForce() const & noexcept;

    /**
     * Changes the time in force of this order, that is how long the order remains active in the order book if it is not
     * fully executed.
     *
     * @param timeInForce the time in force of this order, that is how long the order remains active in the order book
     * if it is not fully executed.
     */
    void setTimeInForce(const NuamTimeInForceType &timeInForce) noexcept;

    /**
     * Changes the time in force of this order, that is how long the order remains active in the order book if it is not
     * fully executed. Returns the current Nuam order.
     *
     * @param timeInForce the time in force of this order, that is how long the order remains active in the order book
     * if it is not fully executed.
     * @return The current Nuam order.
     */
    NuamOrder &withTimeInForce(const NuamTimeInForceType &timeInForce) noexcept;

    /**
     * Returns the additional data of the time in force, used for the time validities that require it.
     *
     * @return the additional data of the time in force, used for the time validities that require it.
     */
    std::int32_t getTimeInForceData() const noexcept;

    /**
     * Changes the additional data of the time in force, used for the time validities that require it.
     *
     * @param timeInForceData the additional data of the time in force, used for the time validities that require it.
     */
    void setTimeInForceData(std::int32_t timeInForceData) noexcept;

    /**
     * Changes the additional data of the time in force, used for the time validities that require it.
     * Returns the current Nuam order.
     *
     * @param timeInForceData the additional data of the time in force, used for the time validities that require it.
     * @return The current Nuam order.
     */
    NuamOrder &withTimeInForceData(std::int32_t timeInForceData) noexcept;

    /**
     * Returns the ID of the order book the order will trigger on.
     *
     * @return the ID of the order book the order will trigger on.
     */
    std::int32_t getTriggerOrderBookId() const noexcept;

    /**
     * Changes the ID of the order book the order will trigger on.
     *
     * @param triggerOrderBookId the ID of the order book the order will trigger on.
     */
    void setTriggerOrderBookId(std::int32_t triggerOrderBookId) noexcept;

    /**
     * Changes the ID of the order book the order will trigger on.
     * Returns the current Nuam order.
     *
     * @param triggerOrderBookId the ID of the order book the order will trigger on.
     * @return The current Nuam order.
     */
    NuamOrder &withTriggerOrderBookId(std::int32_t triggerOrderBookId) noexcept;

    /**
     * Returns the trigger price of the order.
     *
     * @return the trigger price of the order.
     */
    double getTriggerPrice() const noexcept;

    /**
     * Changes the trigger price of the order.
     *
     * @param triggerPrice the trigger price of the order.
     */
    void setTriggerPrice(double triggerPrice) noexcept;

    /**
     * Changes the trigger price of the order.
     * Returns the current Nuam order.
     *
     * @param triggerPrice the trigger price of the order.
     * @return The current Nuam order.
     */
    NuamOrder &withTriggerPrice(double triggerPrice) noexcept;

    /**
     * Returns the session type to trigger on.
     *
     * @return the session type to trigger on.
     */
    std::int32_t getTriggerSessionType() const noexcept;

    /**
     * Changes the session type to trigger on.
     *
     * @param triggerSessionType the session type to trigger on.
     */
    void setTriggerSessionType(std::int32_t triggerSessionType) noexcept;

    /**
     * Changes the session type to trigger on.
     * Returns the current Nuam order.
     *
     * @param triggerSessionType the session type to trigger on.
     * @return The current Nuam order.
     */
    NuamOrder &withTriggerSessionType(std::int32_t triggerSessionType) noexcept;

    /**
     * Returns the type of this order: limit, market, best order, etc..
     *
     * @return the type of this order: limit, market, best order, etc..
     */
    const NuamOrderType &getOrderType() const & noexcept;

    /**
     * Changes the type of this order: limit, market, best order, etc..
     *
     * @param orderType the type of this order: limit, market, best order, etc..
     */
    void setOrderType(const NuamOrderType &orderType) noexcept;

    /**
     * Changes the type of this order: limit, market, best order, etc..
     * Returns the current Nuam order.
     *
     * @param orderType the type of this order: limit, market, best order, etc..
     * @return The current Nuam order.
     */
    NuamOrder &withOrderType(const NuamOrderType &orderType) noexcept;

    /**
     * Returns the initial quantity of the order when it was entered (it can be changed later if the order is amended).
     *
     * @return the initial quantity of the order when it was entered (it can be changed later if the order is amended).
     */
    double getOrderQuantity() const noexcept;

    /**
     * Changes the initial quantity of the order when it was entered (it can be changed later if the order is amended).
     *
     * @param orderQuantity the initial quantity of the order when it was entered (it can be changed later if the order
     * is amended).
     */
    void setOrderQuantity(double orderQuantity) noexcept;

    /**
     * Changes the initial quantity of the order when it was entered (it can be changed later if the order is amended).
     * Returns the current Nuam order.
     *
     * @param orderQuantity the initial quantity of the order when it was entered (it can be changed later if the order
     * is amended).
     * @return The current Nuam order.
     */
    NuamOrder &withOrderQuantity(double orderQuantity) noexcept;

    /**
     * Returns the actual display quantity of a reserve order at the end of the transaction.
     *
     * @return the actual display quantity of a reserve order at the end of the transaction.
     */
    double getDisplayQuantity() const noexcept;

    /**
     * Changes the actual display quantity of a reserve order at the end of the transaction.
     *
     * @param displayQuantity the actual display quantity of a reserve order at the end of the transaction.
     */
    void setDisplayQuantity(double displayQuantity) noexcept;

    /**
     * Changes the actual display quantity of a reserve order at the end of the transaction.
     * Returns the current Nuam order.
     *
     * @param displayQuantity the actual display quantity of a reserve order at the end of the transaction.
     * @return The current Nuam order.
     */
    NuamOrder &withDisplayQuantity(double displayQuantity) noexcept;

    /**
     * Returns the initial display quantity of a reserve order.
     *
     * @return the initial display quantity of a reserve order.
     */
    double getRefreshQuantity() const noexcept;

    /**
     * Changes the initial display quantity of a reserve order.
     *
     * @param refreshQuantity the initial display quantity of a reserve order.
     */
    void setRefreshQuantity(double refreshQuantity) noexcept;

    /**
     * Changes the initial display quantity of a reserve order.
     * Returns the current Nuam order.
     *
     * @param refreshQuantity the initial display quantity of a reserve order.
     * @return The current Nuam order.
     */
    NuamOrder &withRefreshQuantity(double refreshQuantity) noexcept;

    /**
     * Returns the remaining (open) quantity of the order.
     *
     * @return the remaining (open) quantity of the order.
     */
    double getLeavesQuantity() const noexcept;

    /**
     * Changes the remaining (open) quantity of the order.
     *
     * @param leavesQuantity the remaining (open) quantity of the order.
     */
    void setLeavesQuantity(double leavesQuantity) noexcept;

    /**
     * Changes the remaining (open) quantity of the order.
     * Returns the current Nuam order.
     *
     * @param leavesQuantity the remaining (open) quantity of the order.
     * @return The current Nuam order.
     */
    NuamOrder &withLeavesQuantity(double leavesQuantity) noexcept;

    /**
     * Returns the matched quantity within the transaction that updated the order.
     *
     * @return the matched quantity within the transaction that updated the order.
     */
    double getMatchedQuantity() const noexcept;

    /**
     * Changes the matched quantity within the transaction that updated the order.
     *
     * @param matchedQuantity the matched quantity within the transaction that updated the order.
     */
    void setMatchedQuantity(double matchedQuantity) noexcept;

    /**
     * Changes the matched quantity within the transaction that updated the order.
     * Returns the current Nuam order.
     *
     * @param matchedQuantity the matched quantity within the transaction that updated the order.
     * @return The current Nuam order.
     */
    NuamOrder &withMatchedQuantity(double matchedQuantity) noexcept;

    /**
     * Returns a string representation of the current object.
     *
     * @return a string representation
     */
    std::string toString() const override;
};

DXFCPP_END_NAMESPACE

/// @}

DXFCXX_DISABLE_MSC_WARNINGS_POP()

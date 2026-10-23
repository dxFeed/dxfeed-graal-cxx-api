// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/custom/NuamOrder.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../../include/dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp"
#include "../../internal/utils/JavaUtils.hpp"
#include "../../internal/utils/StringConstants.hpp"

#include <dxfg_api.h>
#include <fmt/format.h>
#include <memory>
#include <optional>
#include <string>

DXFCPP_BEGIN_NAMESPACE

const EventTypeEnum &NuamOrder::TYPE = EventTypeEnum::NUAM_ORDER;

void NuamOrder::fillData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    Order::fillData(graalNative);

    const auto graalNuamOrder = static_cast<dxfg_nuam_order_t *>(graalNative);

    nuamOrderData_ = {
        .actorId = graalNuamOrder->actor_id,
        .participantId = graalNuamOrder->participant_id,
        .submitterId = graalNuamOrder->submitter_id,
        .onBehalfOfSubmitterId = graalNuamOrder->on_behalf_of_submitter_id,
        .clientOrderId = toStringOpt(graalNuamOrder->client_order_id),
        .customerAccount = toStringOpt(graalNuamOrder->customer_account),
        .customerInfo = toStringOpt(graalNuamOrder->customer_info),
        .exchangeInfo = toStringOpt(graalNuamOrder->exchange_info),
        .timeInForceData = graalNuamOrder->time_in_force_data,
        .triggerOrderBookId = graalNuamOrder->trigger_order_book_id,
        .triggerPrice = graalNuamOrder->trigger_price,
        .triggerSessionType = graalNuamOrder->trigger_session_type,
        .orderQuantity = graalNuamOrder->order_quantity,
        .displayQuantity = graalNuamOrder->display_quantity,
        .refreshQuantity = graalNuamOrder->refresh_quantity,
        .leavesQuantity = graalNuamOrder->leaves_quantity,
        .matchedQuantity = graalNuamOrder->matched_quantity,
        .nuamFlags = graalNuamOrder->nuam_flags,
    };
}

void NuamOrder::fillGraalData(void *graalNative) const noexcept {
    if (graalNative == nullptr) {
        return;
    }

    Order::fillGraalData(graalNative);

    const auto graalNuamOrder = static_cast<dxfg_nuam_order_t *>(graalNative);

    graalNuamOrder->order_base.order_base.market_event.event_type.clazz = DXFG_EVENT_NUAM_ORDER;
    graalNuamOrder->actor_id = nuamOrderData_.actorId;
    graalNuamOrder->participant_id = nuamOrderData_.participantId;
    graalNuamOrder->submitter_id = nuamOrderData_.submitterId;
    graalNuamOrder->on_behalf_of_submitter_id = nuamOrderData_.onBehalfOfSubmitterId;
    graalNuamOrder->client_order_id = createCString(nuamOrderData_.clientOrderId);
    graalNuamOrder->customer_account = createCString(nuamOrderData_.customerAccount);
    graalNuamOrder->customer_info = createCString(nuamOrderData_.customerInfo);
    graalNuamOrder->exchange_info = createCString(nuamOrderData_.exchangeInfo);
    graalNuamOrder->time_in_force_data = nuamOrderData_.timeInForceData;
    graalNuamOrder->trigger_order_book_id = nuamOrderData_.triggerOrderBookId;
    graalNuamOrder->trigger_price = nuamOrderData_.triggerPrice;
    graalNuamOrder->trigger_session_type = nuamOrderData_.triggerSessionType;
    graalNuamOrder->order_quantity = nuamOrderData_.orderQuantity;
    graalNuamOrder->display_quantity = nuamOrderData_.displayQuantity;
    graalNuamOrder->refresh_quantity = nuamOrderData_.refreshQuantity;
    graalNuamOrder->leaves_quantity = nuamOrderData_.leavesQuantity;
    graalNuamOrder->matched_quantity = nuamOrderData_.matchedQuantity;
    graalNuamOrder->nuam_flags = nuamOrderData_.nuamFlags;
}

void NuamOrder::freeGraalData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    Order::freeGraalData(graalNative);

    const auto graalNuamOrder = static_cast<dxfg_nuam_order_t *>(graalNative);

    delete[] graalNuamOrder->client_order_id;
    delete[] graalNuamOrder->customer_account;
    delete[] graalNuamOrder->customer_info;
    delete[] graalNuamOrder->exchange_info;
}

NuamOrder::Ptr NuamOrder::fromGraal(void *graalNative) {
    if (!graalNative) {
        throw InvalidArgumentException("Unable to create NuamOrder. The `graalNative` parameter is nullptr");
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_NUAM_ORDER) {
        const auto eventType = static_cast<dxfg_event_type_t *>(graalNative)->clazz;

        throw InvalidArgumentException(fmt::format("Unable to create NuamOrder. Wrong event class {}({})! Expected: {}",
                                                   isolated::toString(eventType), std::to_string(eventType),
                                                   isolated::toString(DXFG_EVENT_NUAM_ORDER)));
    }

    auto nuamOrder = std::make_shared<NuamOrder>();

    nuamOrder->fillData(graalNative);

    return nuamOrder;
}

void *NuamOrder::toGraal() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalNuamOrder = new dxfg_nuam_order_t{};

    fillGraalData(graalNuamOrder);

    return graalNuamOrder;
}

void NuamOrder::freeGraal(void *graalNative) {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_NUAM_ORDER) {
        const auto eventType = static_cast<dxfg_event_type_t *>(graalNative)->clazz;

        throw InvalidArgumentException(fmt::format(
            "Unable to free NuamOrder's Graal data. Wrong event class {}({})! Expected: {}",
            isolated::toString(eventType), std::to_string(eventType), isolated::toString(DXFG_EVENT_NUAM_ORDER)));
    }

    const auto graalNuamOrder = static_cast<dxfg_nuam_order_t *>(graalNative);

    freeGraalData(graalNative);

    delete graalNuamOrder;
}

void NuamOrder::assign(std::shared_ptr<EventType> event) {
    Order::assign(event);

    if (const auto other = event->sharedAs<NuamOrder>(); other) {
        nuamOrderData_ = other->nuamOrderData_;
    }
}

NuamOrder::NuamOrder() noexcept {
}

NuamOrder::NuamOrder(const StringLike &eventSymbol) noexcept : Order(eventSymbol) {
}

NuamOrder &NuamOrder::withEventSymbol(const StringLike &eventSymbol) noexcept {
    return dynamic_cast<NuamOrder &>(Order::withEventSymbol(eventSymbol));
}

NuamOrder &NuamOrder::withEventTime(std::int64_t eventTime) noexcept {
    return dynamic_cast<NuamOrder &>(Order::withEventTime(eventTime));
}

NuamOrder &NuamOrder::withSource(const OrderSource &source) noexcept {
    return dynamic_cast<NuamOrder &>(Order::withSource(source));
}

NuamOrder &NuamOrder::withEventFlags(std::int32_t eventFlags) noexcept {
    return dynamic_cast<NuamOrder &>(Order::withEventFlags(eventFlags));
}

NuamOrder &NuamOrder::withEventFlags(const EventFlagsMask &eventFlags) noexcept {
    return dynamic_cast<NuamOrder &>(Order::withEventFlags(eventFlags));
}

NuamOrder &NuamOrder::withIndex(std::int64_t index) {
    return dynamic_cast<NuamOrder &>(Order::withIndex(index));
}

NuamOrder &NuamOrder::withTime(std::int64_t time) noexcept {
    return dynamic_cast<NuamOrder &>(Order::withTime(time));
}

NuamOrder &NuamOrder::withTimeNanoPart(std::int32_t timeNanoPart) noexcept {
    return dynamic_cast<NuamOrder &>(Order::withTimeNanoPart(timeNanoPart));
}

NuamOrder &NuamOrder::withSequence(std::int32_t sequence) {
    return dynamic_cast<NuamOrder &>(Order::withSequence(sequence));
}

NuamOrder &NuamOrder::withTimeNanos(std::int64_t timeNanos) noexcept {
    return dynamic_cast<NuamOrder &>(Order::withTimeNanos(timeNanos));
}

NuamOrder &NuamOrder::withAction(const OrderAction &action) noexcept {
    return dynamic_cast<NuamOrder &>(Order::withAction(action));
}

NuamOrder &NuamOrder::withActionTime(std::int64_t actionTime) noexcept {
    return dynamic_cast<NuamOrder &>(Order::withActionTime(actionTime));
}

NuamOrder &NuamOrder::withOrderId(std::int64_t orderId) noexcept {
    return dynamic_cast<NuamOrder &>(Order::withOrderId(orderId));
}

NuamOrder &NuamOrder::withAuxOrderId(std::int64_t auxOrderId) noexcept {
    return dynamic_cast<NuamOrder &>(Order::withAuxOrderId(auxOrderId));
}

NuamOrder &NuamOrder::withPrice(double price) noexcept {
    return dynamic_cast<NuamOrder &>(Order::withPrice(price));
}

NuamOrder &NuamOrder::withSize(double size) noexcept {
    return dynamic_cast<NuamOrder &>(Order::withSize(size));
}

NuamOrder &NuamOrder::withExecutedSize(double executedSize) noexcept {
    return dynamic_cast<NuamOrder &>(Order::withExecutedSize(executedSize));
}

NuamOrder &NuamOrder::withCount(std::int64_t count) noexcept {
    return dynamic_cast<NuamOrder &>(Order::withCount(count));
}

NuamOrder &NuamOrder::withTradeId(std::int64_t tradeId) noexcept {
    return dynamic_cast<NuamOrder &>(Order::withTradeId(tradeId));
}

NuamOrder &NuamOrder::withTradePrice(double tradePrice) noexcept {
    return dynamic_cast<NuamOrder &>(Order::withTradePrice(tradePrice));
}

NuamOrder &NuamOrder::withTradeSize(double tradeSize) noexcept {
    return dynamic_cast<NuamOrder &>(Order::withTradeSize(tradeSize));
}

NuamOrder &NuamOrder::withExchangeCode(char exchangeCode) {
    return dynamic_cast<NuamOrder &>(Order::withExchangeCode(exchangeCode));
}

NuamOrder &NuamOrder::withExchangeCode(std::int16_t exchangeCode) {
    return dynamic_cast<NuamOrder &>(Order::withExchangeCode(exchangeCode));
}

NuamOrder &NuamOrder::withOrderSide(const Side &side) noexcept {
    return dynamic_cast<NuamOrder &>(Order::withOrderSide(side));
}

NuamOrder &NuamOrder::withScope(const Scope &scope) noexcept {
    return dynamic_cast<NuamOrder &>(Order::withScope(scope));
}

NuamOrder &NuamOrder::withMarketMaker(const StringLike &marketMaker) noexcept {
    return dynamic_cast<NuamOrder &>(Order::withMarketMaker(marketMaker));
}

std::int32_t NuamOrder::getActorId() const noexcept {
    return nuamOrderData_.actorId;
}

void NuamOrder::setActorId(std::int32_t actorId) noexcept {
    nuamOrderData_.actorId = actorId;
}

NuamOrder &NuamOrder::withActorId(std::int32_t actorId) noexcept {
    setActorId(actorId);

    return *this;
}

std::int32_t NuamOrder::getParticipantId() const noexcept {
    return nuamOrderData_.participantId;
}

void NuamOrder::setParticipantId(std::int32_t participantId) noexcept {
    nuamOrderData_.participantId = participantId;
}

NuamOrder &NuamOrder::withParticipantId(std::int32_t participantId) noexcept {
    setParticipantId(participantId);

    return *this;
}

std::int32_t NuamOrder::getSubmitterId() const noexcept {
    return nuamOrderData_.submitterId;
}

void NuamOrder::setSubmitterId(std::int32_t submitterId) noexcept {
    nuamOrderData_.submitterId = submitterId;
}

NuamOrder &NuamOrder::withSubmitterId(std::int32_t submitterId) noexcept {
    setSubmitterId(submitterId);

    return *this;
}

std::int32_t NuamOrder::getOnBehalfOfSubmitterId() const noexcept {
    return nuamOrderData_.onBehalfOfSubmitterId;
}

void NuamOrder::setOnBehalfOfSubmitterId(std::int32_t onBehalfOfSubmitterId) noexcept {
    nuamOrderData_.onBehalfOfSubmitterId = onBehalfOfSubmitterId;
}

NuamOrder &NuamOrder::withOnBehalfOfSubmitterId(std::int32_t onBehalfOfSubmitterId) noexcept {
    setOnBehalfOfSubmitterId(onBehalfOfSubmitterId);

    return *this;
}

const std::string &NuamOrder::getClientOrderId() const & noexcept {
    if (!nuamOrderData_.clientOrderId) {
        return nulString();
    }

    return nuamOrderData_.clientOrderId.value();
}

const std::optional<std::string> &NuamOrder::getClientOrderIdOpt() const & noexcept {
    return nuamOrderData_.clientOrderId;
}

void NuamOrder::setClientOrderId(const StringLike &clientOrderId) noexcept {
    nuamOrderData_.clientOrderId = std::string(clientOrderId);
}

NuamOrder &NuamOrder::withClientOrderId(const StringLike &clientOrderId) noexcept {
    setClientOrderId(clientOrderId);

    return *this;
}

const std::string &NuamOrder::getCustomerAccount() const & noexcept {
    if (!nuamOrderData_.customerAccount) {
        return nulString();
    }

    return nuamOrderData_.customerAccount.value();
}

const std::optional<std::string> &NuamOrder::getCustomerAccountOpt() const & noexcept {
    return nuamOrderData_.customerAccount;
}

void NuamOrder::setCustomerAccount(const StringLike &customerAccount) noexcept {
    nuamOrderData_.customerAccount = std::string(customerAccount);
}

NuamOrder &NuamOrder::withCustomerAccount(const StringLike &customerAccount) noexcept {
    setCustomerAccount(customerAccount);

    return *this;
}

const std::string &NuamOrder::getCustomerInfo() const & noexcept {
    if (!nuamOrderData_.customerInfo) {
        return nulString();
    }

    return nuamOrderData_.customerInfo.value();
}

const std::optional<std::string> &NuamOrder::getCustomerInfoOpt() const & noexcept {
    return nuamOrderData_.customerInfo;
}

void NuamOrder::setCustomerInfo(const StringLike &customerInfo) noexcept {
    nuamOrderData_.customerInfo = std::string(customerInfo);
}

NuamOrder &NuamOrder::withCustomerInfo(const StringLike &customerInfo) noexcept {
    setCustomerInfo(customerInfo);

    return *this;
}

const std::string &NuamOrder::getExchangeInfo() const & noexcept {
    if (!nuamOrderData_.exchangeInfo) {
        return nulString();
    }

    return nuamOrderData_.exchangeInfo.value();
}

const std::optional<std::string> &NuamOrder::getExchangeInfoOpt() const & noexcept {
    return nuamOrderData_.exchangeInfo;
}

void NuamOrder::setExchangeInfo(const StringLike &exchangeInfo) noexcept {
    nuamOrderData_.exchangeInfo = std::string(exchangeInfo);
}

NuamOrder &NuamOrder::withExchangeInfo(const StringLike &exchangeInfo) noexcept {
    setExchangeInfo(exchangeInfo);

    return *this;
}

const NuamTimeInForceType &NuamOrder::getTimeInForce() const & noexcept {
    return NuamTimeInForceType::valueOf(
        getBits(nuamOrderData_.nuamFlags, NUAM_TIME_IN_FORCE_TYPE_MASK, NUAM_TIME_IN_FORCE_TYPE_SHIFT));
}

void NuamOrder::setTimeInForce(const NuamTimeInForceType &timeInForce) noexcept {
    nuamOrderData_.nuamFlags = setBits(nuamOrderData_.nuamFlags, NUAM_TIME_IN_FORCE_TYPE_MASK,
                                       NUAM_TIME_IN_FORCE_TYPE_SHIFT, timeInForce.getCode());
}

NuamOrder &NuamOrder::withTimeInForce(const NuamTimeInForceType &timeInForce) noexcept {
    setTimeInForce(timeInForce);

    return *this;
}

std::int32_t NuamOrder::getTimeInForceData() const noexcept {
    return nuamOrderData_.timeInForceData;
}

void NuamOrder::setTimeInForceData(std::int32_t timeInForceData) noexcept {
    nuamOrderData_.timeInForceData = timeInForceData;
}

NuamOrder &NuamOrder::withTimeInForceData(std::int32_t timeInForceData) noexcept {
    setTimeInForceData(timeInForceData);

    return *this;
}

std::int32_t NuamOrder::getTriggerOrderBookId() const noexcept {
    return nuamOrderData_.triggerOrderBookId;
}

void NuamOrder::setTriggerOrderBookId(std::int32_t triggerOrderBookId) noexcept {
    nuamOrderData_.triggerOrderBookId = triggerOrderBookId;
}

NuamOrder &NuamOrder::withTriggerOrderBookId(std::int32_t triggerOrderBookId) noexcept {
    setTriggerOrderBookId(triggerOrderBookId);

    return *this;
}

double NuamOrder::getTriggerPrice() const noexcept {
    return nuamOrderData_.triggerPrice;
}

void NuamOrder::setTriggerPrice(double triggerPrice) noexcept {
    nuamOrderData_.triggerPrice = triggerPrice;
}

NuamOrder &NuamOrder::withTriggerPrice(double triggerPrice) noexcept {
    setTriggerPrice(triggerPrice);

    return *this;
}

std::int32_t NuamOrder::getTriggerSessionType() const noexcept {
    return nuamOrderData_.triggerSessionType;
}

void NuamOrder::setTriggerSessionType(std::int32_t triggerSessionType) noexcept {
    nuamOrderData_.triggerSessionType = triggerSessionType;
}

NuamOrder &NuamOrder::withTriggerSessionType(std::int32_t triggerSessionType) noexcept {
    setTriggerSessionType(triggerSessionType);

    return *this;
}

const NuamOrderType &NuamOrder::getOrderType() const & noexcept {
    return NuamOrderType::valueOf(getBits(nuamOrderData_.nuamFlags, NUAM_ORDER_TYPE_MASK, NUAM_ORDER_TYPE_SHIFT));
}

void NuamOrder::setOrderType(const NuamOrderType &orderType) noexcept {
    nuamOrderData_.nuamFlags =
        setBits(nuamOrderData_.nuamFlags, NUAM_ORDER_TYPE_MASK, NUAM_ORDER_TYPE_SHIFT, orderType.getCode());
}

NuamOrder &NuamOrder::withOrderType(const NuamOrderType &orderType) noexcept {
    setOrderType(orderType);

    return *this;
}

double NuamOrder::getOrderQuantity() const noexcept {
    return nuamOrderData_.orderQuantity;
}

void NuamOrder::setOrderQuantity(double orderQuantity) noexcept {
    nuamOrderData_.orderQuantity = orderQuantity;
}

NuamOrder &NuamOrder::withOrderQuantity(double orderQuantity) noexcept {
    setOrderQuantity(orderQuantity);

    return *this;
}

double NuamOrder::getDisplayQuantity() const noexcept {
    return nuamOrderData_.displayQuantity;
}

void NuamOrder::setDisplayQuantity(double displayQuantity) noexcept {
    nuamOrderData_.displayQuantity = displayQuantity;
}

NuamOrder &NuamOrder::withDisplayQuantity(double displayQuantity) noexcept {
    setDisplayQuantity(displayQuantity);

    return *this;
}

double NuamOrder::getRefreshQuantity() const noexcept {
    return nuamOrderData_.refreshQuantity;
}

void NuamOrder::setRefreshQuantity(double refreshQuantity) noexcept {
    nuamOrderData_.refreshQuantity = refreshQuantity;
}

NuamOrder &NuamOrder::withRefreshQuantity(double refreshQuantity) noexcept {
    setRefreshQuantity(refreshQuantity);

    return *this;
}

double NuamOrder::getLeavesQuantity() const noexcept {
    return nuamOrderData_.leavesQuantity;
}

void NuamOrder::setLeavesQuantity(double leavesQuantity) noexcept {
    nuamOrderData_.leavesQuantity = leavesQuantity;
}

NuamOrder &NuamOrder::withLeavesQuantity(double leavesQuantity) noexcept {
    setLeavesQuantity(leavesQuantity);

    return *this;
}

double NuamOrder::getMatchedQuantity() const noexcept {
    return nuamOrderData_.matchedQuantity;
}

void NuamOrder::setMatchedQuantity(double matchedQuantity) noexcept {
    nuamOrderData_.matchedQuantity = matchedQuantity;
}

NuamOrder &NuamOrder::withMatchedQuantity(double matchedQuantity) noexcept {
    setMatchedQuantity(matchedQuantity);

    return *this;
}

namespace {

// Java prints a null string as `null`.
const std::string &orNull(const std::optional<std::string> &value) {
    static const std::string NULL_STRING{"null"};

    return value ? *value : NULL_STRING;
}

} // namespace

std::string NuamOrder::toString() const {
    return fmt::format(
        "NuamOrder{{{}, marketMaker='{}', actorId={}, participantId={}, submitterId={}, onBehalfOfSubmitterId={}, "
        "clientOrderId='{}', customerAccount='{}', customerInfo='{}', timeInForce={}, timeInForceData={}, "
        "triggerOrderBookId={}, triggerPrice={}, triggerSessionType={}, orderType={}, orderQuantity={}, "
        "displayQuantity={}, refreshQuantity={}, leavesQuantity={}, matchedQuantity={}}}",
        baseFieldsToString(), orNull(getMarketMakerOpt()), getActorId(), getParticipantId(), getSubmitterId(),
        getOnBehalfOfSubmitterId(), orNull(getClientOrderIdOpt()), orNull(getCustomerAccountOpt()),
        orNull(getCustomerInfoOpt()), getTimeInForce().toString(), getTimeInForceData(), getTriggerOrderBookId(),
        java::doubleToString(getTriggerPrice()), getTriggerSessionType(), getOrderType().toString(),
        java::doubleToString(getOrderQuantity()), java::doubleToString(getDisplayQuantity()),
        java::doubleToString(getRefreshQuantity()), java::doubleToString(getLeavesQuantity()),
        java::doubleToString(getMatchedQuantity()));
}

DXFCPP_END_NAMESPACE

// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/misc/Message.hpp"

#include "../../../include/dxfeed_graal_cpp_api/event/EventType.hpp"
#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../../include/dxfeed_graal_cpp_api/internal/TimeFormat.hpp"

#include <dxfg_api.h>
#include <fmt/format.h>
#include <string>

DXFCPP_BEGIN_NAMESPACE

const EventTypeEnum &Message::TYPE = EventTypeEnum::MESSAGE;

void Message::fillData(void *graalNative) {
    if (graalNative == nullptr) {
        return;
    }

    auto graalMessage = static_cast<dxfg_message_t *>(graalNative);

    if (graalMessage->event_symbol != nullptr) {
        setEventSymbol(dxfcpp::toString(graalMessage->event_symbol));
    }

    if (graalMessage->attachment != nullptr) {
        setAttachment(dxfcpp::toString(dxfcpp::bit_cast<const char *>(graalMessage->attachment)));
    }
}

void Message::fillGraalData(void *graalNative) const {
    if (graalNative == nullptr) {
        return;
    }

    const auto graalMessage = static_cast<dxfg_message_t *>(graalNative);

    graalMessage->event_type.clazz = DXFG_EVENT_MESSAGE;
    graalMessage->event_symbol = createCString(eventSymbol_);
    graalMessage->event_time = eventTime_;
    graalMessage->attachment = dxfcpp::bit_cast<void *>(createCString(attachment_));
}

void Message::freeGraalData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    const auto graalMessage = static_cast<dxfg_message_t *>(graalNative);

    delete[] graalMessage->event_symbol;

    if (graalMessage->attachment != nullptr) {
        delete[] dxfcpp::bit_cast<const char *>(graalMessage->attachment);
    }
}

std::shared_ptr<Message> Message::fromGraal(void *graalNative) {
    if (!graalNative) {
        throw InvalidArgumentException("Unable to create Message. The `graalNative` parameter is nullptr");
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_MESSAGE) {
        throw InvalidArgumentException(fmt::format("Unable to create Message. Wrong event class {}! Expected: {}.",
                                                   std::to_string(static_cast<dxfg_event_type_t *>(graalNative)->clazz),
                                                   std::to_string(DXFG_EVENT_MESSAGE)));
    }

    auto message = std::make_shared<Message>();

    message->fillData(graalNative);

    return message;
}

void *Message::toGraal() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalMessage = new dxfg_message_t{};

    fillGraalData(graalMessage);

    return graalMessage;
}

void Message::freeGraal(void *graalNative) {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_MESSAGE) {
        throw InvalidArgumentException(fmt::format(
            "Unable to free Message's Graal data. Wrong event class {}! Expected: {}.",
            std::to_string(static_cast<dxfg_event_type_t *>(graalNative)->clazz), std::to_string(DXFG_EVENT_MESSAGE)));
    }

    auto graalMessage = static_cast<dxfg_message_t *>(graalNative);

    freeGraalData(graalNative);

    delete graalMessage;
}

void Message::assign(std::shared_ptr<EventType> event) {
    if (const auto other = event->sharedAs<Message>(); other) {
        eventSymbol_ = other->eventSymbol_;
        eventTime_ = other->eventTime_;
        attachment_ = other->attachment_;
    }
}

Message::Message() noexcept {
}

Message::Message(const StringLike &eventSymbol) noexcept : eventSymbol_{eventSymbol} {
}

Message::Message(const StringLike &eventSymbol, const StringLike &attachment) noexcept
    : eventSymbol_{eventSymbol}, attachment_{attachment} {
}

const std::string &Message::getEventSymbol() const & noexcept {
    if (!eventSymbol_) {
        return String::NUL;
    }

    return eventSymbol_.value();
}

const std::optional<std::string> &Message::getEventSymbolOpt() const & noexcept {
    return eventSymbol_;
}

void Message::setEventSymbol(const StringLike &eventSymbol) noexcept {
    // TODO: check invalid utf-8 [EN-8233]
    eventSymbol_ = std::string(eventSymbol);
}

Message &Message::withEventSymbol(const StringLike &eventSymbol) noexcept {
    Message::setEventSymbol(eventSymbol);

    return *this;
}

std::int64_t Message::getEventTime() const noexcept {
    return eventTime_;
}

void Message::setEventTime(std::int64_t eventTime) noexcept {
    eventTime_ = eventTime;
}

Message &Message::withEventTime(std::int64_t eventTime) noexcept {
    Message::setEventTime(eventTime);

    return *this;
}

const std::string &Message::getAttachment() const & {
    if (!attachment_) {
        return String::NUL;
    }

    return attachment_.value();
}

const std::optional<std::string> &Message::getAttachmentOpt() const & noexcept {
    return attachment_;
}

void Message::setAttachment(const StringLike &attachment) {
    attachment_ = std::string(attachment);
}

Message &Message::withAttachment(const StringLike &attachment) noexcept {
    setAttachment(attachment);

    return *this;
}

std::string Message::toString() const {
    return fmt::format("Message{{{}, eventTime={}, attachment={}}}", getEventSymbol(),
                       TimeFormat::DEFAULT_WITH_MILLIS.format(getEventTime()), attachment_.value_or(String::NUL));
}

DXFCPP_END_NAMESPACE
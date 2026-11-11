// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/misc/Message.hpp"

#include "../../../include/dxfeed_graal_cpp_api/event/EventType.hpp"
#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../../include/dxfeed_graal_cpp_api/util/TimeFormat.hpp"
#include "../../internal/utils/debug/Debug.hpp"
#include "../../internal/utils/StringConstants.hpp"
#include "../../internal/Common.hpp"
#include "../../internal/utils/GraalEventUtils.hpp"

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

    setEventTime(graalMessage->event_time);

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
    graalMessage->event_symbol = createCString(eventSymbol_, "Message::eventSymbol");
    graalMessage->event_time = eventTime_;
    graalMessage->attachment = dxfcpp::bit_cast<void *>(createCString(attachment_, "Message::attachment"));
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

    return newGraalEvent<dxfg_message_t>(
        DXFG_EVENT_MESSAGE,
        [this](void *graalNative) {
            fillGraalData(graalNative);
        },
        freeGraalData);
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

Message::Message(StringParam eventSymbol) : eventSymbol_{eventSymbol.view()} {
}

Message::Message(StringParam eventSymbol, StringParam attachment)
    : eventSymbol_{eventSymbol.view()}, attachment_{attachment.view()} {
}

const std::string &Message::getEventSymbol() const & noexcept {
    if (!eventSymbol_) {
        return nulString();
    }

    return *eventSymbol_;
}

const std::optional<std::string> &Message::getEventSymbolOpt() const & noexcept {
    return eventSymbol_;
}

void Message::setEventSymbol(StringParam eventSymbol) {
    eventSymbol_ = std::string(eventSymbol.view());
}

Message &Message::withEventSymbol(StringParam eventSymbol) {
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
        return nulString();
    }

    return attachment_.value();
}

const std::optional<std::string> &Message::getAttachmentOpt() const & noexcept {
    return attachment_;
}

void Message::setAttachment(StringParam attachment) {
    attachment_ = std::string(attachment.view());
}

Message &Message::withAttachment(StringParam attachment) {
    setAttachment(attachment);

    return *this;
}

std::string Message::toString() const {
    return fmt::format("Message{{{}, eventTime={}, attachment={}}}", getEventSymbol(),
                       TimeFormat::DEFAULT_WITH_MILLIS.format(getEventTime()), attachment_.value_or(String::NUL));
}

DXFCPP_END_NAMESPACE

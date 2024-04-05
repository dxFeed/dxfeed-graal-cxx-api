// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstring>
#include <memory>
#include <utf8.h>
#include <utility>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

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

    auto graalMessage = static_cast<dxfg_message_t *>(graalNative);

    graalMessage->event_type.clazz = dxfg_event_clazz_t::DXFG_EVENT_MESSAGE;
    graalMessage->event_symbol = createCString(eventSymbol_);
    graalMessage->event_time = eventTime_;
    graalMessage->attachment = dxfcpp::bit_cast<void *>(createCString(attachment_));
}

void Message::freeGraalData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    auto graalMessage = static_cast<dxfg_message_t *>(graalNative);

    delete[] graalMessage->event_symbol;

    if (graalMessage->attachment != nullptr) {
        delete[] dxfcpp::bit_cast<const char *>(graalMessage->attachment);
    }
}

std::shared_ptr<Message> Message::fromGraal(void *graalNative) {
    if (!graalNative) {
        throw std::invalid_argument("Unable to create Message. The `graalNative` parameter is nullptr");
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != dxfg_event_clazz_t::DXFG_EVENT_MESSAGE) {
        throw std::invalid_argument(
            fmt::format("Unable to create Message. Wrong event class {}! Expected: {}.",
                        std::to_string(static_cast<int>(static_cast<dxfg_event_type_t *>(graalNative)->clazz)),
                        std::to_string(static_cast<int>(dxfg_event_clazz_t::DXFG_EVENT_MESSAGE))));
    }

    auto message = std::make_shared<Message>();

    message->fillData(graalNative);

    return message;
}

void *Message::toGraal() const {
    if constexpr (Debugger::isDebug) {
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalMessage = new dxfg_message_t{};

    fillGraalData(static_cast<void *>(graalMessage));

    return static_cast<void *>(graalMessage);
}

void Message::freeGraal(void *graalNative) {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != dxfg_event_clazz_t::DXFG_EVENT_MESSAGE) {
        throw std::invalid_argument(
            fmt::format("Unable to free Message's Graal data. Wrong event class {}! Expected: {}.",
                        std::to_string(static_cast<int>(static_cast<dxfg_event_type_t *>(graalNative)->clazz)),
                        std::to_string(static_cast<int>(dxfg_event_clazz_t::DXFG_EVENT_MESSAGE))));
    }

    auto graalMessage = static_cast<dxfg_message_t *>(graalNative);

    freeGraalData(graalNative);

    delete graalMessage;
}

std::string Message::toString() const noexcept {
    return fmt::format("Message{{{}, eventTime={}, attachment={}}}", getEventSymbol(),
                       TimeFormat::DEFAULT_WITH_MILLIS.format(getEventTime()), attachment_.value_or(String::NUL));
}

DXFCPP_END_NAMESPACE
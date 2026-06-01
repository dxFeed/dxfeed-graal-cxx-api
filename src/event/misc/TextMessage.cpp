// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/misc/TextMessage.hpp"

#include "../../../include/dxfeed_graal_cpp_api/event/EventType.hpp"
#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../../include/dxfeed_graal_cpp_api/internal/TimeFormat.hpp"

#include <dxfg_api.h>
#include <fmt/format.h>
#include <string>

DXFCPP_BEGIN_NAMESPACE

const EventTypeEnum &TextMessage::TYPE = EventTypeEnum::TEXT_MESSAGE;

void TextMessage::fillData(void *graalNative) {
    if (graalNative == nullptr) {
        return;
    }

    auto graalMessage = static_cast<dxfg_text_message_t *>(graalNative);

    if (graalMessage->event_symbol != nullptr) {
        setEventSymbol(dxfcpp::toString(graalMessage->event_symbol));
    }

    eventTime_ = graalMessage->event_time;
    timeSequence_ = graalMessage->time_sequence;

    if (graalMessage->text != nullptr) {
        setText(dxfcpp::toString(dxfcpp::bit_cast<const char *>(graalMessage->text)));
    } else {
        setText(String::NUL);
    }
}

void TextMessage::fillGraalData(void *graalNative) const {
    if (graalNative == nullptr) {
        return;
    }

    const auto graalMessage = static_cast<dxfg_text_message_t *>(graalNative);

    graalMessage->event_type.clazz = DXFG_EVENT_TEXT_MESSAGE;
    graalMessage->event_symbol = createCString(eventSymbol_);
    graalMessage->event_time = eventTime_;
    graalMessage->time_sequence = timeSequence_;
    graalMessage->text = createCString(text_);
}

void TextMessage::freeGraalData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    auto graalMessage = static_cast<dxfg_text_message_t *>(graalNative);

    delete[] graalMessage->event_symbol;
    delete[] graalMessage->text;
}

std::shared_ptr<TextMessage> TextMessage::fromGraal(void *graalNative) {
    if (!graalNative) {
        throw InvalidArgumentException("Unable to create TextMessage. The `graalNative` parameter is nullptr");
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_TEXT_MESSAGE) {
        throw InvalidArgumentException(fmt::format("Unable to create TextMessage. Wrong event class {}! Expected: {}.",
                                                   std::to_string(static_cast<dxfg_event_type_t *>(graalNative)->clazz),
                                                   std::to_string(DXFG_EVENT_TEXT_MESSAGE)));
    }

    auto message = std::make_shared<TextMessage>();

    message->fillData(graalNative);

    return message;
}

void *TextMessage::toGraal() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalMessage = new dxfg_text_message_t{};

    fillGraalData(graalMessage);

    return graalMessage;
}

void TextMessage::freeGraal(void *graalNative) {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_TEXT_MESSAGE) {
        throw InvalidArgumentException(
            fmt::format("Unable to free TextMessage's Graal data. Wrong event class {}! Expected: {}.",
                        std::to_string(static_cast<dxfg_event_type_t *>(graalNative)->clazz),
                        std::to_string(DXFG_EVENT_TEXT_MESSAGE)));
    }

    const auto graalMessage = static_cast<dxfg_text_message_t *>(graalNative);

    freeGraalData(graalNative);

    delete graalMessage;
}

void TextMessage::assign(std::shared_ptr<EventType> event) {
    if (const auto other = event->sharedAs<TextMessage>(); other) {
        eventSymbol_ = other->eventSymbol_;
        eventTime_ = other->eventTime_;
        timeSequence_ = other->timeSequence_;
        text_ = other->text_;
    }
}

TextMessage::TextMessage() noexcept {
}

TextMessage::TextMessage(const StringLike &eventSymbol) noexcept : eventSymbol_{eventSymbol} {
}

TextMessage::TextMessage(const StringLike &eventSymbol, const StringLike &text) noexcept
    : eventSymbol_{eventSymbol}, text_{text} {
}

TextMessage::TextMessage(const StringLike &eventSymbol, std::int64_t time, const StringLike &text) noexcept
    : eventSymbol_{eventSymbol}, text_{text} {

    setTime(time);
}

const std::string &TextMessage::getEventSymbol() const & noexcept {
    if (!eventSymbol_) {
        return String::NUL;
    }

    return eventSymbol_.value();
}

const std::optional<std::string> &TextMessage::getEventSymbolOpt() const & noexcept {
    return eventSymbol_;
}

void TextMessage::setEventSymbol(const StringLike &eventSymbol) noexcept {
    // TODO: check invalid utf-8 [EN-8233]
    eventSymbol_ = std::string(eventSymbol);
}

TextMessage &TextMessage::withEventSymbol(const StringLike &eventSymbol) noexcept {
    TextMessage::setEventSymbol(eventSymbol);

    return *this;
}

std::int64_t TextMessage::getEventTime() const noexcept {
    return eventTime_;
}

void TextMessage::setEventTime(std::int64_t eventTime) noexcept {
    eventTime_ = eventTime;
}

TextMessage &TextMessage::withEventTime(std::int64_t eventTime) noexcept {
    TextMessage::setEventTime(eventTime);

    return *this;
}

std::int64_t TextMessage::getTimeSequence() const noexcept {
    return timeSequence_;
}

void TextMessage::setTimeSequence(std::int64_t timeSequence) noexcept {
    timeSequence_ = timeSequence;
}

std::int64_t TextMessage::getTime() const noexcept {
    return sar(timeSequence_, SECONDS_SHIFT) * 1000 + andOp(sar(timeSequence_, MILLISECONDS_SHIFT), MILLISECONDS_MASK);
}

void TextMessage::setTime(std::int64_t time) noexcept {
    timeSequence_ = orOp(orOp(sal(static_cast<std::int64_t>(time_util::getSecondsFromTime(time)), SECONDS_SHIFT),
                              sal(static_cast<std::int64_t>(time_util::getMillisFromTime(time)), MILLISECONDS_SHIFT)),
                         getSequence());
}

TextMessage &TextMessage::withTime(std::int64_t time) noexcept {
    setTime(time);

    return *this;
}

std::int32_t TextMessage::getSequence() const noexcept {
    return static_cast<std::int32_t>(andOp(timeSequence_, MAX_SEQUENCE));
}

void TextMessage::setSequence(std::int32_t sequence) {
    if (sequence < 0 || static_cast<std::uint32_t>(sequence) > MAX_SEQUENCE) {
        throw InvalidArgumentException("Invalid sequence value = " + std::to_string(sequence));
    }

    timeSequence_ = orOp(andOp(timeSequence_, ~MAX_SEQUENCE), sequence);
}

TextMessage &TextMessage::withSequence(std::int32_t sequence) noexcept {
    setSequence(sequence);

    return *this;
}

const std::string &TextMessage::getText() const & {
    return text_;
}

void TextMessage::setText(const StringLike &text) {
    // TODO: check invalid utf-8 [EN-8233]
    text_ = std::string(text);
}

TextMessage &TextMessage::withText(const StringLike &text) noexcept {
    setText(text);

    return *this;
}

std::string TextMessage::toString() const {
    return fmt::format("TextMessage{{{}, eventTime={}, time={}, sequence={}, text={}}}", getEventSymbol(),
                       TimeFormat::DEFAULT_WITH_MILLIS.format(getEventTime()),
                       TimeFormat::DEFAULT_WITH_MILLIS.format(getTime()), getSequence(), text_);
}

DXFCPP_END_NAMESPACE
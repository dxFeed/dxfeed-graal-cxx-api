// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/misc/TextConfiguration.hpp"

#include "../../../include/dxfeed_graal_cpp_api/event/EventType.hpp"
#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../../include/dxfeed_graal_cpp_api/internal/TimeFormat.hpp"
#include "../../../include/dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp"
#include "../../internal/utils/StringConstants.hpp"

#include <dxfg_api.h>
#include <fmt/format.h>
#include <string>

DXFCPP_BEGIN_NAMESPACE

const EventTypeEnum &TextConfiguration::TYPE = EventTypeEnum::TEXT_CONFIGURATION;

void TextConfiguration::fillData(void *graalNative) {
    if (graalNative == nullptr) {
        return;
    }

    const auto graalConfiguration = static_cast<dxfg_text_configuration_t *>(graalNative);

    eventSymbol_ = toStringOpt(graalConfiguration->event_symbol);
    eventTime_ = graalConfiguration->event_time;
    timeSequence_ = graalConfiguration->time_sequence;
    version_ = graalConfiguration->version;
    text_ = toStringOpt(graalConfiguration->text);
}

void TextConfiguration::fillGraalData(void *graalNative) const {
    if (graalNative == nullptr) {
        return;
    }

    const auto graalConfiguration = static_cast<dxfg_text_configuration_t *>(graalNative);

    graalConfiguration->event_type.clazz = DXFG_EVENT_TEXT_CONFIGURATION;
    graalConfiguration->event_symbol = createCString(eventSymbol_);
    graalConfiguration->event_time = eventTime_;
    graalConfiguration->time_sequence = timeSequence_;
    graalConfiguration->version = version_;
    graalConfiguration->text = createCString(text_);
}

void TextConfiguration::freeGraalData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    const auto graalConfiguration = static_cast<dxfg_text_configuration_t *>(graalNative);

    delete[] graalConfiguration->event_symbol;
    delete[] graalConfiguration->text;
}

std::shared_ptr<TextConfiguration> TextConfiguration::fromGraal(void *graalNative) {
    if (!graalNative) {
        throw InvalidArgumentException("Unable to create TextConfiguration. The `graalNative` parameter is nullptr");
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_TEXT_CONFIGURATION) {
        const auto eventType = static_cast<dxfg_event_type_t *>(graalNative)->clazz;

        throw InvalidArgumentException(fmt::format(
            "Unable to create TextConfiguration. Wrong event class {}({})! Expected: {}", isolated::toString(eventType),
            std::to_string(eventType), isolated::toString(DXFG_EVENT_TEXT_CONFIGURATION)));
    }

    auto configuration = std::make_shared<TextConfiguration>();

    configuration->fillData(graalNative);

    return configuration;
}

void *TextConfiguration::toGraal() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalConfiguration = new dxfg_text_configuration_t{};

    fillGraalData(graalConfiguration);

    return graalConfiguration;
}

void TextConfiguration::freeGraal(void *graalNative) {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_TEXT_CONFIGURATION) {
        const auto eventType = static_cast<dxfg_event_type_t *>(graalNative)->clazz;

        throw InvalidArgumentException(
            fmt::format("Unable to free TextConfiguration's Graal data. Wrong event class {}({})! Expected: {}",
                        isolated::toString(eventType), std::to_string(eventType),
                        isolated::toString(DXFG_EVENT_TEXT_CONFIGURATION)));
    }

    const auto graalConfiguration = static_cast<dxfg_text_configuration_t *>(graalNative);

    freeGraalData(graalNative);

    delete graalConfiguration;
}

void TextConfiguration::assign(std::shared_ptr<EventType> event) {
    if (const auto other = event->sharedAs<TextConfiguration>(); other) {
        eventSymbol_ = other->eventSymbol_;
        eventTime_ = other->eventTime_;
        timeSequence_ = other->timeSequence_;
        version_ = other->version_;
        text_ = other->text_;
    }
}

TextConfiguration::TextConfiguration() noexcept {
}

TextConfiguration::TextConfiguration(const StringLike &eventSymbol) noexcept : eventSymbol_{eventSymbol} {
}

TextConfiguration::TextConfiguration(const StringLike &eventSymbol, const StringLike &text) noexcept
    : eventSymbol_{eventSymbol}, text_{text} {
}

TextConfiguration::TextConfiguration(const StringLike &eventSymbol, const StringLike &text,
                                     std::int32_t version) noexcept
    : eventSymbol_{eventSymbol}, version_{version}, text_{text} {
}

const std::string &TextConfiguration::getEventSymbol() const & noexcept {
    if (!eventSymbol_) {
        return nulString();
    }

    return *eventSymbol_;
}

const std::optional<std::string> &TextConfiguration::getEventSymbolOpt() const & noexcept {
    return eventSymbol_;
}

void TextConfiguration::setEventSymbol(const StringLike &eventSymbol) noexcept {
    eventSymbol_ = std::string(eventSymbol);
}

TextConfiguration &TextConfiguration::withEventSymbol(const StringLike &eventSymbol) noexcept {
    TextConfiguration::setEventSymbol(eventSymbol);

    return *this;
}

std::int64_t TextConfiguration::getEventTime() const noexcept {
    return eventTime_;
}

void TextConfiguration::setEventTime(std::int64_t eventTime) noexcept {
    eventTime_ = eventTime;
}

TextConfiguration &TextConfiguration::withEventTime(std::int64_t eventTime) noexcept {
    TextConfiguration::setEventTime(eventTime);

    return *this;
}

std::int64_t TextConfiguration::getTimeSequence() const noexcept {
    return timeSequence_;
}

void TextConfiguration::setTimeSequence(std::int64_t timeSequence) noexcept {
    timeSequence_ = timeSequence;
}

std::int64_t TextConfiguration::getTime() const noexcept {
    // Seconds from bits 63..32 (an arithmetic shift keeps the sign of a time before the epoch) plus milliseconds from
    // bits 31..22 (10 bits, 0..999).
    return sar(timeSequence_, SECONDS_SHIFT) * 1000 + andOp(sar(timeSequence_, MILLISECONDS_SHIFT), MILLISECONDS_MASK);
}

void TextConfiguration::setTime(std::int64_t time) noexcept {
    // Rebuilds the field from the seconds and milliseconds of the new time and keeps the sequence in bits 21..0.
    timeSequence_ = orOp(orOp(sal(static_cast<std::int64_t>(time_util::getSecondsFromTime(time)), SECONDS_SHIFT),
                              sal(static_cast<std::int64_t>(time_util::getMillisFromTime(time)), MILLISECONDS_SHIFT)),
                         getSequence());
}

TextConfiguration &TextConfiguration::withTime(std::int64_t time) noexcept {
    setTime(time);

    return *this;
}

std::int32_t TextConfiguration::getSequence() const noexcept {
    // MAX_SEQUENCE is 22 one bits, so it also serves as the mask of bits 21..0.
    return static_cast<std::int32_t>(andOp(timeSequence_, MAX_SEQUENCE));
}

void TextConfiguration::setSequence(std::int32_t sequence) {
    if (sequence < 0 || static_cast<std::uint32_t>(sequence) > MAX_SEQUENCE) {
        throw InvalidArgumentException("Invalid sequence value = " + std::to_string(sequence));
    }

    // Clears bits 21..0 and puts the sequence there; the time in bits 63..22 stays as is.
    timeSequence_ = orOp(andOp(timeSequence_, ~static_cast<std::int64_t>(MAX_SEQUENCE)), sequence);
}

TextConfiguration &TextConfiguration::withSequence(std::int32_t sequence) {
    setSequence(sequence);

    return *this;
}

std::int32_t TextConfiguration::getVersion() const noexcept {
    return version_;
}

void TextConfiguration::setVersion(std::int32_t version) noexcept {
    version_ = version;
}

TextConfiguration &TextConfiguration::withVersion(std::int32_t version) noexcept {
    setVersion(version);

    return *this;
}

const std::string &TextConfiguration::getText() const & noexcept {
    if (!text_) {
        return nulString();
    }

    return *text_;
}

const std::optional<std::string> &TextConfiguration::getTextOpt() const & noexcept {
    return text_;
}

void TextConfiguration::setText(const StringLike &text) noexcept {
    text_ = std::string(text);
}

TextConfiguration &TextConfiguration::withText(const StringLike &text) noexcept {
    setText(text);

    return *this;
}

std::string TextConfiguration::toString() const {
    return fmt::format("TextConfiguration{{{}, eventTime={}, time={}, sequence={}, version={}, text='{}'}}",
                       getEventSymbol(), TimeFormat::DEFAULT_WITH_MILLIS.format(getEventTime()),
                       TimeFormat::DEFAULT_WITH_MILLIS.format(getTime()), getSequence(), getVersion(),
                       text_ ? *text_ : std::string("null"));
}

DXFCPP_END_NAMESPACE

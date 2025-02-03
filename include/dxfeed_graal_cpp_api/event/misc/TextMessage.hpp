// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <cstdint>
#include <string>

#include "../../exceptions/InvalidArgumentException.hpp"
#include "../../internal/Common.hpp"
#include "../EventType.hpp"
#include "../EventTypeEnum.hpp"

DXFCPP_BEGIN_NAMESPACE

struct EventMapper;

/**
 * Message event with text payload. Messages are never conflated and are delivered to
 * all connected subscribers. There is no built-in persistence for messages. They are lost when subscribers
 * are not connected to the message publisher, so they shall be only used for notification purposes in
 * addition to persistence mechanism.
 *
 * <h3>Implementation details</h3>
 *
 * This event is implemented on top of QDS record `TextMessage`.
 */
class DXFCPP_EXPORT TextMessage : public EventTypeWithSymbol<std::string> {
    friend struct EventMapper;

  protected:
    static constexpr std::uint64_t SECONDS_SHIFT = 32ULL;
    static constexpr std::uint64_t MILLISECONDS_SHIFT = 22ULL;
    static constexpr std::uint64_t MILLISECONDS_MASK = 0x3ffULL;

    std::optional<std::string> eventSymbol_{};
    std::int64_t eventTime_{};
    std::int64_t timeSequence_{};
    std::string text_{};

    void fillData(void *graalNative);
    void fillGraalData(void *graalNative) const;
    static void freeGraalData(void *graalNative) noexcept;

  public:
    /**
     * Maximum allowed sequence value.
     *
     * @see TextMessage::setSequence()
     */
    static constexpr std::uint32_t MAX_SEQUENCE = (1U << 22U) - 1U;

    /// The alias to a type of shared pointer to the TextMessage object
    using Ptr = std::shared_ptr<TextMessage>;

    /// The alias to a type of unique pointer to the TextMessage object
    using Unique = std::unique_ptr<TextMessage>;

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

    /**
     * Creates new message with default values.
     */
    TextMessage() noexcept = default;

    /**
     * Creates new message with the specified event symbol.
     * @param eventSymbol event symbol.
     */
    explicit TextMessage(std::string eventSymbol) noexcept : eventSymbol_{std::move(eventSymbol)} {
    }

    /**
     * Creates new message with the specified event symbol and text.
     *
     * @param eventSymbol event symbol.
     * @param text text.
     */
    TextMessage(std::string eventSymbol, std::string text) noexcept
        : eventSymbol_{std::move(eventSymbol)}, text_{std::move(text)} {
    }

    /**
     * Creates new message with the specified event symbol, time and text.
     * @param eventSymbol event symbol.
     * @param time time.
     * @param text text.
     */
    TextMessage(std::string eventSymbol, std::int64_t time, std::string text) noexcept
        : eventSymbol_{std::move(eventSymbol)}, text_{std::move(text)} {

        setTime(time);
    }

    /**
     * Returns symbol of this event.
     *
     * @return symbol of this event or dxfcpp::String::NUL (`std::string{"<null>"}`)
     */
    const std::string &getEventSymbol() const & noexcept override {
        if (!eventSymbol_) {
            return dxfcpp::String::NUL;
        }

        return eventSymbol_.value();
    }

    /**
     * Returns symbol of this event.
     *
     * @return symbol of this event or `std::nullopt`.
     */
    const std::optional<std::string> &getEventSymbolOpt() const & noexcept override {
        return eventSymbol_;
    }

    /**
     * Changes symbol of this event.
     *
     * @param eventSymbol The symbol of this event.
     */
    void setEventSymbol(const std::string &eventSymbol) noexcept override {
        // TODO: check invalid utf-8 [EN-8233]
        eventSymbol_ = eventSymbol;
    }

    /**
     * Changes event's symbol and returns the current message.
     *
     * @param eventSymbol The symbol of this event.
     * @return The current message.
     */
    TextMessage &withEventSymbol(const std::string &eventSymbol) noexcept {
        TextMessage::setEventSymbol(eventSymbol);

        return *this;
    }

    ///
    std::int64_t getEventTime() const noexcept override {
        return eventTime_;
    }

    ///
    void setEventTime(std::int64_t eventTime) noexcept override {
        eventTime_ = eventTime;
    }

    /**
     * Changes event's creation time and returns the current message.
     *
     * @param eventTime the difference, measured in milliseconds, between the event creation time and
     * midnight, January 1, 1970 UTC.
     * @return The current message.
     */
    TextMessage &withEventTime(std::int64_t eventTime) noexcept {
        TextMessage::setEventTime(eventTime);

        return *this;
    }

    /**
     * Returns time and sequence of text message packaged into single long value.
     *
     * @return time and sequence of text message.
     */
    std::int64_t getTimeSequence() const noexcept {
        return timeSequence_;
    }

    /**
     * Changes time and sequence of text message.
     * <b>Do not use this method directly.</b>
     * Change @ref #setTime() "time" and/or @ref #setSequence() "sequence".
     *
     * @param timeSequence the time and sequence.
     * @see #getTimeSequence()
     */
    void setTimeSequence(std::int64_t timeSequence) noexcept {
        timeSequence_ = timeSequence;
    }

    /**
     * Returns time of the text message.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @return time of the text message.
     */
    std::int64_t getTime() const noexcept {
        return sar(timeSequence_, SECONDS_SHIFT) * 1000 +
               andOp(sar(timeSequence_, MILLISECONDS_SHIFT), MILLISECONDS_MASK);
    }

    /**
     * Changes time of the text message.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @param time time of the text message.
     */
    void setTime(std::int64_t time) noexcept {
        timeSequence_ =
            orOp(orOp(sal(static_cast<std::int64_t>(time_util::getSecondsFromTime(time)), SECONDS_SHIFT),
                      sal(static_cast<std::int64_t>(time_util::getMillisFromTime(time)), MILLISECONDS_SHIFT)),
                 getSequence());
    }

    /**
     * Changes time and returns the current message.
     *
     * @param time time of the text message.
     * @return The current message.
     */
    TextMessage &withTime(std::int64_t time) noexcept {
        TextMessage::setTime(time);

        return *this;
    }

    /**
     * Returns sequence number of the text message to distinguish messages that have the same @ref #getTime() "time".
     * This sequence number does not have to be unique and does not need to be sequential. Sequence can range from 0 to
     * #MAX_SEQUENCE.
     *
     * @return sequence of the text message.
     */
    std::int32_t getSequence() const noexcept {
        return static_cast<std::int32_t>(andOp(timeSequence_, MAX_SEQUENCE));
    }

    /**
     * Changes ::getSequence() sequence number of the text message.
     *
     * @param sequence the sequence.
     * @throws InvalidArgumentException if sequence is below zero or above #MAX_SEQUENCE.
     * @see #getSequence()
     */
    void setSequence(std::int32_t sequence) {
        if (sequence < 0 || static_cast<std::uint32_t>(sequence) > MAX_SEQUENCE) {
            throw InvalidArgumentException("Invalid sequence value = " + std::to_string(sequence));
        }

        timeSequence_ = orOp(andOp(timeSequence_, ~MAX_SEQUENCE), sequence);
    }

    /**
     * Changes event's sequence number and returns the current message.
     *
     * @param sequence the sequence.
     * @return The current message.
     * @throws InvalidArgumentException if sequence is below zero or above #MAX_SEQUENCE.
     */
    TextMessage &withSequence(std::int32_t sequence) noexcept {
        TextMessage::setSequence(sequence);

        return *this;
    }

    /**
     * Returns text.
     *
     * @return text.
     */
    const std::string &getText() const & {
        return text_;
    }

    /**
     * Changes text.
     *
     * @param text text.
     */
    void setText(std::string text) {
        // TODO: check invalid utf-8 [EN-8233]
        text_ = std::move(text);
    }

    /**
     * Changes text and returns the current message.
     *
     * @param text text.
     *
     * @return The current message.
     */
    TextMessage &withText(std::string text) noexcept {
        TextMessage::setText(std::move(text));

        return *this;
    }

    std::string toString() const override;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
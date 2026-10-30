// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../exceptions/InvalidArgumentException.hpp"
#include "../../internal/Common.hpp"
#include "../EventType.hpp"
#include "../EventTypeEnum.hpp"
#include "../LastingEvent.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

/**
 * \addtogroup dxfcpp_misc
 * @{
 */

DXFCPP_BEGIN_NAMESPACE

struct EventMapper;

/**
 * Configuration event with the text payload.
 *
 * <h3>Properties</h3>
 *
 * TextConfiguration event has the following properties:
 *
 * - @ref TextConfiguration::getEventSymbol() "eventSymbol" - symbol of this event;
 * - @ref TextConfiguration::getTime() "time" - time of this text configuration;
 * - @ref TextConfiguration::getSequence() "sequence" - sequence of this text configuration;
 * - @ref TextConfiguration::getVersion() "version" - version;
 * - @ref TextConfiguration::getText() "text" - text payload.
 *
 * <h3>Versioning</h3>
 *
 * The @ref TextConfiguration::getVersion() "version" field is used to track changes to the event data.
 * When the version field is enabled (by default), events with a lower version number than the last received
 * will be dropped. This is useful for conflating events, where only the latest version of an event is processed.
 * This is only true when transferring under a <b>TICKER</b> contract and the version field is enabled.
 *
 * <h3>Implementation details</h3>
 *
 * This event is implemented on top of QDS record `TextConfiguration`.
 */
class DXFCPP_EXPORT TextConfiguration final : public EventTypeWithSymbol<std::string>, public LastingEvent {
    friend struct EventMapper;

    /*
     * The time and sequence are packed into one 64-bit field in the following way:
     *   63..32     31..22         21..0
     * +---------+--------------+----------+
     * | Seconds | Milliseconds | Sequence |
     * +---------+--------------+----------+
     */
    static constexpr std::uint64_t SECONDS_SHIFT = 32ULL;
    static constexpr std::uint64_t MILLISECONDS_SHIFT = 22ULL;
    static constexpr std::uint64_t MILLISECONDS_MASK = 0x3ffULL;

    std::optional<std::string> eventSymbol_{};
    std::int64_t eventTime_{};
    std::int64_t timeSequence_{};
    std::int32_t version_{};
    std::optional<std::string> text_{};

    void fillData(void *graalNative);
    void fillGraalData(void *graalNative) const;
    static void freeGraalData(void *graalNative) noexcept;

    public:
    /**
     * Maximum allowed sequence value.
     *
     * @see TextConfiguration::setSequence()
     */
    static constexpr std::uint32_t MAX_SEQUENCE = (1U << 22U) - 1U;

    /// The alias to a type of shared pointer to the TextConfiguration object
    using Ptr = std::shared_ptr<TextConfiguration>;

    /// The alias to a type of unique pointer to the TextConfiguration object
    using Unique = std::unique_ptr<TextConfiguration>;

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

    /**
     * Creates a new text configuration event with default values.
     */
    TextConfiguration() noexcept;

    /**
     * Creates a new text configuration event with the specified event symbol.
     *
     * @param eventSymbol The event symbol.
     */
    explicit TextConfiguration(const StringLike &eventSymbol) noexcept;

    /**
     * Creates a new text configuration event with the specified event symbol and text.
     *
     * @param eventSymbol The event symbol.
     * @param text The text.
     */
    TextConfiguration(const StringLike &eventSymbol, const StringLike &text) noexcept;

    /**
     * Creates a new text configuration event with the specified event symbol, text, and version.
     *
     * @param eventSymbol The event symbol.
     * @param text The text.
     * @param version The version.
     */
    TextConfiguration(const StringLike &eventSymbol, const StringLike &text, std::int32_t version) noexcept;

    /**
     * Returns the symbol of this event.
     *
     * @return The symbol of this event or dxfcpp::String::NUL (`std::string{"<null>"}`).
     */
    const std::string &getEventSymbol() const & noexcept override;

    /**
     * Returns the symbol of this event.
     *
     * @return The symbol of this event or `std::nullopt`.
     */
    const std::optional<std::string> &getEventSymbolOpt() const & noexcept override;

    /**
     * Changes the symbol of this event.
     *
     * @param eventSymbol The symbol of this event.
     */
    void setEventSymbol(const StringLike &eventSymbol) noexcept override;

    /**
     * Changes the event's symbol and returns the current text configuration event.
     *
     * @param eventSymbol The symbol of this event.
     * @return The current text configuration event.
     */
    TextConfiguration &withEventSymbol(const StringLike &eventSymbol) noexcept;

    ///
    std::int64_t getEventTime() const noexcept override;

    ///
    void setEventTime(std::int64_t eventTime) noexcept override;

    /**
     * Changes the event's creation time and returns the current text configuration event.
     *
     * @param eventTime The difference, measured in milliseconds, between the event creation time and
     * midnight, January 1, 1970 UTC.
     * @return The current text configuration event.
     */
    TextConfiguration &withEventTime(std::int64_t eventTime) noexcept;

    /**
     * Returns time and sequence of this event packaged into a single long value.
     *
     * @return The time and sequence of this text configuration event.
     */
    std::int64_t getTimeSequence() const noexcept;

    /**
     * Changes time and sequence of this event.
     * <b>Do not use this method directly.</b>
     * Change @ref TextConfiguration::setTime() "time" and/or @ref TextConfiguration::setSequence() "sequence".
     *
     * @param timeSequence The time and sequence.
     * @see TextConfiguration::getTimeSequence()
     */
    void setTimeSequence(std::int64_t timeSequence) noexcept;

    /**
     * Returns time of this text configuration event.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @return The time of this text configuration event.
     */
    std::int64_t getTime() const noexcept;

    /**
     * Changes time of this text configuration event.
     * Time is measured in milliseconds between the current time and midnight, January 1, 1970 UTC.
     *
     * @param time The time of this text configuration event.
     */
    void setTime(std::int64_t time) noexcept;

    /**
     * Changes time and returns the current text configuration event.
     *
     * @param time The time of this text configuration event.
     * @return The current text configuration event.
     */
    TextConfiguration &withTime(std::int64_t time) noexcept;

    /**
     * Returns the sequence number of this event to distinguish text configuration events that have the same
     * @ref TextConfiguration::getTime() "time". This sequence number does not have to be unique and does not need to
     * be sequential. Sequence can range from 0 to TextConfiguration::MAX_SEQUENCE.
     *
     * @return The sequence of this text configuration event.
     */
    std::int32_t getSequence() const noexcept;

    /**
     * Changes the @ref TextConfiguration::getSequence() "sequence number" of this text configuration event.
     *
     * @param sequence The sequence.
     * @throws InvalidArgumentException if the sequence is below zero or above TextConfiguration::MAX_SEQUENCE.
     * @see TextConfiguration::getSequence()
     */
    void setSequence(std::int32_t sequence);

    /**
     * Changes the event's sequence number and returns the current text configuration event.
     *
     * @param sequence The sequence.
     * @return The current text configuration event.
     * @throws InvalidArgumentException if the sequence is below zero or above TextConfiguration::MAX_SEQUENCE.
     */
    TextConfiguration &withSequence(std::int32_t sequence);

    /**
     * Returns the version of this event.
     *
     * @return The version of this event. A higher value indicates a more recent update.
     */
    std::int32_t getVersion() const noexcept;

    /**
     * Changes the version of this event.
     *
     * @param version The version of this event. A higher value indicates a more recent update.
     */
    void setVersion(std::int32_t version) noexcept;

    /**
     * Changes the version and returns the current text configuration event.
     *
     * @param version The version of this event. A higher value indicates a more recent update.
     * @return The current text configuration event.
     */
    TextConfiguration &withVersion(std::int32_t version) noexcept;

    /**
     * Returns the text.
     *
     * @return The text or dxfcpp::String::NUL (`std::string{"<null>"}`).
     */
    const std::string &getText() const & noexcept;

    /**
     * Returns the text.
     *
     * @return The text or `std::nullopt`.
     */
    const std::optional<std::string> &getTextOpt() const & noexcept;

    /**
     * Changes the text.
     *
     * @param text The text.
     */
    void setText(const StringLike &text) noexcept;

    /**
     * Changes the text, or makes it null with `std::nullopt`.
     * `setText(other.getTextOpt())` copies it from another event, null included.
     *
     * @param text the text or `std::nullopt`.
     */
    template <OptionalString S> void setText(S &&text) noexcept {
        text_ = std::forward<S>(text);
    }

    /**
     * Changes the text and returns the current text configuration event.
     *
     * @param text The text.
     * @return The current text configuration event.
     */
    TextConfiguration &withText(const StringLike &text) noexcept;

    /**
     * Changes the text, or makes it null with `std::nullopt`.
     * Returns the current text configuration.
     *
     * @param text the text or `std::nullopt`.
     * @return The current text configuration.
     */
    template <OptionalString S> TextConfiguration &withText(S &&text) noexcept {
        setText(std::forward<S>(text));

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

/// @}

DXFCXX_DISABLE_MSC_WARNINGS_POP()

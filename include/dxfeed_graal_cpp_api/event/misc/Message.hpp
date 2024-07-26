// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../../internal/Common.hpp"
#include "../EventTypeEnum.hpp"

DXFCPP_BEGIN_NAMESPACE

struct EventMapper;

/**
 * Message event with application-specific attachment. Messages are never conflated and are delivered to
 * all connected subscribers. There is no built-in persistence for messages. They are lost when subscribers
 * are not connected to the message publisher, so they shall be only used for notification purposes in
 * addition to persistence mechanism.
 *
 * <h3>Implementation details</h3>
 *
 * This event is implemented on top of QDS record `Message`.
 */
class DXFCPP_EXPORT Message : public EventTypeWithSymbol<std::string> {
    friend struct EventMapper;

    std::optional<std::string> eventSymbol_{};
    std::int64_t eventTime_{};
    std::optional<std::string> attachment_{};

    void fillData(void *graalNative);
    void fillGraalData(void *graalNative) const;
    static void freeGraalData(void *graalNative) noexcept;

  public:
    /// The alias to a type of shared pointer to the Message object
    using Ptr = std::shared_ptr<Message>;

    /// The alias to a type of unique pointer to the Message object
    using Unique = std::unique_ptr<Message>;

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
    Message() noexcept = default;

    /**
     * Creates new message with the specified event symbol.
     * @param eventSymbol event symbol.
     */
    explicit Message(std::string eventSymbol) noexcept : eventSymbol_{std::move(eventSymbol)} {
    }

    /**
     * Creates new message with the specified event symbol and attachment.
     * @param eventSymbol event symbol.
     * @param attachment attachment.
     */
    Message(std::string eventSymbol, std::string attachment) noexcept
        : eventSymbol_{std::move(eventSymbol)}, attachment_{std::move(attachment)} {
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
    Message &withEventSymbol(const std::string &eventSymbol) noexcept {
        Message::setEventSymbol(eventSymbol);

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
    Message &withEventTime(std::int64_t eventTime) noexcept {
        Message::setEventTime(eventTime);

        return *this;
    }

    /**
     * Returns attachment.
     *
     * @return attachment.
     */
    const std::string &getAttachment() const & {
        if (!attachment_) {
            return dxfcpp::String::NUL;
        }

        return attachment_.value();
    }

    /**
     * Returns attachment of this event.
     *
     * @return attachment of this event or `std::nullopt`.
     */
    const std::optional<std::string> &getAttachmentOpt() const & noexcept {
        return attachment_;
    }

    /**
     * Changes attachment.
     *
     * @param attachment attachment.
     */
    void setAttachment(std::string attachment) {
        attachment_ = std::move(attachment);
    }

    /**
     * Changes attachment. and returns the current message.
     *
     * @param attachment attachment.
     *
     * @return The current message.
     */
    Message &withAttachment(std::string attachment) noexcept {
        Message::setAttachment(std::move(attachment));

        return *this;
    }

    std::string toString() const override;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
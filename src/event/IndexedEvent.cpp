// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/event/IndexedEvent.hpp"

DXFCPP_BEGIN_NAMESPACE

EventFlag::EventFlag(std::uint32_t flag, std::string name) noexcept : flag_{flag}, name_{std::move(name)} {
}

const EventFlag EventFlag::TX_PENDING{0x01u, "TX_PENDING"};
const EventFlag EventFlag::REMOVE_EVENT{0x02u, "REMOVE_EVENT"};
const EventFlag EventFlag::SNAPSHOT_BEGIN{0x04u, "SNAPSHOT_BEGIN"};
const EventFlag EventFlag::SNAPSHOT_END{0x08u, "SNAPSHOT_END"};
const EventFlag EventFlag::SNAPSHOT_SNIP{0x10u, "SNAPSHOT_SNIP"};
// 0x20 is reserved. This flag will fit into 1-byte on the wire in QTP protocol
const EventFlag EventFlag::SNAPSHOT_MODE{0x40u, "SNAPSHOT_MODE"};
const EventFlag EventFlag::REMOVE_SYMBOL{0x80u, "REMOVE_SYMBOL"};

EventFlag::EventFlag() noexcept : flag_{static_cast<unsigned>(-1)}, name_{"INVALID"} {
}

std::uint32_t EventFlag::getFlag() const noexcept {
    return flag_;
}

bool EventFlag::in(std::uint32_t eventFlagsMask) const noexcept {
    return (eventFlagsMask & flag_) != 0;
}

const std::string &EventFlag::getName() const & noexcept {
    return name_;
}

const std::string &EventFlag::toString() const & noexcept {
    return name_;
}

EventFlagsMask::EventFlagsMask() noexcept : mask_{0u} {
}

EventFlagsMask::EventFlagsMask(std::initializer_list<EventFlag> eventFlags) noexcept
    : EventFlagsMask(eventFlags.begin(), eventFlags.end()) {
}

bool EventFlagsMask::contains(const EventFlag &flag) const noexcept {
    return (mask_ & flag.getFlag()) != 0;
}

std::string EventFlagsMask::toString() const {
    bool addOrSign = false;
    std::ostringstream result{};

    for (const auto &flag :
         {EventFlag::TX_PENDING, EventFlag::REMOVE_EVENT, EventFlag::SNAPSHOT_BEGIN, EventFlag::SNAPSHOT_END,
          EventFlag::SNAPSHOT_SNIP, EventFlag::SNAPSHOT_MODE, EventFlag::REMOVE_SYMBOL}) {
        if (flag.in(mask_)) {
            if (addOrSign) {
                result << "|";
            } else {
                addOrSign = true;
            }

            result << flag.toString();
        }
    }

    result << "(0x" << std::hex << mask_ << ")";

    return result.str();
}

IndexedEvent::~IndexedEvent() {
}

const EventFlag IndexedEvent::TX_PENDING = EventFlag::TX_PENDING;
const EventFlag IndexedEvent::REMOVE_EVENT = EventFlag::REMOVE_EVENT;
const EventFlag IndexedEvent::SNAPSHOT_BEGIN = EventFlag::SNAPSHOT_BEGIN;
const EventFlag IndexedEvent::SNAPSHOT_END = EventFlag::SNAPSHOT_END;
const EventFlag IndexedEvent::SNAPSHOT_SNIP = EventFlag::SNAPSHOT_SNIP;
const EventFlag IndexedEvent::SNAPSHOT_MODE = EventFlag::SNAPSHOT_MODE;

DXFCPP_END_NAMESPACE
// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/event/IndexedEvent.hpp"

DXFCPP_BEGIN_NAMESPACE

const EventFlag EventFlag::TX_PENDING{0x01u, "TX_PENDING"};
const EventFlag EventFlag::REMOVE_EVENT{0x02u, "REMOVE_EVENT"};
const EventFlag EventFlag::SNAPSHOT_BEGIN{0x04u, "SNAPSHOT_BEGIN"};
const EventFlag EventFlag::SNAPSHOT_END{0x08u, "SNAPSHOT_END"};
const EventFlag EventFlag::SNAPSHOT_SNIP{0x10u, "SNAPSHOT_SNIP"};
// 0x20 is reserved. This flag will fit into 1-byte on the wire in QTP protocol
const EventFlag EventFlag::SNAPSHOT_MODE{0x40u, "SNAPSHOT_MODE"};
const EventFlag EventFlag::REMOVE_SYMBOL{0x80u, "REMOVE_SYMBOL"};

IndexedEvent::~IndexedEvent() {
}

const EventFlag IndexedEvent::TX_PENDING = EventFlag::TX_PENDING;
const EventFlag IndexedEvent::REMOVE_EVENT = EventFlag::REMOVE_EVENT;
const EventFlag IndexedEvent::SNAPSHOT_BEGIN = EventFlag::SNAPSHOT_BEGIN;
const EventFlag IndexedEvent::SNAPSHOT_END = EventFlag::SNAPSHOT_END;
const EventFlag IndexedEvent::SNAPSHOT_SNIP = EventFlag::SNAPSHOT_SNIP;
const EventFlag IndexedEvent::SNAPSHOT_MODE = EventFlag::SNAPSHOT_MODE;

DXFCPP_END_NAMESPACE
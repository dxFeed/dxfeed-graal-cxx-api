// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// The native list of DXPublisher::publishEvents in one arena (EventMapper::toGraalListInArenaUnique) against the list of
// separate allocations (EventMapper::toGraalListUnique). A leak of a list or an event is reported by LeakSanitizer in
// the sanitizer builds.

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include "../../src/internal/utils/GraalArena.hpp"

#include <dxfg_v4_api.h>

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

using namespace dxfcpp;

namespace {

std::vector<std::shared_ptr<EventType>> mixedEvents() {
    auto trade = std::make_shared<Trade>("AAPL");
    trade->setPrice(101.5);
    trade->setSize(7);

    auto sale = std::make_shared<TimeAndSale>("IBM");
    sale->setPrice(55.25);
    sale->setBuyer("BUYER");
    sale->setSeller("SELLER");
    sale->setExchangeSaleConditions("ABC");

    auto order = std::make_shared<Order>("MSFT");
    order->setMarketMaker("NSDQ");
    order->setPrice(12.5);

    auto profile = std::make_shared<Profile>("GOOG");
    profile->setDescription("Alphabet Inc.");
    profile->setStatusReason("reason");

    auto quote = std::make_shared<Quote>("TSLA");
    quote->setBidPrice(1.25);
    quote->setAskPrice(1.5);

    auto text = std::make_shared<TextMessage>("NEWS", "text of the message");
    auto message = std::make_shared<Message>("MSG", "attachment");
    auto candle = std::make_shared<Candle>(CandleSymbol::valueOf("AAPL{=d}"));
    candle->setClose(3.5);

    auto unsetSymbol = std::make_shared<Trade>();

    return {trade, sale, order, profile, quote, text, message, candle, unsetSymbol};
}

std::vector<std::string> stringsOf(const std::vector<std::shared_ptr<EventType>> &events) {
    std::vector<std::string> result;

    for (const auto &event : events) {
        result.push_back(event->toString());
    }

    return result;
}

// An event type of its own: its structure is allocated with new, not in the arena of the list.
struct ForeignTrade final : EventType {
    std::string symbol;

    explicit ForeignTrade(std::string s) : symbol(std::move(s)) {
    }

    void *toGraal() const override {
        auto *graal = new dxfg_v4_trade_t{};

        graal->trade_base.market_event.event_type.clazz = DXFG_EVENT_TRADE;

        auto *s = new char[symbol.size() + 1];

        std::memcpy(s, symbol.c_str(), symbol.size() + 1);
        graal->trade_base.market_event.event_symbol = s;

        return graal;
    }
};

// A subclass of a library event with a conversion of its own: its structure comes from new, its strings from the
// library (Trade::fillGraalData), and freeGraal frees both with delete.
struct HeapTrade final : Trade {
    using Trade::Trade;

    void *toGraal() const override {
        auto *graal = new dxfg_v4_trade_t{};

        fillGraalData(graal);

        return graal;
    }
};

struct NullConversion final : EventType {
    void *toGraal() const override {
        return nullptr;
    }
};

// Converts lists of its own inside its conversion: the list of an arena and the list of separate allocations.
struct NestedConversion final : EventType {
    void *toGraal() const override {
        const std::vector<std::shared_ptr<EventType>> inner{std::make_shared<Trade>("INNER"),
                                                            std::make_shared<HeapTrade>("INNER_HEAP")};

        CHECK(GraalArena::current() == nullptr);

        const auto inArena = EventMapper::toGraalListInArenaUnique(inner.begin(), inner.end());
        const auto separate = EventMapper::toGraalListUnique(inner.begin(), inner.end());

        CHECK(stringsOf(EventMapper::fromGraalList(inArena.get())) == stringsOf(inner));
        CHECK(stringsOf(EventMapper::fromGraalList(separate.get())) == stringsOf(inner));

        return ForeignTrade("NESTED").toGraal();
    }
};

} // namespace

TEST_CASE("EventMapper: the list in an arena holds the same events as the list of separate allocations") {
    const auto events = mixedEvents();
    const auto separate = EventMapper::toGraalListUnique(events.begin(), events.end());
    const auto arena = EventMapper::toGraalListInArenaUnique(events.begin(), events.end());

    REQUIRE(arena != nullptr);
    CHECK(static_cast<dxfg_v4_event_type_list *>(arena.get())->size == static_cast<std::int32_t>(events.size()));

    const auto fromSeparate = stringsOf(EventMapper::fromGraalList(separate.get()));
    const auto fromArena = stringsOf(EventMapper::fromGraalList(arena.get()));

    CHECK(fromArena == fromSeparate);
    CHECK(fromArena == stringsOf(events));
}

TEST_CASE("EventMapper: the list in an arena takes events by value and by pointer") {
    std::vector<Trade> values{Trade{"A"}, Trade{"B"}};
    std::vector<Trade *> pointers{&values[0], &values[1]};

    const auto byValue = EventMapper::toGraalListInArenaUnique(values.begin(), values.end());
    const auto byPointer = EventMapper::toGraalListInArenaUnique(pointers.begin(), pointers.end());

    CHECK(stringsOf(EventMapper::fromGraalList(byValue.get())) ==
          stringsOf(EventMapper::fromGraalList(byPointer.get())));
    CHECK(EventMapper::fromGraalList(byValue.get()).at(1)->toString() == values[1].toString());
}

TEST_CASE("EventMapper: an empty list in an arena") {
    const std::vector<std::shared_ptr<EventType>> events{};
    const auto list = EventMapper::toGraalListInArenaUnique(events.begin(), events.end());

    REQUIRE(list != nullptr);
    CHECK(static_cast<dxfg_v4_event_type_list *>(list.get())->size == 0);
    CHECK(EventMapper::fromGraalList(list.get()).empty());
}

TEST_CASE("EventMapper: a list in an arena larger than a block, and a string larger than a block") {
    std::vector<std::shared_ptr<EventType>> events;
    const std::string field(100, 'x');

    for (int i = 0; i < 2000; i++) {
        auto sale = std::make_shared<TimeAndSale>("S" + std::to_string(i));

        sale->setBuyer(field);
        sale->setSeller(field);
        events.push_back(sale);
    }

    events.push_back(std::make_shared<TextMessage>("BIG", std::string(3 * GraalArena::BLOCK_CAPACITY, 'y')));

    const auto list = EventMapper::toGraalListInArenaUnique(events.begin(), events.end());

    CHECK(stringsOf(EventMapper::fromGraalList(list.get())) == stringsOf(events));
}

TEST_CASE("EventMapper: a list in an arena whose conversion throws leaves nothing allocated") {
    auto events = mixedEvents();
    auto bad = std::make_shared<TimeAndSale>("BAD");

    bad->setBuyer("ok");
    bad->setSeller("a\xFF");
    events.insert(events.begin() + 3, bad);
    events.push_back(std::make_shared<ForeignTrade>("FOREIGN"));

    CHECK_THROWS_AS(EventMapper::toGraalListInArenaUnique(events.begin(), events.end()), InvalidArgumentException);

    // An event of its own converted before the one that throws is freed too.
    events.insert(events.begin(), std::make_shared<ForeignTrade>("FIRST"));

    CHECK_THROWS_AS(EventMapper::toGraalListInArenaUnique(events.begin(), events.end()), InvalidArgumentException);
    CHECK(GraalArena::current() == nullptr);
}

TEST_CASE("EventMapper: a null event or a null conversion in a list in an arena throws") {
    const std::vector<std::shared_ptr<EventType>> nullEvent{std::make_shared<Trade>("A"), nullptr};
    const std::vector<std::shared_ptr<EventType>> nullConversion{std::make_shared<Trade>("A"),
                                                                 std::make_shared<NullConversion>()};

    CHECK_THROWS_AS(EventMapper::toGraalListInArenaUnique(nullEvent.begin(), nullEvent.end()),
                    InvalidArgumentException);
    CHECK_THROWS_AS(EventMapper::toGraalListInArenaUnique(nullConversion.begin(), nullConversion.end()),
                    InvalidArgumentException);
    CHECK(GraalArena::current() == nullptr);
}

TEST_CASE("EventMapper: a subclass of a library event with its own conversion is converted on the heap") {
    const std::vector<std::shared_ptr<EventType>> events{std::make_shared<Trade>("A"),
                                                         std::make_shared<HeapTrade>("HEAP"),
                                                         std::make_shared<Trade>("B")};
    const auto list = EventMapper::toGraalListInArenaUnique(events.begin(), events.end());

    CHECK(stringsOf(EventMapper::fromGraalList(list.get())) == stringsOf(events));
}

TEST_CASE("EventMapper: a conversion in a list in an arena may convert lists of its own") {
    const std::vector<std::shared_ptr<EventType>> events{std::make_shared<Trade>("A"),
                                                         std::make_shared<NestedConversion>(),
                                                         std::make_shared<Trade>("B")};
    const auto list = EventMapper::toGraalListInArenaUnique(events.begin(), events.end());
    const auto restored = EventMapper::fromGraalList(list.get());

    REQUIRE(restored.size() == 3);
    CHECK(restored[1]->sharedAs<Trade>()->getEventSymbol() == "NESTED");
    CHECK(GraalArena::current() == nullptr);
}

TEST_CASE("EventMapper: an event outside the arena is kept in the list and freed on its own") {
    const std::vector<std::shared_ptr<EventType>> events{std::make_shared<Trade>("A"),
                                                         std::make_shared<ForeignTrade>("FOREIGN"),
                                                         std::make_shared<Trade>("B")};
    const auto list = EventMapper::toGraalListInArenaUnique(events.begin(), events.end());
    const auto restored = EventMapper::fromGraalList(list.get());

    REQUIRE(restored.size() == 3);
    CHECK(restored[1]->sharedAs<Trade>()->getEventSymbol() == "FOREIGN");
    CHECK(restored[2]->sharedAs<Trade>()->getEventSymbol() == "B");
}

TEST_CASE("GraalArena: allocations, blocks and the scope") {
    auto *arena = GraalArena::create(1024);

    CHECK(arena->blockCount() == 1);
    CHECK(GraalArena::current() == nullptr);

    auto *small = arena->allocate(24, 8);

    CHECK(reinterpret_cast<std::uintptr_t>(small) % 8 == 0);
    CHECK(arena->owns(small));
    CHECK(arena->owns(static_cast<char *>(small) + 23));

    // Does not fit into the first block: a new block of the default capacity.
    auto *next = arena->allocate(2048, 1);

    CHECK(arena->blockCount() == 2);
    CHECK(arena->owns(next));

    // An odd offset, then an alignment beyond alignof(std::max_align_t) in the same block.
    arena->allocate(1, 1);

    auto *aligned = arena->allocate(8, 64);

    CHECK(arena->blockCount() == 2);
    CHECK(arena->owns(aligned));
    CHECK(reinterpret_cast<std::uintptr_t>(aligned) % 64 == 0);

    // Larger than a block: a block of its own size, aligned beyond alignof(std::max_align_t).
    auto *large = arena->allocate(GraalArena::BLOCK_CAPACITY + 1, 64);

    CHECK(arena->blockCount() == 3);
    CHECK(arena->owns(large));
    CHECK(arena->owns(static_cast<char *>(large) + GraalArena::BLOCK_CAPACITY));
    CHECK(reinterpret_cast<std::uintptr_t>(large) % 64 == 0);

    const int outside = 0;

    CHECK_FALSE(arena->owns(&outside));

    auto *other = GraalArena::create(1024);

    {
        const GraalArena::Scope scope(arena);

        CHECK(GraalArena::current() == arena);

        {
            const GraalArena::Scope inner(other);

            CHECK(GraalArena::current() == other);
        }

        CHECK(GraalArena::current() == arena);
    }

    CHECK(GraalArena::current() == nullptr);

    GraalArena::destroy(other);
    GraalArena::destroy(arena);
    GraalArena::destroy(nullptr);
}

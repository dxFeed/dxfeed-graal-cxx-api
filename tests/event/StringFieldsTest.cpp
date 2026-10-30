// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

// Null and the text "<null>" in the string fields of events on the way to the native SDK structures and back. As in
// Java, "<null>" is an ordinary text: only an unset (null) field is passed as a null pointer.

#include <doctest.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <dxfg_api.h>

#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

using namespace dxfcpp;

namespace {

std::optional<std::string> textOf(const char *s) {
    return s == nullptr ? std::nullopt : std::optional<std::string>{s};
}

// Sets a string field of a new event with every kind of argument and copies it to another event with
// set(copy, get(event)). The values are short enough for the market maker identifier of MarketMaker.
template <typename Event, typename Set, typename Get> void checkStringField(Set set, Get get) {
    Event event{};
    Event copy{};
    const std::optional<std::string> null{};

    set(event, "LIT");
    CHECK(get(event) == "LIT");
    set(event, std::string{"STR"});
    CHECK(get(event) == "STR");
    set(event, std::string_view{"VIEW"});
    CHECK(get(event) == "VIEW");
    set(event, std::optional<std::string>{"OPT"});
    CHECK(get(event) == "OPT");
    set(copy, get(event));
    CHECK(get(copy) == "OPT");
    set(event, std::nullopt);
    CHECK(get(event) == std::nullopt);
    set(copy, get(event));
    CHECK(get(copy) == std::nullopt);
    set(event, "LIT");
    set(event, null);
    CHECK(get(event) == std::nullopt);
}

// Passes an event with a null symbol and a null text field and an event with the text "<null>" in both to the SDK
// structure and back.
template <typename Graal, typename Event, typename SetText, typename GraalText, typename GetText>
void checkNullAndNullText(SetText setText, GraalText graalText, GetText getText) {
    Event unset{};
    Event text{"<null>"};

    setText(unset, std::nullopt);
    setText(text, "<null>");

    auto *unsetGraal = static_cast<Graal *>(unset.toGraal());
    auto *textGraal = static_cast<Graal *>(text.toGraal());

    CHECK(textOf(unsetGraal->event_symbol) == std::nullopt);
    CHECK(textOf(graalText(unsetGraal)) == std::nullopt);
    CHECK(textOf(textGraal->event_symbol) == "<null>");
    CHECK(textOf(graalText(textGraal)) == "<null>");

    const auto unsetRestored = Event::fromGraal(unsetGraal);
    const auto textRestored = Event::fromGraal(textGraal);

    Event::freeGraal(unsetGraal);
    Event::freeGraal(textGraal);

    CHECK(unsetRestored->getEventSymbolOpt() == std::nullopt);
    CHECK(getText(*unsetRestored) == std::nullopt);
    CHECK(textRestored->getEventSymbolOpt() == "<null>");
    CHECK(getText(*textRestored) == "<null>");
}

} // namespace

#define DXFC_CHECK_SET(Event, Name)                                                                                    \
    checkStringField<Event>([](auto &e, auto &&v) { e.set##Name(std::forward<decltype(v)>(v)); },                      \
                            [](const auto &e) { return e.get##Name##Opt(); })

#define DXFC_CHECK_WITH(Event, Name)                                                                                   \
    checkStringField<Event>(                                                                                           \
        [](auto &e, auto &&v) {                                                                                        \
            static_assert(std::is_same_v<decltype(e.with##Name(std::forward<decltype(v)>(v))), Event &>);              \
            CHECK(&e.with##Name(std::forward<decltype(v)>(v)) == &e);                                                  \
        },                                                                                                             \
        [](const auto &e) { return e.get##Name##Opt(); })

TEST_CASE("The event symbol is passed to the SDK as null when unset and as text when it is \"<null>\"") {
    SUBCASE("A market event") {
        const Profile noSymbol{};
        const Profile textSymbol{"<null>"};
        auto *noSymbolGraal = static_cast<dxfg_profile_t *>(noSymbol.toGraal());
        auto *textSymbolGraal = static_cast<dxfg_profile_t *>(textSymbol.toGraal());

        CHECK(textOf(noSymbolGraal->market_event.event_symbol) == std::nullopt);
        CHECK(textOf(textSymbolGraal->market_event.event_symbol) == "<null>");
        CHECK(Profile::fromGraal(noSymbolGraal)->getEventSymbolOpt() == std::nullopt);
        CHECK(Profile::fromGraal(textSymbolGraal)->getEventSymbolOpt() == "<null>");

        Profile::freeGraal(noSymbolGraal);
        Profile::freeGraal(textSymbolGraal);
    }

    SUBCASE("A candle") {
        const Candle noSymbol{};
        const Candle textSymbol{CandleSymbol::valueOf("<null>")};
        auto *noSymbolGraal = static_cast<dxfg_candle_t *>(noSymbol.toGraal());
        auto *textSymbolGraal = static_cast<dxfg_candle_t *>(textSymbol.toGraal());

        CHECK(textOf(noSymbolGraal->event_symbol) == std::nullopt);
        CHECK(textOf(textSymbolGraal->event_symbol) == "<null>");
        CHECK(Candle::fromGraal(noSymbolGraal)->getEventSymbolOpt() == std::nullopt);
        CHECK(Candle::fromGraal(textSymbolGraal)->getEventSymbolOpt() == CandleSymbol::valueOf("<null>"));

        Candle::freeGraal(noSymbolGraal);
        Candle::freeGraal(textSymbolGraal);
    }
}

TEST_CASE("A string field with the text \"<null>\" is passed to the SDK and back as text") {
    SUBCASE("Profile") {
        Profile profile{"AAPL"};

        profile.setDescription("<null>");

        auto *graal = static_cast<dxfg_profile_t *>(profile.toGraal());

        CHECK(textOf(graal->description) == "<null>");
        CHECK(textOf(graal->status_reason) == std::nullopt);

        const auto restored = Profile::fromGraal(graal);

        Profile::freeGraal(graal);

        CHECK(restored->getDescriptionOpt() == "<null>");
        CHECK(restored->getStatusReasonOpt() == std::nullopt);
    }

    SUBCASE("TextConfiguration") {
        const TextConfiguration configuration{"AAPL", "<null>"};
        auto *graal = static_cast<dxfg_text_configuration_t *>(configuration.toGraal());

        CHECK(textOf(graal->text) == "<null>");

        const auto restored = TextConfiguration::fromGraal(graal);

        TextConfiguration::freeGraal(graal);

        CHECK(restored->getTextOpt() == "<null>");
    }
}

TEST_CASE("Message, TextMessage and TextConfiguration keep null apart from \"<null>\" in the symbol and the text") {
    SUBCASE("Message") {
        checkNullAndNullText<dxfg_message_t, Message>(
            [](auto &e, auto &&v) { e.setAttachment(std::forward<decltype(v)>(v)); },
            [](const dxfg_message_t *g) { return static_cast<const char *>(g->attachment); },
            [](const auto &e) { return e.getAttachmentOpt(); });
    }

    SUBCASE("TextMessage") {
        checkNullAndNullText<dxfg_text_message_t, TextMessage>(
            [](auto &e, auto &&v) { e.setText(std::forward<decltype(v)>(v)); },
            [](const dxfg_text_message_t *g) { return g->text; }, [](const auto &e) { return e.getTextOpt(); });
    }

    SUBCASE("TextConfiguration") {
        checkNullAndNullText<dxfg_text_configuration_t, TextConfiguration>(
            [](auto &e, auto &&v) { e.setText(std::forward<decltype(v)>(v)); },
            [](const dxfg_text_configuration_t *g) { return g->text; },
            [](const auto &e) { return e.getTextOpt(); });
    }
}

TEST_CASE("The string setters take a string, std::optional<std::string> or std::nullopt") {
    DXFC_CHECK_SET(NuamOrder, ClientOrderId);
    DXFC_CHECK_SET(NuamOrder, CustomerAccount);
    DXFC_CHECK_SET(NuamOrder, CustomerInfo);
    DXFC_CHECK_SET(NuamOrder, ExchangeInfo);
    DXFC_CHECK_SET(Order, MarketMaker);
    DXFC_CHECK_SET(NuamOrder, MarketMaker);
    DXFC_CHECK_SET(MarketMaker, MarketMaker);
    DXFC_CHECK_SET(OptionSale, ExchangeSaleConditions);
    DXFC_CHECK_SET(OptionSale, OptionSymbol);
    DXFC_CHECK_SET(Profile, Description);
    DXFC_CHECK_SET(Profile, StatusReason);
    DXFC_CHECK_SET(SpreadOrder, SpreadSymbol);
    DXFC_CHECK_SET(TimeAndSale, ExchangeSaleConditions);
    DXFC_CHECK_SET(TimeAndSale, Buyer);
    DXFC_CHECK_SET(TimeAndSale, Seller);
    DXFC_CHECK_SET(Message, Attachment);
    DXFC_CHECK_SET(TextConfiguration, Text);
    DXFC_CHECK_SET(TextMessage, Text);
}

TEST_CASE("The with-methods of the string fields take a string, std::optional<std::string> or std::nullopt") {
    DXFC_CHECK_WITH(NuamOrder, ClientOrderId);
    DXFC_CHECK_WITH(NuamOrder, CustomerAccount);
    DXFC_CHECK_WITH(NuamOrder, CustomerInfo);
    DXFC_CHECK_WITH(NuamOrder, ExchangeInfo);
    DXFC_CHECK_WITH(Order, MarketMaker);
    DXFC_CHECK_WITH(AnalyticOrder, MarketMaker);
    DXFC_CHECK_WITH(OtcMarketsOrder, MarketMaker);
    DXFC_CHECK_WITH(NuamOrder, MarketMaker);
    DXFC_CHECK_WITH(MarketMaker, MarketMaker);
    DXFC_CHECK_WITH(OptionSale, ExchangeSaleConditions);
    DXFC_CHECK_WITH(OptionSale, OptionSymbol);
    DXFC_CHECK_WITH(SpreadOrder, SpreadSymbol);
    DXFC_CHECK_WITH(Message, Attachment);
    DXFC_CHECK_WITH(TextConfiguration, Text);
    DXFC_CHECK_WITH(TextMessage, Text);
}

TEST_CASE("withMarketMaker() of an order subclass sets null through a reference to Order") {
    NuamOrder order{"AAPL"};
    Order &base = order;

    base.setMarketMaker("MM");
    CHECK(&base.withMarketMaker(std::nullopt) == &base);
    CHECK(order.getMarketMakerOpt() == std::nullopt);

    // The virtual overload is still called through the base class.
    CHECK(&base.withMarketMaker("MM") == &base);
    CHECK(order.getMarketMakerOpt() == "MM");
}

TEST_CASE("MarketMaker stores a null market maker as an empty one, as Java does") {
    MarketMaker marketMaker{};

    marketMaker.setExchangeCode('Q');
    marketMaker.setMarketMaker("MM");
    marketMaker.setMarketMaker(std::nullopt);

    CHECK(marketMaker.getMarketMakerOpt() == std::nullopt);
    CHECK(marketMaker.getIndex() == MarketMaker{}.withExchangeCode('Q').withMarketMaker("").getIndex());
}

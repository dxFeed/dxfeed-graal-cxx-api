// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/market/OrderSource.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../../include/dxfeed_graal_cpp_api/internal/Common.hpp"

#include <dxfg_api.h>

#include <array>
#include <bit>
#include <functional>
#include <mutex>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

DXFCPP_BEGIN_NAMESPACE

constinit const OrderSource OrderSource::COMPOSITE_BID(1, ConstString::Literal{"COMPOSITE_BID"}, 0);
constinit const OrderSource OrderSource::COMPOSITE_ASK(2, ConstString::Literal{"COMPOSITE_ASK"}, 0);
constinit const OrderSource OrderSource::REGIONAL_BID(3, ConstString::Literal{"REGIONAL_BID"}, 0);
constinit const OrderSource OrderSource::REGIONAL_ASK(4, ConstString::Literal{"REGIONAL_ASK"}, 0);
constinit const OrderSource OrderSource::AGGREGATE_BID(5, ConstString::Literal{"AGGREGATE_BID"}, 0);
constinit const OrderSource OrderSource::AGGREGATE_ASK(6, ConstString::Literal{"AGGREGATE_ASK"}, 0);
constinit const OrderSource OrderSource::COMPOSITE(7, ConstString::Literal{"COMPOSITE"}, 0);
constinit const OrderSource OrderSource::REGIONAL(8, ConstString::Literal{"REGIONAL"}, 0);
constinit const OrderSource OrderSource::AGGREGATE(9, ConstString::Literal{"AGGREGATE"}, 0);
constinit const OrderSource OrderSource::DEFAULT(0, ConstString::Literal{"DEFAULT"},
                                                 PUB_ORDER | PUB_ANALYTIC_ORDER | PUB_OTC_MARKETS_ORDER |
                                                     PUB_SPREAD_ORDER | FULL_ORDER_BOOK | PUB_NUAM_ORDER |
                                                     PUB_ORDER_IMBALANCE);

constinit const OrderSource OrderSource::NTV(ConstString::Literal{"NTV"},
                                             PUB_ORDER | FULL_ORDER_BOOK | PUB_ORDER_IMBALANCE);
constinit const OrderSource OrderSource::ntv(ConstString::Literal{"ntv"}, PUB_ORDER);
constinit const OrderSource OrderSource::NFX(ConstString::Literal{"NFX"}, PUB_ORDER);
constinit const OrderSource OrderSource::ESPD(ConstString::Literal{"ESPD"}, PUB_ORDER);
constinit const OrderSource OrderSource::XNFI(ConstString::Literal{"XNFI"}, PUB_ORDER);
constinit const OrderSource OrderSource::ICE(ConstString::Literal{"ICE"}, PUB_ORDER);
constinit const OrderSource OrderSource::ISE(ConstString::Literal{"ISE"}, PUB_ORDER | PUB_SPREAD_ORDER);
constinit const OrderSource OrderSource::DEA(ConstString::Literal{"DEA"}, PUB_ORDER);
constinit const OrderSource OrderSource::DEX(ConstString::Literal{"DEX"}, PUB_ORDER);
constinit const OrderSource OrderSource::dex(ConstString::Literal{"dex"}, PUB_ORDER);
constinit const OrderSource OrderSource::BYX(ConstString::Literal{"BYX"}, PUB_ORDER);
constinit const OrderSource OrderSource::BZX(ConstString::Literal{"BZX"}, PUB_ORDER);
constinit const OrderSource OrderSource::bzx(ConstString::Literal{"bzx"}, PUB_ORDER);
constinit const OrderSource OrderSource::BATE(ConstString::Literal{"BATE"}, PUB_ORDER);
constinit const OrderSource OrderSource::CHIX(ConstString::Literal{"CHIX"}, PUB_ORDER);
constinit const OrderSource OrderSource::CEUX(ConstString::Literal{"CEUX"}, PUB_ORDER);
constinit const OrderSource OrderSource::BXTR(ConstString::Literal{"BXTR"}, PUB_ORDER);
constinit const OrderSource OrderSource::IST(ConstString::Literal{"IST"}, PUB_ORDER);
constinit const OrderSource OrderSource::BI20(ConstString::Literal{"BI20"}, PUB_ORDER);
constinit const OrderSource OrderSource::ABE(ConstString::Literal{"ABE"}, PUB_ORDER);
constinit const OrderSource OrderSource::FAIR(ConstString::Literal{"FAIR"}, PUB_ORDER);
constinit const OrderSource OrderSource::GLBX(ConstString::Literal{"GLBX"}, PUB_ORDER | PUB_ANALYTIC_ORDER);
constinit const OrderSource OrderSource::glbx(ConstString::Literal{"glbx"}, PUB_ORDER);
constinit const OrderSource OrderSource::ERIS(ConstString::Literal{"ERIS"}, PUB_ORDER);
constinit const OrderSource OrderSource::XEUR(ConstString::Literal{"XEUR"}, PUB_ORDER);
constinit const OrderSource OrderSource::xeur(ConstString::Literal{"xeur"}, PUB_ORDER);
constinit const OrderSource OrderSource::CFE(ConstString::Literal{"CFE"}, PUB_ORDER);
constinit const OrderSource OrderSource::C2OX(ConstString::Literal{"C2OX"}, PUB_ORDER);
constinit const OrderSource OrderSource::SMFE(ConstString::Literal{"SMFE"}, PUB_ORDER);
constinit const OrderSource OrderSource::smfe(ConstString::Literal{"smfe"}, PUB_ORDER);
constinit const OrderSource OrderSource::iex(ConstString::Literal{"iex"}, PUB_ORDER);
constinit const OrderSource OrderSource::MEMX(ConstString::Literal{"MEMX"}, PUB_ORDER);
constinit const OrderSource OrderSource::memx(ConstString::Literal{"memx"}, PUB_ORDER);
constinit const OrderSource OrderSource::OCEA(ConstString::Literal{"OCEA"}, PUB_ORDER);
constinit const OrderSource OrderSource::ocea(ConstString::Literal{"ocea"}, PUB_ORDER);
constinit const OrderSource OrderSource::pink(ConstString::Literal{"pink"}, PUB_ORDER | PUB_OTC_MARKETS_ORDER);
constinit const OrderSource OrderSource::ARCA(ConstString::Literal{"ARCA"}, PUB_ORDER | PUB_ORDER_IMBALANCE);
constinit const OrderSource OrderSource::arca(ConstString::Literal{"arca"}, PUB_ORDER);
constinit const OrderSource OrderSource::CEDX(ConstString::Literal{"CEDX"}, PUB_ORDER);
constinit const OrderSource OrderSource::cedx(ConstString::Literal{"cedx"}, PUB_ORDER);
constinit const OrderSource OrderSource::IGC(ConstString::Literal{"IGC"}, PUB_ORDER);
constinit const OrderSource OrderSource::igc(ConstString::Literal{"igc"}, PUB_ORDER);
constinit const OrderSource OrderSource::EDX(ConstString::Literal{"EDX"}, PUB_ORDER);
constinit const OrderSource OrderSource::edx(ConstString::Literal{"edx"}, PUB_ORDER);
constinit const OrderSource OrderSource::NUAM(ConstString::Literal{"NUAM"},
                                              PUB_ORDER | FULL_ORDER_BOOK | PUB_NUAM_ORDER | PUB_ORDER_IMBALANCE);
constinit const OrderSource OrderSource::nuam(ConstString::Literal{"nuam"}, PUB_ORDER | PUB_NUAM_ORDER);
constinit const OrderSource OrderSource::MOON(ConstString::Literal{"MOON"}, PUB_ORDER);
constinit const OrderSource OrderSource::moon(ConstString::Literal{"moon"}, PUB_ORDER);
constinit const OrderSource OrderSource::OTCM(ConstString::Literal{"OTCM"}, PUB_ORDER);
constinit const OrderSource OrderSource::otcm(ConstString::Literal{"otcm"}, PUB_ORDER);
constinit const OrderSource OrderSource::BOSS(ConstString::Literal{"BOSS"}, PUB_ORDER);
constinit const OrderSource OrderSource::boss(ConstString::Literal{"boss"}, PUB_ORDER);
constinit const OrderSource OrderSource::NEO(ConstString::Literal{"NEO"}, PUB_ORDER);
constinit const OrderSource OrderSource::neo(ConstString::Literal{"neo"}, PUB_ORDER);

namespace {

constinit const std::array PREDEFINED{
    std::cref(OrderSource::COMPOSITE_BID), std::cref(OrderSource::COMPOSITE_ASK), std::cref(OrderSource::REGIONAL_BID),
    std::cref(OrderSource::REGIONAL_ASK),  std::cref(OrderSource::AGGREGATE_BID), std::cref(OrderSource::AGGREGATE_ASK),
    std::cref(OrderSource::COMPOSITE),     std::cref(OrderSource::REGIONAL),      std::cref(OrderSource::AGGREGATE),
    std::cref(OrderSource::DEFAULT),       std::cref(OrderSource::NTV),           std::cref(OrderSource::ntv),
    std::cref(OrderSource::NFX),           std::cref(OrderSource::ESPD),          std::cref(OrderSource::XNFI),
    std::cref(OrderSource::ICE),           std::cref(OrderSource::ISE),           std::cref(OrderSource::DEA),
    std::cref(OrderSource::DEX),           std::cref(OrderSource::dex),           std::cref(OrderSource::BYX),
    std::cref(OrderSource::BZX),           std::cref(OrderSource::bzx),           std::cref(OrderSource::BATE),
    std::cref(OrderSource::CHIX),          std::cref(OrderSource::CEUX),          std::cref(OrderSource::BXTR),
    std::cref(OrderSource::IST),           std::cref(OrderSource::BI20),          std::cref(OrderSource::ABE),
    std::cref(OrderSource::FAIR),          std::cref(OrderSource::GLBX),          std::cref(OrderSource::glbx),
    std::cref(OrderSource::ERIS),          std::cref(OrderSource::XEUR),          std::cref(OrderSource::xeur),
    std::cref(OrderSource::CFE),           std::cref(OrderSource::C2OX),          std::cref(OrderSource::SMFE),
    std::cref(OrderSource::smfe),          std::cref(OrderSource::iex),           std::cref(OrderSource::MEMX),
    std::cref(OrderSource::memx),          std::cref(OrderSource::OCEA),          std::cref(OrderSource::ocea),
    std::cref(OrderSource::pink),          std::cref(OrderSource::ARCA),          std::cref(OrderSource::arca),
    std::cref(OrderSource::CEDX),          std::cref(OrderSource::cedx),          std::cref(OrderSource::IGC),
    std::cref(OrderSource::igc),           std::cref(OrderSource::EDX),           std::cref(OrderSource::edx),
    std::cref(OrderSource::NUAM),          std::cref(OrderSource::nuam),          std::cref(OrderSource::MOON),
    std::cref(OrderSource::moon),          std::cref(OrderSource::OTCM),          std::cref(OrderSource::otcm),
    std::cref(OrderSource::BOSS),          std::cref(OrderSource::boss),          std::cref(OrderSource::NEO),
    std::cref(OrderSource::neo),
};

} // namespace

constinit const std::span<const std::reference_wrapper<const OrderSource>> OrderSource::PREDEFINED_{PREDEFINED};

// A nested class has access to composeId(). The keys repeat the ids and the names of the sources: a constinit constant
// is not read in a constant expression (ConstantsTest checks them).
struct OrderSource::PredefinedSources {
    using Map = ConstantMap<std::variant<std::int32_t, std::string_view>, OrderSource>;

    static constexpr auto ENTRIES = Map::sorted({
        {1, COMPOSITE_BID},
        {"COMPOSITE_BID", COMPOSITE_BID},
        {2, COMPOSITE_ASK},
        {"COMPOSITE_ASK", COMPOSITE_ASK},
        {3, REGIONAL_BID},
        {"REGIONAL_BID", REGIONAL_BID},
        {4, REGIONAL_ASK},
        {"REGIONAL_ASK", REGIONAL_ASK},
        {5, AGGREGATE_BID},
        {"AGGREGATE_BID", AGGREGATE_BID},
        {6, AGGREGATE_ASK},
        {"AGGREGATE_ASK", AGGREGATE_ASK},
        {7, COMPOSITE},
        {"COMPOSITE", COMPOSITE},
        {8, REGIONAL},
        {"REGIONAL", REGIONAL},
        {9, AGGREGATE},
        {"AGGREGATE", AGGREGATE},
        {0, DEFAULT},
        {"DEFAULT", DEFAULT},
        {composeId("NTV"), NTV},
        {"NTV", NTV},
        {composeId("ntv"), ntv},
        {"ntv", ntv},
        {composeId("NFX"), NFX},
        {"NFX", NFX},
        {composeId("ESPD"), ESPD},
        {"ESPD", ESPD},
        {composeId("XNFI"), XNFI},
        {"XNFI", XNFI},
        {composeId("ICE"), ICE},
        {"ICE", ICE},
        {composeId("ISE"), ISE},
        {"ISE", ISE},
        {composeId("DEA"), DEA},
        {"DEA", DEA},
        {composeId("DEX"), DEX},
        {"DEX", DEX},
        {composeId("dex"), dex},
        {"dex", dex},
        {composeId("BYX"), BYX},
        {"BYX", BYX},
        {composeId("BZX"), BZX},
        {"BZX", BZX},
        {composeId("bzx"), bzx},
        {"bzx", bzx},
        {composeId("BATE"), BATE},
        {"BATE", BATE},
        {composeId("CHIX"), CHIX},
        {"CHIX", CHIX},
        {composeId("CEUX"), CEUX},
        {"CEUX", CEUX},
        {composeId("BXTR"), BXTR},
        {"BXTR", BXTR},
        {composeId("IST"), IST},
        {"IST", IST},
        {composeId("BI20"), BI20},
        {"BI20", BI20},
        {composeId("ABE"), ABE},
        {"ABE", ABE},
        {composeId("FAIR"), FAIR},
        {"FAIR", FAIR},
        {composeId("GLBX"), GLBX},
        {"GLBX", GLBX},
        {composeId("glbx"), glbx},
        {"glbx", glbx},
        {composeId("ERIS"), ERIS},
        {"ERIS", ERIS},
        {composeId("XEUR"), XEUR},
        {"XEUR", XEUR},
        {composeId("xeur"), xeur},
        {"xeur", xeur},
        {composeId("CFE"), CFE},
        {"CFE", CFE},
        {composeId("C2OX"), C2OX},
        {"C2OX", C2OX},
        {composeId("SMFE"), SMFE},
        {"SMFE", SMFE},
        {composeId("smfe"), smfe},
        {"smfe", smfe},
        {composeId("iex"), iex},
        {"iex", iex},
        {composeId("MEMX"), MEMX},
        {"MEMX", MEMX},
        {composeId("memx"), memx},
        {"memx", memx},
        {composeId("OCEA"), OCEA},
        {"OCEA", OCEA},
        {composeId("ocea"), ocea},
        {"ocea", ocea},
        {composeId("pink"), pink},
        {"pink", pink},
        {composeId("ARCA"), ARCA},
        {"ARCA", ARCA},
        {composeId("arca"), arca},
        {"arca", arca},
        {composeId("CEDX"), CEDX},
        {"CEDX", CEDX},
        {composeId("cedx"), cedx},
        {"cedx", cedx},
        {composeId("IGC"), IGC},
        {"IGC", IGC},
        {composeId("igc"), igc},
        {"igc", igc},
        {composeId("EDX"), EDX},
        {"EDX", EDX},
        {composeId("edx"), edx},
        {"edx", edx},
        {composeId("NUAM"), NUAM},
        {"NUAM", NUAM},
        {composeId("nuam"), nuam},
        {"nuam", nuam},
        {composeId("MOON"), MOON},
        {"MOON", MOON},
        {composeId("moon"), moon},
        {"moon", moon},
        {composeId("OTCM"), OTCM},
        {"OTCM", OTCM},
        {composeId("otcm"), otcm},
        {"otcm", otcm},
        {composeId("BOSS"), BOSS},
        {"BOSS", BOSS},
        {composeId("boss"), boss},
        {"boss", boss},
        {composeId("NEO"), NEO},
        {"NEO", NEO},
        {composeId("neo"), neo},
        {"neo", neo},
    });

    static_assert(ENTRIES.size() == 2 * std::tuple_size_v<decltype(PREDEFINED)>);
};

constinit const OrderSource::PredefinedSources::Map OrderSource::PREDEFINED_SOURCES{PredefinedSources::ENTRIES};

namespace {

// A function-local static: valueOf() is called from the global initializers of other translation units.
struct UserSources {
    std::mutex mutex{};
    std::unordered_map<std::int32_t, OrderSource> sources{};
};

UserSources &userSources() {
    // Destroyed at exit: the user sources must not be used in the destructors of global variables.
    static UserSources instance{};

    return instance;
}

} // namespace

OrderSource::OrderSource(std::int32_t id, const StringLike &name) noexcept : IndexedEventSource(id, name, true) {
}

OrderSource::OrderSource(const OrderSource &other) noexcept
    : IndexedEventSource(other, true), pubFlags_{other.pubFlags_}, builtin_{other.builtin_} {
}

void OrderSource::throwInvalidName(const char *message) {
    throw InvalidArgumentException(message);
}

std::string OrderSource::decodeName(std::int32_t id) {
    if (id == 0) {
        throw InvalidArgumentException("Source name must contain from 1 to 4 characters");
    }

    std::string name(4, '\0');

    std::size_t n = 0;

    for (int i = 24; i >= 0; i -= 8) {
        if (sar(id, i) == 0) { // skip highest contiguous zeros
            continue;
        }

        const char c = static_cast<char>(andOp(sar(id, i), 0xff));

        checkChar(c);
        name[n++] = c;
    }

    return name.substr(0, n);
}

std::uint32_t OrderSource::getEventTypeMask(const EventTypeEnum &eventType) {
    if (eventType == EventTypeEnum::ORDER) {
        return PUB_ORDER;
    }

    if (eventType == EventTypeEnum::ANALYTIC_ORDER) {
        return PUB_ANALYTIC_ORDER;
    }

    if (eventType == EventTypeEnum::OTC_MARKETS_ORDER) {
        return PUB_OTC_MARKETS_ORDER;
    }

    if (eventType == EventTypeEnum::SPREAD_ORDER) {
        return PUB_SPREAD_ORDER;
    }

    if (eventType == EventTypeEnum::ORDER_IMBALANCE) {
        return PUB_ORDER_IMBALANCE;
    }

    if (eventType == EventTypeEnum::NUAM_ORDER) {
        return PUB_NUAM_ORDER;
    }

    throw InvalidArgumentException("Invalid order event type: " + eventType.getName());
}

std::span<const std::reference_wrapper<const OrderSource>> OrderSource::publishableByFlag(std::uint32_t flag) {
    // The lists of the sources with each flag, in the order of their declaration (as in Java).
    static const auto LISTS = [] {
        std::array<std::vector<std::reference_wrapper<const OrderSource>>, FLAGS_SIZE> result{};

        for (const auto &source : PREDEFINED) {
            for (std::uint32_t i = 0; i < FLAGS_SIZE; i++) {
                if ((source.get().pubFlags_ & (1U << i)) != 0) {
                    result[i].push_back(source);
                }
            }
        }

        return result;
    }();

    return LISTS[static_cast<std::size_t>(std::countr_zero(flag))];
}

bool OrderSource::isSpecialSourceId(std::int32_t sourceId) noexcept {
    return sourceId >= COMPOSITE_BID.id() && sourceId <= AGGREGATE.id();
}

const OrderSource &OrderSource::valueOf(std::int32_t sourceId) {
    for (const auto &source : PREDEFINED) {
        if (source.get().id() == sourceId) {
            return source;
        }
    }

    auto &[mutex, sources] = userSources();
    std::lock_guard lock(mutex);

    if (const auto found = sources.find(sourceId); found != sources.end()) {
        return found->second;
    }

    return sources.try_emplace(sourceId, OrderSource(sourceId, decodeName(sourceId))).first->second;
}

const OrderSource &OrderSource::valueOf(const StringLike &name) {
    const std::string_view nameView{name};

    for (const auto &source : PREDEFINED) {
        if (source.get().nameView() == nameView) {
            return source;
        }
    }

    const auto sourceId = composeId(nameView);
    auto &[mutex, sources] = userSources();
    std::lock_guard lock(mutex);

    if (const auto found = sources.find(sourceId); found != sources.end()) {
        return found->second;
    }

    return sources.try_emplace(sourceId, OrderSource(sourceId, name)).first->second;
}

std::span<const std::reference_wrapper<const OrderSource>> OrderSource::publishable(const EventTypeEnum &eventType) {
    return publishableByFlag(getEventTypeMask(eventType));
}

std::span<const std::reference_wrapper<const OrderSource>> OrderSource::fullOrderBook() {
    return publishableByFlag(FULL_ORDER_BOOK);
}

bool OrderSource::isPublishable(const EventTypeEnum &eventType) const {
    return (pubFlags_ & getEventTypeMask(eventType)) != 0;
}

bool OrderSource::isFullOrderBook() const noexcept {
    return (pubFlags_ & FULL_ORDER_BOOK) != 0;
}

DXFCPP_END_NAMESPACE

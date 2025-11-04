// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/market/OrderSource.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"

#include <dxfg_api.h>
#include <type_traits>
#include <unordered_map>
#include <variant>

DXFCPP_BEGIN_NAMESPACE

const OrderSource OrderSource::COMPOSITE_BID(1, "COMPOSITE_BID", 0);
const OrderSource OrderSource::COMPOSITE_ASK(2, "COMPOSITE_ASK", 0);
const OrderSource OrderSource::REGIONAL_BID(3, "REGIONAL_BID", 0);
const OrderSource OrderSource::REGIONAL_ASK(4, "REGIONAL_ASK", 0);
const OrderSource OrderSource::AGGREGATE_BID(5, "AGGREGATE_BID", 0);
const OrderSource OrderSource::AGGREGATE_ASK(6, "AGGREGATE_ASK", 0);
const OrderSource OrderSource::COMPOSITE(7, "COMPOSITE", 0);
const OrderSource OrderSource::REGIONAL(8, "REGIONAL", 0);
const OrderSource OrderSource::AGGREGATE(9, "AGGREGATE", 0);
const OrderSource OrderSource::DEFAULT(0, "DEFAULT",
                                       PUB_ORDER | PUB_ANALYTIC_ORDER | PUB_OTC_MARKETS_ORDER | PUB_SPREAD_ORDER |
                                           FULL_ORDER_BOOK | PUB_NUAM_ORDER);

const OrderSource OrderSource::NTV("NTV", PUB_ORDER | FULL_ORDER_BOOK);
const OrderSource OrderSource::ntv("ntv", PUB_ORDER);
const OrderSource OrderSource::NFX("NFX", PUB_ORDER);
const OrderSource OrderSource::ESPD("ESPD", PUB_ORDER);
const OrderSource OrderSource::XNFI("XNFI", PUB_ORDER);
const OrderSource OrderSource::ICE("ICE", PUB_ORDER);
const OrderSource OrderSource::ISE("ISE", PUB_ORDER | PUB_SPREAD_ORDER);
const OrderSource OrderSource::DEA("DEA", PUB_ORDER);
const OrderSource OrderSource::DEX("DEX", PUB_ORDER);
const OrderSource OrderSource::dex("dex", PUB_ORDER);
const OrderSource OrderSource::BYX("BYX", PUB_ORDER);
const OrderSource OrderSource::BZX("BZX", PUB_ORDER);
const OrderSource OrderSource::bzx("bzx", PUB_ORDER);
const OrderSource OrderSource::BATE("BATE", PUB_ORDER);
const OrderSource OrderSource::CHIX("CHIX", PUB_ORDER);
const OrderSource OrderSource::CEUX("CEUX", PUB_ORDER);
const OrderSource OrderSource::BXTR("BXTR", PUB_ORDER);
const OrderSource OrderSource::IST("IST", PUB_ORDER);
const OrderSource OrderSource::BI20("BI20", PUB_ORDER);
const OrderSource OrderSource::ABE("ABE", PUB_ORDER);
const OrderSource OrderSource::FAIR("FAIR", PUB_ORDER);
const OrderSource OrderSource::GLBX("GLBX", PUB_ORDER | PUB_ANALYTIC_ORDER);
const OrderSource OrderSource::glbx("glbx", PUB_ORDER);
const OrderSource OrderSource::ERIS("ERIS", PUB_ORDER);
const OrderSource OrderSource::XEUR("XEUR", PUB_ORDER);
const OrderSource OrderSource::xeur("xeur", PUB_ORDER);
const OrderSource OrderSource::CFE("CFE", PUB_ORDER);
const OrderSource OrderSource::C2OX("C2OX", PUB_ORDER);
const OrderSource OrderSource::SMFE("SMFE", PUB_ORDER);
const OrderSource OrderSource::smfe("smfe", PUB_ORDER);
const OrderSource OrderSource::iex("iex", PUB_ORDER);
const OrderSource OrderSource::MEMX("MEMX", PUB_ORDER);
const OrderSource OrderSource::memx("memx", PUB_ORDER);
const OrderSource OrderSource::OCEA("OCEA", PUB_ORDER);
const OrderSource OrderSource::ocea("ocea", PUB_ORDER);
const OrderSource OrderSource::pink("pink", PUB_ORDER | PUB_OTC_MARKETS_ORDER);
const OrderSource OrderSource::ARCA("ARCA", PUB_ORDER);
const OrderSource OrderSource::arca("arca", PUB_ORDER);
const OrderSource OrderSource::CEDX("CEDX", PUB_ORDER);
const OrderSource OrderSource::cedx("cedx", PUB_ORDER);
const OrderSource OrderSource::IGC("IGC", PUB_ORDER);
const OrderSource OrderSource::igc("igc", PUB_ORDER);
const OrderSource OrderSource::EDX("EDX", PUB_ORDER);
const OrderSource OrderSource::edx("edx", PUB_ORDER);
const OrderSource OrderSource::NUAM("NUAM", PUB_ORDER | FULL_ORDER_BOOK | PUB_NUAM_ORDER);
const OrderSource OrderSource::nuam("nuam", PUB_ORDER | PUB_NUAM_ORDER);

const std::unordered_map<std::variant<std::int32_t, std::string>, std::reference_wrapper<const OrderSource>>
    OrderSource::PREDEFINED_SOURCES =
        [](std::initializer_list<std::reference_wrapper<const OrderSource>> orders) {
            std::unordered_map<std::variant<std::int32_t, std::string>, std::reference_wrapper<const OrderSource>>
                result{};

            for (auto &&o : orders) {
                result.emplace(o.get().id(), o);
                result.emplace(o.get().name(), o);
            }

            return result;
        }({//
           COMPOSITE_BID, COMPOSITE_ASK, REGIONAL_BID, REGIONAL_ASK, AGGREGATE_BID, AGGREGATE_ASK, COMPOSITE, REGIONAL,
           AGGREGATE,
           //
           DEFAULT,
           //
           NTV, ntv, NFX, ESPD, XNFI, ICE, ISE, DEA, DEX, dex, BYX, BZX, bzx, BATE, CHIX, CEUX, BXTR, IST, BI20, ABE,
           FAIR, GLBX, glbx, ERIS, XEUR, xeur, CFE, C2OX, SMFE, smfe, iex, MEMX, memx, OCEA, ocea, pink, ARCA, arca,
           CEDX, cedx, IGC, igc, EDX, edx, NUAM, nuam});

std::unordered_map<std::int32_t, OrderSource> OrderSource::USER_SOURCES_{};

OrderSource::OrderSource(std::int32_t id, const StringLike &name, std::uint32_t pubFlags) noexcept
    : IndexedEventSource(id, name), pubFlags_{pubFlags}, builtin_{true} {
}

OrderSource::OrderSource(const StringLike &name, std::uint32_t pubFlags)
    : OrderSource(composeId(name), name, pubFlags) {
}

OrderSource::OrderSource(std::int32_t id, const StringLike &name) noexcept : IndexedEventSource(id, name) {
}

std::int32_t OrderSource::composeId(const StringLike &name) {
    std::int32_t sourceId = 0;

    const auto n = name.length();

    if (n == 0 || n > 4) {
        throw InvalidArgumentException("Source name must contain from 1 to 4 characters");
    }

    for (const auto c : name) {
        checkChar(c);
        sourceId = orOp(sal(sourceId, 8), c);
    }

    return sourceId;
}

void OrderSource::checkChar(char c) {
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
        return;
    }

    throw InvalidArgumentException("Source name must contain only alphanumeric characters");
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

    // TODO: MDAPI-243, MDAPI-256
    // if (eventType == EventTypeEnum::NUAM_ORDER) {
    //     return PUB_NUAM_ORDER;
    // }

    throw InvalidArgumentException("Invalid order event type: " + eventType.getName());
}

void *OrderSource::toGraal() const {
    auto *graalSource = new dxfg_indexed_event_source_t{ORDER_SOURCE, id(), createCString(name())};

    return graalSource;
}

std::unique_ptr<void, decltype(&IndexedEventSource::freeGraal)> OrderSource::toGraalUnique() const noexcept {
    return {toGraal(), freeGraal};
}

bool OrderSource::isSpecialSourceId(std::int32_t sourceId) noexcept {
    return sourceId >= COMPOSITE_BID.id() && sourceId <= AGGREGATE.id();
}

const OrderSource &OrderSource::valueOf(std::int32_t sourceId) {
    if (const auto found = PREDEFINED_SOURCES.find(sourceId); found != PREDEFINED_SOURCES.end()) {
        return found->second;
    }

    std::lock_guard lock(MTX_);

    if (const auto found = USER_SOURCES_.find(sourceId); found != USER_SOURCES_.end()) {
        return found->second;
    }

    return USER_SOURCES_.try_emplace(sourceId, OrderSource(sourceId, decodeName(sourceId))).first->second;
}

const OrderSource &OrderSource::valueOf(const StringLike &name) {
    if (const auto found = PREDEFINED_SOURCES.find(name); found != PREDEFINED_SOURCES.end()) {
        return found->second;
    }

    std::lock_guard lock(MTX_);

    const auto sourceId = composeId(name);

    if (const auto found = USER_SOURCES_.find(sourceId); found != USER_SOURCES_.end()) {
        return found->second;
    }

    return USER_SOURCES_.try_emplace(sourceId, OrderSource(sourceId, name)).first->second;
}

DXFCPP_END_NAMESPACE
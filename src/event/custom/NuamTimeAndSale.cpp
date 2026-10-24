// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../../include/dxfeed_graal_cpp_api/event/custom/NuamTimeAndSale.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../../../include/dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp"

#include <dxfg_api.h>
#include <fmt/format.h>
#include <memory>
#include <string>

DXFCPP_BEGIN_NAMESPACE

const EventTypeEnum &NuamTimeAndSale::TYPE = EventTypeEnum::NUAM_TIME_AND_SALE;

void NuamTimeAndSale::fillData(void *graalNative) noexcept {
    if (graalNative == nullptr) {
        return;
    }

    TimeAndSale::fillData(graalNative);

    const auto graalNuamTimeAndSale = static_cast<dxfg_nuam_time_and_sale_t *>(graalNative);

    nuamTimeAndSaleData_ = {.matchId = graalNuamTimeAndSale->match_id};
}

void NuamTimeAndSale::fillGraalData(void *graalNative) const noexcept {
    if (graalNative == nullptr) {
        return;
    }

    TimeAndSale::fillGraalData(graalNative);

    const auto graalNuamTimeAndSale = static_cast<dxfg_nuam_time_and_sale_t *>(graalNative);

    graalNuamTimeAndSale->time_and_sale_base.market_event.event_type.clazz = DXFG_EVENT_NUAM_TIME_AND_SALE;
    graalNuamTimeAndSale->match_id = nuamTimeAndSaleData_.matchId;
}

NuamTimeAndSale::Ptr NuamTimeAndSale::fromGraal(void *graalNative) {
    if (!graalNative) {
        throw InvalidArgumentException("Unable to create NuamTimeAndSale. The `graalNative` parameter is nullptr");
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_NUAM_TIME_AND_SALE) {
        const auto eventType = static_cast<dxfg_event_type_t *>(graalNative)->clazz;

        throw InvalidArgumentException(fmt::format(
            "Unable to create NuamTimeAndSale. Wrong event class {}({})! Expected: {}", isolated::toString(eventType),
            std::to_string(eventType), isolated::toString(DXFG_EVENT_NUAM_TIME_AND_SALE)));
    }

    auto nuamTimeAndSale = std::make_shared<NuamTimeAndSale>();

    nuamTimeAndSale->fillData(graalNative);

    return nuamTimeAndSale;
}

void *NuamTimeAndSale::toGraal() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug(toString() + "::toGraal()");
    }

    auto *graalNuamTimeAndSale = new dxfg_nuam_time_and_sale_t{};

    fillGraalData(graalNuamTimeAndSale);

    return graalNuamTimeAndSale;
}

void NuamTimeAndSale::freeGraal(void *graalNative) {
    if (!graalNative) {
        return;
    }

    if (static_cast<dxfg_event_type_t *>(graalNative)->clazz != DXFG_EVENT_NUAM_TIME_AND_SALE) {
        const auto eventType = static_cast<dxfg_event_type_t *>(graalNative)->clazz;

        throw InvalidArgumentException(
            fmt::format("Unable to free NuamTimeAndSale's Graal data. Wrong event class {}({})! Expected: {}",
                        isolated::toString(eventType), std::to_string(eventType),
                        isolated::toString(DXFG_EVENT_NUAM_TIME_AND_SALE)));
    }

    const auto graalNuamTimeAndSale = static_cast<dxfg_nuam_time_and_sale_t *>(graalNative);

    freeGraalData(graalNative);

    delete graalNuamTimeAndSale;
}

void NuamTimeAndSale::assign(std::shared_ptr<EventType> event) {
    TimeAndSale::assign(event);

    if (const auto other = event->sharedAs<NuamTimeAndSale>(); other) {
        nuamTimeAndSaleData_ = other->nuamTimeAndSaleData_;
    }
}

NuamTimeAndSale::NuamTimeAndSale() noexcept {
}

NuamTimeAndSale::NuamTimeAndSale(const StringLike &eventSymbol) noexcept : TimeAndSale(eventSymbol) {
}

std::int64_t NuamTimeAndSale::getMatchId() const noexcept {
    return nuamTimeAndSaleData_.matchId;
}

void NuamTimeAndSale::setMatchId(std::int64_t matchId) noexcept {
    nuamTimeAndSaleData_.matchId = matchId;
}

std::string NuamTimeAndSale::toString() const {
    return fmt::format("NuamTimeAndSale{{{}, matchId={}}}", baseFieldsToString(), getMatchId());
}

DXFCPP_END_NAMESPACE

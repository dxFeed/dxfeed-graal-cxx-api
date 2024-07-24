// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include "../../internal/Common.hpp"
#include "../EventTypeEnum.hpp"
#include "../IndexedEventSource.hpp"

DXFCPP_BEGIN_NAMESPACE

/**
 * Identifies source of Order, AnalyticOrder, OtcMarketsOrder and SpreadOrder events.
 * There are the following kinds of order sources:
 * <ul>
 * <li><em>Synthetic</em> sources OrderSource::COMPOSITE_BID, OrderSource::COMPOSITE_ASK,
 *     OrderSource::REGIONAL_BID, and OrderSource::REGIONAL_ASK are provided for convenience of a consolidated
 *     order book and are automatically generated based on the corresponding Quote events.
 * <li><em>Aggregate</em> sources OrderSource::AGGREGATE_BID and OrderSource::AGGREGATE_ASK provide
 *     futures depth (aggregated by price level) and NASDAQ Level II (top of book for each market maker).
 *     These source cannot be directly published to via dxFeed API.
 * <li><em>Publishable</em> sources OrderSource::DEFAULT, OrderSource::NTV, OrderSource::ISE etc
 *     support full range of dxFeed API features.
 * </ul>
 */
class DXFCPP_EXPORT OrderSource final : public IndexedEventSource {

    static constexpr std::uint32_t PUB_ORDER = 0x0001U;
    static constexpr std::uint32_t PUB_ANALYTIC_ORDER = 0x0002U;
    static constexpr std::uint32_t PUB_OTC_MARKETS_ORDER = 0x0004;
    static constexpr std::uint32_t PUB_SPREAD_ORDER = 0x0008U;
    static constexpr std::uint32_t FULL_ORDER_BOOK = 0x0010U;

    static constexpr std::uint32_t FLAGS_SIZE = 5U;

  public:
    static const std::unordered_map<std::variant<std::int32_t, std::string>, std::reference_wrapper<const OrderSource>>
        PREDEFINED_SOURCES;

  private:
    static inline std::mutex MTX_{};
    static std::unordered_map<std::int32_t, OrderSource> USER_SOURCES_;

    std::uint32_t pubFlags_{};
    bool builtin_{};

    OrderSource(std::int32_t id, std::string name, std::uint32_t pubFlags) noexcept
        : IndexedEventSource(id, std::move(name)), pubFlags_{pubFlags}, builtin_{true} {
    }

    OrderSource(const std::string &name, std::uint32_t pubFlags) : OrderSource(composeId(name), name, pubFlags) {
    }

    OrderSource(std::int32_t id, const std::string &name) noexcept
        : IndexedEventSource(id, name), pubFlags_{0}, builtin_{false} {
    }

    static std::int32_t composeId(const std::string &name) {
        std::int32_t sourceId = 0;

        auto n = name.length();

        if (n == 0 || n > 4) {
            throw InvalidArgumentException("Source name must contain from 1 to 4 characters");
        }

        for (auto c : name) {
            checkChar(c);
            sourceId = orOp(sal(sourceId, 8), c);
        }

        return sourceId;
    }

    static void checkChar(char c) {
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
            return;
        }

        throw InvalidArgumentException("Source name must contain only alphanumeric characters");
    }

    static std::string decodeName(std::int32_t id) {
        if (id == 0) {
            throw InvalidArgumentException("Source name must contain from 1 to 4 characters");
        }

        std::string name(4, '\0');

        std::size_t n = 0;

        for (int i = 24; i >= 0; i -= 8) {
            if (sar(id, i) == 0) { // skip highest contiguous zeros
                continue;
            }

            char c = static_cast<char>(andOp(sar(id, i), 0xff));

            checkChar(c);
            name[n++] = c;
        }

        return name.substr(0, n);
    }

    static std::uint32_t getEventTypeMask(const EventTypeEnum &eventType) {
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

        throw InvalidArgumentException("Invalid order event type: " + eventType.getName());
    }

    /**
     * Allocates memory for the dxFeed Graal SDK structure (recursively if necessary).
     * Fills the dxFeed Graal SDK structure's fields by the data of the current entity (recursively if necessary).
     * Returns the pointer to the filled structure.
     *
     * @return The pointer to the filled dxFeed Graal SDK structure
     */
    void *toGraal() const override;

  public:
    /**
     * Bid side of a composite Quote.
     * It is a <em>synthetic</em> source.
     * The subscription on composite Quote event is observed when this source is subscribed to.
     */
    static const OrderSource COMPOSITE_BID;

    /**
     * Ask side of a composite Quote.
     * It is a <em>synthetic</em> source.
     * The subscription on composite Quote event is observed when this source is subscribed to.
     */
    static const OrderSource COMPOSITE_ASK;

    /**
     * Bid side of a regional Quote.
     * It is a <em>synthetic</em> source.
     * The subscription on regional Quote event is observed when this source is subscribed to.
     */
    static const OrderSource REGIONAL_BID;

    /**
     * Ask side of a regional Quote.
     * It is a <em>synthetic</em> source.
     * The subscription on regional Quote event is observed when this source is subscribed to.
     */
    static const OrderSource REGIONAL_ASK;

    /**
     * Bid side of an aggregate order book (futures depth and NASDAQ Level II).
     * This source cannot be directly published via dxFeed API, but otherwise it is fully operational.
     */
    static const OrderSource AGGREGATE_BID;

    /**
     * Ask side of an aggregate order book (futures depth and NASDAQ Level II).
     * This source cannot be directly published via dxFeed API, but otherwise it is fully operational.
     */
    static const OrderSource AGGREGATE_ASK;

    /**
     * Default source for publishing custom order books.
     * Order, AnalyticOrder, OtcMarketsOrder and SpreadOrder events are @ref ::isPublishable() "publishable"
     * on this source and the corresponding subscription can be observed via DXPublisher.
     */
    static const OrderSource DEFAULT;

    /**
     * NASDAQ Total View.
     *
     * Order events are @ref ::isPublishable() "publishable" on this source and the corresponding subscription can be
     * observed via DXPublisher.
     */
    static const OrderSource NTV;

    /**
     * NASDAQ Total View. Record for price level book.
     *
     * Order events are @ref ::isPublishable() "publishable" on this source and the corresponding subscription can be
     * observed via DXPublisher.
     */
    static const OrderSource ntv;

    /**
     * NASDAQ Futures Exchange.
     *
     * Order events are @ref ::isPublishable() "publishable" on this source and the corresponding subscription can be
     * observed via DXPublisher.
     */
    static const OrderSource NFX;

    /**
     * NASDAQ eSpeed.
     *
     * Order events are @ref ::isPublishable() "publishable" on this source and the corresponding subscription can be
     * observed via DXPublisher.
     */
    static const OrderSource ESPD;

    /**
     * NASDAQ Fixed Income.
     *
     * Order events are @ref ::isPublishable() "publishable" on this source and the corresponding subscription can be
     * observed via DXPublisher.
     */
    static const OrderSource XNFI;

    /**
     * Intercontinental Exchange.
     *
     * Order events are @ref ::isPublishable() "publishable" on this source and the corresponding subscription can be
     * observed via DXPublisher.
     */
    static const OrderSource ICE;

    /**
     * International Securities Exchange.
     *
     * Order and SpreadOrder events are @ref ::isPublishable() "publishable" on this source and the corresponding
     * subscription can be observed via DXPublisher.
     */
    static const OrderSource ISE;

    /**
     * Direct-Edge EDGA Exchange.
     *
     * Order events are @ref ::isPublishable() "publishable" on this source and the corresponding subscription can be
     * observed via DXPublisher.
     */
    static const OrderSource DEA;

    /**
     * Direct-Edge EDGX Exchange.
     *
     * Order events are @ref ::isPublishable() "publishable" on this source and the corresponding subscription can be
     * observed via DXPublisher.
     */
    static const OrderSource DEX;

    /**
     * Direct-Edge EDGX Exchange. Record for price level book.
     *
     * Order events are @ref ::isPublishable() "publishable" on this source and the corresponding subscription can be
     * observed via DXPublisher.
     */

    static const OrderSource dex;

    /**
     * Bats BYX Exchange.
     *
     * Order events are @ref ::isPublishable() "publishable" on this source and the corresponding subscription can be
     * observed via DXPublisher.
     */
    static const OrderSource BYX;

    /**
     * Bats BZX Exchange.
     *
     * Order events are @ref ::isPublishable() "publishable" on this source and the corresponding subscription can be
     * observed via DXPublisher.
     */
    static const OrderSource BZX;

    /**
     * Bats BZX Exchange. Record for price level book.
     *
     * Order events are @ref ::isPublishable() "publishable" on this source and the corresponding subscription can be
     * observed via DXPublisher.
     */
    static const OrderSource bzx;

    /**
     * Bats Europe BXE Exchange.
     *
     * Order events are @ref ::isPublishable() "publishable" on this source and the corresponding subscription can be
     * observed via DXPublisher.
     */
    static const OrderSource BATE;

    /**
     * Bats Europe CXE Exchange.
     *
     * Order events are @ref ::isPublishable() "publishable" on this source and the corresponding subscription can be
     * observed via DXPublisher.
     */
    static const OrderSource CHIX;

    /**
     * Bats Europe DXE Exchange.
     *
     * Order events are @ref ::isPublishable() "publishable" on this source and the corresponding subscription can be
     * observed via DXPublisher.
     */
    static const OrderSource CEUX;

    /**
     * Bats Europe TRF.
     *
     * Order events are @ref ::isPublishable() "publishable" on this source and the corresponding subscription can be
     * observed via DXPublisher.
     */
    static const OrderSource BXTR;

    /**
     * Borsa Istanbul Exchange.
     *
     * Order events are @ref ::isPublishable() "publishable" on this source and the corresponding subscription can be
     * observed via DXPublisher.
     */
    static const OrderSource IST;

    /**
     * Borsa Istanbul Exchange. Record for particular top 20 order book.
     *
     * Order events are @ref ::isPublishable() "publishable" on this source and the corresponding subscription can be
     * observed via DXPublisher.
     */
    static const OrderSource BI20;

    /**
     * ABE (abe.io) exchange.
     *
     * Order events are @ref ::isPublishable() "publishable" on this source and the corresponding subscription can be
     * observed via DXPublisher.
     */
    static const OrderSource ABE;

    /**
     * FAIR (FairX) exchange.
     *
     * Order events are @ref ::isPublishable() "publishable" on this source and the corresponding subscription can be
     * observed via DXPublisher.
     */
    static const OrderSource FAIR;

    /**
     * CME Globex.
     *
     * Order and AnalyticOrder events are @ref ::isPublishable() "publishable" on this source and the corresponding
     * subscription can be observed via DXPublisher.
     */
    static const OrderSource GLBX;

    /**
     * CME Globex. Record for price level book.
     *
     * Order events are @ref ::isPublishable() "publishable" on this source and the corresponding subscription can be
     * observed via DXPublisher.
     */
    static const OrderSource glbx;

    /**
     * Eris Exchange group of companies.
     *
     * Order events are @ref ::isPublishable() "publishable" on this source and the corresponding subscription can be
     * observed via DXPublisher.
     */
    static const OrderSource ERIS;

    /**
     * Eurex Exchange.
     *
     * Order events are @ref ::isPublishable() "publishable" on this source and the corresponding subscription can be
     * observed via DXPublisher.
     */
    static const OrderSource XEUR;

    /**
     * Eurex Exchange. Record for price level book.
     *
     * Order events are @ref ::isPublishable() "publishable" on this source and the corresponding subscription can be
     * observed via DXPublisher.
     */
    static const OrderSource xeur;

    /**
     * CBOE Futures Exchange.
     *
     * Order events are @ref ::isPublishable() "publishable" on this source and the corresponding subscription can be
     * observed via DXPublisher.
     */
    static const OrderSource CFE;

    /**
     * CBOE Options C2 Exchange.
     *
     * Order events are @ref ::isPublishable() "publishable" on this source and the corresponding subscription can be
     * observed via DXPublisher.
     */
    static const OrderSource C2OX;

    /**
     * Small Exchange.
     *
     * Order events are @ref ::isPublishable() "publishable" on this source and the corresponding subscription can be
     * observed via DXPublisher.
     */
    static const OrderSource SMFE;

    /**
     * Small Exchange. Record for price level book.
     *
     * Order events are @ref ::isPublishable() "publishable" on this source and the corresponding subscription can be
     * observed via DXPublisher.
     */
    static const OrderSource smfe;

    /**
     * Investors exchange. Record for price level book.
     *
     * Order events are @ref ::isPublishable() "publishable" on this source and the corresponding subscription can be
     * observed via DXPublisher.
     */
    static const OrderSource iex;

    /**
     * Members Exchange.
     *
     * Order events are @ref ::isPublishable() "publishable" on this source and the corresponding subscription can be
     * observed via DXPublisher.
     */
    static const OrderSource MEMX;

    /**
     * Members Exchange. Record for price level book.
     *
     * Order events are @ref ::isPublishable() "publishable" on this source and the corresponding subscription can be
     * observed via DXPublisher.
     */
    static const OrderSource memx;

    /**
     * Blue Ocean Technologies Alternative Trading System.
     *
     * Order events are @ref ::isPublishable() "publishable" on this source and the corresponding subscription can be
     * observed via DXPublisher.
     */
    static const OrderSource OCEA;

    /**
     * Pink Sheets. Record for price level book.
     * Pink sheets are listings for stocks that trade over-the-counter (OTC).
     *
     * Order and OtcMarketsOrder events are @ref ::isPublishable() "publishable" on this source and the corresponding
     * subscription can be observed via DXPublisher.
     */
    static const OrderSource pink;

    /**
     * NYSE Arca traded securities
     *
     * Order events are @ref ::isPublishable() "publishable" on this source and the corresponding subscription can be
     * observed via DXPublisher.
     */
    static const OrderSource ARCA;

    /**
     * NYSE Arca traded securities. Record for price level book.
     *
     * Order events are @ref ::isPublishable() "publishable" on this source and the corresponding subscription can be
     * observed via DXPublisher.
     */
    static const OrderSource arca;

    /**
     * Cboe European Derivatives.
     *
     * Order events are @ref ::isPublishable() "publishable" on this source and the corresponding subscription can be
     * observed via DXPublisher.
     */
    static const OrderSource CEDX;

    /**
     * Cboe European Derivatives. Record for price level book.
     *
     * Order events are @ref ::isPublishable() "publishable" on this source and the corresponding subscription can be
     * observed via DXPublisher.
     */
    static const OrderSource cedx;

    /**
     * Determines whether specified source identifier refers to special order source.
     * Special order sources are used for wrapping non-order events into order events.
     *
     * @param sourceId the source identifier.
     * @return `true` if it is a special source identifier
     */
    static bool isSpecialSourceId(std::int32_t sourceId) noexcept {
        return sourceId >= COMPOSITE_BID.id() && sourceId <= AGGREGATE_ASK.id();
    }

    /**
     * Returns order source for the specified source identifier.
     *
     * @param sourceId the source identifier.
     * @return order source.
     */
    static const OrderSource &valueOf(std::int32_t sourceId) {
        if (auto found = PREDEFINED_SOURCES.find(sourceId); found != PREDEFINED_SOURCES.end()) {
            return found->second;
        }

        std::lock_guard lock(MTX_);

        if (auto found = USER_SOURCES_.find(sourceId); found != USER_SOURCES_.end()) {
            return found->second;
        }

        return USER_SOURCES_.try_emplace(sourceId, OrderSource(sourceId, decodeName(sourceId))).first->second;
    }

    /**
     * Returns order source for the specified source name.
     * The name must be either predefined, or contain at most 4 alphanumeric characters.
     *
     * @param name the name of the source.
     * @return order source.
     */
    static const OrderSource &valueOf(const std::string &name) {
        if (auto found = PREDEFINED_SOURCES.find(name); found != PREDEFINED_SOURCES.end()) {
            return found->second;
        }

        std::lock_guard lock(MTX_);

        auto sourceId = composeId(name);

        if (auto found = USER_SOURCES_.find(sourceId); found != USER_SOURCES_.end()) {
            return found->second;
        }

        return USER_SOURCES_.try_emplace(sourceId, OrderSource(sourceId, name)).first->second;
    }
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
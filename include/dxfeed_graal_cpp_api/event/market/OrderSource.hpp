// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "../EventTypeEnum.hpp"
#include "../IndexedEventSource.hpp"

DXFCPP_BEGIN_NAMESPACE

struct EventSourceWrapper;

/**
 * Identifies a source of Order, AnalyticOrder, OtcMarketsOrder, and SpreadOrder events.
 * There are the following kinds of order sources:
 * <ul>
 * <li><em>Synthetic</em> sources OrderSource::COMPOSITE_BID, OrderSource::COMPOSITE_ASK,
 *     OrderSource::REGIONAL_BID, and OrderSource::REGIONAL_ASK are provided for convenience of a consolidated
 *     order book and are automatically generated based on the corresponding Quote events.
 * <li><em>Aggregate</em> sources OrderSource::AGGREGATE_BID and OrderSource::AGGREGATE_ASK provide
 *     futures depth (aggregated by price level) and NASDAQ Level II (top of the book for each market maker).
 *     This source cannot be directly published to via dxFeed API.
 * <li><em>Publishable</em> sources OrderSource::DEFAULT, OrderSource::NTV, OrderSource::ISE etc.
 *     support a full range of dxFeed API features.
 * </ul>
 */
class DXFCPP_EXPORT OrderSource final : public IndexedEventSource {
    friend struct EventSourceWrapper;

    static constexpr std::uint32_t PUB_ORDER = 0x0001U;
    static constexpr std::uint32_t PUB_ANALYTIC_ORDER = 0x0002U;
    static constexpr std::uint32_t PUB_OTC_MARKETS_ORDER = 0x0004;
    static constexpr std::uint32_t PUB_SPREAD_ORDER = 0x0008U;
    static constexpr std::uint32_t FULL_ORDER_BOOK = 0x0010U;
    static constexpr std::uint32_t PUB_NUAM_ORDER  = 0x0020U;

    static constexpr std::uint32_t FLAGS_SIZE = 6U;

    public:
    static const std::unordered_map<std::variant<std::int32_t, std::string>, std::reference_wrapper<const OrderSource>>
        PREDEFINED_SOURCES;

    private:
    static inline std::mutex MTX_{};
    static std::unordered_map<std::int32_t, OrderSource> USER_SOURCES_;

    std::uint32_t pubFlags_{};
    bool builtin_{};

    OrderSource(std::int32_t id, const StringLike& name, std::uint32_t pubFlags) noexcept;

    OrderSource(const StringLike &name, std::uint32_t pubFlags);

    OrderSource(std::int32_t id, const StringLike &name) noexcept;

    static std::int32_t composeId(const StringLike &name);

    static void checkChar(char c);

    static std::string decodeName(std::int32_t id);

    static std::uint32_t getEventTypeMask(const EventTypeEnum &eventType);

    /**
     * Allocates memory for the dxFeed Graal SDK structure (recursively if necessary).
     * Fills the dxFeed Graal SDK structure's fields by the data of the current entity (recursively if necessary).
     * Returns the pointer to the filled structure.
     *
     * @return The pointer to the filled dxFeed Graal SDK structure
     */
    void *toGraal() const override;

    std::unique_ptr<void, decltype(&IndexedEventSource::freeGraal)> toGraalUnique() const noexcept override;

    public:
    /**
     * Bid side of a composite Quote.
     * It is a <em>synthetic</em> source.
     * It cannot be used with the DXFeed::getIndexedEventsPromise method, and it cannot be published directly to.
     * The subscription on the composite Quote event is observed when this source is subscribed to.
     * @deprecated Use the OrderSource::COMPOSITE source.
     */
    static const OrderSource COMPOSITE_BID;

    /**
     * Ask side of a composite Quote.
     * It is a <em>synthetic</em> and <em>separate</em> source.
     * It cannot be used with the DXFeed::getIndexedEventsPromise method, and it cannot be published directly to.
     * The subscription on the composite Quote event is observed when this source is subscribed to.
     * @deprecated Use the OrderSource::COMPOSITE source.
     */
    static const OrderSource COMPOSITE_ASK;

    /**
     * Bid side of a regional Quote.
     * It is a <em>synthetic</em> and <em>separate</em> source.
     * It cannot be used with the DXFeed::getIndexedEventsPromise method, and it cannot be published directly to.
     * The subscription on the regional Quote event is observed when this source is subscribed to.
     * @deprecated Use the OrderSource::REGIONAL source.
     */
    static const OrderSource REGIONAL_BID;

    /**
     * Ask side of a regional Quote.
     * It is a <em>synthetic</em> and <em>separate</em> source.
     * It cannot be used with the DXFeed::getIndexedEventsPromise method, and it cannot be published directly to.
     * The subscription on a regional Quote event is observed when this source is subscribed to.
     * @deprecated Use the OrderSource::REGIONAL source.
     */
    static const OrderSource REGIONAL_ASK;

    /**
     * Bid side of an aggregate order book (futures depth and NASDAQ Level II).
     * It is an <em>aggregate</em> and <em>separate</em> source.
     * This source cannot be directly published via dxFeed API, but otherwise it is fully operational.
     * @deprecated Use the OrderSource::AGGREGATE source.
     */
    static const OrderSource AGGREGATE_BID;

    /**
     * Ask side of an aggregate order book (futures depth and NASDAQ Level II).
     * It is a <em>aggregate</em> and <em>separate</em> source.
     * This source cannot be directly published via dxFeed API, but otherwise it is fully operational.
     * @deprecated Use the OrderSource::AGGREGATE source.
     */
    static const OrderSource AGGREGATE_ASK;

    /**
     * Composite Quote.
     * It is a <em>synthetic</em> and <em>unitary</em> source that represents both bid and ask side.
     * It cannot be used with the DXFeed::getIndexedEventsPromise method, and it cannot be published directly to.
     * The subscription on the composite Quote event is observed when this source is subscribed to.
     * To use this source when subscribing to all sources (e.g., when subscribing to an order without specifying a
     * source), instead of OrderSource::COMPOSITE_ASK and OrderSource::COMPOSITE_BID, set the system property
     * <b>`dxscheme.unitaryOrderSource`</b> to `true`.
     */
    static const OrderSource COMPOSITE;

    /**
     * Regional Quote.
     * It is a <em>synthetic</em> and <em>unitary</em> source that represents both bid and ask side.
     * It cannot be used with the DXFeed::getIndexedEventsPromise method, and it cannot be published directly to.
     * The subscription on a regional Quote event is observed when this source is subscribed to.
     * To use this source when subscribing to all sources (e.g., when subscribing to an order without specifying a
     * source), instead of OrderSource::REGIONAL_ASK and OrderSource::REGIONAL_BID, set the system property
     * <b>`dxscheme.unitaryOrderSource`</b> to `true`.
     */
    static const OrderSource REGIONAL;

    /**
     * Aggregate order book (futures depth and NASDAQ Level II).
     * It is an <em>aggregate</em> and <em>unitary</em> source that represents both bid and ask side.
     * This source cannot be directly published via dxFeed API, but otherwise it is fully operational.
     * To use this source when subscribing to all sources (e.g., when subscribing to an order without specifying a
     * source), instead of OrderSource::AGGREGATE_ASK and OrderSource::AGGREGATE_BID, set the system property
     * <b>`dxscheme.unitaryOrderSource`</b> to `true`.
     */
    static const OrderSource AGGREGATE;

    /**
     * Default source for publishing custom order books.
     * Order, AnalyticOrder, OtcMarketsOrder, and SpreadOrder events are @ref ::isPublishable() "publishable"
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
     * NASDAQ Total View. Record for a price level book.
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
     * Direct-Edge EDGX Exchange. Record for a price level book.
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
     * Bats BZX Exchange. Record for a price level book.
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
     * Borsa Istanbul Exchange. Record for a particular top 20 order book.
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
     * CME Globex. Record for a price level book.
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
     * Eurex Exchange. Record for a price level book.
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
     * Small Exchange. Record for a price level book.
     *
     * Order events are @ref ::isPublishable() "publishable" on this source and the corresponding subscription can be
     * observed via DXPublisher.
     */
    static const OrderSource smfe;

    /**
     * Investors exchange. Record for a price level book.
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
     * Members Exchange. Record for a price level book.
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
     * Blue Ocean Technologies Alternative Trading System. Record for a price level book.
     * Order events are @ref ::isPublishable() "publishable" on this source and the corresponding subscription can be
     * observed via DXPublisher.
     */
    static const OrderSource ocea;

    /**
     * Pink Sheets. Record for a price level book.
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
     * NYSE Arca traded securities. Record for a price level book.
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
     * Cboe European Derivatives. Record for a price level book.
     *
     * Order events are @ref ::isPublishable() "publishable" on this source and the corresponding subscription can be
     * observed via DXPublisher.
     */
    static const OrderSource cedx;

    /**
     * IG CFDs Gate.
     *
     * Order events are @ref ::isPublishable() "publishable" on this source and the corresponding subscription can be
     * observed via DXPublisher.
     */
    static const OrderSource IGC;

    /**
     * IG CFDs Gate. Record for a price level book.
     *
     * Order events are @ref ::isPublishable() "publishable" on this source and the corresponding subscription can be
     * observed via DXPublisher.
     */
    static const OrderSource igc;

    /**
     * EDX Exchange.
     *
     * Order events are @ref ::isPublishable() "publishable" on this source and the corresponding subscription can be
     * observed via DXPublisher.
     */
    static const OrderSource EDX;

    /**
     * EDX Exchange. Record for a price level book.
     *
     * Order events are @ref ::isPublishable() "publishable" on this source and the corresponding subscription can be
     * observed via DXPublisher.
     */
    static const OrderSource edx;

    /**
     * Nuam Exchange Gate.
     *
     * Order and NuamOrder events are @ref ::isPublishable() "publishable" on this source and the corresponding
     * subscription can be observed via DXPublisher.
     */
    static const OrderSource NUAM;

    /**
     * Nuam Exchange Gate. Record for a price level book.
     *
     * Order and NuamOrder events are @ref ::isPublishable() "publishable" on this source and the corresponding
     * subscription can be observed via DXPublisher.
     */
    static const OrderSource nuam;

    /**
     * Determines whether the specified source identifier refers to a special order source.
     * Special order sources are used for wrapping non-order events into order events.
     *
     * @param sourceId the source identifier.
     * @return `true` if it is a special source identifier
     */
    static bool isSpecialSourceId(std::int32_t sourceId) noexcept;

    /**
     * Returns order source for the specified source identifier.
     *
     * @param sourceId the source identifier.
     * @return order source.
     */
    static const OrderSource &valueOf(std::int32_t sourceId);

    /**
     * Returns order source for the specified source name.
     * The name must be either predefined or contain at most 4 alphanumeric characters.
     *
     * @param name the name of the source.
     * @return order source.
     */
    static const OrderSource &valueOf(const StringLike &name);
};

DXFCPP_END_NAMESPACE

template <> struct std::hash<dxfcpp::OrderSource> {
    std::size_t operator()(const dxfcpp::OrderSource &orderSource) const noexcept {
        return static_cast<std::size_t>(orderSource.id());
    }
};

DXFCXX_DISABLE_MSC_WARNINGS_POP()
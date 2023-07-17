// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include "../../internal/Common.hpp"
#include "../IndexedEventSource.hpp"

namespace dxfcpp {

// TODO: implement
class DXFCPP_EXPORT OrderSource final : public IndexedEventSource {

    static constexpr std::uint32_t PUB_ORDER = 0x0001U;
    static constexpr std::uint32_t PUB_ANALYTIC_ORDER = 0x0002U;
    static constexpr std::uint32_t PUB_SPREAD_ORDER = 0x0004U;
    static constexpr std::uint32_t FULL_ORDER_BOOK = 0x0008U;
    static constexpr std::uint32_t FLAGS_SIZE = 4U;

    static const std::unordered_map<std::uint32_t, std::reference_wrapper<const OrderSource>> INTERNAL_;

    static inline std::mutex MTX_{};
    static std::unordered_map<std::uint32_t, std::reference_wrapper<const OrderSource>> USER_;

    std::uint32_t pubFlags_{};
    bool builtin_{};

    OrderSource(std::uint32_t id, std::string name, std::uint32_t pubFlags) noexcept
        : IndexedEventSource(id, std::move(name)), pubFlags_{pubFlags}, builtin_{true} {}

    OrderSource(const std::string &name, std::uint32_t pubFlags) : OrderSource(composeId(name), name, pubFlags) {}

    static std::uint32_t composeId(const std::string & /*name*/) noexcept { return {}; }

    void *toGraal() const noexcept override;

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
     * Order, AnalyticOrder and SpreadOrder events are @ref ::isPublishable() "publishable"
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
};

} // namespace dxfcpp
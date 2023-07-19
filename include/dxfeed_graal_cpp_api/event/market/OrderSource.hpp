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

    static const std::unordered_map<std::int32_t, std::reference_wrapper<const OrderSource>> INTERNAL_;

    static inline std::mutex MTX_{};
    static inline std::unordered_map<std::int32_t, OrderSource> USER_{};

    std::uint32_t pubFlags_{};
    bool builtin_{};

    OrderSource(std::int32_t id, std::string name, std::uint32_t pubFlags) noexcept
        : IndexedEventSource(id, std::move(name)), pubFlags_{pubFlags}, builtin_{true} {}

    static const OrderSource &create(std::int32_t id, const std::string &name, std::uint32_t pubFlags) noexcept {
        static const OrderSource INVALID(-1, "INVALID", 0);

        // Below are sanity and integrity checks for special and builtin pre-defined sources.
        // They also guard against uncoordinated changes of id/name with other methods.
        if (id < 0) {
            // TODO: error handling: throw IllegalArgumentException("id is negative");
            return INVALID;
        }

        if (id > 0 && id < 0x20 && !isSpecialSourceId(id)) {
            // TODO: error handling: throw IllegalArgumentException("id is not marked as special");

            return INVALID;
        }

        if (id >= 0x20 && (id != composeId(name) || name != decodeName(id))) {
            // TODO: error handling: throw IllegalArgumentException("id does not match name");

            return INVALID;
        }

        if (INTERNAL_.contains(id) || USER_.contains(id)) {
            // TODO: error handling: throw IllegalArgumentException("duplicate id and name");

            return INVALID;
        }

        return USER_.emplace(id, OrderSource{id, name, pubFlags}).first->second;
    }

    OrderSource(const std::string &name, std::uint32_t pubFlags) : OrderSource(composeId(name), name, pubFlags) {}

    static std::int32_t composeId(const std::string & /*name*/) noexcept { return -1; }

    static bool checkChar(char c) noexcept {
        if (c >= 'A' && c <= 'Z' || c >= 'a' && c <= 'z' || c >= '0' && c <= '9') {
            return true;
        }

        // TODO: error handling: IllegalArgumentException("Source name must contain only alphanumeric characters");

        return false;
    }

    static std::string decodeName(int id) {
        if (id == 0) {
            // TODO: error handling: throw IllegalArgumentException("Source name must contain from 1 to 4 characters");

            return "";
        }

        std::string name(4, '\0');

        std::size_t n = 0;

        for (int i = 24; i >= 0; i -= 8) {
            if (dxfcpp::shr(id, i) == 0) { // skip highest contiguous zeros
                continue;
            }

            char c = static_cast<char>(dxfcpp::andOp(dxfcpp::shr(id, i), 0xff));

            if (!checkChar(c)) {
                return "";
            }

            name[n++] = c;
        }

        return name.substr(0, n);
    }

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
};

} // namespace dxfcpp
// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../entity/SharedEntity.hpp"
#include "../event/candle/CandleSymbol.hpp"
#include "../event/market/Order.hpp"
#include "../event/market/OrderSource.hpp"
#include "../event/market/Quote.hpp"
#include "../internal/JavaObjectHandle.hpp"
#include "./AuthOrderSource.hpp"

#include <atomic>
#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

/**
 * \addtogroup dxfcpp_orcs
 * @{
 */

DXFCPP_BEGIN_NAMESPACE

/**
 * The client of the Order Reconstruction Service (ORCS): the price levels (Order events) and the quotes (Quote events)
 * of a symbol for a period of time.
 *
 * ```cpp
 * auto service = PriceLevelService::create("orcs.example.com:7777[login=entitle:<token>]");
 * auto orders = service->getOrders(CandleSymbol::valueOf("AAPL{=d,gr=s}"), OrderSource::NTV,
 *                                  TimeFormat::DEFAULT.parse("20260110-100000"),
 *                                  TimeFormat::DEFAULT.parse("20260110-230000"), "sample");
 *
 * service->close();
 * ```
 */
struct DXFCPP_EXPORT PriceLevelService final : RequireMakeShared<PriceLevelService> {
    /// The alias to a type of shared pointer to the PriceLevelService object
    using Ptr = std::shared_ptr<PriceLevelService>;

    /**
     * The key of the custom CandleSymbol attribute that specifies the granularity of the price level updates in the
     * response, in seconds (`AAPL{=d,gr=s}` or `AAPL{=d,gr=10}`).
     */
    static constexpr std::string_view GRANULARITY_ATTRIBUTE_KEY = "gr";

    private:
    JavaObjectHandle<PriceLevelService> handle_;
    std::atomic<bool> closed_{false};

    public:
    explicit PriceLevelService(RequireMakeShared<PriceLevelService>::LockExternalConstructionTag,
                               JavaObjectHandle<PriceLevelService> &&handle);

    /**
     * Closes the service if it is not closed yet.
     */
    ~PriceLevelService() noexcept override;

    /**
     * Creates a new service and connects it to the RMI endpoint at the given address.
     *
     * @param address The address of the RMI endpoint of the service. To pass an authorization token, add
     * `[login=entitle:<token>]` to the address.
     * @return a new service.
     */
    static std::shared_ptr<PriceLevelService> create(const StringLike &address);

    /**
     * Returns the price levels of the given CandleSymbol and OrderSource within the given `from` and `to` times,
     * sorted by time.
     *
     * The candle symbol shall contain a supported CandlePeriod and the custom attribute
     * PriceLevelService::GRANULARITY_ATTRIBUTE_KEY ("gr") with the granularity in seconds (the minimal value is 1
     * second). If the period or the granularity is not supported by the service, the list is empty.
     *
     * @param candleSymbol The candle symbol to request.
     * @param orderSource The order source to request.
     * @param from The time in UTC (milliseconds since the Unix epoch) to request from.
     * @param to The time in UTC (milliseconds since the Unix epoch) to request to.
     * @param caller The caller identifier.
     * @return the Order events sorted by time.
     */
    std::vector<std::shared_ptr<Order>> getOrders(const CandleSymbol &candleSymbol, const OrderSource &orderSource,
                                                  std::int64_t from, std::int64_t to, const StringLike &caller) const;

    /**
     * Returns the price levels of the given CandleSymbol and OrderSource within the given `from` and `to` times,
     * sorted by time. The caller identifier is the default one of the Graal SDK ("dxfg").
     *
     * @param candleSymbol The candle symbol to request.
     * @param orderSource The order source to request.
     * @param from The time in UTC (milliseconds since the Unix epoch) to request from.
     * @param to The time in UTC (milliseconds since the Unix epoch) to request to.
     * @return the Order events sorted by time.
     * @see getOrders(const CandleSymbol &, const OrderSource &, std::int64_t, std::int64_t, const StringLike &) const
     */
    std::vector<std::shared_ptr<Order>> getOrders(const CandleSymbol &candleSymbol, const OrderSource &orderSource,
                                                  std::int64_t from, std::int64_t to) const;

    /**
     * Returns the quotes of the given CandleSymbol within the given `from` and `to` times, sorted by time.
     *
     * @param candleSymbol The candle symbol to request.
     * @param from The time in UTC (milliseconds since the Unix epoch) to request from.
     * @param to The time in UTC (milliseconds since the Unix epoch) to request to.
     * @param caller The caller identifier.
     * @return the Quote events sorted by time.
     */
    std::vector<std::shared_ptr<Quote>> getQuotes(const CandleSymbol &candleSymbol, std::int64_t from, std::int64_t to,
                                                  const StringLike &caller) const;

    /**
     * Returns the quotes of the given CandleSymbol within the given `from` and `to` times, sorted by time.
     * The caller identifier is the default one of the Graal SDK ("dxfg").
     *
     * @param candleSymbol The candle symbol to request.
     * @param from The time in UTC (milliseconds since the Unix epoch) to request from.
     * @param to The time in UTC (milliseconds since the Unix epoch) to request to.
     * @return the Quote events sorted by time.
     */
    std::vector<std::shared_ptr<Quote>> getQuotes(const CandleSymbol &candleSymbol, std::int64_t from,
                                                  std::int64_t to) const;

    /**
     * Returns the order sources and the symbols of each order source available to the client, filtered according to
     * the client permissions. It contains only the existing data as of now (no delisted symbols, for example).
     *
     * @return the available order sources and symbols.
     */
    std::shared_ptr<AuthOrderSource> getAuthOrderSource() const;

    /**
     * Closes (disconnects) the service. The next calls of close() do nothing.
     */
    void close();
};

DXFCPP_END_NAMESPACE

/// @}

DXFCXX_DISABLE_MSC_WARNINGS_POP()

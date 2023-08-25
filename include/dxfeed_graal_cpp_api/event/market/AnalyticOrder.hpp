// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include <cassert>
#include <cstdint>
#include <memory>
#include <string>

#include "../../internal/Common.hpp"
#include "../EventTypeEnum.hpp"

#include "IcebergType.hpp"
#include "Order.hpp"

namespace dxfcpp {

struct EventMapper;

/**
 * Represents an extension of Order introducing analytic information, e.g. adding to this order iceberg related
 * information (@ref AnalyticOrder::getIcebergPeakSize() "icebergPeakSize", @ref AnalyticOrder::getIcebergHiddenSize() "icebergHiddenSize",
 * @ref AnalyticOrder::getIcebergExecutedSize() "icebergExecutedSize").
 * The collection of analytic order events of a symbol represents the most recent analytic information
 * that is available about orders on the market at any given moment of time.
 *
 * <p> Analytic order is similar to a regular Order. In addition this event has few additional properties:
 * <ul>
 *     <li>@ref AnalyticOrder::getIcebergPeakSize() "icebergPeakSize" - the size of the peak, i.e. the visible part of the iceberg,
 *                              that is being continually refilled until the order is fully traded or cancelled;
 *     <li>@ref AnalyticOrder::getIcebergHiddenSize() "icebergHiddenSize" - the prediction of current hidden size of the iceberg, as inferred by the model;
 *     <li>@ref AnalyticOrder::getIcebergExecutedSize() "icebergExecutedSize" - the executed size of the iceberg order. For IcebergType::SYNTHETIC type
 *                             represents total executed size of all orders composing current iceberg;
 *     <li>AnalyticOrder::getIcebergType() - type of the iceberg, either native (exchange-managed) or synthetic (managed outside of the exchange).
 * </ul>
 *
 * <h3>Implementation details</h3>
 * <p>
 * This event is implemented on top of QDS records `AnalyticOrder#<source-id>`,
 * where `<source-id>` is up to 4 ASCII characters with a mnemonic for the source like "GLBX".
 */
class DXFCPP_EXPORT AnalyticOrder final : public Order {
    friend struct EventMapper;

    /*
     * Analytic flags property has several significant bits that are packed into an integer in the following way:
     *      31...2       1        0
     * +--------------+-------+-------+
     * |              |   IcebergType |
     * +--------------+-------+-------+
     */

    // TYPE values are taken from Type enum.
    static constexpr std::uint32_t ICEBERG_TYPE_MASK = 3U;
    static constexpr std::uint32_t ICEBERG_TYPE_SHIFT = 0U;

  protected:
    struct AnalyticOrderData {
        double icebergPeakSize = math::NaN;
        double icebergHiddenSize = math::NaN;
        double icebergExecutedSize = math::NaN;
        std::int32_t icebergFlags{};
    };

    AnalyticOrderData analyticOrderData_{};

    void fillData(void *graalNative) noexcept override;
    void fillGraalData(void *graalNative) const noexcept override;

    static std::shared_ptr<AnalyticOrder> fromGraal(void *graalNative) noexcept;

    /**
     * Allocates memory for the dxFeed Graal SDK structure (recursively if necessary).
     * Fills the dxFeed Graal SDK structure's fields by the data of the current entity (recursively if necessary).
     * Returns the pointer to the filled structure.
     *
     * @return The pointer to the filled dxFeed Graal SDK structure
     */
    void* toGraal() const noexcept override;

    /**
     * Releases the memory occupied by the dxFeed Graal SDK structure (recursively if necessary).
     *
     * @param graalNative The pointer to the dxFeed Graal SDK structure.
     */
    static void freeGraal(void* graalNative) noexcept;

  public:
    /// Type identifier and additional information about the current event class.
    static const EventTypeEnum &TYPE;

    /// Creates new analytic order event with default values.
    AnalyticOrder() noexcept = default;

    /**
     * Creates new analytic order event with the specified event symbol.
     *
     * @param eventSymbol The event symbol.
     */
    explicit AnalyticOrder(std::string eventSymbol) noexcept : Order(std::move(eventSymbol)) {
    }

    /**
     * Returns iceberg peak size of this analytic order.
     *
     * @return iceberg peak size of this analytic order.
     */
    double getIcebergPeakSize() const noexcept {
        return analyticOrderData_.icebergPeakSize;
    }

    /**
     * Changes iceberg peak size of this analytic order.
     *
     * @param icebergPeakSize iceberg peak size of this analytic order.
     */
    void setIcebergPeakSize(double icebergPeakSize) noexcept {
        analyticOrderData_.icebergPeakSize = icebergPeakSize;
    }

    /**
     * Returns iceberg hidden size of this analytic order.
     *
     * @return iceberg hidden size of this analytic order.
     */
    double getIcebergHiddenSize() const noexcept {
        return analyticOrderData_.icebergHiddenSize;
    }

    /**
     * Changes iceberg hidden size of this analytic order.
     *
     * @param icebergHiddenSize iceberg hidden size of this analytic order.
     */
    void setIcebergHiddenSize(double icebergHiddenSize) noexcept {
        analyticOrderData_.icebergHiddenSize = icebergHiddenSize;
    }

    /**
     * Returns iceberg executed size of this analytic order.
     *
     * @return iceberg executed size of this analytic order.
     */

    double getIcebergExecutedSize() const noexcept {
        return analyticOrderData_.icebergExecutedSize;
    }

    /**
     * Changes iceberg executed size of this analytic order.
     *
     * @param icebergExecutedSize iceberg executed size of this analytic order.
     */
    void setIcebergExecutedSize(double icebergExecutedSize) noexcept {
        analyticOrderData_.icebergExecutedSize = icebergExecutedSize;
    }

    /**
     * Returns iceberg type of this analytic order.
     *
     * @return iceberg type of this analytic order.
     */
    const IcebergType &getIcebergType() const & noexcept {
        return IcebergType::valueOf(getBits(analyticOrderData_.icebergFlags, ICEBERG_TYPE_MASK, ICEBERG_TYPE_SHIFT));
    }

    /**
     * Changes iceberg type of this analytic order.
     *
     * @param icebergType iceberg type of this analytic order.
     */
    void setIcebergType(const IcebergType &icebergType) noexcept {
        analyticOrderData_.icebergFlags =
            setBits(analyticOrderData_.icebergFlags, ICEBERG_TYPE_MASK, ICEBERG_TYPE_SHIFT, icebergType.getCode());
    }

    /**
     * Returns a string representation of the current object.
     *
     * @return a string representation
     */
    std::string toString() const noexcept override;
};

} // namespace dxfcpp
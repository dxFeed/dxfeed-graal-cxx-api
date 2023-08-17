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

#include "OrderBase.hpp"

namespace dxfcpp {

struct EventMapper;

// TODO: implement

class DXFCPP_EXPORT SpreadOrder : public OrderBase {
    friend struct EventMapper;

  protected:
    struct SpreadOrderData {
        std::string spreadSymbol{};
    };

    SpreadOrderData spreadOrderData_{};

    void fillData(void *graalNative) noexcept override;

    static std::shared_ptr<SpreadOrder> fromGraal(void *graalNative) noexcept;

  public:
    static const EventTypeEnum &TYPE;

    /// Creates new spread order event with default values.
    SpreadOrder() noexcept = default;

    /**
     * Creates new spread order event with the specified event symbol.
     *
     * @param eventSymbol The event symbol.
     */
    explicit SpreadOrder(std::string eventSymbol) noexcept : OrderBase(std::move(eventSymbol)) {
    }

    /**
     * Returns spread symbol of this event.
     *
     * @return spread symbol of this event.
     */
    const std::string &getSpreadSymbol() const & noexcept {
        return spreadOrderData_.spreadSymbol;
    }

    /**
     * Changes spread symbol of this event.
     *
     * @param spreadSymbol spread symbol of this event.
     */
    void setSpreadSymbol(std::string spreadSymbol) noexcept {
        spreadOrderData_.spreadSymbol = std::move(spreadSymbol);
    }

    /**
     * Returns a string representation of the current object.
     *
     * @return a string representation
     */
    std::string toString() const noexcept override;
};

} // namespace dxfcpp
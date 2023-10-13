// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include "../../internal/Common.hpp"

namespace dxfcpp {

class DXFCPP_EXPORT InstrumentProfileCollector final : public SharedEntity {
    Id<InstrumentProfileCollector> id_;
    JavaObjectHandle<InstrumentProfileCollector> handle_;

    InstrumentProfileCollector() noexcept;

  public:

    /// The alias to a type of shared pointer to the InstrumentProfileCollector object
    using Ptr = std::shared_ptr<InstrumentProfileCollector>;

    /// The alias to a type of unique pointer to the InstrumentProfileCollector object
    using Unique = std::unique_ptr<InstrumentProfileCollector>;

    /**
     * Creates the new InstrumentProfileCollector
     *
     * @return The new InstrumentProfileCollector
     */
    static Ptr create() noexcept;
};

}
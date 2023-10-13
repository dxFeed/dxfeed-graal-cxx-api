// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include "../../internal/Common.hpp"
#include "../../internal/Id.hpp"
#include "../../internal/JavaObjectHandle.hpp"

#include "../../entity/SharedEntity.hpp"

namespace dxfcpp {

class DXFCPP_EXPORT InstrumentProfileConnection final : public SharedEntity {
    Id<InstrumentProfileConnection> id_;
    JavaObjectHandle<InstrumentProfileConnection> handle_;

    //InstrumentProfileConnection() noexcept;

  public:

    /// The alias to a type of shared pointer to the InstrumentProfileConnection object
    using Ptr = std::shared_ptr<InstrumentProfileConnection>;

    /// The alias to a type of unique pointer to the InstrumentProfileConnection object
    using Unique = std::unique_ptr<InstrumentProfileConnection>;
};

}
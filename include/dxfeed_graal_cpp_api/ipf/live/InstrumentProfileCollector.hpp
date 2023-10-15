// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include "../../internal/Common.hpp"
#include "../../internal/Id.hpp"
#include "../../internal/JavaObjectHandle.hpp"

#include "../../entity/SharedEntity.hpp"

namespace dxfcpp {

class InstrumentProfileConnection;

class DXFCPP_EXPORT InstrumentProfileCollector final : public SharedEntity {
    friend InstrumentProfileConnection;

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

    /**
     * Returns last modification time (in milliseconds) of instrument profiles or zero if it is unknown.
     * Note, that while the time is represented in milliseconds, the actual granularity of time here is a second.
     *
     * @return The last modification time (in milliseconds) of instrument profiles or zero if it is unknown.
     */
    std::int64_t getLastUpdateTime() const noexcept;

    /**
     * Returns last modification time (as std::chrono::milliseconds) of instrument profiles or zero if it is unknown.
     * Note, that while the time is represented in milliseconds, the actual granularity of time here is a second.
     *
     * @return The last modification time (as std::chrono::milliseconds) of instrument profiles or zero if it is unknown.
     */
    std::chrono::milliseconds getLastUpdateTimeAsDuration() const noexcept {
        return std::chrono::milliseconds(getLastUpdateTime());
    }
};

} // namespace dxfcpp
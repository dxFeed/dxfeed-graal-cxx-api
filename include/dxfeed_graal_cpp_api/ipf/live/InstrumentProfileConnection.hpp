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

    InstrumentProfileConnection() noexcept;

  public:
    /// The alias to a type of shared pointer to the InstrumentProfileConnection object
    using Ptr = std::shared_ptr<InstrumentProfileConnection>;

    /// The alias to a type of unique pointer to the InstrumentProfileConnection object
    using Unique = std::unique_ptr<InstrumentProfileConnection>;

    /**
     * Creates instrument profile connection with a specified address and collector.
     * Address may be just "<host>:<port>" of server, URL, or a file path.
     * The "[update=<period>]" clause can be optionally added at the end of the address to
     * specify an @ref InstrumentProfileConnection::getUpdatePeriod() "update period" via an address string.
     * Default update period is 1 minute.
     *
     * <p>Connection needs to be @ref InstrumentProfileConnection::start() "started" to begin an actual operation.
     *
     * @param address The address.
     * @param collector The instrument profile collector to push updates into.
     *
     * @return new instrument profile connection.
     */
    static Ptr createConnection(const std::string &address, InstrumentProfileCollector::Ptr collector) noexcept;

    /**
     * Returns the address of this instrument profile connection.
     * It does not include additional options specified as part of the address.
     *
     * @return The address of this instrument profile connection.
     */
    std::string getAddress() const noexcept;

    /**
     * Returns update period in milliseconds.
     * It is period of an update check when the instrument profiles source does not support live updates
     * and/or when connection is dropped.
     * Default update period is 1 minute, unless overridden in an
     * @ref InstrumentProfileConnection::createConnection() "address string".
     *
     * @return The update period in milliseconds.
     */
    std::int64_t getUpdatePeriod() const noexcept;
};

} // namespace dxfcpp
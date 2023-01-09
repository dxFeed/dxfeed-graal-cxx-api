// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <dxfg_endpoint.h>

#include "internal/CEntryPointErrors.hpp"
#include "internal/Common.hpp"
#include "internal/Isolate.hpp"

#include <iostream>
#include <string>
#include <type_traits>
#include <variant>

#include <fmt/format.h>
#include <fmt/std.h>

namespace dxfcpp {
struct DXEndpoint {
    /**
     * Defines property for endpoint name that is used to distinguish multiple endpoints
     * in the same process in logs and in other diagnostic means.
     * Use Builder::withProperty(const std::string&, const std::string&) method.
     * This property is also changed by Builder::withName(const std::string&) method.
     */
    static const std::string NAME_PROPERTY;

    /**
     * Defines path to a file with properties for an endpoint with role Role::FEED or Role::ON_DEMAND_FEED.
     * By default, properties a loaded from a path resource named "dxfeed.properties".
     *
     * @see Builder::withProperty(const std::string&, const std::string&)
     */
    static const std::string DXFEED_PROPERTIES_PROPERTY;

    /**
     * Defines default connection address for an endpoint with role Role::FEED or Role::ON_DEMAND_FEED.
     * Connection is established to this address by role Role::FEED as soon as endpoint is created, while role
     * Role::ON_DEMAND_FEED waits until OnDemandService::replay(Date, double) is invoked before connecting.
     *
     * By default, without this property, connection is not established until ::connect(const std::string&) is invoked.
     *
     * Credentials for access to premium services may be configured with
     * ::DXFEED_USER_PROPERTY and ::DXFEED_PASSWORD_PROPERTY.
     *
     * @see Builder::withProperty(const std::string&, const std::string&)
     */
    static const std::string DXFEED_ADDRESS_PROPERTY;

    /**
     * Defines default user name for an endpoint with role Role::FEED or Role::ON_DEMAND_FEED.
     *
     * @see ::user(const std::string&)
     */
    static const std::string DXFEED_USER_PROPERTY;

    /**
     * Defines default password for an endpoint with role Role::FEED or Role::ON_DEMAND_FEED.
     *
     * @see ::password(const std::string&)
     */
    static const std::string DXFEED_PASSWORD_PROPERTY;
};
} // namespace dxfcpp
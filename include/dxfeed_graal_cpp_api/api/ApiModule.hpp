// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

/**
 * \defgroup dxfcpp_api "API" Module
 * "Entry point" for using the API.
 * \ingroup dxfcpp_modules
 */

#include "./DXEndpoint.hpp"
#include "./DXFeed.hpp"
#include "./DXFeedSubscription.hpp"
#include "./DXPublisher.hpp"
#include "./DXPublisherObservableSubscription.hpp"
#include "./FilteredSubscriptionSymbol.hpp"
#include "./osub/OsubModule.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_POP()
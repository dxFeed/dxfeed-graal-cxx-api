// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../../include/dxfeed_graal_cpp_api/detail/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../../include/dxfeed_graal_cpp_api/ondemand/OnDemandService.hpp"

DXFCPP_BEGIN_NAMESPACE

namespace isolated::ondemand {

namespace IsolatedOnDemandService {

/* dxfg_v4_on_demand_service_t* */ JavaObjectHandle<OnDemandService>
getInstance(/* dxfg_v4_endpoint_t * */ const JavaObjectHandle<DXEndpoint> &endpoint);

bool isReplaySupported(/* dxfg_v4_on_demand_service_t * */ const JavaObjectHandle<OnDemandService> &service);

bool isReplay(/* dxfg_v4_on_demand_service_t * */ const JavaObjectHandle<OnDemandService> &service);

bool isClear(/* dxfg_v4_on_demand_service_t * */ const JavaObjectHandle<OnDemandService> &service);

std::int64_t getTime(/* dxfg_v4_on_demand_service_t * */ const JavaObjectHandle<OnDemandService> &service);

double getSpeed(/* dxfg_v4_on_demand_service_t * */ const JavaObjectHandle<OnDemandService> &service);

bool replay(/* dxfg_v4_on_demand_service_t * */ const JavaObjectHandle<OnDemandService> &service,
            std::int64_t time);

bool replay(/* dxfg_v4_on_demand_service_t * */ const JavaObjectHandle<OnDemandService> &service,
            std::int64_t time, double speed);

bool pause(/* dxfg_v4_on_demand_service_t * */ const JavaObjectHandle<OnDemandService> &service);

bool stopAndResume(/* dxfg_v4_on_demand_service_t * */ const JavaObjectHandle<OnDemandService> &service);

bool stopAndClear(/* dxfg_v4_on_demand_service_t * */ const JavaObjectHandle<OnDemandService> &service);

bool setSpeed(/* dxfg_v4_on_demand_service_t * */ const JavaObjectHandle<OnDemandService> &service, double speed);

} // namespace IsolatedOnDemandService

} // namespace isolated::ondemand

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
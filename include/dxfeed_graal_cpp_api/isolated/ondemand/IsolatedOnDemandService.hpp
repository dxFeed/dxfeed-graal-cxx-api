// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include "../../ondemand/OnDemandService.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

DXFCPP_BEGIN_NAMESPACE

namespace isolated::ondemand {

namespace IsolatedOnDemandService {

/* dxfg_on_demand_service_t* */ JavaObjectHandle<dxfcpp::OnDemandService>
getInstance(/* dxfg_endpoint_t * */ const JavaObjectHandle<dxfcpp::DXEndpoint>& endpoint);

bool isReplaySupported(/* dxfg_on_demand_service_t * */ const JavaObjectHandle<dxfcpp::OnDemandService>& service);

bool isReplay(/* dxfg_on_demand_service_t * */ const JavaObjectHandle<dxfcpp::OnDemandService>& service);

bool isClear(/* dxfg_on_demand_service_t * */ const JavaObjectHandle<dxfcpp::OnDemandService>& service);

std::int64_t getTime(/* dxfg_on_demand_service_t * */ const JavaObjectHandle<dxfcpp::OnDemandService>& service);

double getSpeed(/* dxfg_on_demand_service_t * */ const JavaObjectHandle<dxfcpp::OnDemandService>& service);

bool replay(/* dxfg_on_demand_service_t * */ const JavaObjectHandle<dxfcpp::OnDemandService>& service, std::int64_t time);

bool replay(/* dxfg_on_demand_service_t * */ const JavaObjectHandle<dxfcpp::OnDemandService>& service, std::int64_t time, double speed);

bool pause(/* dxfg_on_demand_service_t * */ const JavaObjectHandle<dxfcpp::OnDemandService>& service);

bool stopAndResume(/* dxfg_on_demand_service_t * */ const JavaObjectHandle<dxfcpp::OnDemandService>& service);

bool stopAndClear(/* dxfg_on_demand_service_t * */ const JavaObjectHandle<dxfcpp::OnDemandService>& service);

bool setSpeed(/* dxfg_on_demand_service_t * */ const JavaObjectHandle<dxfcpp::OnDemandService>& service, double speed);

} // namespace IsolatedOnDemandService

} // namespace isolated::ondemand

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
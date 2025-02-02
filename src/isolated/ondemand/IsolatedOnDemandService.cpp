// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>
#include <dxfeed_graal_cpp_api/isolated/ondemand/IsolatedOnDemandService.hpp>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::ondemand {

namespace IsolatedOnDemandService {

/* dxfg_on_demand_service_t* */ JavaObjectHandle<dxfcpp::OnDemandService>
getInstance(/* dxfg_endpoint_t * */ const JavaObjectHandle<dxfcpp::DXEndpoint> &endpoint) {
    if (!endpoint) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_OnDemandService_getInstance2`. The `endpoint` handle is invalid");
    }

    return JavaObjectHandle<dxfcpp::OnDemandService>{runGraalFunctionAndThrowIfNullptr(
        dxfg_OnDemandService_getInstance2, static_cast<dxfg_endpoint_t *>(endpoint.get()))};
}

bool isReplaySupported(/* dxfg_on_demand_service_t * */ const JavaObjectHandle<dxfcpp::OnDemandService> &service) {
    if (!service) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_OnDemandService_isReplaySupported`. The `service` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_OnDemandService_isReplaySupported,
                                              static_cast<dxfg_on_demand_service_t *>(service.get())) == 1;
}

bool isReplay(/* dxfg_on_demand_service_t * */ const JavaObjectHandle<dxfcpp::OnDemandService> &service) {
    if (!service) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_OnDemandService_isReplay`. The `service` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_OnDemandService_isReplay,
                                              static_cast<dxfg_on_demand_service_t *>(service.get())) == 1;
}

bool isClear(/* dxfg_on_demand_service_t * */ const JavaObjectHandle<dxfcpp::OnDemandService> &service) {
    if (!service) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_OnDemandService_isClear`. The `service` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_OnDemandService_isClear,
                                              static_cast<dxfg_on_demand_service_t *>(service.get())) == 1;
}

std::int64_t getTime(/* dxfg_on_demand_service_t * */ const JavaObjectHandle<dxfcpp::OnDemandService> &service) {
    if (!service) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_OnDemandService_getTime`. The `service` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_OnDemandService_getTime,
                                              static_cast<dxfg_on_demand_service_t *>(service.get()));
}

double getSpeed(/* dxfg_on_demand_service_t * */ const JavaObjectHandle<dxfcpp::OnDemandService> &service) {
    if (!service) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_OnDemandService_getSpeed`. The `service` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusInf(dxfg_OnDemandService_getSpeed,
                                              static_cast<dxfg_on_demand_service_t *>(service.get()));
}

bool replay(/* dxfg_on_demand_service_t * */ const JavaObjectHandle<dxfcpp::OnDemandService> &service,
            std::int64_t time) {
    if (!service) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_OnDemandService_replay`. The `service` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_OnDemandService_replay,
                                              static_cast<dxfg_on_demand_service_t *>(service.get()), time) == 0;
}

bool replay(/* dxfg_on_demand_service_t * */ const JavaObjectHandle<dxfcpp::OnDemandService> &service,
            std::int64_t time, double speed) {
    if (!service) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_OnDemandService_replay2`. The `service` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_OnDemandService_replay2,
                                              static_cast<dxfg_on_demand_service_t *>(service.get()), time, speed) == 0;
}

bool pause(/* dxfg_on_demand_service_t * */ const JavaObjectHandle<dxfcpp::OnDemandService> &service) {
    if (!service) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_OnDemandService_pause`. The `service` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_OnDemandService_pause,
                                              static_cast<dxfg_on_demand_service_t *>(service.get())) == 0;
}

bool stopAndResume(/* dxfg_on_demand_service_t * */ const JavaObjectHandle<dxfcpp::OnDemandService> &service) {
    if (!service) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_OnDemandService_stopAndResume`. The `service` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_OnDemandService_stopAndResume,
                                              static_cast<dxfg_on_demand_service_t *>(service.get())) == 0;
}

bool stopAndClear(/* dxfg_on_demand_service_t * */ const JavaObjectHandle<dxfcpp::OnDemandService> &service) {
    if (!service) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_OnDemandService_stopAndClear`. The `service` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_OnDemandService_stopAndClear,
                                              static_cast<dxfg_on_demand_service_t *>(service.get())) == 0;
}

bool setSpeed(/* dxfg_on_demand_service_t * */ const JavaObjectHandle<dxfcpp::OnDemandService> &service, double speed) {
    if (!service) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_OnDemandService_setSpeed`. The `service` handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_OnDemandService_setSpeed,
                                              static_cast<dxfg_on_demand_service_t *>(service.get()), speed) == 0;
}

} // namespace IsolatedOnDemandService

} // namespace isolated::ondemand

DXFCPP_END_NAMESPACE
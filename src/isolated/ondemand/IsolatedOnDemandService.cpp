// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "IsolatedOnDemandService.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../IsolatedCommon.hpp"
#include "../../../include/dxfeed_graal_cpp_api/symbols/SymbolWrapper.hpp"

#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::ondemand {

namespace IsolatedOnDemandService {

/// dxfg_v4_OnDemandService_getInstanceByEndpoint
/* dxfg_v4_on_demand_service_t* */ JavaObjectHandle<OnDemandService>
getInstance(/* dxfg_v4_endpoint_t * */ const JavaObjectHandle<DXEndpoint> &endpoint) {
    if (!endpoint) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_OnDemandService_getInstanceByEndpoint`. "
                                       "The `endpoint` handle is invalid");
    }

    return JavaObjectHandle<OnDemandService>{runGraalV4FunctionWithNonNullResult(
        dxfg_v4_OnDemandService_getInstanceByEndpoint, static_cast<dxfg_v4_endpoint_t *>(endpoint.get()))};
}

/// dxfg_v4_OnDemandService_isReplaySupported
bool isReplaySupported(/* dxfg_v4_on_demand_service_t * */ const JavaObjectHandle<OnDemandService> &service) {
    if (!service) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_OnDemandService_isReplaySupported`. The "
                                       "`service` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_OnDemandService_isReplaySupported,
                                        static_cast<dxfg_v4_on_demand_service_t *>(service.get())) != 0;
}

/// dxfg_v4_OnDemandService_isReplay
bool isReplay(/* dxfg_v4_on_demand_service_t * */ const JavaObjectHandle<OnDemandService> &service) {
    if (!service) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_OnDemandService_isReplay`. The `service` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_OnDemandService_isReplay,
                                        static_cast<dxfg_v4_on_demand_service_t *>(service.get())) != 0;
}

/// dxfg_v4_OnDemandService_isClear
bool isClear(/* dxfg_v4_on_demand_service_t * */ const JavaObjectHandle<OnDemandService> &service) {
    if (!service) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_OnDemandService_isClear`. The `service` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_OnDemandService_isClear,
                                        static_cast<dxfg_v4_on_demand_service_t *>(service.get())) != 0;
}

/// dxfg_v4_OnDemandService_getTime
std::int64_t getTime(/* dxfg_v4_on_demand_service_t * */ const JavaObjectHandle<OnDemandService> &service) {
    if (!service) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_OnDemandService_getTime`. The `service` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_OnDemandService_getTime,
                                        static_cast<dxfg_v4_on_demand_service_t *>(service.get()));
}

/// dxfg_v4_OnDemandService_getSpeed
double getSpeed(/* dxfg_v4_on_demand_service_t * */ const JavaObjectHandle<OnDemandService> &service) {
    if (!service) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_OnDemandService_getSpeed`. The `service` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_OnDemandService_getSpeed,
                                        static_cast<dxfg_v4_on_demand_service_t *>(service.get()));
}

/// dxfg_v4_OnDemandService_replay
bool replay(/* dxfg_v4_on_demand_service_t * */ const JavaObjectHandle<OnDemandService> &service,
            std::int64_t time) {
    if (!service) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_OnDemandService_replay`. The `service` handle is invalid");
    }

    runGraalV4Function(dxfg_v4_OnDemandService_replay, static_cast<dxfg_v4_on_demand_service_t *>(service.get()), time);

    return true;
}

/// dxfg_v4_OnDemandService_replayWithSpeed
bool replay(/* dxfg_v4_on_demand_service_t * */ const JavaObjectHandle<OnDemandService> &service,
            std::int64_t time, double speed) {
    if (!service) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_OnDemandService_replayWithSpeed`. The `service` handle is invalid");
    }

    runGraalV4Function(dxfg_v4_OnDemandService_replayWithSpeed,
                       static_cast<dxfg_v4_on_demand_service_t *>(service.get()), time, speed);

    return true;
}

/// dxfg_v4_OnDemandService_pause
bool pause(/* dxfg_v4_on_demand_service_t * */ const JavaObjectHandle<OnDemandService> &service) {
    if (!service) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_OnDemandService_pause`. The `service` handle is invalid");
    }

    runGraalV4Function(dxfg_v4_OnDemandService_pause, static_cast<dxfg_v4_on_demand_service_t *>(service.get()));

    return true;
}

/// dxfg_v4_OnDemandService_stopAndResume
bool stopAndResume(/* dxfg_v4_on_demand_service_t * */ const JavaObjectHandle<OnDemandService> &service) {
    if (!service) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_OnDemandService_stopAndResume`. The `service` handle is invalid");
    }

    runGraalV4Function(dxfg_v4_OnDemandService_stopAndResume,
                       static_cast<dxfg_v4_on_demand_service_t *>(service.get()));

    return true;
}

/// dxfg_v4_OnDemandService_stopAndClear
bool stopAndClear(/* dxfg_v4_on_demand_service_t * */ const JavaObjectHandle<OnDemandService> &service) {
    if (!service) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_OnDemandService_stopAndClear`. The `service` handle is invalid");
    }

    runGraalV4Function(dxfg_v4_OnDemandService_stopAndClear,
                       static_cast<dxfg_v4_on_demand_service_t *>(service.get()));

    return true;
}

/// dxfg_v4_OnDemandService_setSpeed
bool setSpeed(/* dxfg_v4_on_demand_service_t * */ const JavaObjectHandle<OnDemandService> &service, double speed) {
    if (!service) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_OnDemandService_setSpeed`. The `service` handle is invalid");
    }

    runGraalV4Function(dxfg_v4_OnDemandService_setSpeed, static_cast<dxfg_v4_on_demand_service_t *>(service.get()),
                       speed);

    return true;
}

} // namespace IsolatedOnDemandService

} // namespace isolated::ondemand

DXFCPP_END_NAMESPACE
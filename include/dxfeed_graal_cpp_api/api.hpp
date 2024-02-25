// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#ifdef _MSC_VER
#    pragma warning(push)
#    pragma warning(disable : 4251)
#    pragma warning(disable : 4996)
#endif

#include "internal/Conf.hpp"

#include "internal/CEntryPointErrors.hpp"
#include "internal/Common.hpp"
#include "internal/Enum.hpp"
#include "internal/EventClassList.hpp"
#include "internal/Id.hpp"
#include "internal/Isolate.hpp"
#include "internal/JavaObjectHandle.hpp"
#include "internal/NonCopyable.hpp"
#include "internal/Platform.hpp"
#include "internal/RawListWrapper.hpp"
#include "internal/StopWatch.hpp"
#include "internal/Timer.hpp"
#include "internal/context/ApiContext.hpp"

#include "internal/managers/DXEndpointManager.hpp"
#include "internal/managers/DXFeedManager.hpp"
#include "internal/managers/DXFeedSubscriptionManager.hpp"
#include "internal/managers/DXPublisherManager.hpp"
#include "internal/managers/InstrumentProfileReaderManager.hpp"
#include "internal/managers/InstrumentProfileCollectorManager.hpp"
#include "internal/managers/InstrumentProfileConnectionManager.hpp"
#include "internal/managers/EntityManager.hpp"
#include "internal/managers/ErrorHandlingManager.hpp"

#include "dxfeed_graal_cpp_api/internal/TimeFormat.hpp"
#include "internal/utils/CmdArgsUtils.hpp"
#include "internal/utils/EnumUtils.hpp"
#include "internal/utils/StringUtils.hpp"
#include "internal/utils/debug/Debug.hpp"

#include "api/ApiModule.hpp"
#include "ipf/IpfModule.hpp"
#include "entity/EntityModule.hpp"
#include "event/EventModule.hpp"
#include "schedule/ScheduleModule.hpp"
#include "symbols/StringSymbol.hpp"
#include "symbols/SymbolWrapper.hpp"
#include "system/System.hpp"

#include "isolated/Isolated.hpp"

#include "native/GraalException.hpp"
#include "native/JavaException.hpp"

#include "ondemand/OnDemandService.hpp"

#ifdef _MSC_VER
#    pragma warning(pop)
#endif
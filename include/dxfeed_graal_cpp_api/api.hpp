// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251 4996)

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
#include "internal/TimeFormat.hpp"

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

#include "internal/utils/CmdArgsUtils.hpp"
#include "internal/utils/EnumUtils.hpp"
#include "internal/utils/StringUtils.hpp"
#include "internal/utils/debug/Debug.hpp"

#include "util/TimePeriod.hpp"

#include "api/ApiModule.hpp"
#include "auth/AuthToken.hpp"
#include "ipf/IpfModule.hpp"
#include "entity/EntityModule.hpp"
#include "event/EventModule.hpp"
#include "schedule/ScheduleModule.hpp"
#include "symbols/StringSymbol.hpp"
#include "symbols/SymbolWrapper.hpp"
#include "system/System.hpp"

#include "isolated/Isolated.hpp"

#include "exceptions/RuntimeException.hpp"
#include "exceptions/InvalidArgumentException.hpp"
#include "exceptions/JavaException.hpp"
#include "exceptions/GraalException.hpp"

#include "isolated/api/IsolatedDXFeed.hpp"
#include "isolated/api/IsolatedDXEndpoint.hpp"
#include "isolated/api/IsolatedDXFeedSubscription.hpp"
#include "isolated/api/IsolatedDXPublisher.hpp"
#include "isolated/api/IsolatedDXPublisherObservableSubscription.hpp"
#include "isolated/api/osub/IsolatedObservableSubscriptionChangeListener.hpp"
#include "isolated/auth/IsolatedAuthToken.hpp"
#include "isolated/event/IsolatedEventType.hpp"
#include "isolated/promise/IsolatedPromise.hpp"
#include "isolated/internal/IsolatedString.hpp"
#include "isolated/internal/IsolatedObject.hpp"
#include "isolated/internal/IsolatedTimeFormat.hpp"
#include "isolated/internal/IsolatedTools.hpp"
#include "isolated/ipf/IsolatedInstrumentProfile.hpp"
#include "isolated/ipf/IsolatedInstrumentProfileReader.hpp"
#include "isolated/util/IsolatedTimePeriod.hpp"

#include "ondemand/OnDemandService.hpp"

#include "promise/Promise.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_POP()
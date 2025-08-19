// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251 4996)

#include "internal/CEntryPointErrors.hpp"
#include "internal/Common.hpp"
#include "internal/Enum.hpp"
#include "internal/EventClassList.hpp"
#include "internal/Handler.hpp"
#include "internal/Id.hpp"
#include "internal/Isolate.hpp"
#include "internal/JavaObjectHandle.hpp"
#include "internal/NonCopyable.hpp"
#include "internal/Platform.hpp"
#include "internal/RawListWrapper.hpp"
#include "internal/StopWatch.hpp"
#include "internal/TimeFormat.hpp"
#include "internal/Timer.hpp"

#include "internal/context/ApiContext.hpp"

#include "internal/managers/EntityManager.hpp"
#include "internal/managers/ErrorHandlingManager.hpp"

#include "internal/utils/CmdArgsUtils.hpp"
#include "internal/utils/EnumUtils.hpp"
#include "internal/utils/StringUtils.hpp"
#include "internal/utils/debug/Debug.hpp"

#include "internal/Metrics.hpp"

#include "util/TimePeriod.hpp"

#include "api/ApiModule.hpp"
#include "auth/AuthToken.hpp"
#include "entity/EntityModule.hpp"
#include "event/EventModule.hpp"
#include "ipf/IpfModule.hpp"
#include "model/IndexedTxModel.hpp"
#include "model/MarketDepthModel.hpp"
#include "model/MarketDepthModelListener.hpp"
#include "model/TimeSeriesTxModel.hpp"
#include "model/TxModelListener.hpp"
#include "schedule/ScheduleModule.hpp"
#include "symbols/StringSymbol.hpp"
#include "symbols/SymbolWrapper.hpp"
#include "system/System.hpp"

#include "exceptions/GraalException.hpp"
#include "exceptions/InvalidArgumentException.hpp"
#include "exceptions/JavaException.hpp"
#include "exceptions/RuntimeException.hpp"

#include "executors/InPlaceExecutor.hpp"

#include "candlewebservice/HistoryEndpoint.hpp"

#include "isolated/api/IsolatedDXEndpoint.hpp"
#include "isolated/api/IsolatedDXFeed.hpp"
#include "isolated/api/IsolatedDXFeedSubscription.hpp"
#include "isolated/api/IsolatedDXPublisher.hpp"
#include "isolated/api/IsolatedDXPublisherObservableSubscription.hpp"
#include "isolated/api/osub/IsolatedObservableSubscriptionChangeListener.hpp"
#include "isolated/auth/IsolatedAuthToken.hpp"
#include "isolated/candlewebservice/IsolatedHistoryEndpoint.hpp"
#include "isolated/event/IsolatedEventType.hpp"
#include "isolated/executors/IsolatedInPlaceExecutor.hpp"
#include "isolated/internal/IsolatedObject.hpp"
#include "isolated/internal/IsolatedString.hpp"
#include "isolated/internal/IsolatedTimeFormat.hpp"
#include "isolated/internal/IsolatedTools.hpp"
#include "isolated/ipf/IsolatedInstrumentProfile.hpp"
#include "isolated/ipf/IsolatedInstrumentProfileReader.hpp"
#include "isolated/ipf/live/IsolatedInstrumentProfileCollector.hpp"
#include "isolated/ipf/live/IsolatedInstrumentProfileConnection.hpp"
#include "isolated/logging/IsolatedLogging.hpp"
#include "isolated/model/IsolatedIndexedTxModel.hpp"
#include "isolated/model/IsolatedTimeSeriesTxModel.hpp"
#include "isolated/model/IsolatedTxModelListener.hpp"
#include "isolated/ondemand/IsolatedOnDemandService.hpp"
#include "isolated/promise/IsolatedPromise.hpp"
#include "isolated/schedule/IsolatedDay.hpp"
#include "isolated/schedule/IsolatedSchedule.hpp"
#include "isolated/schedule/IsolatedSession.hpp"
#include "isolated/util/IsolatedTimePeriod.hpp"

#include "logging/Logging.hpp"

#include "ondemand/OnDemandService.hpp"

#include "promise/Promise.hpp"
#include "promise/Promises.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_POP()
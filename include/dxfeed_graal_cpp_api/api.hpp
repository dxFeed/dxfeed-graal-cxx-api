// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "./detail/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251 4996)

/**
 * \defgroup dxfcpp_modules dxFeed Graal C++ API Modules
 */

#include "./api/ApiModule.hpp"
#include "./auth/AuthToken.hpp"
#include "./candlewebservice/HistoryEndpoint.hpp"
#include "./detail/CallbackBarrier.hpp"
#include "./detail/CEntryPointErrors.hpp"
#include "./detail/Concepts.hpp"
#include "./detail/Enum.hpp"
#include "./detail/EventClassList.hpp"
#include "./detail/JavaObjectHandle.hpp"
#include "./detail/Math.hpp"
#include "./detail/StringLike.hpp"
#include "./entity/EntityModule.hpp"
#include "./event/EventModule.hpp"
#include "./exceptions/ExceptionsModule.hpp"
#include "./executors/InPlaceExecutor.hpp"
#include "./glossary/GlossaryModule.hpp"
#include "./internal/Common.hpp"
#include "./internal/context/ApiContext.hpp"
#include "./internal/Id.hpp"
#include "./internal/Isolate.hpp"
#include "./internal/managers/EntityManager.hpp"
#include "./internal/Metrics.hpp"
#include "./internal/NonCopyable.hpp"
#include "./internal/Platform.hpp"
#include "./internal/RawListWrapper.hpp"
#include "./internal/StopWatch.hpp"
#include "./internal/utils/debug/Debug.hpp"
#include "./internal/utils/EnumUtils.hpp"
#include "./internal/utils/StringUtils.hpp"
#include "./ipf/IpfModule.hpp"
#include "./logging/Logging.hpp"
#include "./model/IndexedEventTxModel.hpp"
#include "./model/IndexedTxModel.hpp"
#include "./model/MarketDepthModel.hpp"
#include "./model/MarketDepthModelListener.hpp"
#include "./model/TimeSeriesTxModel.hpp"
#include "./model/TxModelListener.hpp"
#include "./ondemand/OnDemandService.hpp"
#include "./orcs/OrcsModule.hpp"
#include "./promise/Promise.hpp"
#include "./promise/Promises.hpp"
#include "./schedule/ScheduleModule.hpp"
#include "./symbols/StringSymbol.hpp"
#include "./symbols/SymbolWrapper.hpp"
#include "./system/System.hpp"
#include "./util/CmdArgsUtils.hpp"
#include "./util/Handler.hpp"
#include "./util/StringUtils.hpp"
#include "./util/Time.hpp"
#include "./util/TimeFormat.hpp"
#include "./util/TimePeriod.hpp"
#include "./util/TimePeriodInfo.hpp"
#include "./util/Timer.hpp"
#include "./util/Tristate.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_POP()
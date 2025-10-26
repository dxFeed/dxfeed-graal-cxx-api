// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>
#include <dxfeed_graal_cpp_api/isolated/candlewebservice/IsolatedHistoryEndpoint.hpp>
#include <dxfeed_graal_cpp_api/isolated/event/IsolatedEventType.hpp>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::candlewebservice::IsolatedHistoryEndpoint {

namespace Builder {

void withAddress(const JavaObjectHandle<HistoryEndpoint::Builder> &builder, const StringLike &address) {
    if (!builder) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_HistoryEndpoint_Builder_withAddress`. The `builder` handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_HistoryEndpoint_Builder_withAddress,
                                           static_cast<dxfg_history_endpoint_builder_t *>(builder.get()),
                                           address.c_str());
}

void withUserName(const JavaObjectHandle<HistoryEndpoint::Builder> &builder, const StringLike &userName) {
    if (!builder) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_HistoryEndpoint_Builder_withUserName`. The `builder` handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_HistoryEndpoint_Builder_withUserName,
                                           static_cast<dxfg_history_endpoint_builder_t *>(builder.get()),
                                           userName.c_str());
}

void withPassword(const JavaObjectHandle<HistoryEndpoint::Builder> &builder, const StringLike &password) {
    if (!builder) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_HistoryEndpoint_Builder_withPassword`. The `builder` handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_HistoryEndpoint_Builder_withPassword,
                                           static_cast<dxfg_history_endpoint_builder_t *>(builder.get()),
                                           password.c_str());
}

void withAuthToken(const JavaObjectHandle<HistoryEndpoint::Builder> &builder, const StringLike &authToken) {
    if (!builder) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_HistoryEndpoint_Builder_withAuthToken`. The `builder` handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_HistoryEndpoint_Builder_withAuthToken,
                                           static_cast<dxfg_history_endpoint_builder_t *>(builder.get()),
                                           authToken.c_str());
}

void withCompression(const JavaObjectHandle<HistoryEndpoint::Builder> &builder,
                     HistoryEndpoint::Compression compression) {
    if (!builder) {
        throw InvalidArgumentException("Unable to execute function `dxfg_HistoryEndpoint_Builder_withCompression`. The "
                                       "`builder` handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_HistoryEndpoint_Builder_withCompression,
                                           static_cast<dxfg_history_endpoint_builder_t *>(builder.get()),
                                           static_cast<dxfg_history_endpoint_compression_t>(compression));
}

void withFormat(const JavaObjectHandle<HistoryEndpoint::Builder> &builder, HistoryEndpoint::Format format) {
    if (!builder) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_HistoryEndpoint_Builder_withFormat`. The `builder` handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_HistoryEndpoint_Builder_withFormat,
                                           static_cast<dxfg_history_endpoint_builder_t *>(builder.get()),
                                           static_cast<dxfg_history_endpoint_format_t>(format));
}

JavaObjectHandle<HistoryEndpoint> build(const JavaObjectHandle<HistoryEndpoint::Builder> &builder) {
    if (!builder) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_HistoryEndpoint_Builder_build`. The `builder` handle is invalid");
    }

    dxfg_history_endpoint_t *endpoint{};

    runGraalFunctionAndThrowIfLessThanZero(dxfg_HistoryEndpoint_Builder_build,
                                           static_cast<dxfg_history_endpoint_builder_t *>(builder.get()), &endpoint);

    return JavaObjectHandle<HistoryEndpoint>{&endpoint->handler};
}

} // namespace Builder

JavaObjectHandle<HistoryEndpoint::Builder> newBuilder() {
    dxfg_history_endpoint_builder_t *builder{};

    runGraalFunctionAndThrowIfLessThanZero(dxfg_HistoryEndpoint_newBuilder, &builder);

    return JavaObjectHandle<HistoryEndpoint::Builder>{&builder->handler};
}

std::vector<std::shared_ptr<EventType>> getTimeSeries(const JavaObjectHandle<HistoryEndpoint> &endpoint,
                                                      const EventTypeEnum &eventType, const SymbolWrapper &symbol,
                                                      std::int64_t fromTime, std::int64_t toTime) {
    dxfg_event_type_list *events{};

    runGraalFunctionAndThrowIfLessThanZero(
        dxfg_HistoryEndpoint_getTimeSeries, static_cast<dxfg_history_endpoint_t *>(endpoint.get()),
        static_cast<dxfg_event_clazz_t>(eventType.getId()), static_cast<dxfg_symbol_t *>(symbol.toGraalUnique().get()),
        fromTime, toTime, &events);

    const auto u = event::IsolatedEventTypeList::toUnique(events);

    return EventMapper::fromGraalList(u.get());
}

} // namespace isolated::candlewebservice::IsolatedHistoryEndpoint

DXFCPP_END_NAMESPACE
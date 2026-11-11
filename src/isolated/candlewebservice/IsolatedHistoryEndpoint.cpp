// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "IsolatedHistoryEndpoint.hpp"

#include "../../../include/dxfeed_graal_cpp_api/event/EventMapper.hpp"
#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../IsolatedCommon.hpp"
#include "../event/IsolatedEventType.hpp"

#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::candlewebservice::IsolatedHistoryEndpoint {

namespace Builder {

/// dxfg_v4_HistoryEndpoint_Builder_withAddress
void withAddress(const JavaObjectHandle<HistoryEndpoint::Builder> &builder, std::string_view address) {
    if (!builder) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_HistoryEndpoint_Builder_withAddress`. The "
                                       "`builder` handle is invalid");
    }

    runGraalV4Function(dxfg_v4_HistoryEndpoint_Builder_withAddress,
                       static_cast<dxfg_v4_history_endpoint_builder_t *>(builder.get()),
                       SdkString{address, "HistoryEndpoint::Builder::withAddress(address)"}.c_str());
}

/// dxfg_v4_HistoryEndpoint_Builder_withUserName
void withUserName(const JavaObjectHandle<HistoryEndpoint::Builder> &builder, std::string_view userName) {
    if (!builder) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_HistoryEndpoint_Builder_withUserName`. The "
                                       "`builder` handle is invalid");
    }

    runGraalV4Function(dxfg_v4_HistoryEndpoint_Builder_withUserName,
                       static_cast<dxfg_v4_history_endpoint_builder_t *>(builder.get()),
                       SdkString{userName, StringLabel::secretOf("HistoryEndpoint::Builder::withUserName(userName)")}
                           .c_str());
}

/// dxfg_v4_HistoryEndpoint_Builder_withPassword
void withPassword(const JavaObjectHandle<HistoryEndpoint::Builder> &builder, std::string_view password) {
    if (!builder) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_HistoryEndpoint_Builder_withPassword`. The "
                                       "`builder` handle is invalid");
    }

    runGraalV4Function(dxfg_v4_HistoryEndpoint_Builder_withPassword,
                       static_cast<dxfg_v4_history_endpoint_builder_t *>(builder.get()),
                       SdkString{password, StringLabel::secretOf("HistoryEndpoint::Builder::withPassword(password)")}
                           .c_str());
}

/// dxfg_v4_HistoryEndpoint_Builder_withAuthToken
void withAuthToken(const JavaObjectHandle<HistoryEndpoint::Builder> &builder, std::string_view authToken) {
    if (!builder) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_HistoryEndpoint_Builder_withAuthToken`. "
                                       "The `builder` handle is invalid");
    }

    runGraalV4Function(dxfg_v4_HistoryEndpoint_Builder_withAuthToken,
                       static_cast<dxfg_v4_history_endpoint_builder_t *>(builder.get()),
                       SdkString{authToken, StringLabel::secretOf("HistoryEndpoint::Builder::withAuthToken(authToken)")}
                           .c_str());
}

/// dxfg_v4_HistoryEndpoint_Builder_withCompression
void withCompression(const JavaObjectHandle<HistoryEndpoint::Builder> &builder,
                     HistoryEndpoint::Compression compression) {
    if (!builder) {
        throw InvalidArgumentException("Unable to execute function "
                                       "`dxfg_v4_HistoryEndpoint_Builder_withCompression`. The `builder` handle is "
                                       "invalid");
    }

    runGraalV4Function(dxfg_v4_HistoryEndpoint_Builder_withCompression,
                       static_cast<dxfg_v4_history_endpoint_builder_t *>(builder.get()),
                       static_cast<dxfg_v4_history_endpoint_compression_t>(compression));
}

/// dxfg_v4_HistoryEndpoint_Builder_withFormat
void withFormat(const JavaObjectHandle<HistoryEndpoint::Builder> &builder, HistoryEndpoint::Format format) {
    if (!builder) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_HistoryEndpoint_Builder_withFormat`. The `builder` handle is invalid");
    }

    runGraalV4Function(dxfg_v4_HistoryEndpoint_Builder_withFormat,
                       static_cast<dxfg_v4_history_endpoint_builder_t *>(builder.get()),
                       static_cast<dxfg_v4_history_endpoint_format_t>(format));
}

/// dxfg_v4_HistoryEndpoint_Builder_build
JavaObjectHandle<HistoryEndpoint> build(const JavaObjectHandle<HistoryEndpoint::Builder> &builder) {
    if (!builder) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_v4_HistoryEndpoint_Builder_build`. The `builder` handle is invalid");
    }

    // The handle of the endpoint is taken from the result, so the NULL result is an error.
    auto *endpoint = runGraalV4FunctionWithNonNullResult(
        dxfg_v4_HistoryEndpoint_Builder_build, static_cast<dxfg_v4_history_endpoint_builder_t *>(builder.get()));

    return JavaObjectHandle<HistoryEndpoint>{&endpoint->handler};
}

} // namespace Builder

/// dxfg_v4_HistoryEndpoint_newBuilder
JavaObjectHandle<HistoryEndpoint::Builder> newBuilder() {
    // The handle of the builder is taken from the result, so the NULL result is an error.
    auto *builder = runGraalV4FunctionWithNonNullResult(dxfg_v4_HistoryEndpoint_newBuilder);

    return JavaObjectHandle<HistoryEndpoint::Builder>{&builder->handler};
}

/// dxfg_v4_HistoryEndpoint_getTimeSeries
std::vector<std::shared_ptr<EventType>> getTimeSeries(const JavaObjectHandle<HistoryEndpoint> &endpoint,
                                                      const EventTypeEnum &eventType, const SymbolWrapper &symbol,
                                                      std::int64_t fromTime, std::int64_t toTime) {
    auto *events = runGraalV4FunctionWithResult(
        dxfg_v4_HistoryEndpoint_getTimeSeries, static_cast<dxfg_v4_history_endpoint_t *>(endpoint.get()),
        static_cast<dxfg_v4_event_clazz_t>(eventType.getId()),
        static_cast<dxfg_v4_symbol_t *>(symbol.toGraalUnique().get()), fromTime, toTime);

    const auto u = event::IsolatedEventTypeList::toUnique(events);

    return EventMapper::fromGraalList(u.get());
}

} // namespace isolated::candlewebservice::IsolatedHistoryEndpoint

DXFCPP_END_NAMESPACE
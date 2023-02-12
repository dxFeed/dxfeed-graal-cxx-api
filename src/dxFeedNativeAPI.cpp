// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxFeedNativeCAPI/dxFeedNativeCAPI.h>
#include <dxFeedNativeCppAPI/dxFeedNativeCppAPI.hpp>

#include <cstring>
#include <utf8cpp/utf8.h>
#include <utility>

namespace dxfcpp {

namespace detail {

const CEntryPointErrors CEntryPointErrors::NO_ERROR(0, "No error occurred.");
const CEntryPointErrors CEntryPointErrors::UNSPECIFIED(1, "An unspecified error occurred.");
const CEntryPointErrors CEntryPointErrors::NULL_ARGUMENT(2, "An argument was NULL (nullptr).");
const CEntryPointErrors CEntryPointErrors::UNATTACHED_THREAD(4, "The specified thread is not attached to the isolate.");
const CEntryPointErrors CEntryPointErrors::UNINITIALIZED_ISOLATE(5, "The specified isolate is unknown.");
const CEntryPointErrors CEntryPointErrors::LOCATE_IMAGE_FAILED(6, "Locating the image file failed.");
const CEntryPointErrors CEntryPointErrors::OPEN_IMAGE_FAILED(7, "Opening the located image file failed.");
const CEntryPointErrors CEntryPointErrors::MAP_HEAP_FAILED(8,
                                                           "Mapping the heap from the image file into memory failed.");
const CEntryPointErrors
    CEntryPointErrors::RESERVE_ADDRESS_SPACE_FAILED(801, "Reserving address space for the new isolate failed.");
const CEntryPointErrors
    CEntryPointErrors::INSUFFICIENT_ADDRESS_SPACE(802, "The image heap does not fit in the available address space.");
const CEntryPointErrors CEntryPointErrors::PROTECT_HEAP_FAILED(9, "Setting the protection of the heap memory failed.");
const CEntryPointErrors CEntryPointErrors::UNSUPPORTED_ISOLATE_PARAMETERS_VERSION(
    10, "The version of the specified isolate parameters is unsupported.");
const CEntryPointErrors
    CEntryPointErrors::THREADING_INITIALIZATION_FAILED(11, "Initialization of threading in the isolate failed.");
const CEntryPointErrors CEntryPointErrors::UNCAUGHT_EXCEPTION(12, "Some exception is not caught.");
const CEntryPointErrors CEntryPointErrors::ISOLATE_INITIALIZATION_FAILED(13, "Initialization the isolate failed.");
const CEntryPointErrors CEntryPointErrors::OPEN_AUX_IMAGE_FAILED(14,
                                                                 "Opening the located auxiliary image file failed.");
const CEntryPointErrors
    CEntryPointErrors::READ_AUX_IMAGE_META_FAILED(15, "Reading the opened auxiliary image file failed.");
const CEntryPointErrors CEntryPointErrors::MAP_AUX_IMAGE_FAILED(16,
                                                                "Mapping the auxiliary image file into memory failed.");
const CEntryPointErrors
    CEntryPointErrors::INSUFFICIENT_AUX_IMAGE_MEMORY(17, "Insufficient memory for the auxiliary image.");
const CEntryPointErrors
    CEntryPointErrors::AUX_IMAGE_UNSUPPORTED(18, "Auxiliary images are not supported on this platform or edition.");
const CEntryPointErrors CEntryPointErrors::FREE_ADDRESS_SPACE_FAILED(19,
                                                                     "Releasing the isolate's address space failed.");
const CEntryPointErrors CEntryPointErrors::FREE_IMAGE_HEAP_FAILED(20,
                                                                  "Releasing the isolate's image heap memory failed.");
const CEntryPointErrors CEntryPointErrors::AUX_IMAGE_PRIMARY_IMAGE_MISMATCH(
    21, "The auxiliary image was built from a different primary image.");
const CEntryPointErrors CEntryPointErrors::ARGUMENT_PARSING_FAILED(22, "The isolate arguments could not be parsed.");
const CEntryPointErrors CEntryPointErrors::CPU_FEATURE_CHECK_FAILED(
    23, "Current target does not support the following CPU features that are required by the image.");

const std::unordered_map<std::int32_t, CEntryPointErrors> CEntryPointErrors::ALL{
    {NO_ERROR.getCode(), NO_ERROR},
    {UNSPECIFIED.getCode(), UNSPECIFIED},
    {NULL_ARGUMENT.getCode(), NULL_ARGUMENT},
    {UNATTACHED_THREAD.getCode(), UNATTACHED_THREAD},
    {UNINITIALIZED_ISOLATE.getCode(), UNINITIALIZED_ISOLATE},
    {LOCATE_IMAGE_FAILED.getCode(), LOCATE_IMAGE_FAILED},
    {OPEN_IMAGE_FAILED.getCode(), OPEN_IMAGE_FAILED},
    {MAP_HEAP_FAILED.getCode(), MAP_HEAP_FAILED},
    {RESERVE_ADDRESS_SPACE_FAILED.getCode(), RESERVE_ADDRESS_SPACE_FAILED},
    {INSUFFICIENT_ADDRESS_SPACE.getCode(), INSUFFICIENT_ADDRESS_SPACE},
    {PROTECT_HEAP_FAILED.getCode(), PROTECT_HEAP_FAILED},
    {UNSUPPORTED_ISOLATE_PARAMETERS_VERSION.getCode(), UNSUPPORTED_ISOLATE_PARAMETERS_VERSION},
    {THREADING_INITIALIZATION_FAILED.getCode(), THREADING_INITIALIZATION_FAILED},
    {UNCAUGHT_EXCEPTION.getCode(), UNCAUGHT_EXCEPTION},
    {ISOLATE_INITIALIZATION_FAILED.getCode(), ISOLATE_INITIALIZATION_FAILED},
    {OPEN_AUX_IMAGE_FAILED.getCode(), OPEN_AUX_IMAGE_FAILED},
    {READ_AUX_IMAGE_META_FAILED.getCode(), READ_AUX_IMAGE_META_FAILED},
    {MAP_AUX_IMAGE_FAILED.getCode(), MAP_AUX_IMAGE_FAILED},
    {INSUFFICIENT_AUX_IMAGE_MEMORY.getCode(), INSUFFICIENT_AUX_IMAGE_MEMORY},
    {AUX_IMAGE_UNSUPPORTED.getCode(), AUX_IMAGE_UNSUPPORTED},
    {FREE_ADDRESS_SPACE_FAILED.getCode(), FREE_ADDRESS_SPACE_FAILED},
    {FREE_IMAGE_HEAP_FAILED.getCode(), FREE_IMAGE_HEAP_FAILED},
    {AUX_IMAGE_PRIMARY_IMAGE_MISMATCH.getCode(), AUX_IMAGE_PRIMARY_IMAGE_MISMATCH},
    {ARGUMENT_PARSING_FAILED.getCode(), ARGUMENT_PARSING_FAILED},
    {CPU_FEATURE_CHECK_FAILED.getCode(), CPU_FEATURE_CHECK_FAILED},
};

thread_local Isolate::IsolateThread Isolate::currentIsolateThread_{};

const auto I = Isolate::getInstance();
} // namespace detail

const std::string DXEndpoint::NAME_PROPERTY = "name";
const std::string DXEndpoint::DXFEED_PROPERTIES_PROPERTY = "dxfeed.properties";
const std::string DXEndpoint::DXFEED_ADDRESS_PROPERTY = "dxfeed.address";
const std::string DXEndpoint::DXFEED_USER_PROPERTY = "dxfeed.user";
const std::string DXEndpoint::DXFEED_PASSWORD_PROPERTY = "dxfeed.password";
const std::string DXEndpoint::DXFEED_THREAD_POOL_SIZE_PROPERTY = "dxfeed.threadPoolSize";
const std::string DXEndpoint::DXFEED_AGGREGATION_PERIOD_PROPERTY = "dxfeed.aggregationPeriod";
const std::string DXEndpoint::DXFEED_WILDCARD_ENABLE_PROPERTY = "dxfeed.wildcard.enable";
const std::string DXEndpoint::DXPUBLISHER_PROPERTIES_PROPERTY = "dxpublisher.properties";
const std::string DXEndpoint::DXPUBLISHER_ADDRESS_PROPERTY = "dxpublisher.address";
const std::string DXEndpoint::DXPUBLISHER_THREAD_POOL_SIZE_PROPERTY = "dxpublisher.threadPoolSize";
const std::string DXEndpoint::DXENDPOINT_EVENT_TIME_PROPERTY = "dxendpoint.eventTime";
const std::string DXEndpoint::DXENDPOINT_STORE_EVERYTHING_PROPERTY = "dxendpoint.storeEverything";
const std::string DXEndpoint::DXSCHEME_NANO_TIME_PROPERTY = "dxscheme.nanoTime";
const std::string DXEndpoint::DXSCHEME_ENABLED_PROPERTY_PREFIX = "dxscheme.enabled.";

std::unordered_map<DXEndpoint::Role, std::shared_ptr<DXEndpoint>> DXEndpoint::INSTANCES{};

const EventTypeEnum EventTypeEnum::QUOTE{DXFG_EVENT_QUOTE, "QUOTE", true};
const EventTypeEnum EventTypeEnum::PROFILE{DXFG_EVENT_PROFILE, "PROFILE", true};
const EventTypeEnum EventTypeEnum::SUMMARY{DXFG_EVENT_SUMMARY, "SUMMARY", true};
const EventTypeEnum EventTypeEnum::GREEKS{DXFG_EVENT_GREEKS, "GREEKS", true, true, true};
const EventTypeEnum EventTypeEnum::CANDLE{DXFG_EVENT_CANDLE, "CANDLE", true, true, true};

// const EventTypeEnum EventTypeEnum::DAILY_CANDLE{DXFG_EVENT_DAILY_CANDLE, "DAILY_CANDLE", true, true, true};

const EventTypeEnum EventTypeEnum::UNDERLYING{DXFG_EVENT_UNDERLYING, "UNDERLYING", true, true, true};
const EventTypeEnum EventTypeEnum::THEO_PRICE{DXFG_EVENT_THEO_PRICE, "THEO_PRICE", true, true, true};
const EventTypeEnum EventTypeEnum::TRADE{DXFG_EVENT_TRADE, "TRADE", true};
const EventTypeEnum EventTypeEnum::TRADE_ETH{DXFG_EVENT_TRADE_ETH, "TRADE_ETH", true};
const EventTypeEnum EventTypeEnum::CONFIGURATION{DXFG_EVENT_CONFIGURATION, "CONFIGURATION", true};
const EventTypeEnum EventTypeEnum::MESSAGE{DXFG_EVENT_MESSAGE, "MESSAGE", false};
const EventTypeEnum EventTypeEnum::TIME_AND_SALE{DXFG_EVENT_TIME_AND_SALE, "TIME_AND_SALE", false, true, true};

// const EventTypeEnum EventTypeEnum::ORDER_BASE{DXFG_EVENT_ORDER_BASE, "ORDER_BASE", false, true};

const EventTypeEnum EventTypeEnum::ORDER{DXFG_EVENT_ORDER, "ORDER", false, true};
const EventTypeEnum EventTypeEnum::ANALYTIC_ORDER{DXFG_EVENT_ANALYTIC_ORDER, "ANALYTIC_ORDER", false, true};
const EventTypeEnum EventTypeEnum::SPREAD_ORDER{DXFG_EVENT_SPREAD_ORDER, "SPREAD_ORDER", false, true};
const EventTypeEnum EventTypeEnum::SERIES{DXFG_EVENT_SERIES, "SERIES", false, true};

const IndexedEventSource IndexedEventSource::DEFAULT{0, "DEFAULT"};

const EventFlag EventFlag::TX_PENDING{0x01u, "TX_PENDING"};
const EventFlag EventFlag::REMOVE_EVENT{0x02u, "REMOVE_EVENT"};
const EventFlag EventFlag::SNAPSHOT_BEGIN{0x04u, "SNAPSHOT_BEGIN"};
const EventFlag EventFlag::SNAPSHOT_END{0x08u, "SNAPSHOT_END"};
const EventFlag EventFlag::SNAPSHOT_SNIP{0x10u, "SNAPSHOT_SNIP"};
// 0x20 is reserved. This flag will fit into 1-byte on the wire in QTP protocol
const EventFlag EventFlag::SNAPSHOT_MODE{0x40u, "SNAPSHOT_MODE"};
const EventFlag EventFlag::REMOVE_SYMBOL{0x80u, "REMOVE_SYMBOL"};

const EventFlag IndexedEvent::TX_PENDING = EventFlag::TX_PENDING;
const EventFlag IndexedEvent::REMOVE_EVENT = EventFlag::REMOVE_EVENT;
const EventFlag IndexedEvent::SNAPSHOT_BEGIN = EventFlag::SNAPSHOT_BEGIN;
const EventFlag IndexedEvent::SNAPSHOT_END = EventFlag::SNAPSHOT_END;
const EventFlag IndexedEvent::SNAPSHOT_SNIP = EventFlag::SNAPSHOT_SNIP;
const EventFlag IndexedEvent::SNAPSHOT_MODE = EventFlag::SNAPSHOT_MODE;

std::shared_ptr<DXFeed> DXFeed::getInstance() noexcept {
    if constexpr (detail::isDebug) {
        detail::debug("DXFeed::getInstance()");
    }

    return DXEndpoint::getInstance()->getFeed();
}

void DXFeed::attachSubscription(std::shared_ptr<DXFeedSubscription> subscription) noexcept {
    if constexpr (detail::isDebug) {
        detail::debug("{}::attachSubscription({})", toString(), subscription->toString());
    }

    std::lock_guard guard(mtx_);

    if (!handle_ || !subscription || !subscription->handle_) {
        return;
    }

    if (detail::runIsolatedOrElse(
            [this, subHandle = subscription->handle_](auto threadHandle) {
                return dxfg_DXFeed_attachSubscription(threadHandle, handle_, subHandle) == 0;
            },
            false)) {

        subscriptions_.emplace(subscription);
    }
}

void DXFeed::detachSubscription(std::shared_ptr<DXFeedSubscription> subscription) noexcept {
    if constexpr (detail::isDebug) {
        detail::debug("{}::detachSubscription({})", toString(), subscription->toString());
    }

    std::lock_guard guard(mtx_);

    if (!handle_ || !subscription || !subscription->handle_) {
        return;
    }

    if (detail::runIsolatedOrElse(
            [this, subHandle = subscription->handle_](auto threadHandle) {
                return dxfg_DXFeed_detachSubscription(threadHandle, handle_, subHandle) == 0;
            },
            false)) {

        subscriptions_.erase(subscription);
    }
}

void DXFeed::detachSubscriptionAndClear(std::shared_ptr<DXFeedSubscription> subscription) noexcept {
    if constexpr (detail::isDebug) {
        detail::debug("{}::detachSubscriptionAndClear({})", toString(), subscription->toString());
    }

    std::lock_guard guard(mtx_);

    if (!handle_ || !subscription || !subscription->handle_) {
        return;
    }

    if (detail::runIsolatedOrElse(
            [this, subHandle = subscription->handle_](auto threadHandle) {
                return dxfg_DXFeed_detachSubscriptionAndClear(threadHandle, handle_, subHandle) == 0;
            },
            false)) {

        subscriptions_.erase(subscription);
    }
}

std::shared_ptr<DXFeedSubscription> DXFeed::createSubscription(const EventTypeEnum &eventType) noexcept {
    if constexpr (detail::isDebug) {
        detail::debug("{}::createSubscription(eventType = {})", toString(), eventType.getName());
    }

    auto sub = DXFeedSubscription::create(eventType);

    attachSubscription(sub);

    return sub;
}

std::shared_ptr<DXFeedSubscription>
DXFeed::createSubscription(std::initializer_list<EventTypeEnum> eventTypes) noexcept {
    if constexpr (detail::isDebug) {
        detail::debug("{}::createSubscription(eventTypes = {})", toString(),
                      detail::namesToString(eventTypes.begin(), eventTypes.end()));
    }

    auto sub = DXFeedSubscription::create(eventTypes);

    attachSubscription(sub);

    return sub;
}

void DXFeedSubscription::attach(std::shared_ptr<DXFeed> feed) noexcept {
    if constexpr (detail::isDebug) {
        detail::debug("{}::attach(feed = {})", toString(), feed->toString());
    }

    feed->attachSubscription(shared_from_this());
}

void DXFeedSubscription::detach(std::shared_ptr<DXFeed> feed) noexcept {
    if constexpr (detail::isDebug) {
        detail::debug("{}::detach(feed = {})", toString(), feed->toString());
    }

    feed->detachSubscription(shared_from_this());
}

} // namespace dxfcpp

dxfc_error_code_t dxfc_system_set_property(const char *key, const char *value) {
    return dxfcpp::System::setProperty(key, value) ? DXFC_EC_SUCCESS : DXFC_EC_ERROR;
}

dxfc_error_code_t dxfc_system_get_property(const char *key, char *buffer, size_t bufferSize) {
    auto value = dxfcpp::System::getProperty(key);

    if (buffer != nullptr && bufferSize > 0ULL) {
        auto size = std::min(value.size(), bufferSize - 1);

        buffer[size] = '\0';

        if (size == 0ULL) {
            return DXFC_EC_SUCCESS;
        }

        memcpy(buffer, value.c_str(), size);

        auto begin = buffer;
        auto end = buffer + size;

        // Looks for the first invalid UTF-8 character.
        auto found = utf8::find_invalid(begin, end);

        // An invalid character position was found. Trim the result string buffer to this invalid character.
        if (found != end) {
            *found = '\0';
        }
    }

    return DXFC_EC_SUCCESS;
}

namespace dxfcpp::detail {

struct BuilderHandle {};

struct BuilderRegistry {
    static std::unordered_map<BuilderHandle *, std::shared_ptr<DXEndpoint::Builder>> builders;

    static std::shared_ptr<DXEndpoint::Builder> get(BuilderHandle *handle) {
        if (builders.contains(handle)) {
            return builders[handle];
        }

        return {};
    }

    static BuilderHandle *add(std::shared_ptr<DXEndpoint::Builder> builder) noexcept {
        auto handle = new (std::nothrow) BuilderHandle{};

        builders[handle] = std::move(builder);

        return handle;
    }

    static bool remove(BuilderHandle *handle) {
        if (!handle) {
            return false;
        }

        auto result = builders.erase(handle) == 1;

        if (result) {
            delete handle;
        }

        return result;
    }
};

std::unordered_map<BuilderHandle *, std::shared_ptr<DXEndpoint::Builder>> BuilderRegistry::builders{};

struct EndpointWrapperHandle {};

struct EndpointWrapper : std::enable_shared_from_this<EndpointWrapper> {
    std::shared_ptr<dxfcpp::DXEndpoint> endpoint{};
    void *userData{};
    std::unordered_map<dxfc_dxendpoint_state_change_listener, std::size_t> listeners{};

    EndpointWrapper(std::shared_ptr<dxfcpp::DXEndpoint> endpoint, void *userData)
        : endpoint{std::move(endpoint)}, userData{userData}, listeners{} {}
};

struct EndpointWrapperRegistry {
    static std::unordered_map<EndpointWrapperHandle *, std::shared_ptr<EndpointWrapper>> endpointWrappers;

    static std::shared_ptr<EndpointWrapper> get(EndpointWrapperHandle *handle) {
        if (endpointWrappers.contains(handle)) {
            return endpointWrappers[handle];
        }

        return {};
    }

    static EndpointWrapperHandle *add(std::shared_ptr<EndpointWrapper> endpointWrapper) noexcept {
        auto handle = new (std::nothrow) EndpointWrapperHandle{};

        endpointWrappers[handle] = std::move(endpointWrapper);

        return handle;
    }

    static bool remove(EndpointWrapperHandle *handle) {
        if (!handle) {
            return false;
        }

        auto result = endpointWrappers.erase(handle) == 1;

        if (result) {
            delete handle;
        }

        return result;
    }
};

std::unordered_map<EndpointWrapperHandle *, std::shared_ptr<EndpointWrapper>>
    EndpointWrapperRegistry::endpointWrappers{};

static dxfcpp::DXEndpoint::Role cApiRoleToRole(dxfc_dxendpoint_role_t role) {
    switch (role) {
    case DXFC_DXENDPOINT_ROLE_FEED:
        return dxfcpp::DXEndpoint::Role::FEED;
    case DXFC_DXENDPOINT_ROLE_ON_DEMAND_FEED:
        return dxfcpp::DXEndpoint::Role::ON_DEMAND_FEED;
    case DXFC_DXENDPOINT_ROLE_STREAM_FEED:
        return dxfcpp::DXEndpoint::Role::STREAM_FEED;
    case DXFC_DXENDPOINT_ROLE_PUBLISHER:
        return dxfcpp::DXEndpoint::Role::PUBLISHER;
    case DXFC_DXENDPOINT_ROLE_STREAM_PUBLISHER:
        return dxfcpp::DXEndpoint::Role::STREAM_PUBLISHER;
    case DXFC_DXENDPOINT_ROLE_LOCAL_HUB:
        return dxfcpp::DXEndpoint::Role::LOCAL_HUB;
    }

    return dxfcpp::DXEndpoint::Role::FEED;
}

static dxfc_dxendpoint_role_t roleToCApiRole(dxfcpp::DXEndpoint::Role role) {
    switch (role) {
    case DXEndpoint::Role::FEED:
        return DXFC_DXENDPOINT_ROLE_FEED;
    case DXEndpoint::Role::ON_DEMAND_FEED:
        return DXFC_DXENDPOINT_ROLE_ON_DEMAND_FEED;
    case DXEndpoint::Role::STREAM_FEED:
        return DXFC_DXENDPOINT_ROLE_STREAM_FEED;
    case DXEndpoint::Role::PUBLISHER:
        return DXFC_DXENDPOINT_ROLE_PUBLISHER;
    case DXEndpoint::Role::STREAM_PUBLISHER:
        return DXFC_DXENDPOINT_ROLE_STREAM_PUBLISHER;
    case DXEndpoint::Role::LOCAL_HUB:
        return DXFC_DXENDPOINT_ROLE_LOCAL_HUB;
    }

    return DXFC_DXENDPOINT_ROLE_FEED;
}

static dxfc_dxendpoint_state_t stateToCApiState(dxfcpp::DXEndpoint::State state) {
    switch (state) {
    case DXEndpoint::State::NOT_CONNECTED:
        return DXFC_DXENDPOINT_STATE_NOT_CONNECTED;
    case DXEndpoint::State::CONNECTING:
        return DXFC_DXENDPOINT_STATE_CONNECTING;
    case DXEndpoint::State::CONNECTED:
        return DXFC_DXENDPOINT_STATE_CONNECTED;
    case DXEndpoint::State::CLOSED:
        return DXFC_DXENDPOINT_STATE_CLOSED;
    }

    return DXFC_DXENDPOINT_STATE_NOT_CONNECTED;
}

} // namespace dxfcpp::detail

dxfc_error_code_t dxfc_dxendpoint_new_builder(DXFC_OUT dxfc_dxendpoint_builder_t *builderHandle) {
    if (!builderHandle) {
        return DXFC_EC_ERROR;
    }

    auto result = dxfcpp::detail::BuilderRegistry::add(dxfcpp::DXEndpoint::newBuilder());

    if (!result) {
        return DXFC_EC_ERROR;
    }

    *builderHandle = dxfcpp::detail::bit_cast<dxfc_dxendpoint_builder_t>(result);

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_builder_with_role(dxfc_dxendpoint_builder_t builderHandle,
                                                    dxfc_dxendpoint_role_t role) {
    if (!builderHandle) {
        return DXFC_EC_ERROR;
    }

    auto builder =
        dxfcpp::detail::BuilderRegistry::get(dxfcpp::detail::bit_cast<dxfcpp::detail::BuilderHandle *>(builderHandle));

    if (!builder) {
        return DXFC_EC_ERROR;
    }

    builder->withRole(dxfcpp::detail::cApiRoleToRole(role));

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_builder_with_name(dxfc_dxendpoint_builder_t builderHandle, const char *name) {
    if (!builderHandle || !name) {
        return DXFC_EC_ERROR;
    }

    auto builder =
        dxfcpp::detail::BuilderRegistry::get(dxfcpp::detail::bit_cast<dxfcpp::detail::BuilderHandle *>(builderHandle));

    if (!builder) {
        return DXFC_EC_ERROR;
    }

    builder->withName(name);

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_builder_with_property(dxfc_dxendpoint_builder_t builderHandle, const char *key,
                                                        const char *value) {
    if (!builderHandle || !key || !value) {
        return DXFC_EC_ERROR;
    }

    auto builder =
        dxfcpp::detail::BuilderRegistry::get(dxfcpp::detail::bit_cast<dxfcpp::detail::BuilderHandle *>(builderHandle));

    if (!builder) {
        return DXFC_EC_ERROR;
    }

    builder->withProperty(key, value);

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_builder_with_properties(dxfc_dxendpoint_builder_t builderHandle,
                                                          const dxfc_dxendpoint_property_t **properties, size_t size) {
    if (!builderHandle || !properties || size == 0) {
        return DXFC_EC_ERROR;
    }

    auto builder =
        dxfcpp::detail::BuilderRegistry::get(dxfcpp::detail::bit_cast<dxfcpp::detail::BuilderHandle *>(builderHandle));

    if (!builder) {
        return DXFC_EC_ERROR;
    }

    for (std::size_t i = 0; i < size; i++) {
        if (!properties[i]) {
            continue;
        }

        if (!properties[i]->key || !properties[i]->value) {
            continue;
        }

        builder->withProperty(properties[i]->key, properties[i]->value);
    }

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_builder_supports_property(dxfc_dxendpoint_builder_t builderHandle, const char *key,
                                                            DXFC_OUT int *supports) {
    if (!builderHandle || !key || !supports) {
        return DXFC_EC_ERROR;
    }

    auto builder =
        dxfcpp::detail::BuilderRegistry::get(dxfcpp::detail::bit_cast<dxfcpp::detail::BuilderHandle *>(builderHandle));

    if (!builder) {
        return DXFC_EC_ERROR;
    }

    *supports = builder->supportsProperty(key);

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_builder_build(dxfc_dxendpoint_builder_t builderHandle, void *userData,
                                                DXFC_OUT dxfc_dxendpoint_t *endpointHandle) {
    if (!builderHandle || !endpointHandle) {
        return DXFC_EC_ERROR;
    }

    auto builder =
        dxfcpp::detail::BuilderRegistry::get(dxfcpp::detail::bit_cast<dxfcpp::detail::BuilderHandle *>(builderHandle));

    if (!builder) {
        return DXFC_EC_ERROR;
    }

    auto endpoint = builder->build();

    if (!endpoint) {
        return DXFC_EC_ERROR;
    }

    auto result = dxfcpp::detail::EndpointWrapperRegistry::add(
        std::make_shared<dxfcpp::detail::EndpointWrapper>(endpoint, userData));

    if (!result) {
        return DXFC_EC_ERROR;
    }

    *endpointHandle = dxfcpp::detail::bit_cast<dxfc_dxendpoint_t>(result);

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_builder_free(dxfc_dxendpoint_builder_t builderHandle) {
    if (!builderHandle) {
        return DXFC_EC_ERROR;
    }

    if (!dxfcpp::detail::BuilderRegistry::remove(
            dxfcpp::detail::bit_cast<dxfcpp::detail::BuilderHandle *>(builderHandle))) {
        return DXFC_EC_ERROR;
    }

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_get_instance(void *userData, DXFC_OUT dxfc_dxendpoint_t *endpointHandle) {
    if (!endpointHandle) {
        return DXFC_EC_ERROR;
    }

    auto endpoint = dxfcpp::DXEndpoint::getInstance();

    if (!endpoint) {
        return DXFC_EC_ERROR;
    }

    auto result = dxfcpp::detail::EndpointWrapperRegistry::add(
        std::make_shared<dxfcpp::detail::EndpointWrapper>(endpoint, userData));

    if (!result) {
        return DXFC_EC_ERROR;
    }

    *endpointHandle = dxfcpp::detail::bit_cast<dxfc_dxendpoint_t>(result);

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_get_instance2(dxfc_dxendpoint_role_t role, void *userData,
                                                DXFC_OUT dxfc_dxendpoint_t *endpointHandle) {
    if (!endpointHandle) {
        return DXFC_EC_ERROR;
    }

    auto endpoint = dxfcpp::DXEndpoint::getInstance(dxfcpp::detail::cApiRoleToRole(role));

    if (!endpoint) {
        return DXFC_EC_ERROR;
    }

    auto result = dxfcpp::detail::EndpointWrapperRegistry::add(
        std::make_shared<dxfcpp::detail::EndpointWrapper>(endpoint, userData));

    if (!result) {
        return DXFC_EC_ERROR;
    }

    *endpointHandle = dxfcpp::detail::bit_cast<dxfc_dxendpoint_t>(result);

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_create(void *userData, DXFC_OUT dxfc_dxendpoint_t *endpointHandle) {
    if (!endpointHandle) {
        return DXFC_EC_ERROR;
    }

    auto endpoint = dxfcpp::DXEndpoint::create();

    if (!endpoint) {
        return DXFC_EC_ERROR;
    }

    auto result = dxfcpp::detail::EndpointWrapperRegistry::add(
        std::make_shared<dxfcpp::detail::EndpointWrapper>(endpoint, userData));

    if (!result) {
        return DXFC_EC_ERROR;
    }

    *endpointHandle = dxfcpp::detail::bit_cast<dxfc_dxendpoint_t>(result);

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_create2(dxfc_dxendpoint_role_t role, void *userData,
                                          DXFC_OUT dxfc_dxendpoint_t *endpointHandle) {
    if (!endpointHandle) {
        return DXFC_EC_ERROR;
    }

    auto endpoint = dxfcpp::DXEndpoint::create(dxfcpp::detail::cApiRoleToRole(role));

    if (!endpoint) {
        return DXFC_EC_ERROR;
    }

    auto result = dxfcpp::detail::EndpointWrapperRegistry::add(
        std::make_shared<dxfcpp::detail::EndpointWrapper>(endpoint, userData));

    if (!result) {
        return DXFC_EC_ERROR;
    }

    *endpointHandle = dxfcpp::detail::bit_cast<dxfc_dxendpoint_t>(result);

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_close(dxfc_dxendpoint_t endpointHandle) {
    if (!endpointHandle) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper = dxfcpp::detail::EndpointWrapperRegistry::get(
        dxfcpp::detail::bit_cast<dxfcpp::detail::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    endpointWrapper->endpoint->close();

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_close_and_await_termination(dxfc_dxendpoint_t endpointHandle) {
    if (!endpointHandle) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper = dxfcpp::detail::EndpointWrapperRegistry::get(
        dxfcpp::detail::bit_cast<dxfcpp::detail::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    endpointWrapper->endpoint->closeAndAwaitTermination();

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_get_role(dxfc_dxendpoint_t endpointHandle, DXFC_OUT dxfc_dxendpoint_role_t *role) {
    if (!endpointHandle || !role) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper = dxfcpp::detail::EndpointWrapperRegistry::get(
        dxfcpp::detail::bit_cast<dxfcpp::detail::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    *role = dxfcpp::detail::roleToCApiRole(endpointWrapper->endpoint->getRole());

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_user(dxfc_dxendpoint_t endpointHandle, const char *user) {
    if (!endpointHandle || !user) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper = dxfcpp::detail::EndpointWrapperRegistry::get(
        dxfcpp::detail::bit_cast<dxfcpp::detail::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    endpointWrapper->endpoint->user(user);

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_password(dxfc_dxendpoint_t endpointHandle, const char *password) {
    if (!endpointHandle || !password) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper = dxfcpp::detail::EndpointWrapperRegistry::get(
        dxfcpp::detail::bit_cast<dxfcpp::detail::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    endpointWrapper->endpoint->password(password);

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_connect(dxfc_dxendpoint_t endpointHandle, const char *address) {
    if (!endpointHandle || !address) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper = dxfcpp::detail::EndpointWrapperRegistry::get(
        dxfcpp::detail::bit_cast<dxfcpp::detail::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    endpointWrapper->endpoint->connect(address);

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_reconnect(dxfc_dxendpoint_t endpointHandle) {
    if (!endpointHandle) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper = dxfcpp::detail::EndpointWrapperRegistry::get(
        dxfcpp::detail::bit_cast<dxfcpp::detail::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    endpointWrapper->endpoint->reconnect();

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_disconnect(dxfc_dxendpoint_t endpointHandle) {
    if (!endpointHandle) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper = dxfcpp::detail::EndpointWrapperRegistry::get(
        dxfcpp::detail::bit_cast<dxfcpp::detail::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    endpointWrapper->endpoint->disconnect();

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_disconnect_and_clear(dxfc_dxendpoint_t endpointHandle) {
    if (!endpointHandle) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper = dxfcpp::detail::EndpointWrapperRegistry::get(
        dxfcpp::detail::bit_cast<dxfcpp::detail::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    endpointWrapper->endpoint->disconnectAndClear();

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_await_processed(dxfc_dxendpoint_t endpointHandle) {
    if (!endpointHandle) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper = dxfcpp::detail::EndpointWrapperRegistry::get(
        dxfcpp::detail::bit_cast<dxfcpp::detail::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    endpointWrapper->endpoint->awaitProcessed();

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_await_not_connected(dxfc_dxendpoint_t endpointHandle) {
    if (!endpointHandle) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper = dxfcpp::detail::EndpointWrapperRegistry::get(
        dxfcpp::detail::bit_cast<dxfcpp::detail::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    endpointWrapper->endpoint->awaitNotConnected();

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_get_state(dxfc_dxendpoint_t endpointHandle, DXFC_OUT dxfc_dxendpoint_state_t *state) {
    if (!endpointHandle || !state) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper = dxfcpp::detail::EndpointWrapperRegistry::get(
        dxfcpp::detail::bit_cast<dxfcpp::detail::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    *state = dxfcpp::detail::stateToCApiState(endpointWrapper->endpoint->getState());

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_add_state_change_listener(dxfc_dxendpoint_t endpointHandle,
                                                            dxfc_dxendpoint_state_change_listener listener) {
    if (!endpointHandle || !listener) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper = dxfcpp::detail::EndpointWrapperRegistry::get(
        dxfcpp::detail::bit_cast<dxfcpp::detail::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    if (endpointWrapper->listeners.contains(listener)) {
        return DXFC_EC_SUCCESS;
    }

    endpointWrapper->listeners[listener] = endpointWrapper->endpoint->onStateChange() +=
        [userData = endpointWrapper->userData, listener](dxfcpp::DXEndpoint::State oldState,
                                                         dxfcpp::DXEndpoint::State newState) {
            listener(dxfcpp::detail::stateToCApiState(oldState), dxfcpp::detail::stateToCApiState(newState), userData);
        };

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_remove_state_change_listener(dxfc_dxendpoint_t endpointHandle,
                                                               dxfc_dxendpoint_state_change_listener listener) {
    if (!endpointHandle || !listener) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper = dxfcpp::detail::EndpointWrapperRegistry::get(
        dxfcpp::detail::bit_cast<dxfcpp::detail::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    if (!endpointWrapper->listeners.contains(listener)) {
        return DXFC_EC_SUCCESS;
    }

    endpointWrapper->endpoint->onStateChange() -= endpointWrapper->listeners[listener];

    return DXFC_EC_SUCCESS;
}

// TODO: implement
dxfc_error_code_t dxfc_dxendpoint_get_feed(dxfc_dxendpoint_t endpointHandle, DXFC_OUT dxfc_dxfeed_t *feed) {
    return DXFC_EC_SUCCESS;
}

// TODO: implement
dxfc_error_code_t dxfc_dxendpoint_get_publisher(dxfc_dxendpoint_t endpointHandle,
                                                DXFC_OUT dxfc_dxpublisher_t *publisher) {
    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_free(dxfc_dxendpoint_t endpointHandle) {
    if (!endpointHandle) {
        return DXFC_EC_ERROR;
    }

    if (!dxfcpp::detail::EndpointWrapperRegistry::remove(
            dxfcpp::detail::bit_cast<dxfcpp::detail::EndpointWrapperHandle *>(endpointHandle))) {
        return DXFC_EC_ERROR;
    }

    return DXFC_EC_SUCCESS;
}
// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_native_c_api/api.h>
#include <dxfeed_graal_native_cpp_api/api.hpp>

#include <cstring>
#include <memory>
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

const std::unordered_map<CEntryPointErrors::CodeType, std::reference_wrapper<const CEntryPointErrors>>
    CEntryPointErrors::ALL{
        {NO_ERROR.getCode(), std::cref(NO_ERROR)},
        {UNSPECIFIED.getCode(), std::cref(UNSPECIFIED)},
        {NULL_ARGUMENT.getCode(), std::cref(NULL_ARGUMENT)},
        {UNATTACHED_THREAD.getCode(), std::cref(UNATTACHED_THREAD)},
        {UNINITIALIZED_ISOLATE.getCode(), std::cref(UNINITIALIZED_ISOLATE)},
        {LOCATE_IMAGE_FAILED.getCode(), std::cref(LOCATE_IMAGE_FAILED)},
        {OPEN_IMAGE_FAILED.getCode(), std::cref(OPEN_IMAGE_FAILED)},
        {MAP_HEAP_FAILED.getCode(), std::cref(MAP_HEAP_FAILED)},
        {RESERVE_ADDRESS_SPACE_FAILED.getCode(), std::cref(RESERVE_ADDRESS_SPACE_FAILED)},
        {INSUFFICIENT_ADDRESS_SPACE.getCode(), std::cref(INSUFFICIENT_ADDRESS_SPACE)},
        {PROTECT_HEAP_FAILED.getCode(), std::cref(PROTECT_HEAP_FAILED)},
        {UNSUPPORTED_ISOLATE_PARAMETERS_VERSION.getCode(), std::cref(UNSUPPORTED_ISOLATE_PARAMETERS_VERSION)},
        {THREADING_INITIALIZATION_FAILED.getCode(), std::cref(THREADING_INITIALIZATION_FAILED)},
        {UNCAUGHT_EXCEPTION.getCode(), std::cref(UNCAUGHT_EXCEPTION)},
        {ISOLATE_INITIALIZATION_FAILED.getCode(), std::cref(ISOLATE_INITIALIZATION_FAILED)},
        {OPEN_AUX_IMAGE_FAILED.getCode(), std::cref(OPEN_AUX_IMAGE_FAILED)},
        {READ_AUX_IMAGE_META_FAILED.getCode(), std::cref(READ_AUX_IMAGE_META_FAILED)},
        {MAP_AUX_IMAGE_FAILED.getCode(), std::cref(MAP_AUX_IMAGE_FAILED)},
        {INSUFFICIENT_AUX_IMAGE_MEMORY.getCode(), std::cref(INSUFFICIENT_AUX_IMAGE_MEMORY)},
        {AUX_IMAGE_UNSUPPORTED.getCode(), std::cref(AUX_IMAGE_UNSUPPORTED)},
        {FREE_ADDRESS_SPACE_FAILED.getCode(), std::cref(FREE_ADDRESS_SPACE_FAILED)},
        {FREE_IMAGE_HEAP_FAILED.getCode(), std::cref(FREE_IMAGE_HEAP_FAILED)},
        {AUX_IMAGE_PRIMARY_IMAGE_MISMATCH.getCode(), std::cref(AUX_IMAGE_PRIMARY_IMAGE_MISMATCH)},
        {ARGUMENT_PARSING_FAILED.getCode(), std::cref(ARGUMENT_PARSING_FAILED)},
        {CPU_FEATURE_CHECK_FAILED.getCode(), std::cref(CPU_FEATURE_CHECK_FAILED)},
    };

thread_local Isolate::IsolateThread Isolate::currentIsolateThread_{};

const auto I = Isolate::getInstance();

namespace handler_utils {
void JavaObjectHandler::deleter(void *handler) noexcept {
    runIsolatedOrElse(
        [handler = handler](auto threadHandle) {
            if (handler) {
                return dxfg_JavaObjectHandler_release(threadHandle,
                                                      detail::bit_cast<dxfg_java_object_handler *>(handler)) == 0;
            }

            return true;
        },
        false);
}

struct EventClassList::Impl {
    dxfg_event_clazz_list_t list_;

    Impl() noexcept : list_{0, nullptr} {}

    void set(std::size_t index, std::uint32_t id) const noexcept {
        if (list_.size == 0) {
            return;
        }

        if (index < list_.size) {
            *list_.elements[index] = static_cast<dxfg_event_clazz_t>(id);
        }
    }

    [[nodiscard]] bool isEmpty() const noexcept { return list_.size == 0; }

    [[nodiscard]] std::size_t size() const noexcept { return static_cast<std::size_t>(list_.size); }

    void *getHandler() noexcept { return detail::bit_cast<void *>(&list_); }

    void init(std::uint32_t size) noexcept {
        if (size <= 0) {
            return;
        }

        list_.size = static_cast<std::int32_t>(size);
        list_.elements = new (std::nothrow) dxfg_event_clazz_t *[list_.size];

        if (!list_.elements) {
            release();

            return;
        }

        bool needToRelease = false;

        for (std::int32_t i = 0; i < list_.size; i++) {
            list_.elements[i] = new (std::nothrow) dxfg_event_clazz_t{};

            if (!list_.elements[i]) {
                needToRelease = true;
            }
        }

        if (needToRelease) {
            release();
        }
    }

    void release() {
        if (list_.size == 0 || list_.elements == nullptr) {
            return;
        }

        for (auto i = list_.size - 1; i >= 0; i--) {
            delete list_.elements[i];
        }

        delete[] list_.elements;
        list_.size = 0;
        list_.elements = nullptr;
    }

    ~Impl() noexcept { release(); }
};

EventClassList::EventClassList() noexcept : impl_(std::make_unique<EventClassList::Impl>()) {}

std::unique_ptr<EventClassList> EventClassList::create(std::size_t size) noexcept {
    auto result = std::unique_ptr<EventClassList>(new EventClassList{});

    result->impl_->init(static_cast<std::uint32_t>(size));

    return result;
}

void EventClassList::set(std::size_t index, std::uint32_t id) noexcept { impl_->set(index, id); }

bool EventClassList::isEmpty() const noexcept { return impl_->isEmpty(); }

std::size_t EventClassList::size() const noexcept { return impl_->size(); }

void *EventClassList::getHandler() noexcept { return impl_->getHandler(); }

EventClassList::~EventClassList() noexcept {}

} // namespace handler_utils

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

static dxfg_endpoint_role_t roleToGraalRole(DXEndpoint::Role role) {
    switch (role) {
    case DXEndpoint::Role::FEED:
        return DXFG_ENDPOINT_ROLE_FEED;
    case DXEndpoint::Role::ON_DEMAND_FEED:
        return DXFG_ENDPOINT_ROLE_ON_DEMAND_FEED;
    case DXEndpoint::Role::STREAM_FEED:
        return DXFG_ENDPOINT_ROLE_STREAM_FEED;
    case DXEndpoint::Role::PUBLISHER:
        return DXFG_ENDPOINT_ROLE_PUBLISHER;
    case DXEndpoint::Role::STREAM_PUBLISHER:
        return DXFG_ENDPOINT_ROLE_STREAM_PUBLISHER;
    case DXEndpoint::Role::LOCAL_HUB:
        return DXFG_ENDPOINT_ROLE_LOCAL_HUB;
    }

    return DXFG_ENDPOINT_ROLE_FEED;
}

static DXEndpoint::State graalStateToState(dxfg_endpoint_state_t state) {
    switch (state) {
    case DXFG_ENDPOINT_STATE_NOT_CONNECTED:
        return DXEndpoint::State::NOT_CONNECTED;
    case DXFG_ENDPOINT_STATE_CONNECTING:
        return DXEndpoint::State::CONNECTING;
    case DXFG_ENDPOINT_STATE_CONNECTED:
        return DXEndpoint::State::CONNECTED;
    case DXFG_ENDPOINT_STATE_CLOSED:
        return DXEndpoint::State::CLOSED;
    }

    return DXEndpoint::State::NOT_CONNECTED;
}

std::shared_ptr<DXEndpoint> DXEndpoint::create(void *endpointHandle, DXEndpoint::Role role,
                                               const std::unordered_map<std::string, std::string> &properties) {
    if constexpr (detail::isDebug) {
        detail::debug("DXEndpoint::create{{handle = {}, role = {}, properties[{}]}}()", endpointHandle,
                      roleToString(role), properties.size());
    }

    std::shared_ptr<DXEndpoint> endpoint{new (std::nothrow) DXEndpoint{}};

    endpoint->handler_ = detail::handler_utils::JavaObjectHandler(endpointHandle);
    endpoint->role_ = role;
    endpoint->name_ = properties.contains(NAME_PROPERTY) ? properties.at(NAME_PROPERTY) : std::string{};
    endpoint->stateChangeListenerHandler_ = detail::handler_utils::JavaObjectHandler(detail::runIsolatedOrElse(
        [endpoint](auto threadHandle) {
            return dxfg_PropertyChangeListener_new(
                threadHandle,
                [](graal_isolatethread_t *thread, dxfg_endpoint_state_t oldState, dxfg_endpoint_state_t newState,
                   void *user_data) {
                    detail::bit_cast<DXEndpoint *>(user_data)->onStateChange_(graalStateToState(oldState),
                                                                              graalStateToState(newState));
                },
                endpoint.get());
        },
        nullptr));
    endpoint->setStateChangeListenerImpl();

    return endpoint;
}

void DXEndpoint::setStateChangeListenerImpl() {
    if (handler_ && stateChangeListenerHandler_) {
        detail::runIsolatedOrElse(
            [handler = detail::bit_cast<dxfg_endpoint_t *>(handler_.get()),
             stateChangeListenerHandler = detail::bit_cast<dxfg_endpoint_state_change_listener_t *>(
                 stateChangeListenerHandler_.get())](auto threadHandle) {
                // TODO: finalize function

                return dxfg_DXEndpoint_addStateChangeListener(
                           threadHandle, handler, stateChangeListenerHandler, [](auto, auto) {}, nullptr) == 0;
            },
            false);
    }
}

DXEndpoint::State DXEndpoint::getState() const {
    std::lock_guard guard(mtx_);

    return !handler_ ? State::CLOSED
                     : detail::runIsolatedOrElse(
                           [handler = detail::bit_cast<dxfg_endpoint_t *>(handler_.get())](auto threadHandle) {
                               return graalStateToState(dxfg_DXEndpoint_getState(threadHandle, handler));
                           },
                           State::CLOSED);
}

void DXEndpoint::closeImpl() {
    if (!handler_) {
        return;
    }

    detail::runIsolatedOrElse([handler = detail::bit_cast<dxfg_endpoint_t *>(handler_.get())](
                                  auto threadHandle) { return dxfg_DXEndpoint_close(threadHandle, handler) == 0; },
                              false);

    // TODO: close the Feed and Publisher
}

std::shared_ptr<DXEndpoint> DXEndpoint::user(const std::string &user) {
    if constexpr (detail::isDebug) {
        detail::debug("DXEndpoint{{{}}}::user(user = {})", handler_.toString(), user);
    }

    std::lock_guard guard(mtx_);

    if (handler_) {
        detail::runIsolatedOrElse(
            [user = user, handler = detail::bit_cast<dxfg_endpoint_t *>(handler_.get())](auto threadHandle) {
                return dxfg_DXEndpoint_user(threadHandle, handler, user.c_str()) == 0;
            },
            false);
    }

    return shared_from_this();
}

std::shared_ptr<DXEndpoint> DXEndpoint::password(const std::string &password) {
    if constexpr (detail::isDebug) {
        detail::debug("DXEndpoint{{{}}}::password(password = {})", handler_.toString(), password);
    }

    std::lock_guard guard(mtx_);

    if (handler_) {
        detail::runIsolatedOrElse(
            [password = password, handler = detail::bit_cast<dxfg_endpoint_t *>(handler_.get())](auto threadHandle) {
                return dxfg_DXEndpoint_password(threadHandle, handler, password.c_str()) == 0;
            },
            false);
    }

    return shared_from_this();
}

std::shared_ptr<DXEndpoint> DXEndpoint::connect(const std::string &address) {
    if constexpr (detail::isDebug) {
        detail::debug("DXEndpoint{{{}}}::connect(address = {})", handler_.toString(), address);
    }

    std::lock_guard guard(mtx_);

    if (handler_) {
        detail::runIsolatedOrElse(
            [address = address, handler = detail::bit_cast<dxfg_endpoint_t *>(handler_.get())](auto threadHandle) {
                return dxfg_DXEndpoint_connect(threadHandle, handler, address.c_str()) == 0;
            },
            false);
    }

    return shared_from_this();
}

void DXEndpoint::reconnect() {
    if constexpr (detail::isDebug) {
        detail::debug("DXEndpoint{{{}}}::reconnect()", handler_.toString());
    }

    std::lock_guard guard(mtx_);

    if (!handler_) {
        return;
    }

    detail::runIsolatedOrElse([handler = detail::bit_cast<dxfg_endpoint_t *>(handler_.get())](
                                  auto threadHandle) { return dxfg_DXEndpoint_reconnect(threadHandle, handler) == 0; },
                              false);
}

void DXEndpoint::disconnect() {
    if constexpr (detail::isDebug) {
        detail::debug("DXEndpoint{{{}}}::disconnect()", handler_.toString());
    }

    std::lock_guard guard(mtx_);

    if (!handler_) {
        return;
    }

    detail::runIsolatedOrElse([handler = detail::bit_cast<dxfg_endpoint_t *>(handler_.get())](
                                  auto threadHandle) { return dxfg_DXEndpoint_disconnect(threadHandle, handler) == 0; },
                              false);
}

void DXEndpoint::disconnectAndClear() {
    if constexpr (detail::isDebug) {
        detail::debug("DXEndpoint{{{}}}::disconnectAndClear()", handler_.toString());
    }

    std::lock_guard guard(mtx_);

    if (!handler_) {
        return;
    }

    detail::runIsolatedOrElse(
        [handler = detail::bit_cast<dxfg_endpoint_t *>(handler_.get())](auto threadHandle) {
            return dxfg_DXEndpoint_disconnectAndClear(threadHandle, handler) == 0;
        },
        false);
}

void DXEndpoint::awaitNotConnected() {
    if constexpr (detail::isDebug) {
        detail::debug("DXEndpoint{{{}}}::awaitNotConnected()", handler_.toString());
    }

    std::lock_guard guard(mtx_);

    if (!handler_) {
        return;
    }

    detail::runIsolatedOrElse(
        [handler = detail::bit_cast<dxfg_endpoint_t *>(handler_.get())](auto threadHandle) {
            return dxfg_DXEndpoint_awaitNotConnected(threadHandle, handler) == 0;
        },
        false);
}

void DXEndpoint::awaitProcessed() {
    if constexpr (detail::isDebug) {
        detail::debug("DXEndpoint{{{}}}::awaitProcessed()", handler_.toString());
    }

    std::lock_guard guard(mtx_);

    if (!handler_) {
        return;
    }

    detail::runIsolatedOrElse(
        [handler = detail::bit_cast<dxfg_endpoint_t *>(handler_.get())](auto threadHandle) {
            return dxfg_DXEndpoint_awaitProcessed(threadHandle, handler) == 0;
        },
        false);
}

void DXEndpoint::closeAndAwaitTermination() {
    if constexpr (detail::isDebug) {
        detail::debug("DXEndpoint{{{}}}::closeAndAwaitTermination()", handler_.toString());
    }

    std::lock_guard guard(mtx_);

    if (!handler_) {
        return;
    }

    detail::runIsolatedOrElse(
        [handler = detail::bit_cast<dxfg_endpoint_t *>(handler_.get())](auto threadHandle) {
            return dxfg_DXEndpoint_closeAndAwaitTermination(threadHandle, handler) == 0;
        },
        false);

    // TODO: close the Feed and Publisher
}

std::shared_ptr<DXFeed> DXEndpoint::getFeed() {
    std::lock_guard guard(mtx_);

    if (!feed_) {
        auto feedHandle = !handler_
                              ? nullptr
                              : detail::runIsolatedOrElse(
                                    [handler = detail::bit_cast<dxfg_endpoint_t *>(handler_.get())](auto threadHandle) {
                                        return dxfg_DXEndpoint_getFeed(threadHandle, handler);
                                    },
                                    nullptr);

        feed_ = DXFeed::create(feedHandle);
    }

    return feed_;
}

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
const EventTypeEnum EventTypeEnum::OPTION_SALE{DXFG_EVENT_OPTION_SALE, "OPTION_SALE", false, true};

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

bool System::setProperty(const std::string &key, const std::string &value) {
    if constexpr (dxfcpp::detail::isDebug) {
        detail::debug("System::setProperty(key = '{}', value = '{}')", key, value);
    }

    auto result = detail::runIsolatedOrElse(
        [key = key, value = value](auto threadHandle) {
            return detail::CEntryPointErrors::valueOf(dxfg_system_set_property(
                       threadHandle, key.c_str(), value.c_str())) == detail::CEntryPointErrors::NO_ERROR;
        },
        false);

    if constexpr (dxfcpp::detail::isDebug) {
        detail::debug("System::setProperty(key = '{}', value = '{}') -> {}", key, value, result);
    }

    return result;
}

std::string System::getProperty(const std::string &key) {
    if constexpr (detail::isDebug) {
        detail::debug("System::getProperty(key = {})", key);
    }

    auto result = detail::runIsolatedOrElse(
        [key = key](auto threadHandle) {
            std::string resultString{};

            if (auto result = dxfg_system_get_property(threadHandle, key.c_str()); result != nullptr) {
                resultString = result;
                dxfg_system_release_property(threadHandle, result);
            }

            return resultString;
        },
        std::string{});

    if constexpr (detail::isDebug) {
        detail::debug("System::getProperty(key = '{}') -> '{}'", key, result);
    }

    return result;
}

void DXFeed::attachSubscription(std::shared_ptr<DXFeedSubscription> subscription) noexcept {
    if constexpr (detail::isDebug) {
        detail::debug("{}::attachSubscription({})", toString(), subscription->toString());
    }

    std::lock_guard guard(mtx_);

    if (!handler_ || !subscription || !subscription->handler_) {
        return;
    }

    if (detail::runIsolatedOrElse(
            [handler = detail::bit_cast<dxfg_feed_t *>(handler_.get()),
             subHandler = detail::bit_cast<dxfg_subscription_t *>(subscription->handler_.get())](auto threadHandle) {
                return dxfg_DXFeed_attachSubscription(threadHandle, handler, subHandler) == 0;
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

    if (!handler_ || !subscription || !subscription->handler_) {
        return;
    }

    if (detail::runIsolatedOrElse(
            [handler = detail::bit_cast<dxfg_feed_t *>(handler_.get()),
             subHandler = detail::bit_cast<dxfg_subscription_t *>(subscription->handler_.get())](auto threadHandle) {
                return dxfg_DXFeed_detachSubscription(threadHandle, handler, subHandler) == 0;
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

    if (!handler_ || !subscription || !subscription->handler_) {
        return;
    }

    if (detail::runIsolatedOrElse(
            [handler = detail::bit_cast<dxfg_feed_t *>(handler_.get()),
             subHandler = detail::bit_cast<dxfg_subscription_t *>(subscription->handler_.get())](auto threadHandle) {
                return dxfg_DXFeed_detachSubscriptionAndClear(threadHandle, handler, subHandler) == 0;
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

std::shared_ptr<DXFeed> DXFeed::create(void *feedHandle) noexcept {
    std::shared_ptr<DXFeed> feed{new (std::nothrow) DXFeed{}};

    feed->handler_ = detail::handler_utils::JavaObjectHandler(feedHandle);

    return feed;
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

void DXFeedSubscription::closeImpl() noexcept {
    if (!handler_) {
        return;
    }

    detail::runIsolatedOrElse(
        [handler = detail::bit_cast<dxfg_subscription_t *>(handler_.get())](auto threadHandle) {
            return dxfg_DXFeedSubscription_close(threadHandle, handler) == 0;
        },
        false);
}

void DXFeedSubscription::clearImpl() noexcept {
    if (!handler_) {
        return;
    }

    detail::runIsolatedOrElse(
        [handler = detail::bit_cast<dxfg_subscription_t *>(handler_.get())](auto threadHandle) {
            return dxfg_DXFeedSubscription_clear(threadHandle, handler) == 0;
        },
        false);
}

bool DXFeedSubscription::isClosedImpl() noexcept {
    if (!handler_) {
        return false;
    }

    return detail::runIsolatedOrElse(
        [handler = detail::bit_cast<dxfg_subscription_t *>(handler_.get())](auto threadHandle) {
            return dxfg_DXFeedSubscription_isClosed(threadHandle, handler) != 0;
        },
        false);
}

DXFeedSubscription::DXFeedSubscription(const EventTypeEnum &eventType) noexcept
    : mtx_{}, handler_{detail::handler_utils::JavaObjectHandler(nullptr)},
      eventListenerHandler_{detail::handler_utils::JavaObjectHandler(nullptr)}, onEvent_{} {
    if constexpr (detail::isDebug) {
        detail::debug("DXFeedSubscription(eventType = {})", eventType.getName());
    }

    handler_ = detail::handler_utils::JavaObjectHandler(detail::runIsolatedOrElse(
        [eventType](auto threadHandle) {
            return dxfg_DXFeedSubscription_new(threadHandle, static_cast<dxfg_event_clazz_t>(eventType.getId()));
        },
        nullptr));

    setEventListenerHandler();
}

detail::handler_utils::JavaObjectHandler DXFeedSubscription::createSubscriptionHandlerFromEventClassList(
    const std::unique_ptr<detail::handler_utils::EventClassList> &list) noexcept {
    return detail::handler_utils::JavaObjectHandler(detail::runIsolatedOrElse(
        [listHandler = detail::bit_cast<dxfg_event_clazz_list_t *>(list->getHandler())](auto threadHandle) {
            return dxfg_DXFeedSubscription_new2(threadHandle, listHandler);
        },
        nullptr));
}
void DXFeedSubscription::setEventListenerHandler() noexcept {
    //TODO: sub manager

    eventListenerHandler_ = detail::handler_utils::JavaObjectHandler(detail::runIsolatedOrElse(
        [subscription = this](auto threadHandle) {
            return dxfg_DXFeedEventListener_new(
                threadHandle,
                [](graal_isolatethread_t *thread, dxfg_event_type_list *graalNativeEvents, void *user_data) {
                    auto &&events = EventMapper::fromGraalNativeList(detail::bit_cast<void *>(graalNativeEvents));

                    detail::bit_cast<DXFeedSubscription *>(user_data)->onEvent_(events);
                },
                subscription);
        },
        nullptr));

    if (handler_ && eventListenerHandler_) {
        detail::runIsolatedOrElse(
            [handler = detail::bit_cast<dxfg_subscription_t *>(handler_.get()),
             eventListenerHandler =
                 detail::bit_cast<dxfg_feed_event_listener_t *>(eventListenerHandler_.get())](auto threadHandle) {
                // TODO: finalize function

                return dxfg_DXFeedSubscription_addEventListener(
                           threadHandle, handler, eventListenerHandler, [](auto, auto) {}, nullptr) == 0;
            },
            false);
    }
}

std::shared_ptr<DXEndpoint::Builder> DXEndpoint::Builder::create() noexcept {
    if constexpr (detail::isDebug) {
        detail::debug("DXEndpoint::Builder::create()");
    }

    auto builder = std::shared_ptr<Builder>(new (std::nothrow) Builder{});

    if (builder) {
        builder->handler_ = detail::handler_utils::JavaObjectHandler(detail::runIsolatedOrElse(
            [](auto threadHandle) { return dxfg_DXEndpoint_newBuilder(threadHandle); }, nullptr));
    }

    return builder;
}

void DXEndpoint::Builder::loadDefaultPropertiesImpl() {
    // The default properties file is only valid for the
    // Feed, OnDemandFeed and Publisher roles.
    auto propertiesFileKey = [this]() -> std::string {
        switch (role_) {
        case Role::FEED:
        case Role::ON_DEMAND_FEED:
            return DXFEED_PROPERTIES_PROPERTY;
        case Role::PUBLISHER:
            return DXPUBLISHER_PROPERTIES_PROPERTY;
        case Role::STREAM_FEED:
        case Role::STREAM_PUBLISHER:
        case Role::LOCAL_HUB:
            break;
        }

        return {};
    }();

    if (propertiesFileKey.empty()) {
        return;
    }

    // If propertiesFileKey was set in system properties,
    // don't try to load the default properties file.
    if (!System::getProperty(propertiesFileKey).empty()) {
        return;
    }

    // If there is no propertiesFileKey in user-set properties,
    // try load default properties file from current runtime directory if file exist.
    if (!properties_.contains(propertiesFileKey) && std::filesystem::exists(propertiesFileKey)) {
        if (!handler_) {
            return;
        }

        // The default property file has the same value as the key.
        detail::runIsolatedOrElse(
            [key = propertiesFileKey, value = propertiesFileKey,
             handler = detail::bit_cast<dxfg_endpoint_builder_t *>(handler_.get())](auto threadHandle) {
                return dxfg_DXEndpoint_Builder_withProperty(threadHandle, handler, key.c_str(), value.c_str()) == 0;
            },
            false);
    }
}

std::shared_ptr<DXEndpoint::Builder> DXEndpoint::Builder::withRole(DXEndpoint::Role role) {
    if constexpr (detail::isDebug) {
        detail::debug("DXEndpoint::Builder{{{}}}::withRole(role = {})", handler_.toString(), roleToString(role));
    }

    std::lock_guard guard(mtx_);

    role_ = role;

    if (handler_) {
        detail::runIsolatedOrElse(
            [role = role, handler = detail::bit_cast<dxfg_endpoint_builder_t *>(handler_.get())](auto threadHandle) {
                return dxfg_DXEndpoint_Builder_withRole(threadHandle, handler, roleToGraalRole(role)) == 0;
            },
            false);
    }

    return shared_from_this();
}

std::shared_ptr<DXEndpoint::Builder> DXEndpoint::Builder::withProperty(const std::string &key,
                                                                       const std::string &value) {
    if constexpr (detail::isDebug) {
        detail::debug("DXEndpoint::Builder{{{}}}::withProperty(key = {}, value = {})", handler_.toString(), key, value);
    }

    std::lock_guard guard(mtx_);

    properties_[key] = value;

    if (handler_) {
        detail::runIsolatedOrElse(
            [key = key, value = value,
             handler = detail::bit_cast<dxfg_endpoint_builder_t *>(handler_.get())](auto threadHandle) {
                return dxfg_DXEndpoint_Builder_withProperty(threadHandle, handler, key.c_str(), value.c_str()) == 0;
            },
            false);
    }

    return shared_from_this();
}

bool DXEndpoint::Builder::supportsProperty(const std::string &key) {
    if constexpr (detail::isDebug) {
        detail::debug("DXEndpoint::Builder{{{}}}::supportsProperty(key = {})", handler_.toString(), key);
    }

    std::lock_guard guard(mtx_);

    if (!handler_) {
        return false;
    }

    return detail::runIsolatedOrElse(
        [key = key, handler = detail::bit_cast<dxfg_endpoint_builder_t *>(handler_.get())](auto threadHandle) {
            return dxfg_DXEndpoint_Builder_supportsProperty(threadHandle, handler, key.c_str()) != 0;
        },
        false);
}

std::shared_ptr<DXEndpoint> DXEndpoint::Builder::build() {
    if constexpr (detail::isDebug) {
        detail::debug("DXEndpoint::Builder{{{}}}::build()", handler_.toString());
    }

    std::lock_guard guard(mtx_);

    loadDefaultPropertiesImpl();

    auto endpointHandle =
        !handler_ ? nullptr
                  : detail::runIsolatedOrElse(
                        [handler = detail::bit_cast<dxfg_endpoint_builder_t *>(handler_.get())](auto threadHandle) {
                            return dxfg_DXEndpoint_Builder_build(threadHandle, handler);
                        },
                        nullptr);

    return DXEndpoint::create(endpointHandle, role_, properties_);
}

char utf16to8(std::int16_t in) {
    try {
        std::string out{};
        auto utf16in = {in};

        utf8::utf16to8(std::begin(utf16in), std::end(utf16in), std::back_inserter(out));

        return out.empty() ? char{} : out[0];
    } catch (...) {
        // TODO: error handling
        return char{};
    }
}

std::int16_t utf8to16(char in) {
    try {
        std::u16string out{};
        auto utf8in = {in};

        utf8::utf8to16(std::begin(utf8in), std::end(utf8in), std::back_inserter(out));

        return out.empty() ? std::int16_t{} : static_cast<std::int16_t>(out[0]);
    } catch (...) {
        // TODO: error handling
        return std::int16_t{};
    }
}

const EventTypeEnum Quote::type = EventTypeEnum::QUOTE;

char Quote::getBidExchangeCode() const { return utf16to8(data_.bidExchangeCode); }

void Quote::setBidExchangeCode(char bidExchangeCode) { data_.bidExchangeCode = utf8to16(bidExchangeCode); }

char Quote::getAskExchangeCode() const { return utf16to8(data_.askExchangeCode); }

void Quote::setAskExchangeCode(char askExchangeCode) { data_.askExchangeCode = utf8to16(askExchangeCode); }

std::shared_ptr<Quote> Quote::fromGraalNative(void *graalNative) noexcept {
    if (!graalNative) {
        return {};
    }

    auto eventType = detail::bit_cast<dxfg_event_type_t *>(graalNative);

    if (eventType->clazz != DXFG_EVENT_QUOTE) {
        return {};
    }

    try {
        auto graalQuote = detail::bit_cast<dxfg_quote_t *>(graalNative);
        auto quote = std::make_shared<Quote>(detail::toString(graalQuote->market_event.event_symbol));

        quote->setEventTime(graalQuote->market_event.event_time);

        quote->data_ = {
            graalQuote->time_millis_sequence,
            graalQuote->time_nano_part,
            graalQuote->bid_time,
            graalQuote->bid_exchange_code,
            graalQuote->bid_price,
            graalQuote->bid_size,
            graalQuote->ask_time,
            graalQuote->ask_exchange_code,
            graalQuote->ask_price,
            graalQuote->ask_size,
        };

        return quote;
    } catch (...) {
        // TODO: error handling
        return {};
    }
}

std::vector<std::shared_ptr<EventType>> EventMapper::fromGraalNativeList(void *graalNativeList) {
    auto list = detail::bit_cast<dxfg_event_type_list *>(graalNativeList);

    if (list->size <= 0) {
        return {};
    }

    std::vector<std::shared_ptr<EventType>> result{static_cast<std::size_t>(list->size)};

    for (std::size_t i = 0; i < list->size; i++) {
        auto *e = list->elements[i];

        // TODO: implement other types
        switch (e->clazz) {
        case DXFG_EVENT_QUOTE:
            result[i] = Quote::fromGraalNative(e);

            break;
        case DXFG_EVENT_PROFILE:
            result[i] = Profile::fromGraalNative(e);

            break;
        case DXFG_EVENT_SUMMARY:
            break;
        case DXFG_EVENT_GREEKS:
            break;
        case DXFG_EVENT_CANDLE:
            break;
        case DXFG_EVENT_DAILY_CANDLE:
            break;
        case DXFG_EVENT_UNDERLYING:
            break;
        case DXFG_EVENT_THEO_PRICE:
            break;
        case DXFG_EVENT_TRADE:
            break;
        case DXFG_EVENT_TRADE_ETH:
            break;
        case DXFG_EVENT_CONFIGURATION:
            break;
        case DXFG_EVENT_MESSAGE:
            break;
        case DXFG_EVENT_TIME_AND_SALE:
            break;
        case DXFG_EVENT_ORDER_BASE:
            break;
        case DXFG_EVENT_ORDER:
            break;
        case DXFG_EVENT_ANALYTIC_ORDER:
            break;
        case DXFG_EVENT_SPREAD_ORDER:
            break;
        case DXFG_EVENT_SERIES:
            break;
        case DXFG_EVENT_OPTION_SALE:
            break;
        }
    }

    return result;
}

const ShortSaleRestriction ShortSaleRestriction::UNDEFINED{0, "UNDEFINED"};
const ShortSaleRestriction ShortSaleRestriction::ACTIVE{1, "ACTIVE"};
const ShortSaleRestriction ShortSaleRestriction::INACTIVE{2, "INACTIVE"};

const std::unordered_map<ShortSaleRestriction::CodeType, std::reference_wrapper<const ShortSaleRestriction>>
    ShortSaleRestriction::ALL{
        {ShortSaleRestriction::UNDEFINED.getCode(), std::cref(ShortSaleRestriction::UNDEFINED)},
        {ShortSaleRestriction::ACTIVE.getCode(), std::cref(ShortSaleRestriction::ACTIVE)},
        {ShortSaleRestriction::INACTIVE.getCode(), std::cref(ShortSaleRestriction::INACTIVE)},
    };

const TradingStatus TradingStatus::UNDEFINED{0, "UNDEFINED"};
const TradingStatus TradingStatus::HALTED{1, "HALTED"};
const TradingStatus TradingStatus::ACTIVE{2, "ACTIVE"};

const std::unordered_map<TradingStatus::CodeType, std::reference_wrapper<const TradingStatus>> TradingStatus::ALL{
    {TradingStatus::UNDEFINED.getCode(), std::cref(TradingStatus::UNDEFINED)},
    {TradingStatus::HALTED.getCode(), std::cref(TradingStatus::HALTED)},
    {TradingStatus::ACTIVE.getCode(), std::cref(TradingStatus::ACTIVE)},
};

std::shared_ptr<Profile> Profile::fromGraalNative(void *graalNative) noexcept {
    if (!graalNative) {
        return {};
    }

    auto eventType = detail::bit_cast<dxfg_event_type_t *>(graalNative);

    if (eventType->clazz != DXFG_EVENT_PROFILE) {
        return {};
    }

    try {
        auto graalProfile = detail::bit_cast<dxfg_profile_t *>(graalNative);
        auto profile = std::make_shared<Profile>(detail::toString(graalProfile->market_event.event_symbol));

        profile->setEventTime(graalProfile->market_event.event_time);
        profile->data_ = {
            detail::toString(graalProfile->description),
            detail::toString(graalProfile->status_reason),
            graalProfile->halt_start_time,
            graalProfile->halt_end_time,
            graalProfile->high_limit_price,
            graalProfile->low_limit_price,
            graalProfile->high_52_week_price,
            graalProfile->low_52_week_price,
            graalProfile->beta,
            graalProfile->earnings_per_share,
            graalProfile->dividend_frequency,
            graalProfile->ex_dividend_amount,
            graalProfile->ex_dividend_day_id,
            graalProfile->shares,
            graalProfile->free_float,
            graalProfile->flags,
        };

        return profile;
    } catch (...) {
        // TODO: error handling
        return {};
    }
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
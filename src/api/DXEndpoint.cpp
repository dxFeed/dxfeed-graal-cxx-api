// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_native_c_api/api.h>
#include <dxfeed_graal_native_cpp_api/api.hpp>

#include <string>
#include <memory>
#include <utility>

namespace dxfcpp {

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
    if constexpr (isDebug) {
        debug("DXEndpoint::create{{handle = {}, role = {}, properties[{}]}}()", endpointHandle, roleToString(role),
              properties.size());
    }

    std::shared_ptr<DXEndpoint> endpoint{new (std::nothrow) DXEndpoint{}};

    endpoint->handler_ = handler_utils::JavaObjectHandler<DXEndpoint>(endpointHandle);
    endpoint->role_ = role;
    endpoint->name_ = properties.contains(NAME_PROPERTY) ? properties.at(NAME_PROPERTY) : std::string{};
    endpoint->stateChangeListenerHandler_ =
        handler_utils::JavaObjectHandler<DXEndpointStateChangeListener>(runIsolatedOrElse(
            [endpoint](auto threadHandle) {
                return dxfg_PropertyChangeListener_new(
                    threadHandle,
                    [](graal_isolatethread_t *thread, dxfg_endpoint_state_t oldState, dxfg_endpoint_state_t newState,
                       void *user_data) {
                        bit_cast<DXEndpoint *>(user_data)->onStateChange_(graalStateToState(oldState),
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
        runIsolatedOrElse(
            [handler = bit_cast<dxfg_endpoint_t *>(handler_.get()),
             stateChangeListenerHandler = bit_cast<dxfg_endpoint_state_change_listener_t *>(
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
                     : runIsolatedOrElse(
                           [handler = bit_cast<dxfg_endpoint_t *>(handler_.get())](auto threadHandle) {
                               return graalStateToState(dxfg_DXEndpoint_getState(threadHandle, handler));
                           },
                           State::CLOSED);
}

void DXEndpoint::closeImpl() {
    if (!handler_) {
        return;
    }

    runIsolatedOrElse([handler = bit_cast<dxfg_endpoint_t *>(handler_.get())](
                          auto threadHandle) { return dxfg_DXEndpoint_close(threadHandle, handler) == 0; },
                      false);

    // TODO: close the Feed and Publisher
}

std::shared_ptr<DXEndpoint> DXEndpoint::user(const std::string &user) {
    //TODO: check invalid utf-8
    if constexpr (isDebug) {
        debug("DXEndpoint{{{}}}::user(user = {})", handler_.toString(), user);
    }

    std::lock_guard guard(mtx_);

    if (handler_) {
        runIsolatedOrElse(
            [user = user, handler = bit_cast<dxfg_endpoint_t *>(handler_.get())](auto threadHandle) {
                return dxfg_DXEndpoint_user(threadHandle, handler, user.c_str()) == 0;
            },
            false);
    }

    return shared_from_this();
}

std::shared_ptr<DXEndpoint> DXEndpoint::password(const std::string &password) {
    //TODO: check invalid utf-8
    if constexpr (isDebug) {
        debug("DXEndpoint{{{}}}::password(password = {})", handler_.toString(), password);
    }

    std::lock_guard guard(mtx_);

    if (handler_) {
        runIsolatedOrElse(
            [password = password, handler = bit_cast<dxfg_endpoint_t *>(handler_.get())](auto threadHandle) {
                return dxfg_DXEndpoint_password(threadHandle, handler, password.c_str()) == 0;
            },
            false);
    }

    return shared_from_this();
}

std::shared_ptr<DXEndpoint> DXEndpoint::connect(const std::string &address) {
    //TODO: check invalid utf-8
    if constexpr (isDebug) {
        debug("DXEndpoint{{{}}}::connect(address = {})", handler_.toString(), address);
    }

    std::lock_guard guard(mtx_);

    if (handler_) {
        runIsolatedOrElse(
            [address = address, handler = bit_cast<dxfg_endpoint_t *>(handler_.get())](auto threadHandle) {
                return dxfg_DXEndpoint_connect(threadHandle, handler, address.c_str()) == 0;
            },
            false);
    }

    return shared_from_this();
}

void DXEndpoint::reconnect() {
    if constexpr (isDebug) {
        debug("DXEndpoint{{{}}}::reconnect()", handler_.toString());
    }

    std::lock_guard guard(mtx_);

    if (!handler_) {
        return;
    }

    runIsolatedOrElse([handler = bit_cast<dxfg_endpoint_t *>(handler_.get())](
                          auto threadHandle) { return dxfg_DXEndpoint_reconnect(threadHandle, handler) == 0; },
                      false);
}

void DXEndpoint::disconnect() {
    if constexpr (isDebug) {
        debug("DXEndpoint{{{}}}::disconnect()", handler_.toString());
    }

    std::lock_guard guard(mtx_);

    if (!handler_) {
        return;
    }

    runIsolatedOrElse([handler = bit_cast<dxfg_endpoint_t *>(handler_.get())](
                          auto threadHandle) { return dxfg_DXEndpoint_disconnect(threadHandle, handler) == 0; },
                      false);
}

void DXEndpoint::disconnectAndClear() {
    if constexpr (isDebug) {
        debug("DXEndpoint{{{}}}::disconnectAndClear()", handler_.toString());
    }

    std::lock_guard guard(mtx_);

    if (!handler_) {
        return;
    }

    runIsolatedOrElse([handler = bit_cast<dxfg_endpoint_t *>(handler_.get())](
                          auto threadHandle) { return dxfg_DXEndpoint_disconnectAndClear(threadHandle, handler) == 0; },
                      false);
}

void DXEndpoint::awaitNotConnected() {
    if constexpr (isDebug) {
        debug("DXEndpoint{{{}}}::awaitNotConnected()", handler_.toString());
    }

    std::lock_guard guard(mtx_);

    if (!handler_) {
        return;
    }

    runIsolatedOrElse([handler = bit_cast<dxfg_endpoint_t *>(handler_.get())](
                          auto threadHandle) { return dxfg_DXEndpoint_awaitNotConnected(threadHandle, handler) == 0; },
                      false);
}

void DXEndpoint::awaitProcessed() {
    if constexpr (isDebug) {
        debug("DXEndpoint{{{}}}::awaitProcessed()", handler_.toString());
    }

    std::lock_guard guard(mtx_);

    if (!handler_) {
        return;
    }

    runIsolatedOrElse([handler = bit_cast<dxfg_endpoint_t *>(handler_.get())](
                          auto threadHandle) { return dxfg_DXEndpoint_awaitProcessed(threadHandle, handler) == 0; },
                      false);
}

void DXEndpoint::closeAndAwaitTermination() {
    if constexpr (isDebug) {
        debug("DXEndpoint{{{}}}::closeAndAwaitTermination()", handler_.toString());
    }

    std::lock_guard guard(mtx_);

    if (!handler_) {
        return;
    }

    runIsolatedOrElse(
        [handler = bit_cast<dxfg_endpoint_t *>(handler_.get())](auto threadHandle) {
            return dxfg_DXEndpoint_closeAndAwaitTermination(threadHandle, handler) == 0;
        },
        false);

    // TODO: close the Feed and Publisher
}

std::shared_ptr<DXFeed> DXEndpoint::getFeed() {
    std::lock_guard guard(mtx_);

    if (!feed_) {
        auto feedHandle = !handler_ ? nullptr
                                    : runIsolatedOrElse(
                                          [handler = bit_cast<dxfg_endpoint_t *>(handler_.get())](auto threadHandle) {
                                              return dxfg_DXEndpoint_getFeed(threadHandle, handler);
                                          },
                                          nullptr);

        feed_ = DXFeed::create(feedHandle);
    }

    return feed_;
}

std::shared_ptr<DXEndpoint::Builder> DXEndpoint::Builder::create() noexcept {
    if constexpr (isDebug) {
        debug("DXEndpoint::Builder::create()");
    }

    auto builder = std::shared_ptr<Builder>(new (std::nothrow) Builder{});

    if (builder) {
        builder->handler_ = handler_utils::JavaObjectHandler<DXEndpoint::Builder>(
            runIsolatedOrElse([](auto threadHandle) { return dxfg_DXEndpoint_newBuilder(threadHandle); }, nullptr));
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
        runIsolatedOrElse(
            [key = propertiesFileKey, value = propertiesFileKey,
             handler = bit_cast<dxfg_endpoint_builder_t *>(handler_.get())](auto threadHandle) {
                return dxfg_DXEndpoint_Builder_withProperty(threadHandle, handler, key.c_str(), value.c_str()) == 0;
            },
            false);
    }
}

std::shared_ptr<DXEndpoint::Builder> DXEndpoint::Builder::withRole(DXEndpoint::Role role) {
    if constexpr (isDebug) {
        debug("DXEndpoint::Builder{{{}}}::withRole(role = {})", handler_.toString(), roleToString(role));
    }

    std::lock_guard guard(mtx_);

    role_ = role;

    if (handler_) {
        runIsolatedOrElse(
            [role = role, handler = bit_cast<dxfg_endpoint_builder_t *>(handler_.get())](auto threadHandle) {
                return dxfg_DXEndpoint_Builder_withRole(threadHandle, handler, roleToGraalRole(role)) == 0;
            },
            false);
    }

    return shared_from_this();
}

std::shared_ptr<DXEndpoint::Builder> DXEndpoint::Builder::withProperty(const std::string &key,
                                                                       const std::string &value) {
    //TODO: check invalid utf-8
    if constexpr (isDebug) {
        debug("DXEndpoint::Builder{{{}}}::withProperty(key = {}, value = {})", handler_.toString(), key, value);
    }

    std::lock_guard guard(mtx_);

    properties_[key] = value;

    if (handler_) {
        runIsolatedOrElse(
            [key = key, value = value,
             handler = bit_cast<dxfg_endpoint_builder_t *>(handler_.get())](auto threadHandle) {
                return dxfg_DXEndpoint_Builder_withProperty(threadHandle, handler, key.c_str(), value.c_str()) == 0;
            },
            false);
    }

    return shared_from_this();
}

bool DXEndpoint::Builder::supportsProperty(const std::string &key) {
    //TODO: check invalid utf-8
    if constexpr (isDebug) {
        debug("DXEndpoint::Builder{{{}}}::supportsProperty(key = {})", handler_.toString(), key);
    }

    std::lock_guard guard(mtx_);

    if (!handler_) {
        return false;
    }

    return runIsolatedOrElse(
        [key = key, handler = bit_cast<dxfg_endpoint_builder_t *>(handler_.get())](auto threadHandle) {
            return dxfg_DXEndpoint_Builder_supportsProperty(threadHandle, handler, key.c_str()) != 0;
        },
        false);
}

std::shared_ptr<DXEndpoint> DXEndpoint::Builder::build() {
    if constexpr (isDebug) {
        debug("DXEndpoint::Builder{{{}}}::build()", handler_.toString());
    }

    std::lock_guard guard(mtx_);

    loadDefaultPropertiesImpl();

    auto endpointHandle =
        !handler_
            ? nullptr
            : runIsolatedOrElse([handler = bit_cast<dxfg_endpoint_builder_t *>(handler_.get())](
                                    auto threadHandle) { return dxfg_DXEndpoint_Builder_build(threadHandle, handler); },
                                nullptr);

    return DXEndpoint::create(endpointHandle, role_, properties_);
}

} // namespace dxfcpp
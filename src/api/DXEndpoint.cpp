// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <memory>
#include <string>
#include <utility>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

DXFCPP_BEGIN_NAMESPACE

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

struct DXEndpoint::Impl {
    static inline std::mutex MTX{};
    static std::unordered_map<Role, std::shared_ptr<DXEndpoint>> INSTANCES;

    static void onPropertyChange(graal_isolatethread_t * /*thread*/, dxfg_endpoint_state_t oldState,
                                 dxfg_endpoint_state_t newState, void *userData) {
        auto id = Id<DXEndpoint>::from(dxfcpp::bit_cast<Id<DXEndpoint>::ValueType>(userData));
        auto endpoint = ApiContext::getInstance()->getManager<DXEndpointManager>()->getEntity(id);

        if constexpr (Debugger::isDebug) {
            Debugger::debug("onStateChange: id = " + std::to_string(id.getValue()) +
                            ", endpoint = " + ((endpoint) ? endpoint->toString() : "nullptr"));
        }

        if (endpoint) {
            endpoint->onStateChange_(graalStateToState(oldState), graalStateToState(newState));

            if (newState == DXFG_ENDPOINT_STATE_CLOSED) {
                // TODO: fix endpoint lifetime
                // ApiContext::getInstance()->getManager<DXEndpointManager>()->unregisterEntity(id);
            }
        }
    };

    static std::shared_ptr<DXEndpoint> getInstance(DXEndpoint::Role role) noexcept {
        if constexpr (Debugger::isDebug) {
            Debugger::debug("DXEndpoint::Impl::getInstance(role = " + roleToString(role) + ")");
        }

        std::lock_guard lock(DXEndpoint::Impl::MTX);

        if (DXEndpoint::Impl::INSTANCES.contains(role)) {
            return DXEndpoint::Impl::INSTANCES[role];
        }

        return DXEndpoint::Impl::INSTANCES[role] = DXEndpoint::create(role);
    }
};

std::unordered_map<DXEndpoint::Role, std::shared_ptr<DXEndpoint>> DXEndpoint::Impl::INSTANCES{};

std::shared_ptr<DXEndpoint> DXEndpoint::create(void *endpointHandle, DXEndpoint::Role role,
                                               const std::unordered_map<std::string, std::string> &properties) {
    if (endpointHandle == nullptr) {
        throw std::invalid_argument("Unable to create DXEndpoint. The `endpointHandle` is nullptr");
    }

    if constexpr (Debugger::isDebug) {
        Debugger::debug("DXEndpoint::create(handle = " + dxfcpp::toString(endpointHandle) +
                        ", role = " + roleToString(role) + ", properties[" + std::to_string(properties.size()) + "])");
    }

    std::shared_ptr<DXEndpoint> endpoint{new DXEndpoint{}};

    endpoint->handle_ = JavaObjectHandle<DXEndpoint>(endpointHandle);
    endpoint->role_ = role;
    endpoint->name_ = properties.contains(NAME_PROPERTY) ? properties.at(NAME_PROPERTY) : std::string{};

    auto id = ApiContext::getInstance()->getManager<DXEndpointManager>()->registerEntity(endpoint);

    endpoint->stateChangeListenerHandle_ = isolated::api::DXEndpointStateChangeListener::create(
        dxfcpp::bit_cast<void *>(&DXEndpoint::Impl::onPropertyChange), dxfcpp::bit_cast<void *>(id.getValue()));

    isolated::api::IsolatedDXEndpoint::addStateChangeListener(endpoint->handle_, endpoint->stateChangeListenerHandle_);

    return endpoint;
}

DXEndpoint::State DXEndpoint::getState() const {
    return isolated::api::IsolatedDXEndpoint::getState(handle_);
}

std::shared_ptr<DXEndpoint> DXEndpoint::user(const std::string &user) {
    // TODO: check invalid utf-8 [EN-8233]
    isolated::api::DXEndpoint::user(handle_, user);

    return sharedAs<DXEndpoint>();
}

std::shared_ptr<DXEndpoint> DXEndpoint::password(const std::string &password) noexcept {
    // TODO: check invalid utf-8 [EN-8233]
    if constexpr (Debugger::isDebug) {
        Debugger::debug("DXEndpoint{" + handle_.toString() + "}::password(password = " + password + ")");
    }

    isolated::api::DXEndpoint::password(handle_, password);

    return sharedAs<DXEndpoint>();
}

std::shared_ptr<DXEndpoint> DXEndpoint::connect(const std::string &address) noexcept {
    // TODO: check invalid utf-8 [EN-8233]
    if constexpr (Debugger::isDebug) {
        Debugger::debug("DXEndpoint{" + handle_.toString() + "}::connect(address = " + address + ")");
    }

    isolated::api::DXEndpoint::connect(handle_, address);

    return sharedAs<DXEndpoint>();
}

void DXEndpoint::reconnect() noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("DXEndpoint{" + handle_.toString() + "}::reconnect()");
    }

    isolated::api::DXEndpoint::reconnect(handle_);
}

void DXEndpoint::disconnect() noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("DXEndpoint{" + handle_.toString() + "}::disconnect()");
    }

    isolated::api::DXEndpoint::disconnect(handle_);
}

void DXEndpoint::disconnectAndClear() {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("DXEndpoint{" + handle_.toString() + "}::disconnectAndClear()");
    }

    isolated::api::IsolatedDXEndpoint::disconnectAndClear(handle_);
}

void DXEndpoint::awaitNotConnected() {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("DXEndpoint{" + handle_.toString() + "}::awaitNotConnected()");
    }

    isolated::api::IsolatedDXEndpoint::awaitNotConnected(handle_);
}

void DXEndpoint::awaitProcessed() {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("DXEndpoint{" + handle_.toString() + "}::awaitProcessed()");
    }

    isolated::api::IsolatedDXEndpoint::awaitProcessed(handle_);
}

void DXEndpoint::closeAndAwaitTermination() noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("DXEndpoint{" + handle_.toString() + "}::closeAndAwaitTermination()");
    }

    if (!handle_) {
        return;
    }

    runIsolatedOrElse(
        [handle = static_cast<dxfg_endpoint_t *>(handle_.get())](auto threadHandle) {
            return dxfg_DXEndpoint_closeAndAwaitTermination(static_cast<graal_isolatethread_t *>(threadHandle),
                                                            handle) == 0;
        },
        false);

    // TODO: close the Feed and Publisher
}

std::shared_ptr<DXFeed> DXEndpoint::getFeed() noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("DXEndpoint{" + handle_.toString() + "}::getFeed()");
    }

    if (feed_) {
        return feed_;
    }

    auto feedHandle =
        !handle_ ? nullptr
                 : runIsolatedOrElse(
                       [handle = static_cast<dxfg_endpoint_t *>(handle_.get())](auto threadHandle) {
                           return dxfg_DXEndpoint_getFeed(static_cast<graal_isolatethread_t *>(threadHandle), handle);
                       },
                       nullptr);

    feed_ = DXFeed::create(feedHandle);

    return feed_;
}

std::shared_ptr<DXPublisher> DXEndpoint::getPublisher() noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("DXEndpoint{" + handle_.toString() + "}::getPublisher()");
    }

    if (publisher_) {
        return publisher_;
    }

    auto publisherHandle =
        !handle_
            ? nullptr
            : runIsolatedOrElse(
                  [handle = static_cast<dxfg_endpoint_t *>(handle_.get())](auto threadHandle) {
                      return dxfg_DXEndpoint_getPublisher(static_cast<graal_isolatethread_t *>(threadHandle), handle);
                  },
                  nullptr);

    publisher_ = DXPublisher::create(publisherHandle);

    return publisher_;
}

std::shared_ptr<DXEndpoint::Builder> DXEndpoint::Builder::create() noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("DXEndpoint::Builder::create()");
    }

    auto builder = std::shared_ptr<Builder>(new (std::nothrow) Builder{});

    if (builder) {
        builder->handle_ = JavaObjectHandle<DXEndpoint::Builder>(runIsolatedOrElse(
            [](auto threadHandle) {
                return dxfg_DXEndpoint_newBuilder(static_cast<graal_isolatethread_t *>(threadHandle));
            },
            nullptr));
    }

    return builder;
}

void DXEndpoint::Builder::loadDefaultPropertiesImpl() noexcept {
    // The default properties file is only valid for the
    // Feed, OnDemandFeed and Publisher roles.
    const auto &propertiesFileKey = [](auto role) constexpr -> const auto & {
        switch (role) {
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

        return String::EMPTY;
    }(role_);

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
        if (!handle_) {
            return;
        }

        // The default property file has the same value as the key.
        runIsolatedOrElse(
            [key = propertiesFileKey, value = propertiesFileKey,
             handle = static_cast<dxfg_endpoint_builder_t *>(handle_.get())](auto threadHandle) {
                return dxfg_DXEndpoint_Builder_withProperty(static_cast<graal_isolatethread_t *>(threadHandle), handle,
                                                            key.c_str(), value.c_str()) == 0;
            },
            false);
    }
}

std::shared_ptr<DXEndpoint::Builder> DXEndpoint::Builder::withRole(DXEndpoint::Role role) noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("DXEndpoint::Builder{" + handle_.toString() + "}::withRole(role = " + roleToString(role) + ")");
    }

    role_ = role;

    if (!handle_) {
        return shared_from_this();
    }

    runIsolatedOrElse(
        [role = role, handle = static_cast<dxfg_endpoint_builder_t *>(handle_.get())](auto threadHandle) {
            return dxfg_DXEndpoint_Builder_withRole(static_cast<graal_isolatethread_t *>(threadHandle), handle,
                                                    roleToGraalRole(role)) == 0;
        },
        false);

    return shared_from_this();
}

std::shared_ptr<DXEndpoint::Builder> DXEndpoint::Builder::withProperty(const std::string &key,
                                                                       const std::string &value) noexcept {
    // TODO: check invalid utf-8 [EN-8233]
    if constexpr (Debugger::isDebug) {
        Debugger::debug("DXEndpoint::Builder{" + handle_.toString() + "}::withProperty(key = " + key +
                        ", value = " + value + ")");
    }

    properties_[key] = value;

    if (!handle_) {
        return shared_from_this();
    }

    runIsolatedOrElse(
        [key = key, value = value, handle = static_cast<dxfg_endpoint_builder_t *>(handle_.get())](auto threadHandle) {
            return dxfg_DXEndpoint_Builder_withProperty(static_cast<graal_isolatethread_t *>(threadHandle), handle,
                                                        key.c_str(), value.c_str()) == 0;
        },
        false);

    return shared_from_this();
}

bool DXEndpoint::Builder::supportsProperty(const std::string &key) noexcept {
    // TODO: check invalid utf-8 [EN-8233]
    if constexpr (Debugger::isDebug) {
        Debugger::debug("DXEndpoint::Builder{" + handle_.toString() + "}::supportsProperty(key = " + key + ")");
    }

    if (!handle_) {
        return false;
    }

    return runIsolatedOrElse(
        [key = key, handle = static_cast<dxfg_endpoint_builder_t *>(handle_.get())](auto threadHandle) {
            return dxfg_DXEndpoint_Builder_supportsProperty(static_cast<graal_isolatethread_t *>(threadHandle), handle,
                                                            key.c_str()) != 0;
        },
        false);
}

std::shared_ptr<DXEndpoint> DXEndpoint::Builder::build() noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("DXEndpoint::Builder{" + handle_.toString() + "}::build()");
    }

    loadDefaultPropertiesImpl();

    auto endpointHandle =
        !handle_
            ? nullptr
            : runIsolatedOrElse(
                  [handle = static_cast<dxfg_endpoint_builder_t *>(handle_.get())](auto threadHandle) {
                      return dxfg_DXEndpoint_Builder_build(static_cast<graal_isolatethread_t *>(threadHandle), handle);
                  },
                  nullptr);

    return DXEndpoint::create(endpointHandle, role_, properties_);
}

DXEndpoint::Builder::Builder() noexcept : handle_{}, properties_{} {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("DXEndpoint::Builder::Builder()");
    }
}

DXEndpoint::Builder::~Builder() noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("DXEndpoint::Builder{" + handle_.toString() + "}::~Builder()");
    }
}

std::shared_ptr<DXEndpoint::Builder> DXEndpoint::Builder::withName(const std::string &name) noexcept {
    // TODO: check invalid utf-8 [EN-8233]
    if constexpr (Debugger::isDebug) {
        Debugger::debug("DXEndpoint::Builder{" + handle_.toString() + "}::withName(name = " + name + ")");
    }

    return withProperty(NAME_PROPERTY, name);
}

std::string DXEndpoint::toString() const noexcept {
    return fmt::format("DXEndpoint{{{}}}", handle_.toString());
}

std::string DXEndpoint::roleToString(DXEndpoint::Role role) {
    switch (role) {
    case Role::FEED:
        return "FEED";
    case Role::ON_DEMAND_FEED:
        return "ON_DEMAND_FEED";
    case Role::STREAM_FEED:
        return "STREAM_FEED";
    case Role::PUBLISHER:
        return "PUBLISHER";
    case Role::STREAM_PUBLISHER:
        return "STREAM_PUBLISHER";
    case Role::LOCAL_HUB:
        return "LOCAL_HUB";
    }

    return "";
}

std::string DXEndpoint::stateToString(DXEndpoint::State state) {
    switch (state) {
    case State::NOT_CONNECTED:
        return "NOT_CONNECTED";
    case State::CONNECTING:
        return "CONNECTING";
    case State::CONNECTED:
        return "CONNECTED";
    case State::CLOSED:
        return "CLOSED";
    }

    return "";
}

DXEndpoint::DXEndpoint() noexcept
    : handle_{}, role_{}, feed_{}, publisher_{}, stateChangeListenerHandle_{}, onStateChange_{},
      impl_(std::make_unique<DXEndpoint::Impl>()) {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("DXEndpoint()");
    }
}

DXEndpoint::~DXEndpoint() noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("DXEndpoint{" + handle_.toString() + "}::~DXEndpoint()");
    }
}

std::shared_ptr<DXEndpoint> DXEndpoint::getInstance() noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("DXEndpoint::getInstance()");
    }

    return getInstance(Role::FEED);
}

std::shared_ptr<DXEndpoint> DXEndpoint::getInstance(DXEndpoint::Role role) noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("DXEndpoint::getInstance(role = " + roleToString(role) + ")");
    }

    return Impl::getInstance(role);
}

std::shared_ptr<DXEndpoint::Builder> DXEndpoint::newBuilder() noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("DXEndpoint::newBuilder()");
    }

    return DXEndpoint::Builder::create();
}

std::shared_ptr<DXEndpoint> DXEndpoint::create() noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("DXEndpoint::create()");
    }

    return newBuilder()->build();
}

std::shared_ptr<DXEndpoint> DXEndpoint::create(DXEndpoint::Role role) noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("DXEndpoint::create(role = " + roleToString(role) + ")");
    }

    return newBuilder()->withRole(role)->build();
}

DXEndpoint::Role DXEndpoint::getRole() const noexcept {
    return role_;
}

bool DXEndpoint::isClosed() const {
    return getState() == State::CLOSED;
}

const std::string &DXEndpoint::getName() const & noexcept {
    return name_;
}

void DXEndpoint::removeStateChangeListener(std::size_t listenerId) noexcept {
    onStateChange_ -= listenerId;
}

SimpleHandler<void(DXEndpoint::State, DXEndpoint::State)> &DXEndpoint::onStateChange() noexcept {
    return onStateChange_;
}

void DXEndpoint::close() noexcept {
    if constexpr (Debugger::isDebug) {
        Debugger::debug("DXEndpoint{" + handle_.toString() + "}::close()");
    }

    isolated::api::DXEndpoint::close(handle_);
}

std::unordered_set<EventTypeEnum> DXEndpoint::getEventTypes() noexcept {
    return {EventTypeEnum::ALL.begin(), EventTypeEnum::ALL.end()};
    //return isolated::api::IsolatedDXEndpoint::getEventTypes(handle_);
}

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
        : endpoint{std::move(endpoint)}, userData{userData}, listeners{} {
    }
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

DXFCPP_END_NAMESPACE

dxfc_error_code_t dxfc_dxendpoint_new_builder(DXFC_OUT dxfc_dxendpoint_builder_t *builderHandle) {
    if (!builderHandle) {
        return DXFC_EC_ERROR;
    }

    auto result = dxfcpp::BuilderRegistry::add(dxfcpp::DXEndpoint::newBuilder());

    if (!result) {
        return DXFC_EC_ERROR;
    }

    *builderHandle = static_cast<dxfc_dxendpoint_builder_t>(result);

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_builder_with_role(dxfc_dxendpoint_builder_t builderHandle,
                                                    dxfc_dxendpoint_role_t role) {
    if (!builderHandle) {
        return DXFC_EC_ERROR;
    }

    auto builder = dxfcpp::BuilderRegistry::get(static_cast<dxfcpp::BuilderHandle *>(builderHandle));

    if (!builder) {
        return DXFC_EC_ERROR;
    }

    builder->withRole(dxfcpp::cApiRoleToRole(role));

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_builder_with_name(dxfc_dxendpoint_builder_t builderHandle, const char *name) {
    if (!builderHandle || !name) {
        return DXFC_EC_ERROR;
    }

    auto builder = dxfcpp::BuilderRegistry::get(static_cast<dxfcpp::BuilderHandle *>(builderHandle));

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

    auto builder = dxfcpp::BuilderRegistry::get(static_cast<dxfcpp::BuilderHandle *>(builderHandle));

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

    auto builder = dxfcpp::BuilderRegistry::get(static_cast<dxfcpp::BuilderHandle *>(builderHandle));

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

    auto builder = dxfcpp::BuilderRegistry::get(static_cast<dxfcpp::BuilderHandle *>(builderHandle));

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

    auto builder = dxfcpp::BuilderRegistry::get(static_cast<dxfcpp::BuilderHandle *>(builderHandle));

    if (!builder) {
        return DXFC_EC_ERROR;
    }

    auto endpoint = builder->build();

    if (!endpoint) {
        return DXFC_EC_ERROR;
    }

    auto result = dxfcpp::EndpointWrapperRegistry::add(std::make_shared<dxfcpp::EndpointWrapper>(endpoint, userData));

    if (!result) {
        return DXFC_EC_ERROR;
    }

    *endpointHandle = static_cast<dxfc_dxendpoint_t>(result);

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_builder_free(dxfc_dxendpoint_builder_t builderHandle) {
    if (!builderHandle) {
        return DXFC_EC_ERROR;
    }

    if (!dxfcpp::BuilderRegistry::remove(static_cast<dxfcpp::BuilderHandle *>(builderHandle))) {
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

    auto result = dxfcpp::EndpointWrapperRegistry::add(std::make_shared<dxfcpp::EndpointWrapper>(endpoint, userData));

    if (!result) {
        return DXFC_EC_ERROR;
    }

    *endpointHandle = static_cast<dxfc_dxendpoint_t>(result);

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_get_instance2(dxfc_dxendpoint_role_t role, void *userData,
                                                DXFC_OUT dxfc_dxendpoint_t *endpointHandle) {
    if (!endpointHandle) {
        return DXFC_EC_ERROR;
    }

    auto endpoint = dxfcpp::DXEndpoint::getInstance(dxfcpp::cApiRoleToRole(role));

    if (!endpoint) {
        return DXFC_EC_ERROR;
    }

    auto result = dxfcpp::EndpointWrapperRegistry::add(std::make_shared<dxfcpp::EndpointWrapper>(endpoint, userData));

    if (!result) {
        return DXFC_EC_ERROR;
    }

    *endpointHandle = static_cast<dxfc_dxendpoint_t>(result);

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

    auto result = dxfcpp::EndpointWrapperRegistry::add(std::make_shared<dxfcpp::EndpointWrapper>(endpoint, userData));

    if (!result) {
        return DXFC_EC_ERROR;
    }

    *endpointHandle = static_cast<dxfc_dxendpoint_t>(result);

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_create2(dxfc_dxendpoint_role_t role, void *userData,
                                          DXFC_OUT dxfc_dxendpoint_t *endpointHandle) {
    if (!endpointHandle) {
        return DXFC_EC_ERROR;
    }

    auto endpoint = dxfcpp::DXEndpoint::create(dxfcpp::cApiRoleToRole(role));

    if (!endpoint) {
        return DXFC_EC_ERROR;
    }

    auto result = dxfcpp::EndpointWrapperRegistry::add(std::make_shared<dxfcpp::EndpointWrapper>(endpoint, userData));

    if (!result) {
        return DXFC_EC_ERROR;
    }

    *endpointHandle = static_cast<dxfc_dxendpoint_t>(result);

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_close(dxfc_dxendpoint_t endpointHandle) {
    if (!endpointHandle) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper =
        dxfcpp::EndpointWrapperRegistry::get(static_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle));

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

    auto endpointWrapper =
        dxfcpp::EndpointWrapperRegistry::get(static_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle));

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

    auto endpointWrapper =
        dxfcpp::EndpointWrapperRegistry::get(static_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    *role = dxfcpp::roleToCApiRole(endpointWrapper->endpoint->getRole());

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_user(dxfc_dxendpoint_t endpointHandle, const char *user) {
    if (!endpointHandle || !user) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper =
        dxfcpp::EndpointWrapperRegistry::get(static_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle));

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

    auto endpointWrapper =
        dxfcpp::EndpointWrapperRegistry::get(static_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle));

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

    auto endpointWrapper =
        dxfcpp::EndpointWrapperRegistry::get(static_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle));

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

    auto endpointWrapper =
        dxfcpp::EndpointWrapperRegistry::get(static_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle));

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

    auto endpointWrapper =
        dxfcpp::EndpointWrapperRegistry::get(static_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle));

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

    auto endpointWrapper =
        dxfcpp::EndpointWrapperRegistry::get(static_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle));

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

    auto endpointWrapper =
        dxfcpp::EndpointWrapperRegistry::get(static_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle));

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

    auto endpointWrapper =
        dxfcpp::EndpointWrapperRegistry::get(static_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle));

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

    auto endpointWrapper =
        dxfcpp::EndpointWrapperRegistry::get(static_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    *state = dxfcpp::stateToCApiState(endpointWrapper->endpoint->getState());

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_add_state_change_listener(dxfc_dxendpoint_t endpointHandle,
                                                            dxfc_dxendpoint_state_change_listener listener) {
    if (!endpointHandle || !listener) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper =
        dxfcpp::EndpointWrapperRegistry::get(static_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle));

    if (!endpointWrapper) {
        return DXFC_EC_ERROR;
    }

    if (endpointWrapper->listeners.contains(listener)) {
        return DXFC_EC_SUCCESS;
    }

    endpointWrapper->listeners[listener] = endpointWrapper->endpoint->onStateChange() +=
        [userData = endpointWrapper->userData, listener](dxfcpp::DXEndpoint::State oldState,
                                                         dxfcpp::DXEndpoint::State newState) {
            listener(dxfcpp::stateToCApiState(oldState), dxfcpp::stateToCApiState(newState), userData);
        };

    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_remove_state_change_listener(dxfc_dxendpoint_t endpointHandle,
                                                               dxfc_dxendpoint_state_change_listener listener) {
    if (!endpointHandle || !listener) {
        return DXFC_EC_ERROR;
    }

    auto endpointWrapper =
        dxfcpp::EndpointWrapperRegistry::get(static_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle));

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
dxfc_error_code_t dxfc_dxendpoint_get_feed(dxfc_dxendpoint_t /*endpointHandle*/, DXFC_OUT dxfc_dxfeed_t * /*feed*/) {
    return DXFC_EC_SUCCESS;
}

// TODO: implement
dxfc_error_code_t dxfc_dxendpoint_get_publisher(dxfc_dxendpoint_t /*endpointHandle*/,
                                                DXFC_OUT dxfc_dxpublisher_t * /*publisher*/) {
    return DXFC_EC_SUCCESS;
}

dxfc_error_code_t dxfc_dxendpoint_free(dxfc_dxendpoint_t endpointHandle) {
    if (!endpointHandle) {
        return DXFC_EC_ERROR;
    }

    if (!dxfcpp::EndpointWrapperRegistry::remove(static_cast<dxfcpp::EndpointWrapperHandle *>(endpointHandle))) {
        return DXFC_EC_ERROR;
    }

    return DXFC_EC_SUCCESS;
}
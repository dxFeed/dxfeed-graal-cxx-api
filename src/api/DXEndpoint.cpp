// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/api/DXEndpoint.hpp"

#include "../../include/dxfeed_graal_c_api/api.h"
#include "../../include/dxfeed_graal_cpp_api/isolated/api/IsolatedDXEndpoint.hpp"
#include "../../include/dxfeed_graal_cpp_api/system/System.hpp"

#include <dxfg_api.h>
#include <memory>
#include <string>
#include <utility>

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

struct DXEndpoint::Impl {
    static std::mutex MTX;
    static std::unordered_map<Role, std::shared_ptr<DXEndpoint>> INSTANCES;

    static void onPropertyChange(graal_isolatethread_t * /*thread*/, dxfg_endpoint_state_t oldState,
                                 dxfg_endpoint_state_t newState, void *userData) {
        const auto id = Id<DXEndpoint>::from(dxfcpp::bit_cast<Id<DXEndpoint>::ValueType>(userData));
        const auto endpoint = ApiContext::getInstance()->getManager<EntityManager<DXEndpoint>>()->getEntity(id);

        if constexpr (Debugger::isDebug) {
            // ReSharper disable once CppDFAUnreachableCode
            Debugger::debug("onStateChange: id = " + std::to_string(id.getValue()) +
                            ", endpoint = " + ((endpoint) ? endpoint->toString() : "nullptr"));
        }

        if (endpoint) {
            endpoint->onStateChange_(isolated::api::IsolatedDXEndpoint::graalStateToState(oldState),
                                     isolated::api::IsolatedDXEndpoint::graalStateToState(newState));

            if (newState == DXFG_ENDPOINT_STATE_CLOSED) {
                // TODO: fix endpoint lifetime
                // ApiContext::getInstance()->getManager<EntityManager<DXEndpoint>>()->unregisterEntity(id);
            }
        }
    };

    static std::shared_ptr<DXEndpoint> getInstance(Role role) noexcept {
        if constexpr (Debugger::isDebug) {
            // ReSharper disable once CppDFAUnreachableCode
            Debugger::debug("DXEndpoint::Impl::getInstance(role = " + roleToString(role) + ")");
        }

        std::lock_guard lock(MTX);

        if (INSTANCES.contains(role)) {
            return INSTANCES[role];
        }

        return INSTANCES[role] = create(role);
    }

    mutable std::mutex mutex_{};
    std::shared_ptr<DXFeed> feed_{};
    std::shared_ptr<DXPublisher> publisher_{};

    std::shared_ptr<DXFeed> getFeed(const JavaObjectHandle<DXEndpoint> &handle) {
        std::lock_guard lock{mutex_};

        if (feed_) {
            return feed_;
        }

        return feed_ = DXFeed::create(isolated::api::IsolatedDXEndpoint::getFeed(handle));
    }

    std::shared_ptr<DXPublisher> getPublisher(const JavaObjectHandle<DXEndpoint> &handle) {
        std::lock_guard lock{mutex_};

        if (publisher_) {
            return publisher_;
        }

        return publisher_ = DXPublisher::create(isolated::api::IsolatedDXEndpoint::getPublisher(handle));
    }
};

std::mutex DXEndpoint::Impl::MTX{};
std::unordered_map<DXEndpoint::Role, std::shared_ptr<DXEndpoint>> DXEndpoint::Impl::INSTANCES{};

std::shared_ptr<DXEndpoint> DXEndpoint::create(void *endpointHandle, Role role,
                                               const std::unordered_map<std::string, std::string> &properties) {
    if (endpointHandle == nullptr) {
        throw InvalidArgumentException("Unable to create DXEndpoint. The `endpointHandle` is nullptr");
    }

    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("DXEndpoint::create(handle = " + dxfcpp::toString(endpointHandle) +
                        ", role = " + roleToString(role) + ", properties[" + std::to_string(properties.size()) + "])");
    }

    auto endpoint = createShared(JavaObjectHandle<DXEndpoint>(endpointHandle), role, properties.at(NAME_PROPERTY));
    const auto id = ApiContext::getInstance()->getManager<EntityManager<DXEndpoint>>()->registerEntity(endpoint);

    endpoint->stateChangeListenerHandle_ = isolated::api::IsolatedDXEndpoint::StateChangeListener::create(
        dxfcpp::bit_cast<void *>(&Impl::onPropertyChange), dxfcpp::bit_cast<void *>(id.getValue()));
    isolated::api::IsolatedDXEndpoint::addStateChangeListener(endpoint->handle_, endpoint->stateChangeListenerHandle_);

    return endpoint;
}

void DXEndpoint::executorImpl(const JavaObjectHandle<ExecutorTag> &executor) const {
    isolated::api::IsolatedDXEndpoint::executor(handle_, executor);
}

DXEndpoint::State DXEndpoint::getState() const {
    return isolated::api::IsolatedDXEndpoint::getState(handle_);
}

std::shared_ptr<DXEndpoint> DXEndpoint::user(const StringLike &user) {
    // TODO: check invalid utf-8 [EN-8233]
    isolated::api::IsolatedDXEndpoint::user(handle_, user);

    return sharedAs<DXEndpoint>();
}

std::shared_ptr<DXEndpoint> DXEndpoint::password(const StringLike &password) {
    // TODO: check invalid utf-8 [EN-8233]
    isolated::api::IsolatedDXEndpoint::password(handle_, password);

    return sharedAs<DXEndpoint>();
}

std::shared_ptr<DXEndpoint> DXEndpoint::connect(const StringLike &address) {
    // TODO: check invalid utf-8 [EN-8233]
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("DXEndpoint{" + handle_.toString() + "}::connect(address = " + address + ")");
    }

    isolated::api::IsolatedDXEndpoint::connect(handle_, address);

    return sharedAs<DXEndpoint>();
}

void DXEndpoint::reconnect() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("DXEndpoint{" + handle_.toString() + "}::reconnect()");
    }

    isolated::api::IsolatedDXEndpoint::reconnect(handle_);
}

void DXEndpoint::disconnect() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("DXEndpoint{" + handle_.toString() + "}::disconnect()");
    }

    isolated::api::IsolatedDXEndpoint::disconnect(handle_);
}

void DXEndpoint::disconnectAndClear() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("DXEndpoint{" + handle_.toString() + "}::disconnectAndClear()");
    }

    isolated::api::IsolatedDXEndpoint::disconnectAndClear(handle_);
}

void DXEndpoint::awaitNotConnected() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("DXEndpoint{" + handle_.toString() + "}::awaitNotConnected()");
    }

    isolated::api::IsolatedDXEndpoint::awaitNotConnected(handle_);
}

void DXEndpoint::awaitProcessed() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("DXEndpoint{" + handle_.toString() + "}::awaitProcessed()");
    }

    isolated::api::IsolatedDXEndpoint::awaitProcessed(handle_);
}

void DXEndpoint::closeAndAwaitTermination() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("DXEndpoint{" + handle_.toString() + "}::closeAndAwaitTermination()");
    }

    isolated::api::IsolatedDXEndpoint::closeAndAwaitTermination(handle_);
}

std::shared_ptr<DXFeed> DXEndpoint::getFeed() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("DXEndpoint{" + handle_.toString() + "}::getFeed()");
    }

    return impl_->getFeed(handle_);
}

std::shared_ptr<DXPublisher> DXEndpoint::getPublisher() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("DXEndpoint{" + handle_.toString() + "}::getPublisher()");
    }

    return impl_->getPublisher(handle_);
}

std::shared_ptr<DXEndpoint::Builder> DXEndpoint::Builder::create() {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("DXEndpoint::Builder::create()");
    }

    auto builder = createShared();

    builder->handle_ = JavaObjectHandle<Builder>(isolated::api::IsolatedDXEndpoint::Builder::create());

    return builder;
}

void DXEndpoint::Builder::loadDefaultPropertiesImpl() {
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
        default:;
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
    // try to load the default properties file from the current runtime directory if the file exists.
    if (!properties_.contains(propertiesFileKey) && std::filesystem::exists(propertiesFileKey)) {
        if (!handle_) {
            return;
        }

        // The default property file has the same value as the key.
        withProperty(propertiesFileKey, propertiesFileKey);
    }
}

std::shared_ptr<DXEndpoint::Builder> DXEndpoint::Builder::withRole(Role role) {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("DXEndpoint::Builder{" + handle_.toString() + "}::withRole(role = " + roleToString(role) + ")");
    }

    role_ = role;

    isolated::api::IsolatedDXEndpoint::Builder::withRole(handle_, role);

    return sharedAs<Builder>();
}

std::shared_ptr<DXEndpoint::Builder> DXEndpoint::Builder::withProperty(const StringLike &key, const StringLike &value) {
    // TODO: check invalid utf-8 [EN-8233]
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("DXEndpoint::Builder{" + handle_.toString() + "}::withProperty(key = " + key +
                        ", value = " + value + ")");
    }

    properties_[key] = value;

    isolated::api::IsolatedDXEndpoint::Builder::withProperty(handle_, key, value);

    return sharedAs<Builder>();
}

bool DXEndpoint::Builder::supportsProperty(const StringLike &key) const {
    // TODO: check invalid utf-8 [EN-8233]
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("DXEndpoint::Builder{" + handle_.toString() + "}::supportsProperty(key = " + key + ")");
    }

    return isolated::api::IsolatedDXEndpoint::Builder::supportsProperty(handle_, key);
}

std::shared_ptr<DXEndpoint> DXEndpoint::Builder::build() {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("DXEndpoint::Builder{" + handle_.toString() + "}::build()");
    }

    loadDefaultPropertiesImpl();

    if (auto name = properties_.contains(NAME_PROPERTY) ? properties_.at(NAME_PROPERTY) : std::string{}; name.empty()) {
        const std::size_t id = ApiContext::getInstance()->getManager<EntityManager<DXEndpoint>>()->getLastId() + 1;

        name = std::string("qdcxx") + ((id <= 1) ? String::EMPTY : std::string("-") + std::to_string(id));

        const auto newBuilder = withProperty(NAME_PROPERTY, name);

        return newBuilder->build();
    }

    return DXEndpoint::create(isolated::api::IsolatedDXEndpoint::Builder::build(handle_), role_, properties_);
}

DXEndpoint::Builder::Builder(LockExternalConstructionTag) noexcept {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("DXEndpoint::Builder::Builder()");
    }

#if defined(DXFCXX_ENABLE_METRICS)
    ApiContext::getInstance()->getManager<dxfcpp::MetricsManager>()->add("Entity.DXEndpoint_Builder", 1);
#endif
}

DXEndpoint::Builder::~Builder() noexcept {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("DXEndpoint::Builder{" + handle_.toString() + "}::~Builder()");
    }

#if defined(DXFCXX_ENABLE_METRICS)
    ApiContext::getInstance()->getManager<dxfcpp::MetricsManager>()->add("Entity.DXEndpoint_Builder", -1);
#endif
}

std::shared_ptr<DXEndpoint::Builder> DXEndpoint::Builder::withName(const StringLike &name) {
    // TODO: check invalid utf-8 [EN-8233]
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("DXEndpoint::Builder{" + handle_.toString() + "}::withName(name = " + name + ")");
    }

    return withProperty(NAME_PROPERTY, name);
}

std::string DXEndpoint::toString() const {
    return std::string("DXEndpoint{") + handle_.toString() + "}";
}

std::string DXEndpoint::roleToString(Role role) {
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

std::string DXEndpoint::stateToString(State state) {
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

DXEndpoint::DXEndpoint(LockExternalConstructionTag) : role_{}, impl_(std::make_unique<Impl>()) {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("DXEndpoint()");
    }

#if defined(DXFCXX_ENABLE_METRICS)
    ApiContext::getInstance()->getManager<dxfcpp::MetricsManager>()->add("Entity.DXEndpoint", 1);
#endif
}

DXEndpoint::DXEndpoint(LockExternalConstructionTag, JavaObjectHandle<DXEndpoint> &&handle, Role role, std::string name)
    : role_{role}, name_{std::move(name)}, impl_(std::make_unique<Impl>()) {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("DXEndpoint()");
    }

    handle_ = std::move(handle);

#if defined(DXFCXX_ENABLE_METRICS)
    ApiContext::getInstance()->getManager<dxfcpp::MetricsManager>()->add("Entity.DXEndpoint", 1);
#endif
}

DXEndpoint::~DXEndpoint() noexcept {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("DXEndpoint{" + handle_.toString() + "}::~DXEndpoint()");
    }

#if defined(DXFCXX_ENABLE_METRICS)
    ApiContext::getInstance()->getManager<dxfcpp::MetricsManager>()->add("Entity.DXEndpoint", -1);
#endif
}

std::shared_ptr<DXEndpoint> DXEndpoint::getInstance() {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("DXEndpoint::getInstance()");
    }

    return getInstance(Role::FEED);
}

std::shared_ptr<DXEndpoint> DXEndpoint::getInstance(Role role) {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("DXEndpoint::getInstance(role = " + roleToString(role) + ")");
    }

    return Impl::getInstance(role);
}

std::shared_ptr<DXEndpoint::Builder> DXEndpoint::newBuilder() {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("DXEndpoint::newBuilder()");
    }

    return Builder::create();
}

std::shared_ptr<DXEndpoint> DXEndpoint::create() {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("DXEndpoint::create()");
    }

    return newBuilder()->build();
}

std::shared_ptr<DXEndpoint> DXEndpoint::create(Role role) {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
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

void DXEndpoint::close() const {
    if constexpr (Debugger::isDebug) {
        // ReSharper disable once CppDFAUnreachableCode
        Debugger::debug("DXEndpoint{" + handle_.toString() + "}::close()");
    }

    isolated::api::IsolatedDXEndpoint::close(handle_);
}

std::unordered_set<EventTypeEnum> DXEndpoint::getEventTypes() const {
    return isolated::api::IsolatedDXEndpoint::getEventTypes(handle_);
}

struct BuilderHandle {};

namespace BuilderRegistry {
static std::shared_ptr<DXEndpoint::Builder> get(BuilderHandle *handle) {
    return ApiContext::getInstance()->getManager<EntityManager<DXEndpoint::Builder>>()->getEntity(handle);
}

static BuilderHandle *add(const std::shared_ptr<DXEndpoint::Builder> &builder) noexcept {
    return dxfcpp::bit_cast<BuilderHandle *>(ApiContext::getInstance()
                                                 ->getManager<EntityManager<DXEndpoint::Builder>>()
                                                 ->registerEntity(builder)
                                                 .getValue());
}

static bool remove(BuilderHandle *handle) {
    // ReSharper disable once CppDFAConstantConditions
    if (!handle) {
        // ReSharper disable once CppDFAUnreachableCode
        return false;
    }

    return ApiContext::getInstance()->getManager<EntityManager<DXEndpoint::Builder>>()->unregisterEntity(handle);
}
}; // namespace BuilderRegistry

struct EndpointWrapperHandle {};

struct EndpointWrapper : std::enable_shared_from_this<EndpointWrapper> {
    std::shared_ptr<DXEndpoint> endpoint{};
    void *userData{};
    std::unordered_map<dxfc_dxendpoint_state_change_listener, std::size_t> listeners{};

    EndpointWrapper(std::shared_ptr<DXEndpoint> endpoint, void *userData)
        : endpoint{std::move(endpoint)}, userData{userData} {
    }

    std::string toString() const noexcept {
        return "EndpointWrapper{" + endpoint->toString() + "}";
    }
};

namespace EndpointWrapperRegistry {
static std::shared_ptr<EndpointWrapper> get(EndpointWrapperHandle *handle) {
    return ApiContext::getInstance()->getManager<EntityManager<EndpointWrapper>>()->getEntity(handle);
}

static EndpointWrapperHandle *add(const std::shared_ptr<EndpointWrapper> &endpointWrapper) noexcept {
    return dxfcpp::bit_cast<EndpointWrapperHandle *>(ApiContext::getInstance()
                                                         ->getManager<EntityManager<EndpointWrapper>>()
                                                         ->registerEntity(endpointWrapper)
                                                         .getValue());
}

static bool remove(EndpointWrapperHandle *handle) {
    // ReSharper disable once CppDFAConstantConditions
    if (!handle) {
        // ReSharper disable once CppDFAUnreachableCode
        return false;
    }

    return ApiContext::getInstance()->getManager<EntityManager<EndpointWrapper>>()->unregisterEntity(handle);
}
}; // namespace EndpointWrapperRegistry

static DXEndpoint::Role cApiRoleToRole(dxfc_dxendpoint_role_t role) {
    switch (role) {
    case DXFC_DXENDPOINT_ROLE_FEED:
        return DXEndpoint::Role::FEED;
    case DXFC_DXENDPOINT_ROLE_ON_DEMAND_FEED:
        return DXEndpoint::Role::ON_DEMAND_FEED;
    case DXFC_DXENDPOINT_ROLE_STREAM_FEED:
        return DXEndpoint::Role::STREAM_FEED;
    case DXFC_DXENDPOINT_ROLE_PUBLISHER:
        return DXEndpoint::Role::PUBLISHER;
    case DXFC_DXENDPOINT_ROLE_STREAM_PUBLISHER:
        return DXEndpoint::Role::STREAM_PUBLISHER;
    case DXFC_DXENDPOINT_ROLE_LOCAL_HUB:
        return DXEndpoint::Role::LOCAL_HUB;
    }

    return DXEndpoint::Role::FEED;
}

static dxfc_dxendpoint_role_t roleToCApiRole(DXEndpoint::Role role) {
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

static dxfc_dxendpoint_state_t stateToCApiState(DXEndpoint::State state) {
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

    const auto result = dxfcpp::BuilderRegistry::add(dxfcpp::DXEndpoint::newBuilder());

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

    const auto builder = dxfcpp::BuilderRegistry::get(static_cast<dxfcpp::BuilderHandle *>(builderHandle));

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

    const auto builder = dxfcpp::BuilderRegistry::get(static_cast<dxfcpp::BuilderHandle *>(builderHandle));

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

    const auto builder = dxfcpp::BuilderRegistry::get(static_cast<dxfcpp::BuilderHandle *>(builderHandle));

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

    const auto builder = dxfcpp::BuilderRegistry::get(static_cast<dxfcpp::BuilderHandle *>(builderHandle));

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

    const auto builder = dxfcpp::BuilderRegistry::get(static_cast<dxfcpp::BuilderHandle *>(builderHandle));

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

    const auto builder = dxfcpp::BuilderRegistry::get(static_cast<dxfcpp::BuilderHandle *>(builderHandle));

    if (!builder) {
        return DXFC_EC_ERROR;
    }

    auto endpoint = builder->build();

    if (!endpoint) {
        return DXFC_EC_ERROR;
    }

    const auto result =
        dxfcpp::EndpointWrapperRegistry::add(std::make_shared<dxfcpp::EndpointWrapper>(endpoint, userData));

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

    const auto result =
        dxfcpp::EndpointWrapperRegistry::add(std::make_shared<dxfcpp::EndpointWrapper>(endpoint, userData));

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

    const auto result =
        dxfcpp::EndpointWrapperRegistry::add(std::make_shared<dxfcpp::EndpointWrapper>(endpoint, userData));

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

    const auto result =
        dxfcpp::EndpointWrapperRegistry::add(std::make_shared<dxfcpp::EndpointWrapper>(endpoint, userData));

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

    const auto result =
        dxfcpp::EndpointWrapperRegistry::add(std::make_shared<dxfcpp::EndpointWrapper>(endpoint, userData));

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

    const auto endpointWrapper =
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

    const auto endpointWrapper =
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

    const auto endpointWrapper =
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

    const auto endpointWrapper =
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

    const auto endpointWrapper =
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

    const auto endpointWrapper =
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

    const auto endpointWrapper =
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

    const auto endpointWrapper =
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

    const auto endpointWrapper =
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

    const auto endpointWrapper =
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

    const auto endpointWrapper =
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

    const auto endpointWrapper =
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
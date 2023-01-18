// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <dxfg_endpoint.h>

#include "internal/CEntryPointErrors.hpp"
#include "internal/Common.hpp"
#include "internal/Isolate.hpp"

#include <iostream>
#include <string>
#include <type_traits>
#include <variant>

#include <fmt/format.h>
#include <fmt/std.h>

namespace dxfcpp {
/**
 * Manages network connections to @ref DXFeed "feed" or
 * @ref DXPublisher "publisher". There are per-process (per GraalVM Isolate for now) ready-to-use singleton instances
 * that are available with ::getInstance() and ::getInstance(Role) methods as well as
 * factory methods ::create() and ::create(Role), and a number of configuration methods. Advanced
 * properties can be configured using
 * @ref ::newBuilder() "newBuilder()".@ref Builder#withProperty(const std::string&, const std::string&)
 * "withProperty(key, value)".@ref Builder::build() "build()".
 *
 * See DXFeed for details on how to subscribe to symbols and receive events.
 *
 * <h3>Endpoint role</h3>
 *
 * Each endpoint has a role that is specified on its creation and cannot be changed afterwards.
 * The default factory method ::create() creates an endpoint with a @ref Role::FEED "FEED" role.
 * Endpoints with other roles are created with ::create(Role) factory method. Endpoint role is
 * represented by @ref Role "DXEndpoint::Role" enumeration.
 *
 * Endpoint role defines the behavior of its @ref #connect(const std::string&) "connect" method:
 *
 * - @ref Role::FEED "FEED" connects to the remote data feed provider and is optimized for real-time or
 *   delayed data processing (<b>this is a default role</b>).
 *   ::getFeed() method returns a feed object that subscribes to this remote data feed provider and receives events
 *   from it. When event processing threads cannot keep up (don't have enough CPU time), data is dynamically conflated
 *   to minimize latency between received events and their processing time.
 *   For example:
 *   - <b>`DXEndpoint::create()->connect("demo.dxfeed.com:7300")->getFeed()`</b> returns a demo feed from dxFeed with
 *     sample market quotes.
 *   - <b>`DXEndpoint::create()->connect("localhost:7400")->getFeed()`</b> returns a feed that is connected to a
 *     publisher that is running on the same host. See example below.
 *   - <b>`DXEndpoint::create()->connect("file:demo-sample.data")->getFeed()`</b> returns a feed that is connected to
 *     a "demo-sample.data" file and plays back it as if it was received in real time.
 *
 *   This endpoint is automatically connected to the configured data feed as explained in
 *   <a href="#defaultPropertiesSection">default properties section</a>.
 * - @ref Role::ON_DEMAND_FEED "ON_DEMAND_FEED" is similar to @ref Role::FEED "FEED", but it is designed to be used with
 *   OnDemandService for historical data replay only. It is configured with <a href="#defaultPropertiesSection">default
 * properties</a>, but is not connected automatically to the data provider until @ref OnDemandService#replay(Date,
 * double) "OnDemandService.replay" method is invoked.
 * - @ref Role::STREAM_FEED "STREAM_FEED" is similar to @ref Role::FEED "FEED" and also connects to the remote data
 *   feed provider, but is designed for bulk parsing of data from files. DXEndpoint::getFeed() method returns feed
 *   object that subscribes to the data from the opened files and receives events from them. Events from the files are
 *   not conflated and are processed as fast as possible. Note, that in this role, DXFeed::getLastEvent method does not
 *   work and time-series subscription is not supported.
 *   For example:
 *   ```cpp
 *   auto endpoint = DXEndpoint::create(DXEndpoint::Role::STREAM_FEED);
 *   auto feed = endpoint->getFeed();
 *   ```
 *   creates a feed that is ready to read data from file as soon as the following code is invoked:
 *   ```cpp
 *   endpoint->connect("file:demo-sample.data[speed=max]");
 *   ```
 *   "[speed=max]" clause forces to the file reader to play back all the data from "demo-sample.data" file as fast as
 *   data subscribers are processing it.
 * - @ref Role::PUBLISHER "PUBLISHER" connects to the remote publisher hub (also known as multiplexor) or creates a
 *   publisher on the local host. ::getPublisher() method returns a publisher object that publishes events to all
 *   connected feeds.
 *   For example: <b>`DXEndpoint->create(DXEndpoint::Role::PUBLISHER)->connect(":7400")->getPublisher()`</b>
 *   returns a publisher that is waiting for connections on TCP/IP port 7400. The published events will be delivered to
 *   all feeds that are connected to this publisher.
 *   This endpoint is automatically connected to the configured data feed as explained in <a
 * href="#defaultPropertiesSection">default properties section</a>.
 * - @ref Role::LOCAL_HUB "LOCAL_HUB" creates a local hub without ability to establish network connections.
 *   Events that are published via {@link #getPublisher() publisher} are delivered to local @ref ::getFeed() "feed"
 * only.
 *
 * <h3>Endpoint state</h3>
 *
 * Each endpoint has a state that can be retrieved with ::getState() method.
 * When endpoint is created with any role and default address is not specified in
 * <a href="#defaultPropertiesSection">default properties</a>, then it is not connected to any remote endpoint.
 * Its state is @ref State::NOT_CONNECTED "NOT_CONNECTED".
 *
 * @ref Role#FEED "Feed" and @ref Role#PUBLISHER "publisher" endpoints can connect to remote endpoints of the opposite
 * role. Connection is initiated by @ref ::connect(const std::string&) "connect" method.
 * The endpoint state becomes @ref State::CONNECTING "CONNECTING".
 *
 * When the actual connection to the remote endpoint is established, the endpoint state becomes
 * @ref State::CONNECTED "CONNECTED".
 *
 * Network connections can temporarily break and return endpoint back into @ref State::CONNECTING "CONNECTING" state.
 * File connections can be completed and return endpoint into @ref State::NOT_CONNECTED "NOT_CONNECTED" state.
 *
 * Connection to the remote endpoint can be terminated with ::disconnect() method.
 * The endpoint state becomes @ref State::NOT_CONNECTED "NOT_CONNECTED".
 *
 * Endpoint can be closed with ::close() method. The endpoint state becomes @ref State::CLOSED "CLOSED". This is a
 * final state. All connection are terminated and all internal resources that are held by this endpoint are freed.
 * No further connections can be initiated.
 *
 * <h3>Event times</h3>
 *
 * The EventType::getEventTime() on received events is available only when the endpoint is created with
 * ::DXENDPOINT_EVENT_TIME_PROPERTY property and the data source has embedded event times. This is typically true only
 * for data events that are read from historical tape files (see above) and from OnDemandService.
 * Events that are coming from a network connections do not have an embedded event time information and
 * event time is not available for them anyway.
 *
 * <h3><a name="defaultPropertiesSection">Default properties</a></h3>
 *
 * Default properties are loaded from "dxfeed.properties" or "dxpublisher.properties" file depending on
 * the @ref Role "role" of created endpoint. "dxfeed.properties" is used for
 * @ref Role::FEED "FEED" and @ref Role::ON_DEMAND_FEED "ON_DEMAND_FEED",
 * "dxpublisher.properties" is used for @ref Role::PUBLISHER "PUBLISHER"}.
 * @ref Role::STREAM_FEED "STREAM_FEED" and @ref Role::LOCAL_HUB "LOCAL_HUB" do not support properties file.
 *
 * The location of this file can be specified using
 * @ref Builder::withProperty(const std::string&, const std::string&) "withProperty"(::DXFEED_PROPERTIES_PROPERTY, path)
 * or
 * @ref Builder::withProperty(const std::string&, const std::string&) "withProperty"(::DXPUBLISHER_PROPERTIES_PROPERTY,
 * path) correspondingly. When the location of this file is not explicitly specified using
 * @ref Builder::withProperty(const std::string&, const std::string&) "withProperty" method, then the file path is taken
 * from a system property with the corresponding name.
 *
 * When the path to the above properties file is not provided, then a resource named "dxfeed.properties" or
 * "dxpublisher.properties" is loaded from classpath. When classpath is set to "." (current directory),
 * it means that the corresponding file can be placed into the current directory with any need to specify additional
 * properties.
 *
 * Defaults for individual properties can be also provided using system properties when they are not specified
 * in the configuration file. System properties override configuration loaded from classpath resource, but don't
 * override configuration from the user-specified configuration file.
 *
 * The ::NAME_PROPERTY is the exception from the above rule. It is never loaded from system properties.
 * It can be only specified in configuration file or programmatically. There is a convenience
 * @ref Builder::withName(const std::string&) "Builder.withName" method for it. It is recommended to assign short and
 * meaningful endpoint names when multiple endpoints are used in the same process (one GraalVM Isolate for now).
 * The name of the endpoint shall describe its role in the particular application.
 *
 * Note, that individual properties that are programmatically set using @ref Builder::withProperty(const std::string&,
 * const std::string&) "withProperty" method always take precedence.
 *
 * @ref Role::FEED "FEED" and @ref Role::PUBLISHER "PUBLISHER" automatically establish connection on creation
 * when the corresponding ::DXFEED_ADDRESS_PROPERTY or ::DXPUBLISHER_ADDRESS_PROPERTY is specified.
 *
 * <h3>Permanent subscription</h3>
 *
 * Endpoint properties can define permanent subscription for specific sets of symbols and event types in
 * the data feed, so that DXFeed methods like @ref DXFeed::getLastEventIfSubscribed "getLastEventIfSubscribed",
 * @ref DXFeed::getIndexedEventsIfSubscribed "getIndexedEventsIfSubscribed", and
 * @ref DXFeed::getTimeSeriesIfSubscribed "getTimeSeriesIfSubscribed" can be used without a need to create a
 * separate DXFeedSubscription object. Please, contact dxFeed support for details
 * on the required configuration.
 *
 * <h3>Threads and locks</h3>
 *
 * This class is thread-safe and can be used concurrently from multiple threads without external synchronization.
 */
struct DXEndpoint {
    /**
     * Defines property for endpoint name that is used to distinguish multiple endpoints
     * in the same process in logs and in other diagnostic means.
     * Use Builder::withProperty(const std::string&, const std::string&) method.
     * This property is also changed by Builder::withName(const std::string&) method.
     */
    static const std::string NAME_PROPERTY;

    /**
     * Defines path to a file with properties for an endpoint with role @ref Role::FEED "FEED" or
     * @ref Role::ON_DEMAND_FEED "ON_DEMAND_FEED".
     * By default, properties a loaded from a path resource named "dxfeed.properties".
     *
     * @see Builder::withProperty(const std::string&, const std::string&)
     */
    static const std::string DXFEED_PROPERTIES_PROPERTY;

    /**
     * Defines default connection address for an endpoint with role @ref Role::FEED "FEED"
     * or @ref Role::ON_DEMAND_FEED "ON_DEMAND_FEED".
     * Connection is established to this address by role @ref Role::FEED "FEED" as soon as endpoint is created, while
     * role @ref Role::ON_DEMAND_FEED "ON_DEMAND_FEED" waits until OnDemandService::replay(Date, double) is invoked
     * before connecting.
     *
     * By default, without this property, connection is not established until @ref ::connect(const std::string&)
     * "connect(address)" is invoked.
     *
     * Credentials for access to premium services may be configured with
     * ::DXFEED_USER_PROPERTY and ::DXFEED_PASSWORD_PROPERTY.
     *
     * @see Builder::withProperty(const std::string&, const std::string&)
     */
    static const std::string DXFEED_ADDRESS_PROPERTY;

    /**
     * Defines default user name for an endpoint with role @ref Role::FEED "FEED" or @ref Role::ON_DEMAND_FEED
     * "ON_DEMAND_FEED".
     *
     * @see ::user(const std::string&)
     */
    static const std::string DXFEED_USER_PROPERTY;

    /**
     * Defines default password for an endpoint with role @ref Role::FEED "FEED" or @ref Role::ON_DEMAND_FEED
     * "ON_DEMAND_FEED".
     *
     * @see ::password(const std::string&)
     */
    static const std::string DXFEED_PASSWORD_PROPERTY;

    /**
     * Defines thread pool size for an endpoint with role @ref Role::FEED "FEED".
     * By default, the thread pool size is equal to the number of available processors.
     * @see Builder::withProperty(const std::string&, const std::string&)
     */
    static const std::string DXFEED_THREAD_POOL_SIZE_PROPERTY;

    /**
     * Defines data aggregation period an endpoint with role @ref Role::FEED "FEED" that
     * limits the rate of data notifications. For example, setting the value of this property
     * to "0.1s" limits notification to once every 100ms (at most 10 per second).
     * @see Builder::withProperty(const std::string&, const std::string&)
     */
    static const std::string DXFEED_AGGREGATION_PERIOD_PROPERTY;

    /**
     * Set this property to `true` to turns on wildcard support.
     * By default, the endpoint does not support wildcards. This property is needed for
     * WildcardSymbol support and for the use of "tape:..." address in DXPublisher.
     */
    static const std::string DXFEED_WILDCARD_ENABLE_PROPERTY;

    /**
     * Defines path to a file with properties for an endpoint with role @ref Role::PUBLISHER "PUBLISHER".
     * By default, properties a loaded from a classpath resource named "dxpublisher.properties".
     * @see Builder::withProperty(const std::string&, const std::string&)
     */
    static const std::string DXPUBLISHER_PROPERTIES_PROPERTY;

    /**
     * Defines default connection address for an endpoint with role @ref Role::PUBLISHER "PUBLISHER".
     * Connection is established to this address as soon as endpoint is created.
     * By default, connection is not established until ::connect(const std::string&) is invoked.
     * @see Builder::withProperty(const std::string&, const std::string&)
     */
    static const std::string DXPUBLISHER_ADDRESS_PROPERTY;

    /**
     * Defines thread pool size for an endpoint with role @ref Role::PUBLISHER "PUBLISHER".
     * By default, the thread pool size is equal to the number of available processors.
     * @see Builder#withProperty(const std::string&, const std::string&)
     */
    static const std::string DXPUBLISHER_THREAD_POOL_SIZE_PROPERTY;

    /**
     * Set this property to `true` to enable @ref EventType::getEventTime() "event time" support.
     * By default, the endpoint does not support event time.
     *
     * The event time is available only when the corresponding DXEndpoint is created with this property and
     * the data source has embedded event times. This is typically true only for data events
     * that are read from historical tape files and from OnDemandService.
     * Events that are coming from a network connections do not have an embedded event time information and
     * event time is not available for them anyway.
     *
     * Use this property if you need to work with historical data coming from files
     * or from OnDemandService or writing data with times to file via DXPublisher using "tape:..." address.
     */
    static const std::string DXENDPOINT_EVENT_TIME_PROPERTY;

    /**
     * Set this property to  to store all @ref LastingEvent "lasting" and @ref IndexedEvent "indexed" events even when
     * there is no subscription on them. By default, the endpoint stores only events from subscriptions. It works in
     * the same way both for DXFeed and DXPublisher.
     *
     * Use this property with extreme care,
     * since API does not currently provide any means to remove those events from the storage and there might
     * be an effective memory leak if the spaces of symbols on which events are published grows without bound.
     */
    static const std::string DXENDPOINT_STORE_EVERYTHING_PROPERTY;

    /**
     * Set this property to `true` to turn on nanoseconds precision business time.
     * By default, this feature is turned off.
     * Business time in most events is available with
     * millisecond precision by default, while Quote events business @ref Quote::getTime() "time" is available with
     * seconds precision.
     *
     * This method provides a higher-level control than turning on individual properties that are responsible
     * for nano-time via ::DXSCHEME_ENABLED_PROPERTY_PREFIX. The later can be used to override of fine-time
     * nano-time support for individual fields. Setting this property to `true` is essentially
     * equivalent to setting:
     * ```ini
     * dxscheme.enabled.Sequence=*
     * dxscheme.enabled.TimeNanoPart=*
     * ```
     */
    static const std::string DXSCHEME_NANO_TIME_PROPERTY;

    /**
     * Defines whether a specified field from the scheme should be enabled instead of it's default behaviour.
     * Use it according to following format:<br>
     * <b>`dxscheme.enabled.<field_property_name>=<event_name_mask_glob>`</b>
     *
     * For example, <b>`dxscheme.enabled.TimeNanoPart=Trade`</b> enables `NanoTimePart` internal field
     * only in Trade events.
     *
     * There is a shortcut for turning on nano-time support using ::DXSCHEME_NANO_TIME_PROPERTY.
     */
    static const std::string DXSCHEME_ENABLED_PROPERTY_PREFIX;

    /**
     * Represents the role of endpoint that was specified during its @ref DXEndpoint::create() "creation".
     *
     * @see DXEndpoint
     */
    enum class Role : std::int32_t {
        /**
         * `FEED` endpoint connects to the remote data feed provider and is optimized for real-time or
         * delayed data processing (<b>this is a default role</b>). DXEndpoint::getFeed() method
         * returns feed object that subscribes to the remote data feed provider and receives events from it.
         * When event processing threads cannot keep up (don't have enough CPU time), data is dynamically conflated to
         * minimize latency between received events and their processing time.
         *
         * This endpoint is automatically connected to the configured data feed as explained in
         * <a href="#defaultPropertiesSection">default properties section</a>.
         */
        FEED,

        /**
         * `ON_DEMAND_FEED` endpoint is similar to ::FEED, but it is designed to be used with OnDemandService for
         * historical data replay only. It is configured with
         * <a href="#defaultPropertiesSection">default properties</a>, but is not connected automatically
         * to the data provider until @ref OnDemandService::replay(Date, double) "OnDemandService.replay"
         * method is invoked.
         *
         * `ON_DEMAND_FEED` endpoint cannot be connected to an ordinary data feed at all.
         * OnDemandService::stopAndResume() will have a similar effect to OnDemandService::stopAndClear().
         *
         * @see OnDemandService
         */
        ON_DEMAND_FEED,

        /**
         * `STREAM_FEED` endpoint is similar to ::FEED and also connects to the remote data feed provider,
         * but is designed for bulk parsing of data from files. DXEndpoint::getFeed() method
         * returns feed object that subscribes to the data from the opened files and receives events from them.
         * Events from the files are not conflated, are not skipped, and are processed as fast as possible.
         * Note, that in this role, DXFeed::getLastEvent method does not work.
         */
        STREAM_FEED,

        /**
         * `PUBLISHER` endpoint connects to the remote publisher hub (also known as multiplexor) or
         * creates a publisher on the local host. DXEndpoint#getPublisher() method returns
         * a publisher object that publishes events to all connected feeds.
         * Note, that in this role, DXFeed::getLastEvent method does not work and
         * time-series subscription is not supported.
         *
         * This endpoint is automatically connected to the configured data feed as explained in
         * <a href="#defaultPropertiesSection">default properties section</a>.
         */
        PUBLISHER,

        /**
         * `STREAM_PUBLISHER` endpoint is similar to ::PUBLISHER and also connects to the remote publisher
         * hub, but is designed for bulk publishing of data. DXEndpoint::getPublisher() method returns a
         * publisher object that publishes events to all connected feeds. Published events are not conflated, are not
         * skipped, and are processed as fast as possible. Note, that in this role, DXFeed::getLastEvent method
         * does not work and time-series subscription is not supported.
         */
        STREAM_PUBLISHER,

        /**
         * `LOCAL_HUB` endpoint is a local hub without ability to establish network connections.
         * Events that are published via @ref DXEndpoint::getPublisher() "publisher" are delivered to local
         * @ref DXEndpoint::getFeed() "feed" only.
         */
        LOCAL_HUB
    };

    static dxfg_endpoint_role_t roleToGraalRole(Role role) {
        switch (role) {
        case Role::FEED:
            return DXFG_ENDPOINT_ROLE_FEED;
        case Role::ON_DEMAND_FEED:
            return DXFG_ENDPOINT_ROLE_ON_DEMAND_FEED;
        case Role::STREAM_FEED:
            return DXFG_ENDPOINT_ROLE_STREAM_FEED;
        case Role::PUBLISHER:
            return DXFG_ENDPOINT_ROLE_PUBLISHER;
        case Role::STREAM_PUBLISHER:
            return DXFG_ENDPOINT_ROLE_STREAM_PUBLISHER;
        case Role::LOCAL_HUB:
            return DXFG_ENDPOINT_ROLE_LOCAL_HUB;
        }
    }

    /**
     * Represents the current state of endpoint.
     *
     * @see DXEndpoint
     */
    enum class State : std::int32_t {
        /**
         * Endpoint was created by is not connected to remote endpoints.
         */
        NOT_CONNECTED,

        /**
         * The @ref DXEndpoint::connect(const std::string&) "connect" method was called to establish connection to
         * remove endpoint, but connection is not actually established yet or was lost.
         */
        CONNECTING,

        /**
         * The connection to remote endpoint is established.
         */
        CONNECTED,

        /**
         * Endpoint was @ref DXEndpoint::close() "closed".
         */
        CLOSED
    };

  protected:
    DXEndpoint() {}

  private:
    static std::unordered_map<Role, std::shared_ptr<DXEndpoint>> INSTANCES;

  public:
    /**
     * Returns a default application-wide singleton instance of DXEndpoint with a @ref Role::FEED "FEED" role.
     * Most applications use only a single data-source and should rely on this method to get one.
     * This method creates an endpoint on the first use with a default
     * configuration as explained in
     * <a href="#defaultPropertiesSection">default properties section</a> of DXEndpoint class documentation.
     * You can provide configuration via classpath or via system properties as explained there.
     *
     * This is a shortcut to
     * @ref ::getInstance(Role) "getInstance"(@ref DXEndpoint "DXEndpoint"::@ref DXEndpoint::Role "Role"::@ref
     * DXEndpoint.Role::FEED "FEED").
     * @see #getInstance(Role)
     */
    static std::shared_ptr<DXEndpoint> getInstance() { return getInstance(Role::FEED); }

    /**
     * Returns a default application-wide singleton instance of DXEndpoint for a specific role.
     * Most applications use only a single data-source and should rely on this method to get one.
     * This method creates an endpoint with the corresponding role on the first use with a default
     * configuration as explained in
     * <a href="#defaultPropertiesSection">default properties section</a> of DXEndpoint class documentation.
     * You can provide configuration via classpath or via system properties as explained there.
     *
     * The configuration does not have to include an address. You can use @ref ::connect(const std::string&)
     * "connect(address)" and ::disconnect() methods on the instance that is returned by this method to programmatically
     * establish and tear-down connection to a user-provided address.
     *
     * If you need a fully programmatic configuration and/or multiple endpoints of the same role in your
     * application, then create a custom instance of DXEndpoint with DXEndpoint::newBuilder() method, configure it,
     * and use Builder::build() method.
     *
     * @param role The role of DXEndpoint instance
     * @return The DXEndpoint instance
     */
    static std::shared_ptr<DXEndpoint> getInstance(Role role) {
        //        if (INSTANCES.contains(role)) {
        //            return INSTANCES[role];
        //        } else {
        //            return INSTANCES[role] = newBuilder().withRole(role).build();
        //        }

        return {};
    }

    /**
     * Creates an endpoint with @ref Role::FEED "FEED" role.
     * The result of this method is the same as <b>`create(DXEndpoint::Role::FEED)`</b>.
     * This is a shortcut to
     * @ref ::newBuilder() "newBuilder()".@ref Builder::build() "build()"
     *
     * @return the created endpoint.
     */
    static std::shared_ptr<DXEndpoint> create() { return {}; }

    class Builder : public std::enable_shared_from_this<Builder> {
        friend DXEndpoint;

        std::recursive_mutex mtx_;
        dxfg_endpoint_builder_t *handle_;
        Role role_ = Role::FEED;
        Builder() : mtx_(), handle_() {}

        static std::shared_ptr<Builder> create() {
            auto result = std::visit(
                [](auto &&arg) -> dxfg_endpoint_builder_t * {
                    using T = std::decay_t<decltype(arg)>;

                    if constexpr (std::is_same_v<T, detail::CEntryPointErrors>) {
                        return nullptr;
                    } else {
                        return arg;
                    }
                },
                detail::Isolate::getInstance()->runIsolated([](detail::GraalIsolateThreadHandle threadHandle) {
                    return dxfg_DXEndpoint_newBuilder(threadHandle);
                }));

            auto builder = std::shared_ptr<Builder>(new Builder{});
            builder->handle_ = result;

            return builder;
        }

      public:

        virtual ~Builder() {

        }

        std::shared_ptr<Builder> withRole(Role role) {
            std::lock_guard guard(mtx_);

            role_ = role;

            std::visit(
                [](auto &&arg) -> bool {
                    using T = std::decay_t<decltype(arg)>;

                    if constexpr (std::is_same_v<T, detail::CEntryPointErrors>) {
                        return false;
                    } else {
                        return arg;
                    }
                },
                detail::Isolate::getInstance()->runIsolated(
                    [role = role, this](detail::GraalIsolateThreadHandle threadHandle) {
                        return dxfg_DXEndpoint_Builder_withRole(threadHandle, handle_, roleToGraalRole(role)) == 0;
                    }));

            return shared_from_this();
        }
    };

    static std::shared_ptr<Builder> newBuilder() {
        return Builder::create();
    }
};
} // namespace dxfcpp
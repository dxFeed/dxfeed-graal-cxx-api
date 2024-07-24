// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../internal/CEntryPointErrors.hpp"
#include "../internal/Common.hpp"
#include "../internal/Handler.hpp"
#include "../internal/Isolate.hpp"
#include "../internal/JavaObjectHandle.hpp"
#include "DXFeed.hpp"
#include "DXPublisher.hpp"

#include <filesystem>
#include <iostream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <variant>

DXFCPP_BEGIN_NAMESPACE

struct DXPublisher;
struct DXFeed;
struct OnDemandService;

/**
 * Manages network connections to @ref DXFeed "feed" or
 * @ref DXPublisher "publisher". There are per-process (per GraalVM Isolate for now) ready-to-use singleton instances
 * that are available with DXEndpoint::getInstance() and DXEndpoint::getInstance(Role) methods as well as
 * factory methods DXEndpoint::create() and DXEndpoint::create(Role), and a number of configuration methods. Advanced
 * properties can be configured using
 * @ref DXEndpoint::newBuilder() "newBuilder()".@ref DXEndpoint::Builder::withProperty(const std::string&, const
 * std::string&) "withProperty(key, value)".@ref DXEndpoint::Builder::build() "build()".
 *
 * See DXFeed for details on how to subscribe to symbols and receive events.
 *
 * <h3>Endpoint role</h3>
 *
 * Each endpoint has a role that is specified on its creation and cannot be changed afterwards.
 * The default factory method DXEndpoint::create() creates an endpoint with a @ref Role::FEED "FEED" role.
 * Endpoints with other roles are created with DXEndpoint::create(Role) factory method. Endpoint role is
 * represented by @ref Role "DXEndpoint::Role" enumeration.
 *
 * Endpoint role defines the behavior of its @ref DXEndpoint::connect(const std::string&) "connect" method:
 *
 * - @ref Role::FEED "FEED" connects to the remote data feed provider and is optimized for real-time or
 *   delayed data processing (<b>this is a default role</b>).
 *   DXEndpoint::getFeed() method returns a feed object that subscribes to this remote data feed provider and receives
 * events from it. When event processing threads cannot keep up (don't have enough CPU time), data is dynamically
 * conflated to minimize latency between received events and their processing time. For example:
 *   - <b>`DXEndpoint::create()->connect("demo.dxfeed.com:7300")->getFeed()`</b> returns a demo feed from dxFeed with
 *     sample market quotes.
 *   - <b>`DXEndpoint::create()->connect("localhost:7400")->getFeed()`</b> returns a feed that is connected to a
 *     publisher that is running on the same host. See example below.
 *   - <b>`DXEndpoint::create()->connect("file:demo-sample.data")->getFeed()`</b> returns a feed that is connected to
 *     a "demo-sample.data" file and plays back it as if it was received in real time.
 *
 *   This endpoint is automatically connected to the configured data feed as explained in default properties section.
 * - @ref Role::ON_DEMAND_FEED "ON_DEMAND_FEED" is similar to @ref Role::FEED "FEED", but it is designed to be used with
 *   OnDemandService for historical data replay only. It is configured with default properties, but is not connected
 *   automatically to the data provider until @ref OnDemandService::replay(std::int64_t,double)
 * "OnDemandService::replay" method is invoked.
 * - @ref Role::STREAM_FEED "STREAM_FEED" is similar to @ref Role::FEED "FEED" and also connects to the remote data
 *   feed provider, but is designed for bulk parsing of data from files. DXEndpoint::getFeed() method returns feed
 *   object that subscribes to the data from the opened files and receives events from them. Events from the files are
 *   not conflated and are processed as fast as possible. Note, that in this role, DXFeed::getLastEvent() method does
 * not work and time-series subscription is not supported. For example:
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
 *   publisher on the local host. DXEndpoint::getPublisher() method returns a publisher object that publishes events to
 * all connected feeds. For example:
 * <b>`DXEndpoint->create(DXEndpoint::Role::PUBLISHER)->connect(":7400")->getPublisher()`</b> returns a publisher that
 * is waiting for connections on TCP/IP port 7400. The published events will be delivered to all feeds that are
 * connected to this publisher. This endpoint is automatically connected to the configured data feed as explained in
 * default properties section.
 * - @ref Role::LOCAL_HUB "LOCAL_HUB" creates a local hub without ability to establish network connections.
 *   Events that are published via @ref DXEndpoint::getPublisher() "publisher" are delivered to local @ref
 * DXEndpoint::getFeed() "feed" only.
 *
 * <h3>Endpoint state</h3>
 *
 * Each endpoint has a state that can be retrieved with DXEndpoint::getState() method.
 * When endpoint is created with any role and default address is not specified in default properties, then it is not
 * connected to any remote endpoint.
 * Its state is @ref State::NOT_CONNECTED "NOT_CONNECTED".
 *
 * @ref Role::FEED "Feed" and @ref Role::PUBLISHER "publisher" endpoints can connect to remote endpoints of the opposite
 * role. Connection is initiated by @ref DXEndpoint::connect(const std::string&) "connect" method.
 * The endpoint state becomes @ref State::CONNECTING "CONNECTING".
 *
 * When the actual connection to the remote endpoint is established, the endpoint state becomes
 * @ref State::CONNECTED "CONNECTED".
 *
 * Network connections can temporarily break and return endpoint back into @ref State::CONNECTING "CONNECTING" state.
 * File connections can be completed and return endpoint into @ref State::NOT_CONNECTED "NOT_CONNECTED" state.
 *
 * Connection to the remote endpoint can be terminated with DXEndpoint::disconnect() method.
 * The endpoint state becomes @ref State::NOT_CONNECTED "NOT_CONNECTED".
 *
 * Endpoint can be closed with DXEndpoint::close() method. The endpoint state becomes @ref State::CLOSED "CLOSED". This
 * is a final state. All connection are terminated and all internal resources that are held by this endpoint are freed.
 * No further connections can be initiated.
 *
 * <h3>Event times</h3>
 *
 * The EventType::getEventTime() on received events is available only when the endpoint is created with
 * DXEndpoint::DXENDPOINT_EVENT_TIME_PROPERTY property and the data source has embedded event times. This is typically
 * true only for data events that are read from historical tape files (see above) and from OnDemandService. Events that
 * are coming from a network connections do not have an embedded event time information and event time is not available
 * for them anyway.
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
 * The DXEndpoint::NAME_PROPERTY is the exception from the above rule. It is never loaded from system properties.
 * It can be only specified in configuration file or programmatically. There is a convenience
 * @ref Builder::withName(const std::string&) "Builder.withName" method for it. It is recommended to assign short and
 * meaningful endpoint names when multiple endpoints are used in the same process (one GraalVM Isolate for now).
 * The name of the endpoint shall describe its role in the particular application.
 *
 * Note, that individual properties that are programmatically set using @ref Builder::withProperty(const std::string&,
 * const std::string&) "withProperty" method always take precedence.
 *
 * @ref Role::FEED "FEED" and @ref Role::PUBLISHER "PUBLISHER" automatically establish connection on creation
 * when the corresponding DXEndpoint::DXFEED_ADDRESS_PROPERTY or DXEndpoint::DXPUBLISHER_ADDRESS_PROPERTY is specified.
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
 *
 * [Javadoc.](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/api/DXEndpoint.html)
 *
 * Some methods that are not marked `noexcept` may throw exceptions:
 *
 * @throws InvalidArgumentException if handle is invalid.
 * @throws JavaException if something happened with the dxFeed API backend
 * @throws GraalException if something happened with the GraalVM
 */
struct DXFCPP_EXPORT DXEndpoint : public RequireMakeShared<DXEndpoint> {
    /// The alias to a type of shared pointer to the DXEndpoint object
    using Ptr = std::shared_ptr<DXEndpoint>;

    /// The alias to a type of unique pointer to the DXEndpoint object
    using Unique = std::unique_ptr<DXEndpoint>;

    friend struct OnDemandService;

    /**
     * `"name"`
     *
     * Defines property for endpoint name that is used to distinguish multiple endpoints
     * in the same process in logs and in other diagnostic means.
     * Use Builder::withProperty(const std::string&, const std::string&) method.
     * This property is also changed by Builder::withName(const std::string&) method.
     */
    static const std::string NAME_PROPERTY;

    /**
     * `"dxfeed.properties"`
     *
     * Defines path to a file with properties for an endpoint with role @ref Role::FEED "FEED" or
     * @ref Role::ON_DEMAND_FEED "ON_DEMAND_FEED".
     * By default, properties a loaded from a path resource named "dxfeed.properties".
     *
     * @see Builder::withProperty(const std::string&, const std::string&)
     */
    static const std::string DXFEED_PROPERTIES_PROPERTY;

    /**
     * `"dxfeed.address"`
     *
     * Defines default connection address for an endpoint with role @ref Role::FEED "FEED"
     * or @ref Role::ON_DEMAND_FEED "ON_DEMAND_FEED".
     * Connection is established to this address by role @ref Role::FEED "FEED" as soon as endpoint is created, while
     * role @ref Role::ON_DEMAND_FEED "ON_DEMAND_FEED" waits until OnDemandService::(std::int64_t,double) is invoked
     * before connecting.
     *
     * By default, without this property, connection is not established until @ref DXEndpoint::connect(const
     * std::string&) "connect(address)" is invoked.
     *
     * Credentials for access to premium services may be configured with
     * DXEndpoint::DXFEED_USER_PROPERTY and DXEndpoint::DXFEED_PASSWORD_PROPERTY.
     *
     * @see Builder::withProperty(const std::string&, const std::string&)
     */
    static const std::string DXFEED_ADDRESS_PROPERTY;

    /**
     * `"dxfeed.user"`
     *
     * Defines default user name for an endpoint with role @ref Role::FEED "FEED" or @ref Role::ON_DEMAND_FEED
     * "ON_DEMAND_FEED".
     *
     * @see DXEndpoint::user(const std::string&)
     */
    static const std::string DXFEED_USER_PROPERTY;

    /**
     * `"dxfeed.password"`
     *
     * Defines default password for an endpoint with role @ref Role::FEED "FEED" or @ref Role::ON_DEMAND_FEED
     * "ON_DEMAND_FEED".
     *
     * @see DXEndpoint::password(const std::string&)
     */
    static const std::string DXFEED_PASSWORD_PROPERTY;

    /**
     * `"dxfeed.threadPoolSize"`
     *
     * Defines thread pool size for an endpoint with role @ref Role::FEED "FEED".
     * By default, the thread pool size is equal to the number of available processors.
     * @see Builder::withProperty(const std::string&, const std::string&)
     */
    static const std::string DXFEED_THREAD_POOL_SIZE_PROPERTY;

    /**
     * `"dxfeed.aggregationPeriod"`
     *
     * Defines data aggregation period an endpoint with role @ref Role::FEED "FEED" that
     * limits the rate of data notifications. For example, setting the value of this property
     * to "0.1s" limits notification to once every 100ms (at most 10 per second).
     * @see Builder::withProperty(const std::string&, const std::string&)
     */
    static const std::string DXFEED_AGGREGATION_PERIOD_PROPERTY;

    /**
     * `"dxfeed.wildcard.enable"`
     *
     * Set this property to `true` to turns on wildcard support.
     * By default, the endpoint does not support wildcards. This property is needed for
     * WildcardSymbol support and for the use of "tape:..." address in DXPublisher.
     */
    static const std::string DXFEED_WILDCARD_ENABLE_PROPERTY;

    /**
     * `"dxpublisher.properties"`
     *
     * Defines path to a file with properties for an endpoint with role @ref Role::PUBLISHER "PUBLISHER".
     * By default, properties a loaded from a classpath resource named "dxpublisher.properties".
     * @see Builder::withProperty(const std::string&, const std::string&)
     */
    static const std::string DXPUBLISHER_PROPERTIES_PROPERTY;

    /**
     * `"dxpublisher.address"`
     *
     * Defines default connection address for an endpoint with role @ref Role::PUBLISHER "PUBLISHER".
     * Connection is established to this address as soon as endpoint is created.
     * By default, connection is not established until DXEndpoint::connect(const std::string&) is invoked.
     * @see Builder::withProperty(const std::string&, const std::string&)
     */
    static const std::string DXPUBLISHER_ADDRESS_PROPERTY;

    /**
     * `"dxpublisher.threadPoolSize"`
     *
     * Defines thread pool size for an endpoint with role @ref Role::PUBLISHER "PUBLISHER".
     * By default, the thread pool size is equal to the number of available processors.
     * @see Builder#withProperty(const std::string&, const std::string&)
     */
    static const std::string DXPUBLISHER_THREAD_POOL_SIZE_PROPERTY;

    /**
     * `"dxendpoint.eventTime"`
     *
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
     * `"dxendpoint.storeEverything"`
     *
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
     * `"dxscheme.nanoTime"`
     *
     * Set this property to `true` to turn on nanoseconds precision business time.
     * By default, this feature is turned off.
     * Business time in most events is available with
     * millisecond precision by default, while Quote events business @ref Quote::getTime() "time" is available with
     * seconds precision.
     *
     * This method provides a higher-level control than turning on individual properties that are responsible
     * for nano-time via DXEndpoint::DXSCHEME_ENABLED_PROPERTY_PREFIX. The later can be used to override of fine-time
     * nano-time support for individual fields. Setting this property to `true` is essentially
     * equivalent to setting:
     * ```ini
     * dxscheme.enabled.Sequence=*
     * dxscheme.enabled.TimeNanoPart=*
     * ```
     */
    static const std::string DXSCHEME_NANO_TIME_PROPERTY;

    /**
     * `"dxscheme.enabled."`
     *
     * Defines whether a specified field from the scheme should be enabled instead of it's default behaviour.
     * Use it according to following format:<br>
     * <b>`dxscheme.enabled.<field_property_name>=<event_name_mask_glob>`</b>
     *
     * For example, <b>`dxscheme.enabled.TimeNanoPart=Trade`</b> enables `NanoTimePart` internal field
     * only in Trade events.
     *
     * There is a shortcut for turning on nano-time support using DXEndpoint::DXSCHEME_NANO_TIME_PROPERTY.
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
         * `ON_DEMAND_FEED` endpoint is similar to DXEndpoint::FEED, but it is designed to be used with OnDemandService
         * for historical data replay only. It is configured with <a href="#defaultPropertiesSection">default
         * properties</a>, but is not connected automatically to the data provider until @ref
         * OnDemandService::(std::int64_t,double) "OnDemandService.replay" method is invoked.
         *
         * `ON_DEMAND_FEED` endpoint cannot be connected to an ordinary data feed at all.
         * OnDemandService::stopAndResume() will have a similar effect to OnDemandService::stopAndClear().
         *
         * @see OnDemandService
         */
        ON_DEMAND_FEED,

        /**
         * `STREAM_FEED` endpoint is similar to DXEndpoint::FEED and also connects to the remote data feed provider,
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
         * `STREAM_PUBLISHER` endpoint is similar to DXEndpoint::PUBLISHER and also connects to the remote publisher
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

    static std::string roleToString(Role role);

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

    static std::string stateToString(State state);

  private:
    JavaObjectHandle<DXEndpoint> handle_;
    Role role_ = Role::FEED;
    std::string name_{};
    JavaObjectHandle<DXEndpointStateChangeListener> stateChangeListenerHandle_;
    SimpleHandler<void(State, State)> onStateChange_{};

    // Throws:
    //   - std::bad_alloc if it was not possible to allocate the required amount of memory
    //   - InvalidArgumentException if endpointHandle is nullptr
    //   - JavaException if something happened with the dxFeed API backend
    //   - GraalException if something happened with the GraalVM
    static std::shared_ptr<DXEndpoint> create(void *endpointHandle, Role role,
                                              const std::unordered_map<std::string, std::string> &properties);

    struct Impl;

    std::unique_ptr<Impl> impl_;

  public:
    explicit DXEndpoint(LockExternalConstructionTag);
    DXEndpoint(LockExternalConstructionTag, JavaObjectHandle<DXEndpoint> &&handle, Role role, std::string name);

    ~DXEndpoint() noexcept override;

    /**
     * Returns a default application-wide singleton instance of DXEndpoint with a @ref Role::FEED "FEED" role.
     * Most applications use only a single data-source and should rely on this method to get one.
     * This method creates an endpoint on the first use with a default
     * configuration as explained in
     * <a href="#defaultPropertiesSection">default properties section</a> of DXEndpoint class documentation.
     * You can provide configuration via system properties as explained there.
     *
     * This is a shortcut to
     * @ref DXEndpoint::getInstance(Role) "getInstance"(@ref DXEndpoint "DXEndpoint"::@ref DXEndpoint::Role "Role"::@ref
     * DXEndpoint.Role::FEED "FEED").
     * @see DXEndpoint::getInstance(Role)
     * @throws InvalidArgumentException
     * @throws JavaException
     * @throws GraalException
     */
    static std::shared_ptr<DXEndpoint> getInstance();

    /**
     * Returns a default application-wide singleton instance of DXEndpoint for a specific role.
     * Most applications use only a single data-source and should rely on this method to get one.
     * This method creates an endpoint with the corresponding role on the first use with a default
     * configuration as explained in
     * <a href="#defaultPropertiesSection">default properties section</a> of DXEndpoint class documentation.
     * You can provide configuration via system properties as explained there.
     *
     * The configuration does not have to include an address. You can use @ref DXEndpoint::connect(const std::string&)
     * "connect(address)" and DXEndpoint::disconnect() methods on the instance that is returned by this method to
     * programmatically establish and tear-down connection to a user-provided address.
     *
     * If you need a fully programmatic configuration and/or multiple endpoints of the same role in your
     * application, then create a custom instance of DXEndpoint with DXEndpoint::newBuilder() method, configure it,
     * and use Builder::build() method.
     *
     * @param role The role of DXEndpoint instance
     * @return The DXEndpoint instance
     * @throws InvalidArgumentException
     * @throws JavaException
     * @throws GraalException
     */
    static std::shared_ptr<DXEndpoint> getInstance(Role role);

    class Builder;

    /**
     * Creates new Builder instance.
     * Use Builder::build() to build an instance of DXEndpoint when all configuration properties were set.
     *
     * @return the created endpoint builder.
     * @throws InvalidArgumentException
     * @throws JavaException
     * @throws GraalException
     */
    static std::shared_ptr<Builder> newBuilder();

    /**
     * Creates an endpoint with @ref Role::FEED "FEED" role.
     * The result of this method is the same as <b>`create(DXEndpoint::Role::FEED)`</b>.
     * This is a shortcut to
     * @ref DXEndpoint::newBuilder() "newBuilder()"->@ref Builder::build() "build()"
     *
     * @return the created endpoint.
     * @throws InvalidArgumentException
     * @throws JavaException
     * @throws GraalException
     */
    static std::shared_ptr<DXEndpoint> create();

    /**
     * Creates an endpoint with a specified role.
     * This is a shortcut to
     * @ref DXEndpoint::newBuilder() "newBuilder()"->@ref Builder::withRole(Role) "withRole(role)"->@ref
     * Builder::build() "build()"
     *
     * @param role the role.
     * @return the created endpoint.
     * @throws InvalidArgumentException
     * @throws JavaException
     * @throws GraalException
     */
    static std::shared_ptr<DXEndpoint> create(Role role);

    /**
     * Returns the role of this endpoint.
     *
     * @return the role.
     *
     * @see DXEndpoint
     */
    Role getRole() const noexcept;

    /**
     * Returns the state of this endpoint.
     *
     * @return the state.
     *
     * @see DXEndpoint
     * @throws InvalidArgumentException
     * @throws JavaException
     * @throws GraalException
     */
    State getState() const;

    /**
     * @return `true` if the endpoint is closed
     * @throws InvalidArgumentException
     * @throws JavaException
     * @throws GraalException
     */
    bool isClosed() const;

    /**
     * @return The user defined endpoint's name
     */
    const std::string &getName() const & noexcept;

    /**
     * Adds listener that is notified about changes in @ref DXEndpoint::getState() "state" property.
     *
     * <p>Installed listener can be removed by `id` with DXEndpoint::removeStateChangeListener method or by call
     * `::onStateChange() -= id`;
     *
     * @tparam StateChangeListener The listener type. It can be any callable with signature: `void(State, State)`
     * @param listener The listener to add
     * @return the listener id
     */
    template <typename StateChangeListener>
    std::size_t addStateChangeListener(StateChangeListener &&listener) noexcept
#if __cpp_concepts
        requires requires {
            { listener(State{}, State{}) } -> std::same_as<void>;
        }
#endif
    {
        return onStateChange_ += listener;
    }

    /**
     * Removes listener that is notified about changes in @ref DXEndpoint::getState() "state" property.
     * It removes the listener that was previously installed with DXEndpoint::addStateChangeListener method.
     *
     * @param listenerId The listener id to remove
     */
    void removeStateChangeListener(std::size_t listenerId) noexcept;

    /**
     * Returns the onStateChange @ref SimpleHandler<void(ArgTypes...)> "handler" that can be used to add or remove
     * listeners.
     *
     * @return onStateChange handler with `void(State, State)` signature
     */
    SimpleHandler<void(DXEndpoint::State, DXEndpoint::State)> &onStateChange() noexcept;

    /**
     * Changes user name for this endpoint.
     * This method shall be called before @ref DXEndpoint::connect(const std::string&) "connect" together
     * with @ref DXEndpoint::password(const std::string&) "password" to configure service access credentials.
     *
     * @param user The user name.
     *
     * @return this DXEndpoint.
     * @throws InvalidArgumentException
     * @throws JavaException
     * @throws GraalException
     */
    std::shared_ptr<DXEndpoint> user(const std::string &user);

    /**
     * Changes password for this endpoint.
     * This method shall be called before @ref DXEndpoint::connect(const std::string&) "connect" together
     * with @ref DXEndpoint::user(const std::string&) "user" to configure service access credentials.
     *
     * @param password The password.
     *
     * @return this DXEndpoint.
     * @throws InvalidArgumentException
     * @throws JavaException
     * @throws GraalException
     */
    std::shared_ptr<DXEndpoint> password(const std::string &password);

    /**
     * Connects to the specified remote address. Previously established connections are closed if
     * the new address is different from the old one.
     * This method does nothing if address does not change or if this endpoint is @ref State::CLOSED "CLOSED".
     * The endpoint @ref DXEndpoint::getState() "state" immediately becomes @ref State::CONNECTING "CONNECTING"
     * otherwise.
     *
     * <p> The address string is provided with the market data vendor agreement.
     * Use "demo.dxfeed.com:7300" for a demo quote feed.
     *
     * <p> The simplest address strings have the following format:
     * * `host:port` to establish a TCP/IP connection.
     * * `:port` to listen for a TCP/IP connection with a plain socket connector (good for up to a
     * few hundred of connections).
     *
     * <p>For premium services access credentials must be configured before invocation of `connect` method
     * using @ref ::user(const std::string&) "user" and @ref ::password(const std::string&) "password" methods.
     *
     * <p> <b>This method does not wait until connection actually gets established</b>. The actual connection
     * establishment happens asynchronously after the invocation of this method. However, this method waits until
     * notification about state transition from State::NOT_CONNECTED to State::CONNECTING gets processed by all
     * listeners.
     *
     * [Javadoc.](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/api/DXEndpoint.html#connect-java.lang.String-)
     *
     * @param address The data source address.
     * @return this DXEndpoint.
     *
     * @throws InvalidArgumentException
     * @throws JavaException if something happened with the dxFeed API backend or if address string is malformed.
     * @throws GraalException
     */
    std::shared_ptr<DXEndpoint> connect(const std::string &address);

    /**
     * Terminates all established network connections and initiates connecting again with the same address.
     *
     * <p>The effect of the method is alike to invoking ::disconnect() and ::connect(const std::string&)
     * with the current address, but internal resources used for connections may be reused by implementation.
     * TCP connections with multiple target addresses will try switch to an alternative address, configured
     * reconnect timeouts will apply.
     *
     * <p><b>Note:</b> The method will not connect endpoint that was not initially connected with
     * ::connect(const std::string&) method or was disconnected with ::disconnect() method.
     *
     * <p>The method initiates a short-path way for reconnecting, so whether observers will have a chance to see
     * an intermediate state State#NOT_CONNECTED depends on the implementation.
     *
     * [Javadoc.](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/api/DXEndpoint.html#reconnect--)
     *
     * @throws InvalidArgumentException
     * @throws JavaException
     * @throws GraalException
     */
    void reconnect();

    /**
     * Terminates all remote network connections.
     * This method does nothing if this endpoint is @ref State#CLOSED "CLOSED".
     * The endpoint @ref ::getState() "state" immediately becomes @ref State::NOT_CONNECTED "NOT_CONNECTED" otherwise.
     *
     * <p>This method does not release all resources that are associated with this endpoint.
     * Use ::close() method to release all resources.
     *
     * [Javadoc.](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/api/DXEndpoint.html#disconnect--)
     *
     * @throws InvalidArgumentException
     * @throws JavaException
     * @throws GraalException
     */
    void disconnect();

    /**
     * Terminates all remote network connections and clears stored data.
     * This method does nothing if this endpoint is @ref State::CLOSED "CLOSED".
     * The endpoint @ref ::getState() "state" immediately becomes @ref State::NOT_CONNECTED "NOT_CONNECTED" otherwise.
     *
     * <p>This method does not release all resources that are associated with this endpoint.
     * Use close() method to release all resources.
     *
     * [Javadoc.](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/api/DXEndpoint.html#disconnectAndClear--)
     *
     * @throws InvalidArgumentException
     * @throws JavaException
     * @throws GraalException
     */
    void disconnectAndClear();

    /**
     * Closes this endpoint. All network connection are terminated as with ::disconnect() method and no further
     * connections can be established.
     *
     * The endpoint @ref ::getState() "state" immediately becomes @ref State::CLOSED "CLOSED".
     * All resources associated with this endpoint are released.
     *
     * [Javadoc.](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/api/DXEndpoint.html#close--)
     *
     * @throws InvalidArgumentException
     * @throws JavaException
     * @throws GraalException
     */
    void close();

    /**
     * Waits while this endpoint @ref ::getState() "state" becomes @ref State::NOT_CONNECTED "NOT_CONNECTED" or
     * @ref State::CLOSED "CLOSED". It is a signal that any files that were opened with
     * @ref connect(const std::string&) "connect(\"file:...\")" method were finished reading, but not necessary were
     * completely processed by the corresponding subscription listeners. Use closeAndAwaitTermination() after this
     * method returns to make sure that all processing has completed.
     *
     * <p><b>This method is blocking.</b>
     *
     * [Javadoc.](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/api/DXEndpoint.html#awaitNotConnected--)
     *
     * @throws InvalidArgumentException
     * @throws JavaException
     * @throws GraalException
     */
    void awaitNotConnected();

    /**
     * Waits until this endpoint stops processing data (becomes quiescent).
     * This is important when writing data to file via "tape:..." connector to make sure that
     * all published data was written before closing this endpoint.
     *
     * [Javadoc.](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/api/DXEndpoint.html#awaitProcessed--)
     *
     * @throws InvalidArgumentException
     * @throws JavaException
     * @throws GraalException
     */
    void awaitProcessed();

    /**
     * Closes this endpoint and wait until all pending data processing tasks are completed.
     * This  method performs the same actions as close ::close(), but also awaits
     * termination of all outstanding data processing tasks. It is designed to be used
     * with @ref Role::STREAM_FEED "STREAM_FEED" role after ::awaitNotConnected() method returns
     * to make sure that file was completely processed.
     *
     * <p><b>This method is blocking.</b>
     *
     * [Javadoc.](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/api/DXEndpoint.html#closeAndAwaitTermination--)
     *
     * @throws InvalidArgumentException
     * @throws JavaException
     * @throws GraalException
     */
    void closeAndAwaitTermination();

    // TODO: implement [EN-8234]
    std::unordered_set<EventTypeEnum> getEventTypes() noexcept;

    /**
     * @return The feed that is associated with this endpoint.
     * @throws InvalidArgumentException
     * @throws JavaException
     * @throws GraalException
     */
    std::shared_ptr<DXFeed> getFeed();

    /**
     * @return The publisher that is associated with this endpoint.
     * @throws InvalidArgumentException
     * @throws JavaException
     * @throws GraalException
     */
    std::shared_ptr<DXPublisher> getPublisher();

    /**
     * Builder class for DXEndpoint that supports additional configuration properties.
     *
     * Some methods that are not marked `noexcept` may throw exceptions:
     *
     * @throws InvalidArgumentException if handle is invalid.
     * @throws JavaException if something happened with the dxFeed API backend
     * @throws GraalException if something happened with the GraalVM
     */
    class DXFCPP_EXPORT Builder : public RequireMakeShared<Builder> {
        friend DXEndpoint;

        JavaObjectHandle<Builder> handle_;
        Role role_ = Role::FEED;
        std::unordered_map<std::string, std::string> properties_;

        // Throws:
        //   - std::bad_alloc if it was not possible to allocate the required amount of memory
        //   - JavaException if something happened with the dxFeed API backend
        //   - GraalException if something happened with the GraalVM
        static std::shared_ptr<Builder> create();

        /**
         * Tries to load the default properties file for Role::FEED, Role::ON_DEMAND_FEED or Role::PUBLISHER role.
         *
         * The default properties file is loaded only if there are no system properties or user properties set with the
         * same key and the file itself exists and is readable.
         *
         * This file must be in the <a href="https://en.wikipedia.org/wiki/.properties">Java properties file format</a>.
         * File be named "dxfeed.properties" for Role::FEED and Role::ON_DEMAND_FEED roles
         * or "dxpublisher.properties" for the Role::PUBLISHER role.
         *
         * Non thread-safe.
         * @throws InvalidArgumentException
         * @throws JavaException
         * @throws GraalException
         */
        void loadDefaultPropertiesImpl();

      public:
        explicit Builder(LockExternalConstructionTag) noexcept;

        /// Releases the GraalVM handle
        ~Builder() noexcept override;

        /**
         * Changes name that is used to distinguish multiple endpoints
         * in the same process (GraalVM Isolate) in logs and in other diagnostic means.
         * This is a shortcut for @ref ::withProperty "withProperty"(::NAME_PROPERTY, `name`)
         *
         * @param name The endpoint's name
         *
         * @return `this` endpoint builder.
         * @throws InvalidArgumentException
         * @throws JavaException
         * @throws GraalException
         */
        std::shared_ptr<Builder> withName(const std::string &name);

        /**
         * Sets role for the created DXEndpoint.
         * Default role is @ref Role::FEED "FEED".
         *
         * @param role The endpoint's role
         *
         * @return `this` endpoint builder.
         * @throws InvalidArgumentException
         * @throws JavaException
         * @throws GraalException
         */
        std::shared_ptr<Builder> withRole(Role role);

        /**
         * Sets the specified property. Unsupported properties are ignored.
         *
         * @param key The endpoint's property key
         * @param value The endpoint's property value
         * @return `this` endpoint builder.
         *
         * @see ::supportsProperty(const std::string&)
         * @throws InvalidArgumentException
         * @throws JavaException
         * @throws GraalException
         */
        std::shared_ptr<Builder> withProperty(const std::string &key, const std::string &value);

        /**
         * Sets all supported properties from the provided properties object.
         *
         * @tparam Properties The properties' type (std::map, std::unordered_map etc)
         * @param properties The endpoint's properties
         * @return `this` endpoint builder.
         *
         * @see ::withProperty(const std::string&, const std::string&)
         * @throws InvalidArgumentException
         * @throws JavaException
         * @throws GraalException
         */
        template <typename Properties> std::shared_ptr<Builder> withProperties(Properties &&properties) {
            if constexpr (Debugger::isDebug) {
                Debugger::debug("DXEndpoint::Builder{" + handle_.toString() + "}::withProperties(properties[" +
                                std::to_string(properties.size()) + "])");
            }

            for (auto &&[k, v] : properties) {
                withProperty(k, v);
            }

            return sharedAs<Builder>();
        }

        /**
         * Checks if a property is supported
         *
         * @param key The property's key to be checked for support
         * @return `true` if the corresponding property key is supported.
         *
         * @see ::withProperty(const std::string&, const std::string&)
         * @throws InvalidArgumentException
         * @throws JavaException
         * @throws GraalException
         */
        bool supportsProperty(const std::string &key);

        /**
         * Builds DXEndpoint instance.
         *
         * @return the created endpoint.
         * @throws InvalidArgumentException
         * @throws JavaException
         * @throws GraalException
         */
        std::shared_ptr<DXEndpoint> build();
    };

    std::string toString() const override;
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
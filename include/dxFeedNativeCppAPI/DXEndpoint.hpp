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
 *
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
 * @ref Builder::withProperty(const std::string&, const std::string&) "withProperty"(::DXFEED_PROPERTIES_PROPERTY, path) or
 * @ref Builder::withProperty(const std::string&, const std::string&) "withProperty"(::DXPUBLISHER_PROPERTIES_PROPERTY, path)
 * correspondingly. When the location of this file is not explicitly specified using
 * @ref Builder::withProperty(const std::string&, const std::string&) "withProperty" method, then the file path is taken from a system
 * property with the corresponding name.
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
 * @link Builder::withName(const std::string&) "Builder.withName" method for it. It is recommended to assign short and
 * meaningful endpoint names when multiple endpoints are used in the same JVM. The name of the endpoint shall
 * describe its role in the particular application.
 *
 * Note, that individual properties that are programmatically set using @ref Builder::withProperty(const std::string&, const std::string&) "withProperty"
 * method always take precedence.
 *
 * @ref Role::FEED "FEED" and @ref Role::PUBLISHER "PUBLISHER" automatically establish connection on creation
 * when the corresponding ::DXFEED_ADDRESS_PROPERTY or ::DXPUBLISHER_ADDRESS_PROPERTY is specified.
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
    enum Role {
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

    /**
     * Creates an endpoint with @ref Role::FEED "FEED" role.
     * The result of this method is the same as <b>`create(DXEndpoint::Role::FEED)`</b>.
     * This is a shortcut to
     * @ref ::newBuilder() "newBuilder()".@ref Builder::build() "build()"
     *
     * @return the created endpoint.
     */
    static std::shared_ptr<DXEndpoint> create() { return {}; }
};
} // namespace dxfcpp
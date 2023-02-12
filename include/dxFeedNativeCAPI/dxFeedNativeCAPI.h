// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#ifndef DXFEED_NATIVE_API_H_INCLUDED
#define DXFEED_NATIVE_API_H_INCLUDED

#ifdef __cplusplus
#    include <cstddef>
#    include <cstdint>

extern "C" {
#else
#    include <stddef.h>
#    include <stdint.h>
#endif

#define DXFC_OUT

/**
 * @file
 * @brief dxFeed Native C API enums, structs and functions declarations
 */

/**
 * @brief List of error codes.
 */
typedef enum dxfc_error_code_t {
    /**
     * @brief OK.
     */
    DXFC_EC_SUCCESS = 0,
    /**
     * @brief dxFeed Graal Native API error.
     */
    DXFC_EC_G_ERR,

    /// The error returned if the current operation cannot be completed.
    DXFC_EC_ERROR = 10000,
} dxfc_error_code_t;

/**
 * Sets the system property indicated by the specified key.
 *
 * @param key The name of the system property (UTF-8 string).
 * @param value The value of the system property (UTF-8 string).
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_system_set_property(const char *key, const char *value);

/**
 * Gets the system property indicated by the specified key.
 *
 * The buffer must be allocated in advance. If the system property value does not fit into the buffer, it will be
 * truncated. The buffer should be large enough to the \0 at the end.
 * Invalid UTF-8 characters resulting from a string that does not fit in the buffer will be discarded.
 *
 * @param key The name of the system property (UTF-8 string)
 * @param[out] buffer The buffer to store the system property (UTF-8 string)
 * @param buffer_size The buffer's size.
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_system_get_property(const char *key, DXFC_OUT char *buffer, size_t buffer_size);

/**
 * Represents the role of endpoint that was specified during its @ref dxfc_dxendpoint_create() "creation".
 */
typedef enum dxfc_dxendpoint_role_t {
    /**
     * `FEED` endpoint connects to the remote data feed provider and is optimized for real-time or
     * delayed data processing (<b>this is a default role</b>). dxfc_dxendpoint_get_feed() function
     * returns feed object that subscribes to the remote data feed provider and receives events from it.
     * When event processing threads cannot keep up (don't have enough CPU time), data is dynamically conflated to
     * minimize latency between received events and their processing time.
     *
     * This endpoint is automatically connected to the configured data feed.
     */
    DXFC_DXENDPOINT_ROLE_FEED = 0,

    /**
     * `ON_DEMAND_FEED` endpoint is similar to ::DXFC_DXENDPOINT_ROLE_FEED, but it is designed to be used with
     * dxfc_on_demand_service_t for historical data replay only.
     *
     * `ON_DEMAND_FEED` endpoint cannot be connected to an ordinary data feed at all.
     */
    DXFC_DXENDPOINT_ROLE_ON_DEMAND_FEED = 1,

    /**
     * `STREAM_FEED` endpoint is similar to ::DXFC_DXENDPOINT_ROLE_FEED and also connects to the remote data feed
     * provider, but is designed for bulk parsing of data from files. dxfc_dxendpoint_get_feed() function returns feed
     * object that subscribes to the data from the opened files and receives events from them. Events from the files are
     * not conflated, are not skipped, and are processed as fast as possible. Note, that in this role,
     * dxfc_dxfeed_get_last_event() function does not work.
     */
    DXFC_DXENDPOINT_ROLE_STREAM_FEED = 2,

    /**
     * `PUBLISHER` endpoint connects to the remote publisher hub (also known as multiplexor) or
     * creates a publisher on the local host. dxfc_dxendpoint_get_publisher() function returns
     * a publisher object that publishes events to all connected feeds.
     * Note, that in this role, dxfc_dxfeed_get_last_event() function does not work and
     * time-series subscription is not supported.
     *
     * This endpoint is automatically connected to the configured data feed.
     */
    DXFC_DXENDPOINT_ROLE_PUBLISHER = 3,

    /**
     * `STREAM_PUBLISHER` endpoint is similar to ::DXFC_DXENDPOINT_ROLE_PUBLISHER and also connects to the remote
     * publisher hub, but is designed for bulk publishing of data. dxfc_dxendpoint_get_publisher() function returns a
     * publisher object that publishes events to all connected feeds. Published events are not conflated, are not
     * skipped, and are processed as fast as possible. Note, that in this role, dxfc_dxfeed_get_last_event() function
     * does not work and time-series subscription is not supported.
     */
    DXFC_DXENDPOINT_ROLE_STREAM_PUBLISHER = 4,

    /**
     * `LOCAL_HUB` endpoint is a local hub without ability to establish network connections.
     * Events that are published via @ref dxfc_dxendpoint_get_publisher() "publisher" are delivered to local
     * @ref dxfc_dxendpoint_get_feed() "feed" only.
     */
    DXFC_DXENDPOINT_ROLE_LOCAL_HUB = 5,
} dxfc_dxendpoint_role_t;

/**
 * Represents the current state of endpoint.
 */
typedef enum dxfc_dxendpoint_state_t {
    /**
     * Endpoint was created by is not connected to remote endpoints.
     */
    DXFC_DXENDPOINT_STATE_NOT_CONNECTED = 0,

    /**
     * The @ref dxfc_dxendpoint_connect() "connect" function was called to establish connection to
     * remove endpoint, but connection is not actually established yet or was lost.
     */
    DXFC_DXENDPOINT_STATE_CONNECTING = 1,

    /**
     * The connection to remote endpoint is established.
     */
    DXFC_DXENDPOINT_STATE_CONNECTED = 2,

    /**
     * Endpoint was @ref dxfc_dxendpoint_close() "closed".
     */
    DXFC_DXENDPOINT_STATE_CLOSED = 3,
} dxfc_dxendpoint_state_t;

/**
 * The endpoint current state change listener
 *
 * @see dxfc_dxendpoint_add_state_change_listener()
 * @see dxfc_dxendpoint_remove_state_change_listener()
 */
typedef void (*dxfc_dxendpoint_state_change_listener)(dxfc_dxendpoint_state_t old_state,
                                                      dxfc_dxendpoint_state_t new_state, void *user_data);

/**
 * The simple key-value structure that represents an endpoint's property
 */
typedef struct dxfc_dxendpoint_property_t {
    /// The property's key
    const char *key;
    /// The property's value
    const char *value;
} dxfc_dxendpoint_property_t;

/**
 * The dxFeed endpoint handle. It is used for searching, adding endpoints to the "registry", deleting endpoints from
 * the "registry" and calling functions associated with a particular endpoint.
 *
 * The "registry" is responsible for neatly closing resources, shutting down work, and freeing up memory on shutdown.
 * This can be forced by calling the dxfc_dxendpoint_free() function.
 */
typedef void *dxfc_dxendpoint_t;

/**
 * The dxFeed endpoint's builder handle. It is used for searching, adding builders to the "registry", deleting builders
 * from the "registry" and calling functions associated with a particular builder.
 *
 * The "registry" is responsible for neatly closing resources, shutting down work, and freeing up memory on shutdown.
 * This can be forced by calling the dxfc_dxendpoint_builder_free() function.
 */
typedef void *dxfc_dxendpoint_builder_t;

/**
 * The dxFeed handle.
 */
typedef void *dxfc_dxfeed_t;

/**
 * The dxFeed publisher handle.
 */
typedef void *dxfc_dxpublisher_t;

/**
 * Creates new dxFeed endpoint's builder instance.
 * Use dxfc_dxendpoint_builder_build to build an instance of dxFeed endpoint when all configuration properties were set.
 *
 * @param[out] builder The created endpoint's builder.
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_dxendpoint_new_builder(DXFC_OUT dxfc_dxendpoint_builder_t *builder);

/**
 * Sets role for the created dxFeed endpoint.
 * Default role is @ref DXFC_DXENDPOINT_ROLE_FEED "FEED".
 *
 * @param builder The endpoint's builder
 * @param role The endpoint's role
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_dxendpoint_builder_with_role(dxfc_dxendpoint_builder_t builder, dxfc_dxendpoint_role_t role);

/**
 * Changes name that is used to distinguish multiple endpoints
 * in the same process (GraalVM Isolate) in logs and in other diagnostic means.
 *
 * @param builder The endpoint's builder
 * @param name The endpoint's name
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_dxendpoint_builder_with_name(dxfc_dxendpoint_builder_t builder, const char *name);

/**
 * Sets the specified property. Unsupported properties are ignored.
 *
 * @param builder The endpoint's builder
 * @param key The endpoint's property key
 * @param value The endpoint's property value
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_dxendpoint_builder_with_property(dxfc_dxendpoint_builder_t builder, const char *key,
                                                        const char *value);

/**
 * Sets all supported properties from the provided properties object.
 *
 * @param builder The endpoint's builder
 * @param properties The endpoint's properties
 * @param size The size of the endpoint's properties
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_dxendpoint_builder_with_properties(dxfc_dxendpoint_builder_t builder,
                                                          const dxfc_dxendpoint_property_t **properties, size_t size);

/**
 * Checks if a property is supported
 *
 * @param builder The endpoint's builder
 * @param key The property's key to be checked for support
 * @param[out] supports `1` if the corresponding property key is supported.
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_dxendpoint_builder_supports_property(dxfc_dxendpoint_builder_t builder, const char *key,
                                                            DXFC_OUT int *supports);

/**
 * Builds the new dxFeed endpoint instance.
 *
 * @param builder The endpoint's builder
 * @param user_data User data that will be passed to the endpoint's state transition listeners.
 * @param[out] endpoint The created endpoint
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_dxendpoint_builder_build(dxfc_dxendpoint_builder_t builder, void *user_data,
                                                DXFC_OUT dxfc_dxendpoint_t *endpoint);

/**
 * Removes a builder from the registry.
 *
 * @param builder The endpoint's builder
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_dxendpoint_builder_free(dxfc_dxendpoint_builder_t builder);

/**
 * Returns a default application-wide singleton instance of dxFeed endpoint with a @ref DXFC_DXENDPOINT_ROLE_FEED "FEED"
 * role. Most applications use only a single data-source and should rely on this function to get one. This function
 * creates an endpoint on the first use with a default configuration. You can provide configuration via system
 * properties.
 *
 * This is a shortcut to
 * ```c
 *  dxfc_dxendpoint_t endpoint;
 *
 *  dxfc_dxendpoint_get_instance2(DXFC_DXENDPOINT_ROLE_FEED, user_data, &endpoint);
 *  ```
 *
 * @param user_data User data that will be passed to the endpoint's state transition listeners.
 * @param[out] endpoint The created endpoint
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_dxendpoint_get_instance(void *user_data, DXFC_OUT dxfc_dxendpoint_t *endpoint);

/**
 * Returns a default application-wide singleton instance of DXEndpoint for a specific role.
 * Most applications use only a single data-source and should rely on this function to get one.
 * This function creates an endpoint with the corresponding role on the first use with a default
 * configuration.
 * You can provide configuration via system properties.
 *
 * The configuration does not have to include an address. You can use @ref dxfc_dxendpoint_connect()
 * "connect(endpoint, address)" and dxfc_dxendpoint_disconnect() functions on the instance that is returned by this
 * function to programmatically
 * establish and tear-down connection to a user-provided address.
 *
 * If you need a fully programmatic configuration and/or multiple endpoints of the same role in your
 * application, then create a custom instance of dxFeed endpoint with dxfc_dxendpoint_new_builder() function, configure
 * it, and use dxfc_dxendpoint_builder_build() function.
 *
 * @param role The role of dxFeed endpoint instance
 * @param user_data User data that will be passed to the endpoint's state transition listeners.
 * @param[out] endpoint The created endpoint
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_dxendpoint_get_instance2(dxfc_dxendpoint_role_t role, void *user_data,
                                                DXFC_OUT dxfc_dxendpoint_t *endpoint);

/**
 * Creates an endpoint with @ref DXFC_DXENDPOINT_ROLE_FEED "FEED" role.
 * The result of this function is the same as <b>`dxfc_dxendpoint_create2(DXFC_DXENDPOINT_ROLE_FEED, user_data,
 * &endpoint)`</b>. This is a shortcut to
 * ```c
 * dxfc_dxendpoint_builder_t builder;
 * dxfc_dxendpoint_t endpoint;
 *
 * dxfc_dxendpoint_new_builder(&builder);
 * dxfc_dxendpoint_builder_build(builder, user_data, &endpoint);
 * ```
 *
 * @param user_data User data that will be passed to the endpoint's state transition listeners.
 * @param[out] endpoint The created endpoint
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_dxendpoint_create(void *user_data, DXFC_OUT dxfc_dxendpoint_t *endpoint);

/**
 * Creates an endpoint with a specified role.
 * This is a shortcut to
 * ```c
 * dxfc_dxendpoint_builder_t builder;
 * dxfc_dxendpoint_t endpoint;
 *
 * dxfc_dxendpoint_new_builder(&builder);
 * dxfc_dxendpoint_builder_with_role(builder, role);
 * dxfc_dxendpoint_builder_build(builder, user_data, &endpoint);
 * ```
 *
 * @param role The role of dxFeed endpoint instance
 * @param user_data User data that will be passed to the endpoint's state transition listeners.
 * @param[out] endpoint The created endpoint
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_dxendpoint_create2(dxfc_dxendpoint_role_t role, void *user_data,
                                          DXFC_OUT dxfc_dxendpoint_t *endpoint);

/**
 * Closes this endpoint. All network connection are terminated as with dxfc_dxendpoint_disconnect() function and no
 * further connections can be established.
 *
 * The endpoint @ref dxfc_dxendpoint_get_state() "state" immediately becomes @ref DXFC_DXENDPOINT_STATE_CLOSED "CLOSED".
 * All resources associated with this endpoint are released.
 *
 * @param endpoint The dxFeed endpoint to close
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_dxendpoint_close(dxfc_dxendpoint_t endpoint);

/**
 * Closes this endpoint and wait until all pending data processing tasks are completed.
 * This function performs the same actions as dxfc_dxendpoint_close(), but also awaits
 * termination of all outstanding data processing tasks. It is designed to be used
 * with @ref DXFC_DXENDPOINT_ROLE_STREAM_FEED "STREAM_FEED" role after dxfc_dxendpoint_await_not_connected() function
 * returns to make sure that file was completely processed.
 *
 * <p><b>This function is blocking.</b>
 *
 * @param endpoint The dxFeed endpoint
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_dxendpoint_close_and_await_termination(dxfc_dxendpoint_t endpoint);

/**
 * Returns the role of this endpoint.
 *
 * @param endpoint The dxFeed endpoint
 * @param[out] role The role of this endpoint
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_dxendpoint_get_role(dxfc_dxendpoint_t endpoint, DXFC_OUT dxfc_dxendpoint_role_t *role);

/**
 * Changes user name for this endpoint.
 * This function shall be called before @ref dxfc_dxendpoint_connect() "connect" together
 * with @ref dxfc_dxendpoint_password() "password" to configure service access credentials.
 *
 * @param endpoint The dxFeed endpoint
 * @param user The user name
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_dxendpoint_user(dxfc_dxendpoint_t endpoint, const char *user);

/**
 * Changes password for this endpoint.
 * This function shall be called before @ref dxfc_dxendpoint_connect() "connect" together
 * with @ref dxfc_dxendpoint_user() "user" to configure service access credentials.
 *
 * @param endpoint The dxFeed endpoint
 * @param password The user password
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_dxendpoint_password(dxfc_dxendpoint_t endpoint, const char *password);

/**
 * Connects to the specified remote address. Previously established connections are closed if
 * the new address is different from the old one.
 * This function does nothing if address does not change or if this endpoint is @ref DXFC_DXENDPOINT_STATE_CLOSED
 * "CLOSED". The endpoint @ref dxfc_dxendpoint_get_state() "state" immediately becomes @ref
 * DXFC_DXENDPOINT_STATE_CONNECTING "CONNECTING" otherwise.
 *
 * <p> The address string is provided with the market data vendor agreement.
 * Use "demo.dxfeed.com:7300" for a demo quote feed.
 *
 * <p> The simplest address strings have the following format:
 * * `host:port` to establish a TCP/IP connection.
 * * `:port` to listen for a TCP/IP connection with a plain socket connector (good for up to a
 * few hundred of connections).
 *
 * <p>For premium services access credentials must be configured before invocation of `connect` function
 * using @ref dxfc_dxendpoint_user() "user" and @ref dxfc_dxendpoint_password() "password" functions.
 *
 * <p> <b>This function does not wait until connection actually gets established</b>. The actual connection
 * establishment happens asynchronously after the invocation of this function. However, this function waits until
 * notification about state transition from DXFC_DXENDPOINT_STATE_NOT_CONNECTED to DXFC_DXENDPOINT_STATE_CONNECTING gets
 * processed by all listeners.
 *
 * [Javadoc.](https://docs.dxfeed.com/dxfeed/api/com/dxfeed/api/DXEndpoint.html#connect-java.lang.String-)
 *
 * @param endpoint The dxFeed endpoint
 * @param address The data source address
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_dxendpoint_connect(dxfc_dxendpoint_t endpoint, const char *address);

/**
 * Terminates all established network connections and initiates connecting again with the same address.
 *
 * <p>The effect of the function is alike to invoking dxfc_dxendpoint_disconnect() and dxfc_dxendpoint_connect()
 * with the current address, but internal resources used for connections may be reused by implementation.
 * TCP connections with multiple target addresses will try switch to an alternative address, configured
 * reconnect timeouts will apply.
 *
 * <p><b>Note:</b> The function will not connect endpoint that was not initially connected with
 * dxfc_dxendpoint_connect() function or was disconnected with dxfc_dxendpoint_disconnect() function.
 *
 * <p>The function initiates a short-path way for reconnecting, so whether observers will have a chance to see
 * an intermediate state DXFC_DXENDPOINT_STATE_NOT_CONNECTED depends on the implementation.
 *
 * @param endpoint The dxFeed endpoint to reconnect
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_dxendpoint_reconnect(dxfc_dxendpoint_t endpoint);

/**
 * Terminates all remote network connections.
 * This function does nothing if this endpoint is @ref DXFC_DXENDPOINT_STATE_CLOSED "CLOSED".
 * The endpoint @ref dxfc_dxendpoint_get_state() "state" immediately becomes @ref DXFC_DXENDPOINT_STATE_NOT_CONNECTED
 * "NOT_CONNECTED" otherwise.
 *
 * <p>This function does not release all resources that are associated with this endpoint.
 * Use dxfc_dxendpoint_disconnect() function to release all resources.
 *
 * @param endpoint The dxFeed endpoint to disconnect
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_dxendpoint_disconnect(dxfc_dxendpoint_t endpoint);

/**
 * Terminates all remote network connections and clears stored data.
 * This function does nothing if this endpoint is @ref DXFC_DXENDPOINT_STATE_CLOSED "CLOSED".
 * The endpoint @ref dxfc_dxendpoint_get_state() "state" immediately becomes @ref DXFC_DXENDPOINT_STATE_NOT_CONNECTED
 * "NOT_CONNECTED" otherwise.
 *
 * <p>This function does not release all resources that are associated with this endpoint.
 * Use dxfc_dxendpoint_close() function to release all resources.
 *
 * @param endpoint The dxFeed endpoint
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_dxendpoint_disconnect_and_clear(dxfc_dxendpoint_t endpoint);

/**
 * Waits until this endpoint stops processing data (becomes quiescent).
 * This is important when writing data to file via "tape:..." connector to make sure that
 * all published data was written before closing this endpoint.
 *
 * @param endpoint The dxFeed endpoint
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_dxendpoint_await_processed(dxfc_dxendpoint_t endpoint);

/**
 * Waits while this endpoint @ref dxfc_dxendpoint_get_state() "state" becomes @ref DXFC_DXENDPOINT_STATE_NOT_CONNECTED
 * "NOT_CONNECTED" or
 * @ref DXFC_DXENDPOINT_STATE_CLOSED "CLOSED". It is a signal that any files that were opened with
 * @ref dxfc_dxendpoint_connect() "dxfc_dxendpoint_connect(endpoint, \"file:...\")" function were finished reading, but
 * not necessary were completely processed by the corresponding subscription listeners. Use
 * dxfc_dxendpoint_and_await_termination() after this function returns to make sure that all processing has completed.
 *
 * <p><b>This function is blocking.</b>
 *
 * @param endpoint The dxFeed endpoint
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_dxendpoint_await_not_connected(dxfc_dxendpoint_t endpoint);

/**
 * Returns the state of this endpoint.
 *
 * @param endpoint The dxFeed endpoint
 * @param[out] state The state of this endpoint
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_dxendpoint_get_state(dxfc_dxendpoint_t endpoint, DXFC_OUT dxfc_dxendpoint_state_t *state);

/**
 * Adds listener that is notified about changes in @ref dxfc_dxendpoint_get_state() "state" property.
 *
 * <p>Installed listener can be removed by dxfc_dxendpoint_remove_state_change_listener() function.
 *
 * @param endpoint The dxFeed endpoint
 * @param listener The listener to add
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_dxendpoint_add_state_change_listener(dxfc_dxendpoint_t endpoint,
                                                            dxfc_dxendpoint_state_change_listener listener);

/**
 * Removes listener that is notified about changes in @ref dxfc_dxendpoint_get_state() "state" property.
 * It removes the listener that was previously installed with dxfc_dxendpoint_add_state_change_listener() function.
 *
 * @param endpoint The dxFeed endpoint
 * @param listener The listener to remove
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_dxendpoint_remove_state_change_listener(dxfc_dxendpoint_t endpoint,
                                                               dxfc_dxendpoint_state_change_listener listener);

/**
 *
 * @param endpoint The dxFeed endpoint
 * @param[out] feed
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_dxendpoint_get_feed(dxfc_dxendpoint_t endpoint, DXFC_OUT dxfc_dxfeed_t *feed);

/**
 *
 * @param endpoint The dxFeed endpoint
 * @param[out] publisher
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_dxendpoint_get_publisher(dxfc_dxendpoint_t endpoint, DXFC_OUT dxfc_dxpublisher_t *publisher);

/**
 * Removes the dxFeed endpoint from the registry.
 * @param endpoint The dxFeed endpoint to remove
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_dxendpoint_free(dxfc_dxendpoint_t endpoint);

#ifdef __cplusplus
}
#endif

#endif // DXFEED_NATIVE_API_H_INCLUDED
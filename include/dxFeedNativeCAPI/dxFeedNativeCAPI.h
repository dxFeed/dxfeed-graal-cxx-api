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
 *
 */
typedef enum dxfc_endpoint_role_t {
    ///
    DXFC_ENDPOINT_ROLE_FEED = 0,
    ///
    DXFC_ENDPOINT_ROLE_ON_DEMAND_FEED,
    ///
    DXFC_ENDPOINT_ROLE_STREAM_FEED,
    ///
    DXFC_ENDPOINT_ROLE_PUBLISHER,
    ///
    DXFC_ENDPOINT_ROLE_STREAM_PUBLISHER,
    ///
    DXFC_ENDPOINT_ROLE_LOCAL_HUB,
} dxfc_endpoint_role_t;

/**
 *
 */
typedef enum dxfc_endpoint_state_t {
    ///
    DXFC_ENDPOINT_STATE_NOT_CONNECTED = 0,
    ///
    DXFC_ENDPOINT_STATE_CONNECTING,
    ///
    DXFC_ENDPOINT_STATE_CONNECTED,
    ///
    DXFC_ENDPOINT_STATE_CLOSED,
} dxfc_endpoint_state_t;

/**
 *
 */
typedef void (*dxfc_endpoint_state_change_listener)(dxfc_endpoint_state_t old_state, dxfc_endpoint_state_t new_state,
                                                    void *user_data);

/**
 *
 */
typedef struct dxfc_enpoint_property_t {
    const char *key;
    const char *value;
} dxfc_enpoint_property_t;

/**
 *
 */
typedef void *dxfc_endpoint_t;

/**
 *
 */
typedef void *dxfc_endpoint_builder_t;

/**
 *
 */
typedef void *dxfc_feed_t;

/**
 *
 */
typedef void *dxfc_publisher_t;

/**
 *
 * @param[out] builder
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_endpoint_new_builder(DXFC_OUT dxfc_endpoint_builder_t *builder);

/**
 *
 * @param builder
 * @param role
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_endpoint_builder_with_role(dxfc_endpoint_builder_t builder, dxfc_endpoint_role_t role);

/**
 *
 * @param builder
 * @param name
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_endpoint_builder_with_name(dxfc_endpoint_builder_t builder, const char *name);

/**
 *
 * @param builder
 * @param key
 * @param value
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_endpoint_builder_with_property(dxfc_endpoint_builder_t builder, const char *key,
                                                      const char *value);

/**
 *
 * @param builder
 * @param properties
 * @param size
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_endpoint_builder_with_properties(dxfc_endpoint_builder_t builder,
                                                        const dxfc_enpoint_property_t **properties, size_t size);

/**
 *
 * @param builder
 * @param key
 * @return
 */
int dxfc_endpoint_builder_supports_property(dxfc_endpoint_builder_t builder, const char *key);

/**
 *
 * @param user_data
 * @param[out] endpoint
 * @return
 */
dxfc_error_code_t dxfc_endpoint_builder_build(void *user_data, DXFC_OUT dxfc_endpoint_t *endpoint);

/**
 *
 * @param builder
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_endpoint_builder_free(dxfc_endpoint_builder_t builder);

/**
 *
 * @param user_data
 * @param[out] endpoint
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_endpoint_get_instance(void *user_data, DXFC_OUT dxfc_endpoint_t *endpoint);

/**
 *
 * @param role
 * @param user_data
 * @param[out] endpoint
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_endpoint_get_instance2(dxfc_endpoint_role_t role, void *user_data,
                                              DXFC_OUT dxfc_endpoint_t *endpoint);

/**
 *
 * @param user_data
 * @param[out] endpoint
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_endpoint_create(void *user_data, DXFC_OUT dxfc_endpoint_t *endpoint);

/**
 *
 * @param role
 * @param user_data
 * @param[out] endpoint
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_endpoint_create2(dxfc_endpoint_role_t role, void *user_data, DXFC_OUT dxfc_endpoint_t *endpoint);

/**
 *
 * @param endpoint
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_endpoint_close(dxfc_endpoint_t endpoint);

/**
 *
 * @param endpoint
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_endpoint_close_and_await_termination(dxfc_endpoint_t endpoint);

/**
 *
 * @param endpoint
 * @param[out] role
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_endpoint_get_role(dxfc_endpoint_t endpoint, DXFC_OUT dxfc_endpoint_role_t *role);

/**
 *
 * @param endpoint
 * @param user
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_endpoint_user(dxfc_endpoint_t endpoint, const char *user);

/**
 *
 * @param endpoint
 * @param password
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_endpoint_password(dxfc_endpoint_t endpoint, const char *password);

/**
 *
 * @param endpoint
 * @param address
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_endpoint_connect(dxfc_endpoint_t endpoint, const char *address);

/**
 *
 * @param endpoint
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_endpoint_reconnect(dxfc_endpoint_t endpoint);

/**
 *
 * @param endpoint
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_endpoint_disconnect(dxfc_endpoint_t endpoint);

/**
 *
 * @param endpoint
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_endpoint_disconnect_and_clear(dxfc_endpoint_t endpoint);

/**
 *
 * @param endpoint
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_endpoint_await_processed(dxfc_endpoint_t endpoint);

/**
 *
 * @param endpoint
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_endpoint_await_not_connected(dxfc_endpoint_t endpoint);

/**
 *
 * @param endpoint
 * @param[out] state
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_endpoint_get_state(dxfc_endpoint_t endpoint, DXFC_OUT dxfc_endpoint_state_t *state);

/**
 *
 * @param endpoint
 * @param listener
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_endpoint_add_state_change_listener(dxfc_endpoint_t endpoint,
                                                          dxfc_endpoint_state_change_listener listener);

/**
 *
 * @param endpoint
 * @param listener
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_endpoint_remove_state_change_listener(dxfc_endpoint_t endpoint,
                                                             dxfc_endpoint_state_change_listener listener);

/**
 *
 * @param endpoint
 * @param[out] feed
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_endpoint_get_feed(dxfc_endpoint_t endpoint, DXFC_OUT dxfc_feed_t *feed);

/**
 *
 * @param endpoint
 * @param[out] publisher
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfg_endpoint_get_publisher(dxfc_endpoint_t endpoint, DXFC_OUT dxfc_publisher_t *publisher);

/**
 *
 * @param endpoint
 * @return DXFC_EC_SUCCESS - if the operation was successful; otherwise - DXFC_EC_ERROR.
 */
dxfc_error_code_t dxfc_endpoint_free(dxfc_endpoint_t endpoint);

#ifdef __cplusplus
}
#endif

#endif // DXFEED_NATIVE_API_H_INCLUDED
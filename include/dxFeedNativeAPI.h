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
dxfc_error_code_t dxfc_system_get_property(const char *key, char *buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif // DXFEED_NATIVE_API_H_INCLUDED
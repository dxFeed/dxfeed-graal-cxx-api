// SPDX-License-Identifier: MPL-2.0

#ifndef DXFEED_NATIVE_API_H_INCLUDED
#define DXFEED_NATIVE_API_H_INCLUDED

#include <dxf_graal_error_codes.h>

#ifdef __cplusplus
#    include <cstdint>
extern "C" {
#else
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
     * @brief (dxFeed Graal Native API) Shouldn't happen.
     */
    DXFC_EC_G_UNKNOWN_ERR,
    /**
     * @brief (dxFeed Graal Native API) Represents NullPointerException.
     */
    DXFC_EC_G_NULL_POINTER_EX,
    /**
     * @brief (dxFeed Graal Native API) Represents IllegalArgumentException.
     */
    DXFC_EC_G_ILLEGAL_ARGUMENT_EX,
    /**
     * @brief (dxFeed Graal Native API) Represents SecurityException.
     */
    DXFC_EC_G_SECURITY_EX,
    /**
     * @brief (dxFeed Graal Native API) Represents IllegalStateException.
     */
    DXFC_EC_G_ILLEGAL_STATE_EX,
    /**
     * @brief (dxFeed Graal Native API) An unknown descriptor was passed.
     */
    DXFC_EC_G_UNKNOWN_DESCRIPTOR,
} dxfc_error_code_t;

dxfc_error_code_t dxfc_system_set_property(const char *key, const char *value);

dxfc_error_code_t dxfc_system_get_property(const char *key, char *buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif // DXFEED_NATIVE_API_H_INCLUDED
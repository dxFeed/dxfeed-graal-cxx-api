// SPDX-License-Identifier: MPL-2.0

#ifndef DXFEED_NATIVE_API_H_INCLUDED
#define DXFEED_NATIVE_API_H_INCLUDED

#include <dxf_graal_error_codes.h>

#ifdef __cplusplus
extern "C" {
#endif

ERROR_CODE dxfc_system_set_property(const char *key, const char *value);

ERROR_CODE dxfc_system_get_property(const char *key, char *value);

#ifdef __cplusplus
}
#endif

#endif // DXFEED_NATIVE_API_H_INCLUDED
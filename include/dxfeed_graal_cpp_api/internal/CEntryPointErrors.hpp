// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <compare>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>

#include "Common.hpp"

#ifdef NO_ERROR
#    undef NO_ERROR
#endif

DXFCPP_BEGIN_NAMESPACE

/**
 * Enum of the possible error codes returned by internal GraalVM functions
 *
 *  [Graal:CEntryPointErrors](https://github.com/oracle/graal/blob/master/substratevm/src/com.oracle.svm.core/src/com/oracle/svm/core/c/function/CEntryPointErrors.java)
 */
enum class CEntryPointErrorsEnum : uint32_t {
    /// 0 - No error occurred.
    NO_ERROR = 0,

    /// 1 - An unspecified error occurred.
    UNSPECIFIED = 1,

    /// 2 - An argument was NULL (nullptr).
    NULL_ARGUMENT = 2,

    /// 3 - Memory allocation failed, the OS is probably out of memory..
    ALLOCATION_FAILED = 3,

    /// 4 - The specified thread is not attached to the isolate.
    UNATTACHED_THREAD = 4,

    /// 5 - The specified isolate is unknown.
    UNINITIALIZED_ISOLATE = 5,

    /// 6 - Locating the image file failed.
    LOCATE_IMAGE_FAILED = 6,

    /// 7 - Opening the located image file failed.
    OPEN_IMAGE_FAILED = 7,

    /// 8 - Mapping the heap from the image file into memory failed.
    MAP_HEAP_FAILED = 8,

    /// 801 - Reserving address space for the new isolate failed.
    RESERVE_ADDRESS_SPACE_FAILED = 801,

    /// 802 - The image heap does not fit in the available address space.
    INSUFFICIENT_ADDRESS_SPACE = 802,

    /// 9 - Setting the protection of the heap memory failed.
    PROTECT_HEAP_FAILED = 9,

    /// 10 - The version of the specified isolate parameters is unsupported.
    UNSUPPORTED_ISOLATE_PARAMETERS_VERSION = 10,

    /// 11 - Initialization of threading in the isolate failed.
    THREADING_INITIALIZATION_FAILED = 11,

    /// 12 - Some exception is not caught.
    UNCAUGHT_EXCEPTION = 12,

    /// 13 - Initialization the isolate failed.
    ISOLATE_INITIALIZATION_FAILED = 13,

    /// 14 - Opening the located auxiliary image file failed.
    OPEN_AUX_IMAGE_FAILED = 14,

    /// 15 - Reading the opened auxiliary image file failed.
    READ_AUX_IMAGE_META_FAILED = 15,

    /// 16 - Mapping the auxiliary image file into memory failed.
    MAP_AUX_IMAGE_FAILED = 16,

    /// 17 - Insufficient memory for the auxiliary image.
    INSUFFICIENT_AUX_IMAGE_MEMORY = 17,

    /// 18 - Auxiliary images are not supported on this platform or edition.
    AUX_IMAGE_UNSUPPORTED = 18,

    /// 19 - Releasing the isolate's address space failed.
    FREE_ADDRESS_SPACE_FAILED = 19,

    /// 20 - Releasing the isolate's image heap memory failed.
    FREE_IMAGE_HEAP_FAILED = 20,

    /// 21 - The auxiliary image was built from a different primary image.
    AUX_IMAGE_PRIMARY_IMAGE_MISMATCH = 21,

    /// 22 - The isolate arguments could not be parsed.
    ARGUMENT_PARSING_FAILED = 22,

    /// 23 - Current target does not support the following CPU features that are required by the image.
    CPU_FEATURE_CHECK_FAILED = 23,

    /// 24 - Image page size is incompatible with run-time page size. Rebuild image with -H:PageSize=[pagesize] to set
    /// appropriately.
    PAGE_SIZE_CHECK_FAILED = 24,

    /// 25 - Creating an in-memory file for the GOT failed.
    DYNAMIC_METHOD_ADDRESS_RESOLUTION_GOT_FD_CREATE_FAILED = 25,

    /// 26 - Resizing the in-memory file for the GOT failed.
    DYNAMIC_METHOD_ADDRESS_RESOLUTION_GOT_FD_RESIZE_FAILED = 26,

    /// 27 - Mapping and populating the in-memory file for the GOT failed.
    DYNAMIC_METHOD_ADDRESS_RESOLUTION_GOT_FD_MAP_FAILED = 27,

    /// 28 - Mapping the GOT before an isolate's heap failed (no mapping).
    DYNAMIC_METHOD_ADDRESS_RESOLUTION_GOT_MMAP_FAILED = 28,

    /// 29 - Mapping the GOT before an isolate's heap failed (wrong mapping).
    DYNAMIC_METHOD_ADDRESS_RESOLUTION_GOT_WRONG_MMAP = 29,

    /// 30 - Mapping the GOT before an isolate's heap failed (invalid file).
    DYNAMIC_METHOD_ADDRESS_RESOLUTION_GOT_FD_INVALID = 30,

    /// 31 - Could not create unique GOT file even after retrying.
    DYNAMIC_METHOD_ADDRESS_RESOLUTION_GOT_UNIQUE_FILE_CREATE_FAILED = 31,

    /// 32 - Could not determine the stack boundaries.
    UNKNOWN_STACK_BOUNDARIES = 32,
};

inline constexpr const char *CEntryPointErrorsEnumToStr(CEntryPointErrorsEnum enumValue) {
    switch (enumValue) {
    case CEntryPointErrorsEnum::NO_ERROR:
        return "No error occurred.";
    case CEntryPointErrorsEnum::UNSPECIFIED:
        return "An unspecified error occurred.";
    case CEntryPointErrorsEnum::NULL_ARGUMENT:
        return "An argument was NULL (nullptr).";
    case CEntryPointErrorsEnum::ALLOCATION_FAILED:
        return "Memory allocation failed, the OS is probably out of memory.";
    case CEntryPointErrorsEnum::UNATTACHED_THREAD:
        return "The specified thread is not attached to the isolate.";
    case CEntryPointErrorsEnum::UNINITIALIZED_ISOLATE:
        return "The specified isolate is unknown.";
    case CEntryPointErrorsEnum::LOCATE_IMAGE_FAILED:
        return "Locating the image file failed.";
    case CEntryPointErrorsEnum::OPEN_IMAGE_FAILED:
        return "Opening the located image file failed.";
    case CEntryPointErrorsEnum::MAP_HEAP_FAILED:
        return "Mapping the heap from the image file into memory failed.";
    case CEntryPointErrorsEnum::RESERVE_ADDRESS_SPACE_FAILED:
        return "Reserving address space for the new isolate failed.";
    case CEntryPointErrorsEnum::INSUFFICIENT_ADDRESS_SPACE:
        return "The image heap does not fit in the available address space.";
    case CEntryPointErrorsEnum::PROTECT_HEAP_FAILED:
        return "The version of the specified isolate parameters is unsupported.";
    case CEntryPointErrorsEnum::UNSUPPORTED_ISOLATE_PARAMETERS_VERSION:
        return "The version of the specified isolate parameters is unsupported.";
    case CEntryPointErrorsEnum::THREADING_INITIALIZATION_FAILED:
        return "Initialization of threading in the isolate failed.";
    case CEntryPointErrorsEnum::UNCAUGHT_EXCEPTION:
        return "Some exception is not caught.";
    case CEntryPointErrorsEnum::ISOLATE_INITIALIZATION_FAILED:
        return "Initialization the isolate failed.";
    case CEntryPointErrorsEnum::OPEN_AUX_IMAGE_FAILED:
        return "Opening the located auxiliary image file failed.";
    case CEntryPointErrorsEnum::READ_AUX_IMAGE_META_FAILED:
        return "Reading the opened auxiliary image file failed.";
    case CEntryPointErrorsEnum::MAP_AUX_IMAGE_FAILED:
        return "Mapping the auxiliary image file into memory failed.";
    case CEntryPointErrorsEnum::INSUFFICIENT_AUX_IMAGE_MEMORY:
        return "Insufficient memory for the auxiliary image.";
    case CEntryPointErrorsEnum::AUX_IMAGE_UNSUPPORTED:
        return "Auxiliary images are not supported on this platform or edition.";
    case CEntryPointErrorsEnum::FREE_ADDRESS_SPACE_FAILED:
        return "Releasing the isolate's address space failed.";
    case CEntryPointErrorsEnum::FREE_IMAGE_HEAP_FAILED:
        return "Releasing the isolate's image heap memory failed.";
    case CEntryPointErrorsEnum::AUX_IMAGE_PRIMARY_IMAGE_MISMATCH:
        return "The auxiliary image was built from a different primary image.";
    case CEntryPointErrorsEnum::ARGUMENT_PARSING_FAILED:
        return "The isolate arguments could not be parsed.";
    case CEntryPointErrorsEnum::CPU_FEATURE_CHECK_FAILED:
        return "Current target does not support the following CPU features that are required by the image.";
    case CEntryPointErrorsEnum::PAGE_SIZE_CHECK_FAILED:
        return "Image page size is incompatible with run-time page size. Rebuild image with -H:PageSize=[pagesize] to set appropriately.";
    case CEntryPointErrorsEnum::DYNAMIC_METHOD_ADDRESS_RESOLUTION_GOT_FD_CREATE_FAILED:
        return "Creating an in-memory file for the GOT failed.";
    case CEntryPointErrorsEnum::DYNAMIC_METHOD_ADDRESS_RESOLUTION_GOT_FD_RESIZE_FAILED:
        return "Resizing the in-memory file for the GOT failed.";
    case CEntryPointErrorsEnum::DYNAMIC_METHOD_ADDRESS_RESOLUTION_GOT_FD_MAP_FAILED:
        return "Mapping and populating the in-memory file for the GOT failed.";
    case CEntryPointErrorsEnum::DYNAMIC_METHOD_ADDRESS_RESOLUTION_GOT_MMAP_FAILED:
        return "Mapping the GOT before an isolate's heap failed (no mapping).";
    case CEntryPointErrorsEnum::DYNAMIC_METHOD_ADDRESS_RESOLUTION_GOT_WRONG_MMAP:
        return "Mapping the GOT before an isolate's heap failed (wrong mapping).";
    case CEntryPointErrorsEnum::DYNAMIC_METHOD_ADDRESS_RESOLUTION_GOT_FD_INVALID:
        return "Mapping the GOT before an isolate's heap failed (invalid file).";
    case CEntryPointErrorsEnum::DYNAMIC_METHOD_ADDRESS_RESOLUTION_GOT_UNIQUE_FILE_CREATE_FAILED:
        return "Could not create unique GOT file even after retrying.";
    case CEntryPointErrorsEnum::UNKNOWN_STACK_BOUNDARIES:
        return "Could not determine the stack boundaries.";
    }

    return nullptr;
}

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
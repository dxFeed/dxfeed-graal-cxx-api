// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <utility>

#ifdef NO_ERROR
#    undef NO_ERROR
#endif

namespace dxfcpp {

const CEntryPointErrors CEntryPointErrors::NO_ERROR(0, "No error occurred.");
const CEntryPointErrors CEntryPointErrors::UNSPECIFIED(1, "An unspecified error occurred.");
const CEntryPointErrors CEntryPointErrors::NULL_ARGUMENT(2, "An argument was NULL (nullptr).");
const CEntryPointErrors CEntryPointErrors::UNATTACHED_THREAD(4, "The specified thread is not attached to the isolate.");
const CEntryPointErrors CEntryPointErrors::UNINITIALIZED_ISOLATE(5, "The specified isolate is unknown.");
const CEntryPointErrors CEntryPointErrors::LOCATE_IMAGE_FAILED(6, "Locating the image file failed.");
const CEntryPointErrors CEntryPointErrors::OPEN_IMAGE_FAILED(7, "Opening the located image file failed.");
const CEntryPointErrors CEntryPointErrors::MAP_HEAP_FAILED(8,
                                                           "Mapping the heap from the image file into memory failed.");
const CEntryPointErrors
    CEntryPointErrors::RESERVE_ADDRESS_SPACE_FAILED(801, "Reserving address space for the new isolate failed.");
const CEntryPointErrors
    CEntryPointErrors::INSUFFICIENT_ADDRESS_SPACE(802, "The image heap does not fit in the available address space.");
const CEntryPointErrors CEntryPointErrors::PROTECT_HEAP_FAILED(9, "Setting the protection of the heap memory failed.");
const CEntryPointErrors CEntryPointErrors::UNSUPPORTED_ISOLATE_PARAMETERS_VERSION(
    10, "The version of the specified isolate parameters is unsupported.");
const CEntryPointErrors
    CEntryPointErrors::THREADING_INITIALIZATION_FAILED(11, "Initialization of threading in the isolate failed.");
const CEntryPointErrors CEntryPointErrors::UNCAUGHT_EXCEPTION(12, "Some exception is not caught.");
const CEntryPointErrors CEntryPointErrors::ISOLATE_INITIALIZATION_FAILED(13, "Initialization the isolate failed.");
const CEntryPointErrors CEntryPointErrors::OPEN_AUX_IMAGE_FAILED(14,
                                                                 "Opening the located auxiliary image file failed.");
const CEntryPointErrors
    CEntryPointErrors::READ_AUX_IMAGE_META_FAILED(15, "Reading the opened auxiliary image file failed.");
const CEntryPointErrors CEntryPointErrors::MAP_AUX_IMAGE_FAILED(16,
                                                                "Mapping the auxiliary image file into memory failed.");
const CEntryPointErrors
    CEntryPointErrors::INSUFFICIENT_AUX_IMAGE_MEMORY(17, "Insufficient memory for the auxiliary image.");
const CEntryPointErrors
    CEntryPointErrors::AUX_IMAGE_UNSUPPORTED(18, "Auxiliary images are not supported on this platform or edition.");
const CEntryPointErrors CEntryPointErrors::FREE_ADDRESS_SPACE_FAILED(19,
                                                                     "Releasing the isolate's address space failed.");
const CEntryPointErrors CEntryPointErrors::FREE_IMAGE_HEAP_FAILED(20,
                                                                  "Releasing the isolate's image heap memory failed.");
const CEntryPointErrors CEntryPointErrors::AUX_IMAGE_PRIMARY_IMAGE_MISMATCH(
    21, "The auxiliary image was built from a different primary image.");
const CEntryPointErrors CEntryPointErrors::ARGUMENT_PARSING_FAILED(22, "The isolate arguments could not be parsed.");
const CEntryPointErrors CEntryPointErrors::CPU_FEATURE_CHECK_FAILED(
    23, "Current target does not support the following CPU features that are required by the image.");

const std::unordered_map<CEntryPointErrors::CodeType, std::reference_wrapper<const CEntryPointErrors>>
    CEntryPointErrors::ALL{
        {NO_ERROR.getCode(), std::cref(NO_ERROR)},
        {UNSPECIFIED.getCode(), std::cref(UNSPECIFIED)},
        {NULL_ARGUMENT.getCode(), std::cref(NULL_ARGUMENT)},
        {UNATTACHED_THREAD.getCode(), std::cref(UNATTACHED_THREAD)},
        {UNINITIALIZED_ISOLATE.getCode(), std::cref(UNINITIALIZED_ISOLATE)},
        {LOCATE_IMAGE_FAILED.getCode(), std::cref(LOCATE_IMAGE_FAILED)},
        {OPEN_IMAGE_FAILED.getCode(), std::cref(OPEN_IMAGE_FAILED)},
        {MAP_HEAP_FAILED.getCode(), std::cref(MAP_HEAP_FAILED)},
        {RESERVE_ADDRESS_SPACE_FAILED.getCode(), std::cref(RESERVE_ADDRESS_SPACE_FAILED)},
        {INSUFFICIENT_ADDRESS_SPACE.getCode(), std::cref(INSUFFICIENT_ADDRESS_SPACE)},
        {PROTECT_HEAP_FAILED.getCode(), std::cref(PROTECT_HEAP_FAILED)},
        {UNSUPPORTED_ISOLATE_PARAMETERS_VERSION.getCode(), std::cref(UNSUPPORTED_ISOLATE_PARAMETERS_VERSION)},
        {THREADING_INITIALIZATION_FAILED.getCode(), std::cref(THREADING_INITIALIZATION_FAILED)},
        {UNCAUGHT_EXCEPTION.getCode(), std::cref(UNCAUGHT_EXCEPTION)},
        {ISOLATE_INITIALIZATION_FAILED.getCode(), std::cref(ISOLATE_INITIALIZATION_FAILED)},
        {OPEN_AUX_IMAGE_FAILED.getCode(), std::cref(OPEN_AUX_IMAGE_FAILED)},
        {READ_AUX_IMAGE_META_FAILED.getCode(), std::cref(READ_AUX_IMAGE_META_FAILED)},
        {MAP_AUX_IMAGE_FAILED.getCode(), std::cref(MAP_AUX_IMAGE_FAILED)},
        {INSUFFICIENT_AUX_IMAGE_MEMORY.getCode(), std::cref(INSUFFICIENT_AUX_IMAGE_MEMORY)},
        {AUX_IMAGE_UNSUPPORTED.getCode(), std::cref(AUX_IMAGE_UNSUPPORTED)},
        {FREE_ADDRESS_SPACE_FAILED.getCode(), std::cref(FREE_ADDRESS_SPACE_FAILED)},
        {FREE_IMAGE_HEAP_FAILED.getCode(), std::cref(FREE_IMAGE_HEAP_FAILED)},
        {AUX_IMAGE_PRIMARY_IMAGE_MISMATCH.getCode(), std::cref(AUX_IMAGE_PRIMARY_IMAGE_MISMATCH)},
        {ARGUMENT_PARSING_FAILED.getCode(), std::cref(ARGUMENT_PARSING_FAILED)},
        {CPU_FEATURE_CHECK_FAILED.getCode(), std::cref(CPU_FEATURE_CHECK_FAILED)},
    };

thread_local Isolate::IsolateThread Isolate::currentIsolateThread_{};

const auto I = Isolate::getInstance();

std::unordered_map<DXEndpoint::Role, std::shared_ptr<DXEndpoint>> DXEndpoint::INSTANCES{};

}

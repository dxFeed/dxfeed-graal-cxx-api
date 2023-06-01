// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Conf.hpp"

#include <compare>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <functional>

#include "Common.hpp"

#ifdef NO_ERROR
#    undef NO_ERROR
#endif

namespace dxfcpp {

/**
 * Possible error codes returned by internal GraalVM functions
 *
 *  [Graal:CEntryPointErrors](https://github.com/oracle/graal/blob/96a1a66347bd4e5e00ae4e8e79812ebaf8cd5e33/substratevm/src/com.oracle.svm.core/src/com/oracle/svm/core/c/function/CEntryPointErrors.java#L43)
 */
struct DXFCPP_EXPORT CEntryPointErrors {
    using CodeType = std::uint32_t;

  private:
    CodeType code_{};
    std::string description_{};

    template <Integral Code>
    CEntryPointErrors(Code code, std::string description) noexcept
        : code_{static_cast<CodeType>(code)}, description_{std::move(description)} {}

  public:
    /// 0 - No error occurred.
    static const CEntryPointErrors NO_ERROR;

    /// 1 - An unspecified error occurred.
    static const CEntryPointErrors UNSPECIFIED;

    /// 2 - An argument was NULL (nullptr).
    static const CEntryPointErrors NULL_ARGUMENT;

    /// 4 - The specified thread is not attached to the isolate.
    static const CEntryPointErrors UNATTACHED_THREAD;

    /// 5 - The specified isolate is unknown.
    static const CEntryPointErrors UNINITIALIZED_ISOLATE;

    /// 6 - Locating the image file failed.
    static const CEntryPointErrors LOCATE_IMAGE_FAILED;

    /// 7 - Opening the located image file failed.
    static const CEntryPointErrors OPEN_IMAGE_FAILED;

    /// 8 - Mapping the heap from the image file into memory failed.
    static const CEntryPointErrors MAP_HEAP_FAILED;

    /// 801 - Reserving address space for the new isolate failed.
    static const CEntryPointErrors RESERVE_ADDRESS_SPACE_FAILED;

    /// 802 - The image heap does not fit in the available address space.
    static const CEntryPointErrors INSUFFICIENT_ADDRESS_SPACE;

    /// 9 - Setting the protection of the heap memory failed.
    static const CEntryPointErrors PROTECT_HEAP_FAILED;

    /// 10 - The version of the specified isolate parameters is unsupported.
    static const CEntryPointErrors UNSUPPORTED_ISOLATE_PARAMETERS_VERSION;

    /// 11 - Initialization of threading in the isolate failed.
    static const CEntryPointErrors THREADING_INITIALIZATION_FAILED;

    /// 12 - Some exception is not caught.
    static const CEntryPointErrors UNCAUGHT_EXCEPTION;

    /// 13 - Initialization the isolate failed.
    static const CEntryPointErrors ISOLATE_INITIALIZATION_FAILED;

    /// 14 - Opening the located auxiliary image file failed.
    static const CEntryPointErrors OPEN_AUX_IMAGE_FAILED;

    /// 15 - Reading the opened auxiliary image file failed.
    static const CEntryPointErrors READ_AUX_IMAGE_META_FAILED;

    /// 16 - Mapping the auxiliary image file into memory failed.
    static const CEntryPointErrors MAP_AUX_IMAGE_FAILED;

    /// 17 - Insufficient memory for the auxiliary image.
    static const CEntryPointErrors INSUFFICIENT_AUX_IMAGE_MEMORY;

    /// 18 - Auxiliary images are not supported on this platform or edition.
    static const CEntryPointErrors AUX_IMAGE_UNSUPPORTED;

    /// 19 - Releasing the isolate's address space failed.
    static const CEntryPointErrors FREE_ADDRESS_SPACE_FAILED;

    /// 20 - Releasing the isolate's image heap memory failed.
    static const CEntryPointErrors FREE_IMAGE_HEAP_FAILED;

    /// 21 - The auxiliary image was built from a different primary image.
    static const CEntryPointErrors AUX_IMAGE_PRIMARY_IMAGE_MISMATCH;

    /// 22 - The isolate arguments could not be parsed.
    static const CEntryPointErrors ARGUMENT_PARSING_FAILED;

    /// 23 - Current target does not support the following CPU features that are required by the image.
    static const CEntryPointErrors CPU_FEATURE_CHECK_FAILED;

    static const std::unordered_map<CodeType, std::reference_wrapper<const CEntryPointErrors>> ALL;

    template <Integral Code> static const CEntryPointErrors &valueOf(Code code) {
        if (auto found = ALL.find(static_cast<CodeType>(code)); found != ALL.end()) {
            return found->second;
        }

        return UNSPECIFIED;
    }

    /// Returns the code
    [[nodiscard]] CodeType getCode() const { return code_; }

    /// Returns the description
    [[nodiscard]] const std::string &getDescription() const & { return description_; }

    bool operator==(const CEntryPointErrors &errors) const { return this->getCode() == errors.getCode(); }
};
} // namespace dxfcpp
// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "IsolatedCFI.hpp"

#include "../../../include/dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp"
#include "../IsolatedCommon.hpp"
#include "../internal/IsolatedString.hpp"

#include <dxfg_api.h>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::glossary::IsolatedCFI {

namespace {

// Clones the handles of the list: the handles of the list itself are released by dxfg_v4_cfi_value_list_release().
std::vector<void *> cloneValues(const dxfg_v4_cfi_value_list *values) {
    std::vector<void *> result{};

    if (values->size > 0) {
        result.reserve(static_cast<std::size_t>(values->size));
    }

    try {
        for (std::int32_t i = 0; i < values->size; i++) {
            auto *clone = runGraalV4FunctionWithResult(dxfg_v4_JavaObjectHandler_clone, &values->elements[i]->handler);

            result.emplace_back(clone);
        }
    } catch (...) {
        for (auto &value : result) {
            runGraalV4Function(dxfg_v4_JavaObjectHandler_release, static_cast<dxfg_v4_java_object_handler *>(value));
        }

        throw;
    }

    return result;
}

} // namespace

/// dxfg_v4_CFI_EMPTY
JavaObjectHandle<CFI> EMPTY() {
    return JavaObjectHandle<CFI>{runGraalV4FunctionWithResult(dxfg_v4_CFI_EMPTY)};
}

/// dxfg_v4_CFI_valueOf
JavaObjectHandle<CFI> valueOf(std::string_view code) {
    return JavaObjectHandle<CFI>{runGraalV4FunctionWithResult(dxfg_v4_CFI_valueOf, SdkString{code}.c_str())};
}

/// dxfg_v4_CFI_valueOfIntCode
JavaObjectHandle<CFI> valueOf(std::int32_t intCode) {
    return JavaObjectHandle<CFI>{runGraalV4FunctionWithResult(dxfg_v4_CFI_valueOfIntCode, intCode)};
}

/// dxfg_v4_CFI_getCode
std::string getCode(const JavaObjectHandle<CFI> &cfi) {
    if (!cfi) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_CFI_getCode`. The "
                                       "`cfi` handle is invalid");
    }

    auto uCode = internal::IsolatedString::toUnique(
        runGraalV4FunctionWithResult(dxfg_v4_CFI_getCode, static_cast<dxfg_v4_cfi_t *>(cfi.get())));
    auto result = dxfcpp::toString(uCode.get());

    return result;
}

/// dxfg_v4_CFI_getIntCode
std::int32_t getIntCode(const JavaObjectHandle<CFI> &cfi) {
    if (!cfi) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_CFI_getIntCode`. The "
                                       "`cfi` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_CFI_getIntCode, static_cast<dxfg_v4_cfi_t *>(cfi.get()));
}

/// dxfg_v4_CFI_getCategory
std::int16_t getCategory(const JavaObjectHandle<CFI> &cfi) {
    if (!cfi) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_CFI_getCategory`. The "
                                       "`cfi` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_CFI_getCategory, static_cast<dxfg_v4_cfi_t *>(cfi.get()));
}

/// dxfg_v4_CFI_getGroup
std::int16_t getGroup(const JavaObjectHandle<CFI> &cfi) {
    if (!cfi) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_CFI_getGroup`. The "
                                       "`cfi` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_CFI_getGroup, static_cast<dxfg_v4_cfi_t *>(cfi.get()));
}

/// dxfg_v4_CFI_isEquity
bool isEquity(const JavaObjectHandle<CFI> &cfi) {
    if (!cfi) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_CFI_isEquity`. The "
                                       "`cfi` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_CFI_isEquity, static_cast<dxfg_v4_cfi_t *>(cfi.get())) != 0;
}

/// dxfg_v4_CFI_isDebtInstrument
bool isDebtInstrument(const JavaObjectHandle<CFI> &cfi) {
    if (!cfi) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_CFI_isDebtInstrument`. The "
                                       "`cfi` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_CFI_isDebtInstrument, static_cast<dxfg_v4_cfi_t *>(cfi.get())) != 0;
}

/// dxfg_v4_CFI_isEntitlement
bool isEntitlement(const JavaObjectHandle<CFI> &cfi) {
    if (!cfi) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_CFI_isEntitlement`. The "
                                       "`cfi` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_CFI_isEntitlement, static_cast<dxfg_v4_cfi_t *>(cfi.get())) != 0;
}

/// dxfg_v4_CFI_isOption
bool isOption(const JavaObjectHandle<CFI> &cfi) {
    if (!cfi) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_CFI_isOption`. The "
                                       "`cfi` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_CFI_isOption, static_cast<dxfg_v4_cfi_t *>(cfi.get())) != 0;
}

/// dxfg_v4_CFI_isFuture
bool isFuture(const JavaObjectHandle<CFI> &cfi) {
    if (!cfi) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_CFI_isFuture`. The "
                                       "`cfi` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_CFI_isFuture, static_cast<dxfg_v4_cfi_t *>(cfi.get())) != 0;
}

/// dxfg_v4_CFI_isOther
bool isOther(const JavaObjectHandle<CFI> &cfi) {
    if (!cfi) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_CFI_isOther`. The "
                                       "`cfi` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_CFI_isOther, static_cast<dxfg_v4_cfi_t *>(cfi.get())) != 0;
}

/// dxfg_v4_CFI_decipher
std::vector<void *> decipher(const JavaObjectHandle<CFI> &cfi) {
    if (!cfi) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_CFI_decipher`. The "
                                       "`cfi` handle is invalid");
    }

    auto *values = runGraalV4FunctionWithResult(dxfg_v4_CFI_decipher, static_cast<dxfg_v4_cfi_t *>(cfi.get()));

    if (values == nullptr) {
        return {};
    }

    DXFCPP_FINALLY([&] {
        runGraalV4Function(dxfg_v4_cfi_value_list_release, values);
    });

    return cloneValues(values);
}

/// dxfg_v4_CFI_describe
std::string describe(const JavaObjectHandle<CFI> &cfi) {
    if (!cfi) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_CFI_describe`. The "
                                       "`cfi` handle is invalid");
    }

    auto uDescription = internal::IsolatedString::toUnique(
        runGraalV4FunctionWithResult(dxfg_v4_CFI_describe, static_cast<dxfg_v4_cfi_t *>(cfi.get())));
    auto result = dxfcpp::toString(uDescription.get());

    return result;
}

namespace IsolatedAttribute {
/// dxfg_v4_CFI_Attribute_getName
std::string getName(const JavaObjectHandle<CFI::Attribute> &cfiAttribute) {
    if (!cfiAttribute) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_CFI_Attribute_getName`. The "
                                       "`cfiAttribute` handle is invalid");
    }

    auto uName = internal::IsolatedString::toUnique(runGraalV4FunctionWithResult(
        dxfg_v4_CFI_Attribute_getName, static_cast<dxfg_v4_cfi_attribute_t *>(cfiAttribute.get())));
    auto result = dxfcpp::toString(uName.get());

    return result;
}

/// dxfg_v4_CFI_Attribute_getDescription
std::string getDescription(const JavaObjectHandle<CFI::Attribute> &cfiAttribute) {
    if (!cfiAttribute) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_CFI_Attribute_getDescription`. The "
                                       "`cfiAttribute` handle is invalid");
    }

    auto uDescription = internal::IsolatedString::toUnique(runGraalV4FunctionWithResult(
        dxfg_v4_CFI_Attribute_getDescription, static_cast<dxfg_v4_cfi_attribute_t *>(cfiAttribute.get())));
    auto result = dxfcpp::toString(uDescription.get());

    return result;
}

/// dxfg_v4_CFI_Attribute_getValues
std::vector<void *> getValues(const JavaObjectHandle<CFI::Attribute> &cfiAttribute) {
    if (!cfiAttribute) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_CFI_Attribute_getValues`. The "
                                       "`cfiAttribute` handle is invalid");
    }

    auto *values = runGraalV4FunctionWithResult(dxfg_v4_CFI_Attribute_getValues,
                                                static_cast<dxfg_v4_cfi_attribute_t *>(cfiAttribute.get()));

    if (values == nullptr) {
        return {};
    }

    DXFCPP_FINALLY([&] {
        runGraalV4Function(dxfg_v4_cfi_value_list_release, values);
    });

    return cloneValues(values);
}
} // namespace IsolatedAttribute

namespace IsolatedValue {
/// dxfg_v4_CFI_Value_getAttribute
JavaObjectHandle<CFI::Attribute> getAttribute(const JavaObjectHandle<CFI::Value> &cfiValue) {
    if (!cfiValue) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_CFI_Value_getAttribute`. The "
                                       "`cfiValue` handle is invalid");
    }

    return JavaObjectHandle<CFI::Attribute>{runGraalV4FunctionWithResult(
        dxfg_v4_CFI_Value_getAttribute, static_cast<dxfg_v4_cfi_value_t *>(cfiValue.get()))};
}

/// dxfg_v4_CFI_Value_getCode
std::int16_t getCode(const JavaObjectHandle<CFI::Value> &cfiValue) {
    if (!cfiValue) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_CFI_Value_getCode`. The "
                                       "`cfiValue` handle is invalid");
    }

    return runGraalV4FunctionWithResult(dxfg_v4_CFI_Value_getCode, static_cast<dxfg_v4_cfi_value_t *>(cfiValue.get()));
}

/// dxfg_v4_CFI_Value_getName
std::string getName(const JavaObjectHandle<CFI::Value> &cfiValue) {
    if (!cfiValue) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_CFI_Value_getName`. The "
                                       "`cfiValue` handle is invalid");
    }

    auto uName = internal::IsolatedString::toUnique(
        runGraalV4FunctionWithResult(dxfg_v4_CFI_Value_getName, static_cast<dxfg_v4_cfi_value_t *>(cfiValue.get())));
    auto result = dxfcpp::toString(uName.get());

    return result;
}

/// dxfg_v4_CFI_Value_getDescription
std::string getDescription(const JavaObjectHandle<CFI::Value> &cfiValue) {
    if (!cfiValue) {
        throw InvalidArgumentException("Unable to execute function `dxfg_v4_CFI_Value_getDescription`. The "
                                       "`cfiValue` handle is invalid");
    }

    auto uDescription = internal::IsolatedString::toUnique(runGraalV4FunctionWithResult(
        dxfg_v4_CFI_Value_getDescription, static_cast<dxfg_v4_cfi_value_t *>(cfiValue.get())));
    auto result = dxfcpp::toString(uDescription.get());

    return result;
}
} // namespace IsolatedValue

} // namespace isolated::glossary::IsolatedCFI

DXFCPP_END_NAMESPACE

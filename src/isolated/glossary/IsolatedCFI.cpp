// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>
#include <dxfeed_graal_cpp_api/isolated/glossary/IsolatedCFI.hpp>
#include <dxfeed_graal_cpp_api/isolated/internal/IsolatedString.hpp>
#include <dxfeed_graal_cpp_api/exceptions/InvalidArgumentException.hpp>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::glossary::IsolatedCFI {

// int32_t dxfg_CFI_EMPTY(graal_isolatethread_t *thread, DXFG_OUT dxfg_cfi_t **emptyCfi);
JavaObjectHandle<CFI> EMPTY() {
    dxfg_cfi_t *isolatedCfi{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_CFI_EMPTY, &isolatedCfi);

    return JavaObjectHandle<CFI>{isolatedCfi};
}

// int32_t dxfg_CFI_valueOf(graal_isolatethread_t *thread, const char *code, DXFG_OUT dxfg_cfi_t **cfi);
JavaObjectHandle<CFI> valueOf(const StringLike &code) {
    dxfg_cfi_t *isolatedCfi{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_CFI_valueOf, code.c_str(), &isolatedCfi);

    return JavaObjectHandle<CFI>{isolatedCfi};
}

// int32_t dxfg_CFI_valueOf2(graal_isolatethread_t *thread, int32_t intCode, DXFG_OUT dxfg_cfi_t **cfi);
JavaObjectHandle<CFI> valueOf(std::int32_t intCode) {
    dxfg_cfi_t *isolatedCfi{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_CFI_valueOf2, intCode, &isolatedCfi);

    return JavaObjectHandle<CFI>{isolatedCfi};
}

// int32_t dxfg_CFI_getCode(graal_isolatethread_t *thread, dxfg_cfi_t *cfi, DXFG_OUT char **code);
std::string getCode(const JavaObjectHandle<CFI> &cfi) {
    if (!cfi) {
        throw InvalidArgumentException("Unable to execute function `dxfg_CFI_getCode`. The "
                                       "`cfi` handle is invalid");
    }

    char *code{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_CFI_getCode, static_cast<dxfg_cfi_t *>(cfi.get()), &code);

    auto uCode = internal::IsolatedString::toUnique(code);
    auto result = dxfcpp::toString(uCode.get());

    return result;
}

// int32_t dxfg_CFI_getIntCode(graal_isolatethread_t *thread, dxfg_cfi_t *cfi, DXFG_OUT int32_t *intCode);
std::int32_t getIntCode(const JavaObjectHandle<CFI> &cfi) {
    if (!cfi) {
        throw InvalidArgumentException("Unable to execute function `dxfg_CFI_getIntCode`. The "
                                       "`cfi` handle is invalid");
    }

    std::int32_t intCode{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_CFI_getIntCode, static_cast<dxfg_cfi_t *>(cfi.get()), &intCode);

    return intCode;
}

// int32_t dxfg_CFI_getCategory(graal_isolatethread_t *thread, dxfg_cfi_t *cfi, DXFG_OUT int16_t *category);
std::int16_t getCategory(const JavaObjectHandle<CFI> &cfi) {
    if (!cfi) {
        throw InvalidArgumentException("Unable to execute function `dxfg_CFI_getCategory`. The "
                                       "`cfi` handle is invalid");
    }

    std::int16_t category{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_CFI_getCategory, static_cast<dxfg_cfi_t *>(cfi.get()), &category);

    return category;
}

// int32_t dxfg_CFI_getGroup(graal_isolatethread_t *thread, dxfg_cfi_t *cfi, DXFG_OUT int16_t *group);
std::int16_t getGroup(const JavaObjectHandle<CFI> &cfi) {
    if (!cfi) {
        throw InvalidArgumentException("Unable to execute function `dxfg_CFI_getGroup`. The "
                                       "`cfi` handle is invalid");
    }

    std::int16_t group{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_CFI_getGroup, static_cast<dxfg_cfi_t *>(cfi.get()), &group);

    return group;
}

// int32_t dxfg_CFI_isEquity(graal_isolatethread_t *thread, dxfg_cfi_t *cfi, DXFG_OUT int32_t *isEquity);
bool isEquity(const JavaObjectHandle<CFI> &cfi) {
    if (!cfi) {
        throw InvalidArgumentException("Unable to execute function `dxfg_CFI_isEquity`. The "
                                       "`cfi` handle is invalid");
    }

    std::int32_t result{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_CFI_isEquity, static_cast<dxfg_cfi_t *>(cfi.get()), &result);

    return result != 0;
}

// int32_t dxfg_CFI_isDebtInstrument(graal_isolatethread_t *thread, dxfg_cfi_t *cfi, DXFG_OUT int32_t
// *isDebtInstrument);
bool isDebtInstrument(const JavaObjectHandle<CFI> &cfi) {
    if (!cfi) {
        throw InvalidArgumentException("Unable to execute function `dxfg_CFI_isDebtInstrument`. The "
                                       "`cfi` handle is invalid");
    }

    std::int32_t result{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_CFI_isDebtInstrument, static_cast<dxfg_cfi_t *>(cfi.get()), &result);

    return result != 0;
}

// int32_t dxfg_CFI_isEntitlement(graal_isolatethread_t *thread, dxfg_cfi_t *cfi, DXFG_OUT int32_t *isEntitlement);
bool isEntitlement(const JavaObjectHandle<CFI> &cfi) {
    if (!cfi) {
        throw InvalidArgumentException("Unable to execute function `dxfg_CFI_isEntitlement`. The "
                                       "`cfi` handle is invalid");
    }

    std::int32_t result{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_CFI_isEntitlement, static_cast<dxfg_cfi_t *>(cfi.get()), &result);

    return result != 0;
}

// int32_t dxfg_CFI_isOption(graal_isolatethread_t *thread, dxfg_cfi_t *cfi, DXFG_OUT int32_t *isOption);
bool isOption(const JavaObjectHandle<CFI> &cfi) {
    if (!cfi) {
        throw InvalidArgumentException("Unable to execute function `dxfg_CFI_isOption`. The "
                                       "`cfi` handle is invalid");
    }

    std::int32_t result{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_CFI_isOption, static_cast<dxfg_cfi_t *>(cfi.get()), &result);

    return result != 0;
}

// int32_t dxfg_CFI_isFuture(graal_isolatethread_t *thread, dxfg_cfi_t *cfi, DXFG_OUT int32_t *isFuture);
bool isFuture(const JavaObjectHandle<CFI> &cfi) {
    if (!cfi) {
        throw InvalidArgumentException("Unable to execute function `dxfg_CFI_isFuture`. The "
                                       "`cfi` handle is invalid");
    }

    std::int32_t result{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_CFI_isFuture, static_cast<dxfg_cfi_t *>(cfi.get()), &result);

    return result != 0;
}

// int32_t dxfg_CFI_isOther(graal_isolatethread_t *thread, dxfg_cfi_t *cfi, DXFG_OUT int32_t *isOther);
bool isOther(const JavaObjectHandle<CFI> &cfi) {
    if (!cfi) {
        throw InvalidArgumentException("Unable to execute function `dxfg_CFI_isOther`. The "
                                       "`cfi` handle is invalid");
    }

    std::int32_t result{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_CFI_isOther, static_cast<dxfg_cfi_t *>(cfi.get()), &result);

    return result != 0;
}

// int32_t dxfg_CFI_decipher(graal_isolatethread_t *thread, dxfg_cfi_t *cfi, DXFG_OUT dxfg_cfi_value_t **values,
// DXFG_OUT int32_t *size);
std::vector<void *> decipher(const JavaObjectHandle<CFI> &cfi) {
    if (!cfi) {
        throw InvalidArgumentException("Unable to execute function `dxfg_CFI_decipher`. The "
                                       "`cfi` handle is invalid");
    }

    dxfg_cfi_value_t *values{};
    std::int32_t size{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_CFI_decipher, static_cast<dxfg_cfi_t *>(cfi.get()), &values, &size);

    if (values == nullptr) {
        return {};
    }

    DXFCPP_FINALLY([&] {
        runGraalFunctionAndThrowIfMinusOne(dxfg_JavaObjectHandler_array_release,
                                           dxfcpp::bit_cast<const dxfg_java_object_handler **>(values), size);
    });

    std::vector<void *> result{};
    result.reserve(size);

    try {
        for (std::int32_t i = 0; i < size; i++) {
            dxfg_java_object_handler *clone{};

            runGraalFunctionAndThrowIfMinusOne(dxfg_JavaObjectHandler_clone, &values[i].handler, &clone);
            result.emplace_back(clone);
        }
    } catch (...) {
        for (auto &value : result) {
            runGraalFunctionAndThrowIfMinusOne(dxfg_JavaObjectHandler_release,
                                               dxfcpp::bit_cast<dxfg_java_object_handler *>(value));
        }

        throw;
    }

    return result;
}

// int32_t dxfg_CFI_describe(graal_isolatethread_t *thread, dxfg_cfi_t *cfi, DXFG_OUT char **description);
std::string describe(const JavaObjectHandle<CFI> &cfi) {
    if (!cfi) {
        throw InvalidArgumentException("Unable to execute function `dxfg_CFI_describe`. The "
                                       "`cfi` handle is invalid");
    }

    char *description{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_CFI_describe, static_cast<dxfg_cfi_t *>(cfi.get()), &description);

    auto uDescription = internal::IsolatedString::toUnique(description);
    auto result = dxfcpp::toString(uDescription.get());

    return result;
}

namespace IsolatedAttribute {
// int32_t dxfg_CFI_Attribute_getName(graal_isolatethread_t *thread, dxfg_cfi_attribute_t *cfiAttribute, DXFG_OUT char
// **name);
std::string getName(const JavaObjectHandle<CFI::Attribute> &cfiAttribute) {
    if (!cfiAttribute) {
        throw InvalidArgumentException("Unable to execute function `dxfg_CFI_Attribute_getName`. The "
                                       "`cfiAttribute` handle is invalid");
    }

    char *name{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_CFI_Attribute_getName,
                                       static_cast<dxfg_cfi_attribute_t *>(cfiAttribute.get()), &name);

    auto uName = internal::IsolatedString::toUnique(name);
    auto result = dxfcpp::toString(uName.get());

    return result;
}

// int32_t dxfg_CFI_Attribute_getDescription(graal_isolatethread_t *thread, dxfg_cfi_attribute_t *cfiAttribute, DXFG_OUT
// char **description);
std::string getDescription(const JavaObjectHandle<CFI::Attribute> &cfiAttribute) {
    if (!cfiAttribute) {
        throw InvalidArgumentException("Unable to execute function `dxfg_CFI_Attribute_getDescription`. The "
                                       "`cfiAttribute` handle is invalid");
    }

    char *description{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_CFI_Attribute_getDescription,
                                       static_cast<dxfg_cfi_attribute_t *>(cfiAttribute.get()), &description);

    auto uDescription = internal::IsolatedString::toUnique(description);
    auto result = dxfcpp::toString(uDescription.get());

    return result;
}

// int32_t dxfg_CFI_Attribute_getValues(graal_isolatethread_t *thread, dxfg_cfi_attribute_t *cfiAttribute, DXFG_OUT
// dxfg_cfi_value_t **values, DXFG_OUT int32_t *size);
std::vector<void *> getValues(const JavaObjectHandle<CFI::Attribute> &cfiAttribute) {
    if (!cfiAttribute) {
        throw InvalidArgumentException("Unable to execute function `dxfg_CFI_Attribute_getValues`. The "
                                       "`cfiAttribute` handle is invalid");
    }

    dxfg_cfi_value_t *values{};
    std::int32_t size{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_CFI_Attribute_getValues,
                                       static_cast<dxfg_cfi_attribute_t *>(cfiAttribute.get()), &values, &size);

    if (values == nullptr) {
        return {};
    }

    DXFCPP_FINALLY([&] {
        runGraalFunctionAndThrowIfMinusOne(dxfg_JavaObjectHandler_array_release,
                                           dxfcpp::bit_cast<const dxfg_java_object_handler **>(values), size);
    });

    std::vector<void *> result{};
    result.reserve(size);

    try {
        for (std::int32_t i = 0; i < size; i++) {
            dxfg_java_object_handler *clone{};

            runGraalFunctionAndThrowIfMinusOne(dxfg_JavaObjectHandler_clone, &values[i].handler, &clone);
            result.emplace_back(clone);
        }
    } catch (...) {
        for (auto &value : result) {
            runGraalFunctionAndThrowIfMinusOne(dxfg_JavaObjectHandler_release,
                                               dxfcpp::bit_cast<dxfg_java_object_handler *>(value));
        }

        throw;
    }

    return result;
}
} // namespace IsolatedAttribute

namespace IsolatedValue {
// int32_t dxfg_CFI_Value_getAttribute(graal_isolatethread_t *thread, dxfg_cfi_value_t *cfiValue, DXFG_OUT
// dxfg_cfi_attribute_t **attribute);
JavaObjectHandle<CFI::Attribute> getAttribute(const JavaObjectHandle<CFI::Value> &cfiValue) {
    if (!cfiValue) {
        throw InvalidArgumentException("Unable to execute function `dxfg_CFI_Value_getAttribute`. The "
                                       "`cfiValue` handle is invalid");
    }

    dxfg_cfi_attribute_t *attribute{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_CFI_Value_getAttribute, static_cast<dxfg_cfi_value_t *>(cfiValue.get()),
                                       &attribute);

    return JavaObjectHandle<CFI::Attribute>{attribute};
}

// int32_t dxfg_CFI_Value_getCode(graal_isolatethread_t *thread, dxfg_cfi_value_t *cfiValue, DXFG_OUT int16_t *code);
std::int16_t getCode(const JavaObjectHandle<CFI::Value> &cfiValue) {
    if (!cfiValue) {
        throw InvalidArgumentException("Unable to execute function `dxfg_CFI_Value_getCode`. The "
                                       "`cfiValue` handle is invalid");
    }

    std::int16_t code{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_CFI_Value_getCode, static_cast<dxfg_cfi_value_t *>(cfiValue.get()), &code);

    return code;
}

// int32_t dxfg_CFI_Value_getName(graal_isolatethread_t *thread, dxfg_cfi_value_t *cfiValue, DXFG_OUT char **name);
std::string getName(const JavaObjectHandle<CFI::Value> &cfiValue) {
    if (!cfiValue) {
        throw InvalidArgumentException("Unable to execute function `dxfg_CFI_Value_getName`. The "
                                       "`cfiValue` handle is invalid");
    }

    char *name{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_CFI_Value_getName, static_cast<dxfg_cfi_value_t *>(cfiValue.get()), &name);

    auto uName = internal::IsolatedString::toUnique(name);
    auto result = dxfcpp::toString(uName.get());

    return result;
}

// int32_t dxfg_CFI_Value_getDescription(graal_isolatethread_t *thread, dxfg_cfi_value_t *cfiValue, DXFG_OUT char
// **description);
std::string getDescription(const JavaObjectHandle<CFI::Value> &cfiValue) {
    if (!cfiValue) {
        throw InvalidArgumentException("Unable to execute function `dxfg_CFI_Value_getDescription`. The "
                                       "`cfiValue` handle is invalid");
    }

    char *description{};

    runGraalFunctionAndThrowIfMinusOne(dxfg_CFI_Value_getDescription, static_cast<dxfg_cfi_value_t *>(cfiValue.get()),
                                       &description);

    auto uDescription = internal::IsolatedString::toUnique(description);
    auto result = dxfcpp::toString(uDescription.get());

    return result;
}
} // namespace IsolatedValue

} // namespace isolated::glossary::IsolatedCFI

DXFCPP_END_NAMESPACE
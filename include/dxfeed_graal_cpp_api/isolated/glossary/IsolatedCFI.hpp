// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include "../../glossary/CFI.hpp"

DXFCPP_BEGIN_NAMESPACE

namespace isolated::glossary::IsolatedCFI {

// int32_t dxfg_CFI_EMPTY(graal_isolatethread_t *thread, DXFG_OUT dxfg_cfi_t **emptyCfi);
JavaObjectHandle<CFI> EMPTY();

// int32_t dxfg_CFI_valueOf(graal_isolatethread_t *thread, const char *code, DXFG_OUT dxfg_cfi_t **cfi);
JavaObjectHandle<CFI> valueOf(const StringLike &code);

// int32_t dxfg_CFI_valueOf2(graal_isolatethread_t *thread, int32_t intCode, DXFG_OUT dxfg_cfi_t **cfi);
JavaObjectHandle<CFI> valueOf(std::int32_t intCode);

// int32_t dxfg_CFI_getCode(graal_isolatethread_t *thread, dxfg_cfi_t *cfi, DXFG_OUT char **code);
std::string getCode(const JavaObjectHandle<CFI> &cfi);

// int32_t dxfg_CFI_getIntCode(graal_isolatethread_t *thread, dxfg_cfi_t *cfi, DXFG_OUT int32_t *intCode);
std::int32_t getIntCode(const JavaObjectHandle<CFI> &cfi);

// int32_t dxfg_CFI_getCategory(graal_isolatethread_t *thread, dxfg_cfi_t *cfi, DXFG_OUT int16_t *category);
std::int16_t getCategory(const JavaObjectHandle<CFI> &cfi);

// int32_t dxfg_CFI_getGroup(graal_isolatethread_t *thread, dxfg_cfi_t *cfi, DXFG_OUT int16_t *group);
std::int16_t getGroup(const JavaObjectHandle<CFI> &cfi);

// int32_t dxfg_CFI_isEquity(graal_isolatethread_t *thread, dxfg_cfi_t *cfi, DXFG_OUT int32_t *isEquity);
bool isEquity(const JavaObjectHandle<CFI> &cfi);

// int32_t dxfg_CFI_isDebtInstrument(graal_isolatethread_t *thread, dxfg_cfi_t *cfi, DXFG_OUT int32_t *isDebtInstrument);
bool isDebtInstrument(const JavaObjectHandle<CFI> &cfi);

// int32_t dxfg_CFI_isEntitlement(graal_isolatethread_t *thread, dxfg_cfi_t *cfi, DXFG_OUT int32_t *isEntitlement);
bool isEntitlement(const JavaObjectHandle<CFI> &cfi);

// int32_t dxfg_CFI_isOption(graal_isolatethread_t *thread, dxfg_cfi_t *cfi, DXFG_OUT int32_t *isOption);
bool isOption(const JavaObjectHandle<CFI> &cfi);

// int32_t dxfg_CFI_isFuture(graal_isolatethread_t *thread, dxfg_cfi_t *cfi, DXFG_OUT int32_t *isFuture);
bool isFuture(const JavaObjectHandle<CFI> &cfi);

// int32_t dxfg_CFI_isOther(graal_isolatethread_t *thread, dxfg_cfi_t *cfi, DXFG_OUT int32_t *isOther);
bool isOther(const JavaObjectHandle<CFI> &cfi);

// int32_t dxfg_CFI_decipher(graal_isolatethread_t *thread, dxfg_cfi_t *cfi, DXFG_OUT dxfg_cfi_value_t **values, DXFG_OUT int32_t *size);
std::vector<void*> decipher(const JavaObjectHandle<CFI> &cfi);

// int32_t dxfg_CFI_describe(graal_isolatethread_t *thread, dxfg_cfi_t *cfi, DXFG_OUT char **description);
std::string describe(const JavaObjectHandle<CFI> &cfi);

namespace IsolatedAttribute {
// int32_t dxfg_CFI_Attribute_getName(graal_isolatethread_t *thread, dxfg_cfi_attribute_t *cfiAttribute, DXFG_OUT char **name);
std::string getName(const JavaObjectHandle<CFI::Attribute> &cfiAttribute);

// int32_t dxfg_CFI_Attribute_getDescription(graal_isolatethread_t *thread, dxfg_cfi_attribute_t *cfiAttribute, DXFG_OUT char **description);
std::string getDescription(const JavaObjectHandle<CFI::Attribute> &cfiAttribute);

// int32_t dxfg_CFI_Attribute_getValues(graal_isolatethread_t *thread, dxfg_cfi_attribute_t *cfiAttribute, DXFG_OUT dxfg_cfi_value_t **values, DXFG_OUT int32_t *size);
std::vector<void*> getValues(const JavaObjectHandle<CFI::Attribute> &cfiAttribute);
}

namespace IsolatedValue {
// int32_t dxfg_CFI_Value_getAttribute(graal_isolatethread_t *thread, dxfg_cfi_value_t *cfiValue, DXFG_OUT dxfg_cfi_attribute_t **attribute);
JavaObjectHandle<CFI::Attribute> getAttribute(const JavaObjectHandle<CFI::Value> &cfiValue);

// int32_t dxfg_CFI_Value_getCode(graal_isolatethread_t *thread, dxfg_cfi_value_t *cfiValue, DXFG_OUT int16_t *code);
std::int16_t getCode(const JavaObjectHandle<CFI::Value> &cfiValue);

// int32_t dxfg_CFI_Value_getName(graal_isolatethread_t *thread, dxfg_cfi_value_t *cfiValue, DXFG_OUT char **name);
std::string getName(const JavaObjectHandle<CFI::Value> &cfiValue);

// int32_t dxfg_CFI_Value_getDescription(graal_isolatethread_t *thread, dxfg_cfi_value_t *cfiValue, DXFG_OUT char **description);
std::string getDescription(const JavaObjectHandle<CFI::Value> &cfiValue);
}

}

DXFCPP_END_NAMESPACE
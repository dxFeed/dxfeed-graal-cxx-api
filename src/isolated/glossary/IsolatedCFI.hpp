// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../../include/dxfeed_graal_cpp_api/detail/Conf.hpp"

#include "../../../include/dxfeed_graal_cpp_api/glossary/CFI.hpp"

DXFCPP_BEGIN_NAMESPACE

namespace isolated::glossary::IsolatedCFI {

/// dxfg_v4_CFI_EMPTY
/* dxfg_v4_cfi_t* */ JavaObjectHandle<CFI> EMPTY();

/// dxfg_v4_CFI_valueOf
/* dxfg_v4_cfi_t* */ JavaObjectHandle<CFI> valueOf(std::string_view code);

/// dxfg_v4_CFI_valueOfIntCode
/* dxfg_v4_cfi_t* */ JavaObjectHandle<CFI> valueOf(std::int32_t intCode);

/// dxfg_v4_CFI_getCode
std::string getCode(/* dxfg_v4_cfi_t* */ const JavaObjectHandle<CFI> &cfi);

/// dxfg_v4_CFI_getIntCode
std::int32_t getIntCode(/* dxfg_v4_cfi_t* */ const JavaObjectHandle<CFI> &cfi);

/// dxfg_v4_CFI_getCategory
std::int16_t getCategory(/* dxfg_v4_cfi_t* */ const JavaObjectHandle<CFI> &cfi);

/// dxfg_v4_CFI_getGroup
std::int16_t getGroup(/* dxfg_v4_cfi_t* */ const JavaObjectHandle<CFI> &cfi);

/// dxfg_v4_CFI_isEquity
bool isEquity(/* dxfg_v4_cfi_t* */ const JavaObjectHandle<CFI> &cfi);

/// dxfg_v4_CFI_isDebtInstrument
bool isDebtInstrument(/* dxfg_v4_cfi_t* */ const JavaObjectHandle<CFI> &cfi);

/// dxfg_v4_CFI_isEntitlement
bool isEntitlement(/* dxfg_v4_cfi_t* */ const JavaObjectHandle<CFI> &cfi);

/// dxfg_v4_CFI_isOption
bool isOption(/* dxfg_v4_cfi_t* */ const JavaObjectHandle<CFI> &cfi);

/// dxfg_v4_CFI_isFuture
bool isFuture(/* dxfg_v4_cfi_t* */ const JavaObjectHandle<CFI> &cfi);

/// dxfg_v4_CFI_isOther
bool isOther(/* dxfg_v4_cfi_t* */ const JavaObjectHandle<CFI> &cfi);

/// dxfg_v4_CFI_decipher
std::vector<void *> decipher(/* dxfg_v4_cfi_t* */ const JavaObjectHandle<CFI> &cfi);

/// dxfg_v4_CFI_describe
std::string describe(/* dxfg_v4_cfi_t* */ const JavaObjectHandle<CFI> &cfi);

namespace IsolatedAttribute {
/// dxfg_v4_CFI_Attribute_getName
std::string getName(/* dxfg_v4_cfi_attribute_t* */ const JavaObjectHandle<CFI::Attribute> &cfiAttribute);

/// dxfg_v4_CFI_Attribute_getDescription
std::string getDescription(/* dxfg_v4_cfi_attribute_t* */ const JavaObjectHandle<CFI::Attribute> &cfiAttribute);

/// dxfg_v4_CFI_Attribute_getValues
std::vector<void *> getValues(/* dxfg_v4_cfi_attribute_t* */ const JavaObjectHandle<CFI::Attribute> &cfiAttribute);
} // namespace IsolatedAttribute

namespace IsolatedValue {
/// dxfg_v4_CFI_Value_getAttribute
/* dxfg_v4_cfi_attribute_t* */ JavaObjectHandle<CFI::Attribute>
getAttribute(/* dxfg_v4_cfi_value_t* */ const JavaObjectHandle<CFI::Value> &cfiValue);

/// dxfg_v4_CFI_Value_getCode
std::int16_t getCode(/* dxfg_v4_cfi_value_t* */ const JavaObjectHandle<CFI::Value> &cfiValue);

/// dxfg_v4_CFI_Value_getName
std::string getName(/* dxfg_v4_cfi_value_t* */ const JavaObjectHandle<CFI::Value> &cfiValue);

/// dxfg_v4_CFI_Value_getDescription
std::string getDescription(/* dxfg_v4_cfi_value_t* */ const JavaObjectHandle<CFI::Value> &cfiValue);
} // namespace IsolatedValue

} // namespace isolated::glossary::IsolatedCFI

DXFCPP_END_NAMESPACE

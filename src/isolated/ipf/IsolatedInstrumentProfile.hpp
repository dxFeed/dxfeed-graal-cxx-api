// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../../include/dxfeed_graal_cpp_api/detail/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../../../include/dxfeed_graal_cpp_api/detail/JavaObjectHandle.hpp"
#include "../../../include/dxfeed_graal_cpp_api/ipf/InstrumentProfile.hpp"

#include <cstdint>
#include <string>
#include <vector>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::ipf::IsolatedInstrumentProfile {

// dxfg_v4_InstrumentProfile_new
/* dxfg_v4_instrument_profile_t* */ JavaObjectHandle<InstrumentProfile> create();

// dxfg_v4_InstrumentProfile_newCopy
/* dxfg_v4_instrument_profile_t* */ JavaObjectHandle<InstrumentProfile>
create(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile);

// dxfg_v4_InstrumentProfile_getType
/* const char* */ std::string
getType(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile);

// dxfg_v4_InstrumentProfile_setType
/* std::int32_t */ void
setType(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile,
        std::string_view value);

// dxfg_v4_InstrumentProfile_getSymbol
/* const char* */ std::string
getSymbol(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile);

// dxfg_v4_InstrumentProfile_setSymbol
/* std::int32_t */ void
setSymbol(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile,
          std::string_view value);

// dxfg_v4_InstrumentProfile_getDescription
/* const char* */ std::string
getDescription(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile);

// dxfg_v4_InstrumentProfile_setDescription
/* std::int32_t */ void
setDescription(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile,
               std::string_view value);

// dxfg_v4_InstrumentProfile_getLocalSymbol
/* const char* */ std::string
getLocalSymbol(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile);

// dxfg_v4_InstrumentProfile_setLocalSymbol
/* std::int32_t */ void
setLocalSymbol(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile,
               std::string_view value);

// dxfg_v4_InstrumentProfile_getLocalDescription
/* const char* */ std::string
getLocalDescription(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile);

// dxfg_v4_InstrumentProfile_setLocalDescription
/* std::int32_t */ void
setLocalDescription(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile,
                    std::string_view value);

// dxfg_v4_InstrumentProfile_getCountry
/* const char* */ std::string
getCountry(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile);

// dxfg_v4_InstrumentProfile_setCountry
/* std::int32_t */ void
setCountry(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile,
           std::string_view value);

// dxfg_v4_InstrumentProfile_getOPOL
/* const char* */ std::string
getOPOL(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile);

// dxfg_v4_InstrumentProfile_setOPOL
/* std::int32_t */ void
setOPOL(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile,
        std::string_view value);

// dxfg_v4_InstrumentProfile_getExchangeData
/* const char* */ std::string
getExchangeData(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile);

// dxfg_v4_InstrumentProfile_setExchangeData
/* std::int32_t */ void
setExchangeData(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile,
                std::string_view value);

// dxfg_v4_InstrumentProfile_getExchanges
/* const char* */ std::string
getExchanges(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile);

// dxfg_v4_InstrumentProfile_setExchanges
/* std::int32_t */ void
setExchanges(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile,
             std::string_view value);

// dxfg_v4_InstrumentProfile_getCurrency
/* const char* */ std::string
getCurrency(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile);

// dxfg_v4_InstrumentProfile_setCurrency
/* std::int32_t */ void
setCurrency(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile,
            std::string_view value);

// dxfg_v4_InstrumentProfile_getBaseCurrency
/* const char* */ std::string
getBaseCurrency(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile);

// dxfg_v4_InstrumentProfile_setBaseCurrency
/* std::int32_t */ void
setBaseCurrency(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile,
                std::string_view value);

// dxfg_v4_InstrumentProfile_getCFI
/* const char* */ std::string
getCFI(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile);

// dxfg_v4_InstrumentProfile_setCFI
/* std::int32_t */ void
setCFI(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile,
       std::string_view value);

// dxfg_v4_InstrumentProfile_getISIN
/* const char* */ std::string
getISIN(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile);

// dxfg_v4_InstrumentProfile_setISIN
/* std::int32_t */ void
setISIN(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile,
        std::string_view value);

// dxfg_v4_InstrumentProfile_getSEDOL
/* const char* */ std::string
getSEDOL(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile);

// dxfg_v4_InstrumentProfile_setSEDOL
/* std::int32_t */ void
setSEDOL(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile,
         std::string_view value);

// dxfg_v4_InstrumentProfile_getCUSIP
/* const char* */ std::string
getCUSIP(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile);

// dxfg_v4_InstrumentProfile_setCUSIP
/* std::int32_t */ void
setCUSIP(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile,
         std::string_view value);

// dxfg_v4_InstrumentProfile_getICB
std::int32_t getICB(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile);

// dxfg_v4_InstrumentProfile_setICB
/* std::int32_t */ void
setICB(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile,
       std::int32_t value);

// dxfg_v4_InstrumentProfile_getSIC
std::int32_t getSIC(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile);

// dxfg_v4_InstrumentProfile_setSIC
/* std::int32_t */ void
setSIC(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile,
       std::int32_t value);

// dxfg_v4_InstrumentProfile_getMultiplier
double getMultiplier(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile);

// dxfg_v4_InstrumentProfile_setMultiplier
/* std::int32_t */ void
setMultiplier(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile,
              double value);

// dxfg_v4_InstrumentProfile_getProduct
/* const char* */ std::string
getProduct(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile);

// dxfg_v4_InstrumentProfile_setProduct
/* std::int32_t */ void
setProduct(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile,
           std::string_view value);

// dxfg_v4_InstrumentProfile_getUnderlying
/* const char* */ std::string
getUnderlying(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile);

// dxfg_v4_InstrumentProfile_setUnderlying
/* std::int32_t */ void
setUnderlying(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile,
              std::string_view value);

// dxfg_v4_InstrumentProfile_getSPC
double getSPC(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile);

// dxfg_v4_InstrumentProfile_setSPC
/* std::int32_t */ void
setSPC(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile, double value);

// dxfg_v4_InstrumentProfile_getAdditionalUnderlyings
/* const char* */ std::string getAdditionalUnderlyings(
    /* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile);

// dxfg_v4_InstrumentProfile_setAdditionalUnderlyings
/* std::int32_t */ void setAdditionalUnderlyings(
    /* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile,
    std::string_view value);

// dxfg_v4_InstrumentProfile_getMMY
/* const char* */ std::string
getMMY(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile);

// dxfg_v4_InstrumentProfile_setMMY
/* std::int32_t */ void
setMMY(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile,
       std::string_view value);

// dxfg_v4_InstrumentProfile_getExpiration
std::int32_t
getExpiration(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile);

// dxfg_v4_InstrumentProfile_setExpiration
/* std::int32_t */ void
setExpiration(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile,
              std::int32_t value);

// dxfg_v4_InstrumentProfile_getLastTrade
std::int32_t
getLastTrade(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile);

// dxfg_v4_InstrumentProfile_setLastTrade
/* std::int32_t */ void
setLastTrade(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile,
             std::int32_t value);

// dxfg_v4_InstrumentProfile_getStrike
double getStrike(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile);

// dxfg_v4_InstrumentProfile_setStrike
/* std::int32_t */ void
setStrike(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile,
          double value);

// dxfg_v4_InstrumentProfile_getOptionType
/* const char* */ std::string
getOptionType(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile);

// dxfg_v4_InstrumentProfile_setOptionType
/* std::int32_t */ void
setOptionType(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile,
              std::string_view value);

// dxfg_v4_InstrumentProfile_getExpirationStyle
/* const char* */ std::string
getExpirationStyle(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile);

// dxfg_v4_InstrumentProfile_setExpirationStyle
/* std::int32_t */ void
setExpirationStyle(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile,
                   std::string_view value);

// dxfg_v4_InstrumentProfile_getSettlementStyle
/* const char* */ std::string
getSettlementStyle(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile);

// dxfg_v4_InstrumentProfile_setSettlementStyle
/* std::int32_t */ void
setSettlementStyle(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile,
                   std::string_view value);

// dxfg_v4_InstrumentProfile_getPriceIncrements
/* const char* */ std::string
getPriceIncrements(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile);

// dxfg_v4_InstrumentProfile_setPriceIncrements
/* std::int32_t */ void
setPriceIncrements(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile,
                   std::string_view value);

// dxfg_v4_InstrumentProfile_getTradingHours
/* const char* */ std::string
getTradingHours(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile);

// dxfg_v4_InstrumentProfile_setTradingHours
/* std::int32_t */ void
setTradingHours(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile,
                std::string_view value);

// dxfg_v4_InstrumentProfile_getField
/* const char* */ std::string
getField(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile,
         std::string_view name);

// dxfg_v4_InstrumentProfile_setField
/* std::int32_t */ void
setField(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile,
         std::string_view name, std::string_view value);

// dxfg_v4_InstrumentProfile_getNumericField
double getNumericField(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile,
                       std::string_view name);

// dxfg_v4_InstrumentProfile_setNumericField
/* std::int32_t */ void
setNumericField(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile,
                std::string_view name, double value);

// dxfg_v4_InstrumentProfile_getDateField
std::int32_t
getDateField(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile,
             std::string_view name);

// dxfg_v4_InstrumentProfile_setDateField
/* std::int32_t */ void
setDateField(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile,
             std::string_view name, std::int32_t value);

// dxfg_v4_InstrumentProfile_getNonEmptyCustomFieldNames
/* dxfg_v4_string_list* */ std::vector<std::string> getNonEmptyCustomFieldNames(
    /* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &instrumentProfile);

// dxfg_v4_InstrumentProfile_release
// /* std::int32_t */ void release(/* dxfg_v4_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile>&
// instrumentProfile);

} // namespace isolated::ipf::IsolatedInstrumentProfile

namespace isolated::ipf::IsolatedInstrumentProfileList {
void release(/* dxfg_v4_instrument_profile_list * */ void *list);
void releaseWrapper(/* dxfg_v4_instrument_profile_list * */ void *list);

std::unique_ptr<void, decltype(&releaseWrapper)> toUniqueWrapper(/* dxfg_v4_instrument_profile_list * */ void *list);
} // namespace isolated::ipf::IsolatedInstrumentProfileList

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
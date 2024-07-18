// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include "../../ipf/InstrumentProfile.hpp"

#include <cstdint>
#include <string>
#include <vector>

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

DXFCPP_BEGIN_NAMESPACE

namespace isolated::ipf::IsolatedInstrumentProfile {

// dxfg_InstrumentProfile_new
/* dxfg_instrument_profile_t* */ JavaObjectHandle<InstrumentProfile> create();

// dxfg_InstrumentProfile_new2
/* dxfg_instrument_profile_t* */ JavaObjectHandle<InstrumentProfile>
create(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip);

// dxfg_InstrumentProfile_getType
/* const char* */ std::string getType(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip);

// dxfg_InstrumentProfile_setType
/* std::int32_t */ void setType(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                const StringLikeWrapper &value);

// dxfg_InstrumentProfile_getSymbol
/* const char* */ std::string getSymbol(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip);

// dxfg_InstrumentProfile_setSymbol
/* std::int32_t */ void setSymbol(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                  const StringLikeWrapper &value);

// dxfg_InstrumentProfile_getDescription
/* const char* */ std::string
getDescription(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip);

// dxfg_InstrumentProfile_setDescription
/* std::int32_t */ void setDescription(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                       const StringLikeWrapper &value);

// dxfg_InstrumentProfile_getLocalSymbol
/* const char* */ std::string
getLocalSymbol(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip);

// dxfg_InstrumentProfile_setLocalSymbol
/* std::int32_t */ void setLocalSymbol(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                       const StringLikeWrapper &value);

// dxfg_InstrumentProfile_getLocalDescription
/* const char* */ std::string
getLocalDescription(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip);

// dxfg_InstrumentProfile_setLocalDescription
/* std::int32_t */ void
setLocalDescription(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                    const StringLikeWrapper &value);

// dxfg_InstrumentProfile_getCountry
/* const char* */ std::string
getCountry(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip);

// dxfg_InstrumentProfile_setCountry
/* std::int32_t */ void setCountry(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                   const StringLikeWrapper &value);

// dxfg_InstrumentProfile_getOPOL
/* const char* */ std::string getOPOL(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip);

// dxfg_InstrumentProfile_setOPOL
/* std::int32_t */ void setOPOL(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                const StringLikeWrapper &value);

// dxfg_InstrumentProfile_getExchangeData
/* const char* */ std::string
getExchangeData(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip);

// dxfg_InstrumentProfile_setExchangeData
/* std::int32_t */ void setExchangeData(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                        const StringLikeWrapper &value);

// dxfg_InstrumentProfile_getExchanges
/* const char* */ std::string
getExchanges(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip);

// dxfg_InstrumentProfile_setExchanges
/* std::int32_t */ void setExchanges(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                     const StringLikeWrapper &value);

// dxfg_InstrumentProfile_getCurrency
/* const char* */ std::string
getCurrency(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip);

// dxfg_InstrumentProfile_setCurrency
/* std::int32_t */ void setCurrency(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                    const StringLikeWrapper &value);

// dxfg_InstrumentProfile_getBaseCurrency
/* const char* */ std::string
getBaseCurrency(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip);

// dxfg_InstrumentProfile_setBaseCurrency
/* std::int32_t */ void setBaseCurrency(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                        const StringLikeWrapper &value);

// dxfg_InstrumentProfile_getCFI
/* const char* */ std::string getCFI(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip);

// dxfg_InstrumentProfile_setCFI
/* std::int32_t */ void setCFI(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                               const StringLikeWrapper &value);

// dxfg_InstrumentProfile_getISIN
/* const char* */ std::string getISIN(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip);

// dxfg_InstrumentProfile_setISIN
/* std::int32_t */ void setISIN(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                const StringLikeWrapper &value);

// dxfg_InstrumentProfile_getSEDOL
/* const char* */ std::string getSEDOL(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip);

// dxfg_InstrumentProfile_setSEDOL
/* std::int32_t */ void setSEDOL(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                 const StringLikeWrapper &value);

// dxfg_InstrumentProfile_getCUSIP
/* const char* */ std::string getCUSIP(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip);

// dxfg_InstrumentProfile_setCUSIP
/* std::int32_t */ void setCUSIP(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                 const StringLikeWrapper &value);

// dxfg_InstrumentProfile_getICB
std::int32_t getICB(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip);

// dxfg_InstrumentProfile_setICB
/* std::int32_t */ void setICB(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                               std::int32_t value);

// dxfg_InstrumentProfile_getSIC
std::int32_t getSIC(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip);

// dxfg_InstrumentProfile_setSIC
/* std::int32_t */ void setSIC(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                               std::int32_t value);

// dxfg_InstrumentProfile_getMultiplier
double getMultiplier(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip);

// dxfg_InstrumentProfile_setMultiplier
/* std::int32_t */ void setMultiplier(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                      double value);

// dxfg_InstrumentProfile_getProduct
/* const char* */ std::string
getProduct(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip);

// dxfg_InstrumentProfile_setProduct
/* std::int32_t */ void setProduct(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                   const StringLikeWrapper &value);

// dxfg_InstrumentProfile_getUnderlying
/* const char* */ std::string
getUnderlying(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip);

// dxfg_InstrumentProfile_setUnderlying
/* std::int32_t */ void setUnderlying(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                      const StringLikeWrapper &value);

// dxfg_InstrumentProfile_getSPC
double getSPC(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip);

// dxfg_InstrumentProfile_setSPC
/* std::int32_t */ void setSPC(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                               double value);

// dxfg_InstrumentProfile_getAdditionalUnderlyings
/* const char* */ std::string
getAdditionalUnderlyings(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip);

// dxfg_InstrumentProfile_setAdditionalUnderlyings
/* std::int32_t */ void
setAdditionalUnderlyings(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                         const StringLikeWrapper &value);

// dxfg_InstrumentProfile_getMMY
/* const char* */ std::string getMMY(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip);

// dxfg_InstrumentProfile_setMMY
/* std::int32_t */ void setMMY(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                               const StringLikeWrapper &value);

// dxfg_InstrumentProfile_getExpiration
std::int32_t getExpiration(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip);

// dxfg_InstrumentProfile_setExpiration
/* std::int32_t */ void setExpiration(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                      std::int32_t value);

// dxfg_InstrumentProfile_getLastTrade
std::int32_t getLastTrade(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip);

// dxfg_InstrumentProfile_setLastTrade
/* std::int32_t */ void setLastTrade(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                     std::int32_t value);

// dxfg_InstrumentProfile_getStrike
double getStrike(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip);

// dxfg_InstrumentProfile_setStrike
/* std::int32_t */ void setStrike(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                  double value);

// dxfg_InstrumentProfile_getOptionType
/* const char* */ std::string
getOptionType(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip);

// dxfg_InstrumentProfile_setOptionType
/* std::int32_t */ void setOptionType(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                      const StringLikeWrapper &value);

// dxfg_InstrumentProfile_getExpirationStyle
/* const char* */ std::string
getExpirationStyle(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip);

// dxfg_InstrumentProfile_setExpirationStyle
/* std::int32_t */ void
setExpirationStyle(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                   const StringLikeWrapper &value);

// dxfg_InstrumentProfile_getSettlementStyle
/* const char* */ std::string
getSettlementStyle(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip);

// dxfg_InstrumentProfile_setSettlementStyle
/* std::int32_t */ void
setSettlementStyle(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                   const StringLikeWrapper &value);

// dxfg_InstrumentProfile_getPriceIncrements
/* const char* */ std::string
getPriceIncrements(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip);

// dxfg_InstrumentProfile_setPriceIncrements
/* std::int32_t */ void
setPriceIncrements(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                   const StringLikeWrapper &value);

// dxfg_InstrumentProfile_getTradingHours
/* const char* */ std::string
getTradingHours(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip);

// dxfg_InstrumentProfile_setTradingHours
/* std::int32_t */ void setTradingHours(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                        const StringLikeWrapper &value);

// dxfg_InstrumentProfile_getField
/* const char* */ std::string getField(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                       const StringLikeWrapper &name);

// dxfg_InstrumentProfile_setField
/* std::int32_t */ void setField(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                 const StringLikeWrapper &name, const StringLikeWrapper &value);

// dxfg_InstrumentProfile_getNumericField
double getNumericField(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                       const StringLikeWrapper &name);

// dxfg_InstrumentProfile_setNumericField
/* std::int32_t */ void setNumericField(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                        const StringLikeWrapper &name, double value);

// dxfg_InstrumentProfile_getDateField
std::int32_t getDateField(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                          const StringLikeWrapper &name);

// dxfg_InstrumentProfile_setDateField
/* std::int32_t */ void setDateField(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                     const StringLikeWrapper &name, std::int32_t value);

// dxfg_InstrumentProfile_getNonEmptyCustomFieldNames
/* dxfg_string_list* */ std::vector<std::string>
getNonEmptyCustomFieldNames(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip);

// dxfg_InstrumentProfile_release
// /* std::int32_t */ void release(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile>& ip);

} // namespace isolated::ipf::IsolatedInstrumentProfile

namespace isolated::ipf::IsolatedInstrumentProfileList {
void release(/* dxfg_instrument_profile_list * */ void *list);
void releaseWrapper(/* dxfg_instrument_profile_list * */ void *list);

std::unique_ptr<void, decltype(&releaseWrapper)> toUniqueWrapper(/* dxfg_instrument_profile_list * */ void *list);
} // namespace isolated::ipf::IsolatedInstrumentProfileList

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../../internal/Conf.hpp"

#include <cstdint>
#include <string>
#include <string_view>

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

DXFCPP_BEGIN_NAMESPACE

namespace isolated::ipf::IsolatedInstrumentProfile {

/*
dxfg_instrument_profile_t*                    dxfg_InstrumentProfile_new(graal_isolatethread_t *thread);
dxfg_instrument_profile_t*                    dxfg_InstrumentProfile_new2(graal_isolatethread_t *thread, dxfg_instrument_profile_t* ip);
const char*                                   dxfg_InstrumentProfile_getType(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip);
int32_t                                       dxfg_InstrumentProfile_setType(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip, const char *value);
const char*                                   dxfg_InstrumentProfile_getSymbol(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip);
int32_t                                       dxfg_InstrumentProfile_setSymbol(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip, const char *value);
const char*                                   dxfg_InstrumentProfile_getDescription(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip);
int32_t                                       dxfg_InstrumentProfile_setDescription(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip, const char *value);
const char*                                   dxfg_InstrumentProfile_getLocalSymbol(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip);
int32_t                                       dxfg_InstrumentProfile_setLocalSymbol(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip, const char *value);
const char*                                   dxfg_InstrumentProfile_getLocalDescription(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip);
int32_t                                       dxfg_InstrumentProfile_setLocalDescription(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip, const char *value);
const char*                                   dxfg_InstrumentProfile_getCountry(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip);
int32_t                                       dxfg_InstrumentProfile_setCountry(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip, const char *value);
const char*                                   dxfg_InstrumentProfile_getOPOL(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip);
int32_t                                       dxfg_InstrumentProfile_setOPOL(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip, const char *value);
const char*                                   dxfg_InstrumentProfile_getExchangeData(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip);
int32_t                                       dxfg_InstrumentProfile_setExchangeData(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip, const char *value);
const char*                                   dxfg_InstrumentProfile_getExchanges(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip);
int32_t                                       dxfg_InstrumentProfile_setExchanges(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip, const char *value);
const char*                                   dxfg_InstrumentProfile_getCurrency(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip);
int32_t                                       dxfg_InstrumentProfile_setCurrency(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip, const char *value);
const char*                                   dxfg_InstrumentProfile_getBaseCurrency(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip);
int32_t                                       dxfg_InstrumentProfile_setBaseCurrency(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip, const char *value);
const char*                                   dxfg_InstrumentProfile_getCFI(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip);
int32_t                                       dxfg_InstrumentProfile_setCFI(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip, const char *value);
const char*                                   dxfg_InstrumentProfile_getISIN(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip);
int32_t                                       dxfg_InstrumentProfile_setISIN(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip, const char *value);
const char*                                   dxfg_InstrumentProfile_getSEDOL(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip);
int32_t                                       dxfg_InstrumentProfile_setSEDOL(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip, const char *value);
const char*                                   dxfg_InstrumentProfile_getCUSIP(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip);
int32_t                                       dxfg_InstrumentProfile_setCUSIP(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip, const char *value);
int32_t                                       dxfg_InstrumentProfile_getICB(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip);
int32_t                                       dxfg_InstrumentProfile_setICB(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip, int32_t value);
int32_t                                       dxfg_InstrumentProfile_getSIC(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip);
int32_t                                       dxfg_InstrumentProfile_setSIC(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip, int32_t value);
double                                        dxfg_InstrumentProfile_getMultiplier(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip);
int32_t                                       dxfg_InstrumentProfile_setMultiplier(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip, double value);
const char*                                   dxfg_InstrumentProfile_getProduct(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip);
int32_t                                       dxfg_InstrumentProfile_setProduct(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip, const char *value);
const char*                                   dxfg_InstrumentProfile_getUnderlying(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip);
int32_t                                       dxfg_InstrumentProfile_setUnderlying(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip, const char *value);
double                                        dxfg_InstrumentProfile_getSPC(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip);
int32_t                                       dxfg_InstrumentProfile_setSPC(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip, double value);
const char*                                   dxfg_InstrumentProfile_getAdditionalUnderlyings(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip);
int32_t                                       dxfg_InstrumentProfile_setAdditionalUnderlyings(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip, const char *value);
const char*                                   dxfg_InstrumentProfile_getMMY(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip);
int32_t                                       dxfg_InstrumentProfile_setMMY(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip, const char *value);
int32_t                                       dxfg_InstrumentProfile_getExpiration(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip);
int32_t                                       dxfg_InstrumentProfile_setExpiration(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip, int32_t value);
int32_t                                       dxfg_InstrumentProfile_getLastTrade(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip);
int32_t                                       dxfg_InstrumentProfile_setLastTrade(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip, int32_t value);
double                                        dxfg_InstrumentProfile_getStrike(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip);
int32_t                                       dxfg_InstrumentProfile_setStrike(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip, double value);
const char*                                   dxfg_InstrumentProfile_getOptionType(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip);
int32_t                                       dxfg_InstrumentProfile_setOptionType(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip, const char *value);
const char*                                   dxfg_InstrumentProfile_getExpirationStyle(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip);
int32_t                                       dxfg_InstrumentProfile_setExpirationStyle(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip, const char *value);
const char*                                   dxfg_InstrumentProfile_getSettlementStyle(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip);
int32_t                                       dxfg_InstrumentProfile_setSettlementStyle(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip, const char *value);
const char*                                   dxfg_InstrumentProfile_getPriceIncrements(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip);
int32_t                                       dxfg_InstrumentProfile_setPriceIncrements(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip, const char *value);
const char*                                   dxfg_InstrumentProfile_getTradingHours(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip);
int32_t                                       dxfg_InstrumentProfile_setTradingHours(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip, const char *value);
const char*                                   dxfg_InstrumentProfile_getField(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip, const char *name);
int32_t                                       dxfg_InstrumentProfile_setField(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip, const char *name, const char *value);
double                                        dxfg_InstrumentProfile_getNumericField(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip, const char *name);
int32_t                                       dxfg_InstrumentProfile_setNumericField(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip, const char *name, double value);
int32_t                                       dxfg_InstrumentProfile_getDateField(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip, const char *name);
int32_t                                       dxfg_InstrumentProfile_setDateField(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip, const char *name, int32_t value);
dxfg_string_list*                             dxfg_InstrumentProfile_getNonEmptyCustomFieldNames(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip);
int32_t                                       dxfg_InstrumentProfile_release(graal_isolatethread_t *thread, dxfg_instrument_profile_t *ip);
*/



}

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()
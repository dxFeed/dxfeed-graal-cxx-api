// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_cpp_api/isolated/IsolatedCommon.hpp>
#include <dxfeed_graal_cpp_api/isolated/internal/IsolatedString.hpp>
#include <dxfeed_graal_cpp_api/isolated/ipf/IsolatedInstrumentProfile.hpp>

DXFCPP_BEGIN_NAMESPACE

namespace isolated::ipf::IsolatedInstrumentProfile {

// dxfg_InstrumentProfile_new
/* dxfg_instrument_profile_t* */ JavaObjectHandle<InstrumentProfile> create() {
    return JavaObjectHandle<InstrumentProfile>(runGraalFunctionAndThrowIfNullptr(dxfg_InstrumentProfile_new));
}

// dxfg_InstrumentProfile_new2
/* dxfg_instrument_profile_t* */ JavaObjectHandle<InstrumentProfile>
create(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_new2`. The ip handle is invalid");
    }

    return JavaObjectHandle<InstrumentProfile>(runGraalFunctionAndThrowIfNullptr(
        dxfg_InstrumentProfile_new2, static_cast<dxfg_instrument_profile_t *>(ip.get())));
}

// dxfg_InstrumentProfile_getType
/* const char* */ std::string getType(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_getType`. The ip handle is invalid");
    }

    const auto string = internal::IsolatedString::toUnique(runGraalFunctionAndThrowIfNullptr(
        dxfg_InstrumentProfile_getType, static_cast<dxfg_instrument_profile_t *>(ip.get())));
    auto result = dxfcpp::toString(string.get());

    return result;
}

// dxfg_InstrumentProfile_setType
/* int32_t */ void setType(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                           const StringLikeWrapper &value) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_setType`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfile_setType,
                                       static_cast<dxfg_instrument_profile_t *>(ip.get()), value.c_str());
}

// dxfg_InstrumentProfile_getSymbol
/* const char* */ std::string
getSymbol(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_getSymbol`. The ip handle is invalid");
    }

    const auto string = internal::IsolatedString::toUnique(runGraalFunctionAndThrowIfNullptr(
        dxfg_InstrumentProfile_getSymbol, static_cast<dxfg_instrument_profile_t *>(ip.get())));
    auto result = dxfcpp::toString(string.get());

    return result;
}

// dxfg_InstrumentProfile_setSymbol
/* std::int32_t */ void setSymbol(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                  const StringLikeWrapper &value) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_setSymbol`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfile_setSymbol,
                                       static_cast<dxfg_instrument_profile_t *>(ip.get()), value.c_str());
}

// dxfg_InstrumentProfile_getDescription
/* const char* */ std::string
getDescription(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_getDescription`. The ip handle is invalid");
    }

    const auto string = internal::IsolatedString::toUnique(runGraalFunctionAndThrowIfNullptr(
        dxfg_InstrumentProfile_getDescription, static_cast<dxfg_instrument_profile_t *>(ip.get())));
    auto result = dxfcpp::toString(string.get());

    return result;
}

// dxfg_InstrumentProfile_setDescription
/* std::int32_t */ void setDescription(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                       const StringLikeWrapper &value) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_setDescription`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfile_setDescription,
                                       static_cast<dxfg_instrument_profile_t *>(ip.get()), value.c_str());
}

// dxfg_InstrumentProfile_getLocalSymbol
/* const char* */ std::string
getLocalSymbol(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_getLocalSymbol`. The ip handle is invalid");
    }

    const auto string = internal::IsolatedString::toUnique(runGraalFunctionAndThrowIfNullptr(
        dxfg_InstrumentProfile_getLocalSymbol, static_cast<dxfg_instrument_profile_t *>(ip.get())));
    auto result = dxfcpp::toString(string.get());

    return result;
}

// dxfg_InstrumentProfile_setLocalSymbol
/* std::int32_t */ void setLocalSymbol(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                       const StringLikeWrapper &value) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_setLocalSymbol`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfile_setLocalSymbol,
                                       static_cast<dxfg_instrument_profile_t *>(ip.get()), value.c_str());
}

// dxfg_InstrumentProfile_getLocalDescription
/* const char* */ std::string
getLocalDescription(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_getLocalDescription`. The ip handle is invalid");
    }

    const auto string = internal::IsolatedString::toUnique(runGraalFunctionAndThrowIfNullptr(
        dxfg_InstrumentProfile_getLocalDescription, static_cast<dxfg_instrument_profile_t *>(ip.get())));
    auto result = dxfcpp::toString(string.get());

    return result;
}

// dxfg_InstrumentProfile_setLocalDescription
/* std::int32_t */ void
setLocalDescription(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                    const StringLikeWrapper &value) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_setLocalDescription`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfile_setLocalDescription,
                                       static_cast<dxfg_instrument_profile_t *>(ip.get()), value.c_str());
}

// dxfg_InstrumentProfile_getCountry
/* const char* */ std::string
getCountry(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_getCountry`. The ip handle is invalid");
    }

    const auto string = internal::IsolatedString::toUnique(runGraalFunctionAndThrowIfNullptr(
        dxfg_InstrumentProfile_getCountry, static_cast<dxfg_instrument_profile_t *>(ip.get())));
    auto result = dxfcpp::toString(string.get());

    return result;
}

// dxfg_InstrumentProfile_setCountry
/* std::int32_t */ void setCountry(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                   const StringLikeWrapper &value) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_setCountry`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfile_setCountry,
                                       static_cast<dxfg_instrument_profile_t *>(ip.get()), value.c_str());
}

// dxfg_InstrumentProfile_getOPOL
/* const char* */ std::string getOPOL(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_getOPOL`. The ip handle is invalid");
    }

    const auto string = internal::IsolatedString::toUnique(runGraalFunctionAndThrowIfNullptr(
        dxfg_InstrumentProfile_getOPOL, static_cast<dxfg_instrument_profile_t *>(ip.get())));
    auto result = dxfcpp::toString(string.get());

    return result;
}

// dxfg_InstrumentProfile_setOPOL
/* std::int32_t */ void setOPOL(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                const StringLikeWrapper &value) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_setOPOL`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfile_setOPOL,
                                       static_cast<dxfg_instrument_profile_t *>(ip.get()), value.c_str());
}

// dxfg_InstrumentProfile_getExchangeData
/* const char* */ std::string
getExchangeData(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_getExchangeData`. The ip handle is invalid");
    }

    const auto string = internal::IsolatedString::toUnique(runGraalFunctionAndThrowIfNullptr(
        dxfg_InstrumentProfile_getExchangeData, static_cast<dxfg_instrument_profile_t *>(ip.get())));
    auto result = dxfcpp::toString(string.get());

    return result;
}

// dxfg_InstrumentProfile_setExchangeData
/* std::int32_t */ void setExchangeData(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                        const StringLikeWrapper &value) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_setExchangeData`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfile_setExchangeData,
                                       static_cast<dxfg_instrument_profile_t *>(ip.get()), value.c_str());
}

// dxfg_InstrumentProfile_getExchanges
/* const char* */ std::string
getExchanges(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_getExchanges`. The ip handle is invalid");
    }

    const auto string = internal::IsolatedString::toUnique(runGraalFunctionAndThrowIfNullptr(
        dxfg_InstrumentProfile_getExchanges, static_cast<dxfg_instrument_profile_t *>(ip.get())));
    auto result = dxfcpp::toString(string.get());

    return result;
}

// dxfg_InstrumentProfile_setExchanges
/* std::int32_t */ void setExchanges(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                     const StringLikeWrapper &value) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_setExchanges`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfile_setExchanges,
                                       static_cast<dxfg_instrument_profile_t *>(ip.get()), value.c_str());
}

// dxfg_InstrumentProfile_getCurrency
/* const char* */ std::string
getCurrency(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_getCurrency`. The ip handle is invalid");
    }

    const auto string = internal::IsolatedString::toUnique(runGraalFunctionAndThrowIfNullptr(
        dxfg_InstrumentProfile_getCurrency, static_cast<dxfg_instrument_profile_t *>(ip.get())));
    auto result = dxfcpp::toString(string.get());

    return result;
}

// dxfg_InstrumentProfile_setCurrency
/* std::int32_t */ void setCurrency(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                    const StringLikeWrapper &value) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_setCurrency`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfile_setCurrency,
                                       static_cast<dxfg_instrument_profile_t *>(ip.get()), value.c_str());
}

// dxfg_InstrumentProfile_getBaseCurrency
/* const char* */ std::string
getBaseCurrency(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_getBaseCurrency`. The ip handle is invalid");
    }

    const auto string = internal::IsolatedString::toUnique(runGraalFunctionAndThrowIfNullptr(
        dxfg_InstrumentProfile_getBaseCurrency, static_cast<dxfg_instrument_profile_t *>(ip.get())));
    auto result = dxfcpp::toString(string.get());

    return result;
}

// dxfg_InstrumentProfile_setBaseCurrency
/* std::int32_t */ void setBaseCurrency(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                        const StringLikeWrapper &value) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_setBaseCurrency`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfile_setBaseCurrency,
                                       static_cast<dxfg_instrument_profile_t *>(ip.get()), value.c_str());
}

// dxfg_InstrumentProfile_getCFI
/* const char* */ std::string getCFI(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_getCFI`. The ip handle is invalid");
    }

    const auto string = internal::IsolatedString::toUnique(runGraalFunctionAndThrowIfNullptr(
        dxfg_InstrumentProfile_getCFI, static_cast<dxfg_instrument_profile_t *>(ip.get())));
    auto result = dxfcpp::toString(string.get());

    return result;
}

// dxfg_InstrumentProfile_setCFI
/* std::int32_t */ void setCFI(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                               const StringLikeWrapper &value) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_setCFI`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfile_setCFI,
                                       static_cast<dxfg_instrument_profile_t *>(ip.get()), value.c_str());
}

// dxfg_InstrumentProfile_getISIN
/* const char* */ std::string getISIN(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_getISIN`. The ip handle is invalid");
    }

    const auto string = internal::IsolatedString::toUnique(runGraalFunctionAndThrowIfNullptr(
        dxfg_InstrumentProfile_getISIN, static_cast<dxfg_instrument_profile_t *>(ip.get())));
    auto result = dxfcpp::toString(string.get());

    return result;
}

// dxfg_InstrumentProfile_setISIN
/* std::int32_t */ void setISIN(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                const StringLikeWrapper &value) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_setISIN`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfile_setISIN,
                                       static_cast<dxfg_instrument_profile_t *>(ip.get()), value.c_str());
}

// dxfg_InstrumentProfile_getSEDOL
/* const char* */ std::string getSEDOL(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_getSEDOL`. The ip handle is invalid");
    }

    const auto string = internal::IsolatedString::toUnique(runGraalFunctionAndThrowIfNullptr(
        dxfg_InstrumentProfile_getSEDOL, static_cast<dxfg_instrument_profile_t *>(ip.get())));
    auto result = dxfcpp::toString(string.get());

    return result;
}

// dxfg_InstrumentProfile_setSEDOL
/* std::int32_t */ void setSEDOL(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                 const StringLikeWrapper &value) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_setSEDOL`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfile_setSEDOL,
                                       static_cast<dxfg_instrument_profile_t *>(ip.get()), value.c_str());
}

// dxfg_InstrumentProfile_getCUSIP
/* const char* */ std::string getCUSIP(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_getCUSIP`. The ip handle is invalid");
    }

    const auto string = internal::IsolatedString::toUnique(runGraalFunctionAndThrowIfNullptr(
        dxfg_InstrumentProfile_getCUSIP, static_cast<dxfg_instrument_profile_t *>(ip.get())));
    auto result = dxfcpp::toString(string.get());

    return result;
}

// dxfg_InstrumentProfile_setCUSIP
/* std::int32_t */ void setCUSIP(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                 const StringLikeWrapper &value) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_setCUSIP`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfile_setCUSIP,
                                       static_cast<dxfg_instrument_profile_t *>(ip.get()), value.c_str());
}

// dxfg_InstrumentProfile_getICB
std::int32_t getICB(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_getICB`. The ip handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfile_getICB,
                                              static_cast<dxfg_instrument_profile_t *>(ip.get()));
}

// dxfg_InstrumentProfile_setICB
/* std::int32_t */ void setICB(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                               std::int32_t value) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_setICB`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfile_setICB,
                                       static_cast<dxfg_instrument_profile_t *>(ip.get()), value);
}

// dxfg_InstrumentProfile_getSIC
std::int32_t getSIC(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_getSIC`. The ip handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfile_getSIC,
                                              static_cast<dxfg_instrument_profile_t *>(ip.get()));
}

// dxfg_InstrumentProfile_setSIC
/* std::int32_t */ void setSIC(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                               std::int32_t value) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_setSIC`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfile_setSIC,
                                       static_cast<dxfg_instrument_profile_t *>(ip.get()), value);
}

// dxfg_InstrumentProfile_getMultiplier
double getMultiplier(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_getMultiplier`. The ip handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusInf(dxfg_InstrumentProfile_getMultiplier,
                                              static_cast<dxfg_instrument_profile_t *>(ip.get()));
}

// dxfg_InstrumentProfile_setMultiplier
/* std::int32_t */ void setMultiplier(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                      double value) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_setMultiplier`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfile_setMultiplier,
                                       static_cast<dxfg_instrument_profile_t *>(ip.get()), value);
}

// dxfg_InstrumentProfile_getProduct
/* const char* */ std::string
getProduct(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_getProduct`. The ip handle is invalid");
    }

    const auto string = internal::IsolatedString::toUnique(runGraalFunctionAndThrowIfNullptr(
        dxfg_InstrumentProfile_getProduct, static_cast<dxfg_instrument_profile_t *>(ip.get())));
    auto result = dxfcpp::toString(string.get());

    return result;
}

// dxfg_InstrumentProfile_setProduct
/* std::int32_t */ void setProduct(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                   const StringLikeWrapper &value) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_setProduct`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfile_setProduct,
                                       static_cast<dxfg_instrument_profile_t *>(ip.get()), value.c_str());
}

// dxfg_InstrumentProfile_getUnderlying
/* const char* */ std::string
getUnderlying(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_getUnderlying`. The ip handle is invalid");
    }

    const auto string = internal::IsolatedString::toUnique(runGraalFunctionAndThrowIfNullptr(
        dxfg_InstrumentProfile_getUnderlying, static_cast<dxfg_instrument_profile_t *>(ip.get())));
    auto result = dxfcpp::toString(string.get());

    return result;
}

// dxfg_InstrumentProfile_setUnderlying
/* std::int32_t */ void setUnderlying(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                      const StringLikeWrapper &value) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_setUnderlying`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfile_setUnderlying,
                                       static_cast<dxfg_instrument_profile_t *>(ip.get()), value.c_str());
}

// dxfg_InstrumentProfile_getSPC
double getSPC(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_getSPC`. The ip handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusInf(dxfg_InstrumentProfile_getSPC,
                                              static_cast<dxfg_instrument_profile_t *>(ip.get()));
}

// dxfg_InstrumentProfile_setSPC
/* std::int32_t */ void setSPC(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                               double value) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_setSPC`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfile_setSPC,
                                       static_cast<dxfg_instrument_profile_t *>(ip.get()), value);
}

// dxfg_InstrumentProfile_getAdditionalUnderlyings
/* const char* */ std::string
getAdditionalUnderlyings(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_getAdditionalUnderlyings`. The ip handle is invalid");
    }

    const auto string = internal::IsolatedString::toUnique(runGraalFunctionAndThrowIfNullptr(
        dxfg_InstrumentProfile_getAdditionalUnderlyings, static_cast<dxfg_instrument_profile_t *>(ip.get())));
    auto result = dxfcpp::toString(string.get());

    return result;
}

// dxfg_InstrumentProfile_setAdditionalUnderlyings
/* std::int32_t */ void
setAdditionalUnderlyings(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                         const StringLikeWrapper &value) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_setAdditionalUnderlyings`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfile_setAdditionalUnderlyings,
                                       static_cast<dxfg_instrument_profile_t *>(ip.get()), value.c_str());
}

// dxfg_InstrumentProfile_getMMY
/* const char* */ std::string getMMY(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_getMMY`. The ip handle is invalid");
    }

    const auto string = internal::IsolatedString::toUnique(runGraalFunctionAndThrowIfNullptr(
        dxfg_InstrumentProfile_getMMY, static_cast<dxfg_instrument_profile_t *>(ip.get())));
    auto result = dxfcpp::toString(string.get());

    return result;
}

// dxfg_InstrumentProfile_setMMY
/* std::int32_t */ void setMMY(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                               const StringLikeWrapper &value) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_setMMY`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfile_setMMY,
                                       static_cast<dxfg_instrument_profile_t *>(ip.get()), value.c_str());
}

// dxfg_InstrumentProfile_getExpiration
std::int32_t getExpiration(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_getExpiration`. The ip handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfile_getExpiration,
                                              static_cast<dxfg_instrument_profile_t *>(ip.get()));
}

// dxfg_InstrumentProfile_setExpiration
/* std::int32_t */ void setExpiration(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                      std::int32_t value) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_setExpiration`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfile_setExpiration,
                                       static_cast<dxfg_instrument_profile_t *>(ip.get()), value);
}

// dxfg_InstrumentProfile_getLastTrade
std::int32_t getLastTrade(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_getLastTrade`. The ip handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfile_getLastTrade,
                                              static_cast<dxfg_instrument_profile_t *>(ip.get()));
}

// dxfg_InstrumentProfile_setLastTrade
/* std::int32_t */ void setLastTrade(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                     std::int32_t value) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_setLastTrade`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfile_setLastTrade,
                                       static_cast<dxfg_instrument_profile_t *>(ip.get()), value);
}

// dxfg_InstrumentProfile_getStrike
double getStrike(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_getStrike`. The ip handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusInf(dxfg_InstrumentProfile_getStrike,
                                              static_cast<dxfg_instrument_profile_t *>(ip.get()));
}

// dxfg_InstrumentProfile_setStrike
/* std::int32_t */ void setStrike(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                  double value) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_setStrike`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfile_setStrike,
                                       static_cast<dxfg_instrument_profile_t *>(ip.get()), value);
}

// dxfg_InstrumentProfile_getOptionType
/* const char* */ std::string
getOptionType(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_getOptionType`. The ip handle is invalid");
    }

    const auto string = internal::IsolatedString::toUnique(runGraalFunctionAndThrowIfNullptr(
        dxfg_InstrumentProfile_getOptionType, static_cast<dxfg_instrument_profile_t *>(ip.get())));
    auto result = dxfcpp::toString(string.get());

    return result;
}

// dxfg_InstrumentProfile_setOptionType
/* std::int32_t */ void setOptionType(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                      const StringLikeWrapper &value) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_setOptionType`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfile_setOptionType,
                                       static_cast<dxfg_instrument_profile_t *>(ip.get()), value.c_str());
}

// dxfg_InstrumentProfile_getExpirationStyle
/* const char* */ std::string
getExpirationStyle(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_getExpirationStyle`. The ip handle is invalid");
    }

    const auto string = internal::IsolatedString::toUnique(runGraalFunctionAndThrowIfNullptr(
        dxfg_InstrumentProfile_getExpirationStyle, static_cast<dxfg_instrument_profile_t *>(ip.get())));
    auto result = dxfcpp::toString(string.get());

    return result;
}

// dxfg_InstrumentProfile_setExpirationStyle
/* std::int32_t */ void
setExpirationStyle(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                   const StringLikeWrapper &value) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_setExpirationStyle`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfile_setExpirationStyle,
                                       static_cast<dxfg_instrument_profile_t *>(ip.get()), value.c_str());
}

// dxfg_InstrumentProfile_getSettlementStyle
/* const char* */ std::string
getSettlementStyle(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_getSettlementStyle`. The ip handle is invalid");
    }

    const auto string = internal::IsolatedString::toUnique(runGraalFunctionAndThrowIfNullptr(
        dxfg_InstrumentProfile_getSettlementStyle, static_cast<dxfg_instrument_profile_t *>(ip.get())));
    auto result = dxfcpp::toString(string.get());

    return result;
}

// dxfg_InstrumentProfile_setSettlementStyle
/* std::int32_t */ void
setSettlementStyle(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                   const StringLikeWrapper &value) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_setSettlementStyle`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfile_setSettlementStyle,
                                       static_cast<dxfg_instrument_profile_t *>(ip.get()), value.c_str());
}

// dxfg_InstrumentProfile_getPriceIncrements
/* const char* */ std::string
getPriceIncrements(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_getPriceIncrements`. The ip handle is invalid");
    }

    const auto string = internal::IsolatedString::toUnique(runGraalFunctionAndThrowIfNullptr(
        dxfg_InstrumentProfile_getPriceIncrements, static_cast<dxfg_instrument_profile_t *>(ip.get())));
    auto result = dxfcpp::toString(string.get());

    return result;
}

// dxfg_InstrumentProfile_setPriceIncrements
/* std::int32_t */ void
setPriceIncrements(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                   const StringLikeWrapper &value) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_setPriceIncrements`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfile_setPriceIncrements,
                                       static_cast<dxfg_instrument_profile_t *>(ip.get()), value.c_str());
}

// dxfg_InstrumentProfile_getTradingHours
/* const char* */ std::string
getTradingHours(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_getTradingHours`. The ip handle is invalid");
    }

    const auto string = internal::IsolatedString::toUnique(runGraalFunctionAndThrowIfNullptr(
        dxfg_InstrumentProfile_getTradingHours, static_cast<dxfg_instrument_profile_t *>(ip.get())));
    auto result = dxfcpp::toString(string.get());

    return result;
}

// dxfg_InstrumentProfile_setTradingHours
/* std::int32_t */ void setTradingHours(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                        const StringLikeWrapper &value) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_setTradingHours`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfile_setTradingHours,
                                       static_cast<dxfg_instrument_profile_t *>(ip.get()), value.c_str());
}

// dxfg_InstrumentProfile_getField
/* const char* */ std::string getField(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                       const StringLikeWrapper &name) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_getField`. The ip handle is invalid");
    }

    const auto string = internal::IsolatedString::toUnique(runGraalFunctionAndThrowIfNullptr(
        dxfg_InstrumentProfile_getField, static_cast<dxfg_instrument_profile_t *>(ip.get()), name.c_str()));
    auto result = dxfcpp::toString(string.get());

    return result;
}

// dxfg_InstrumentProfile_setField
/* std::int32_t */ void setField(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                 const StringLikeWrapper &name, const StringLikeWrapper &value) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_setField`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfile_setField,
                                       static_cast<dxfg_instrument_profile_t *>(ip.get()), name.c_str(), value.c_str());
}

// dxfg_InstrumentProfile_getNumericField
double getNumericField(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                       const StringLikeWrapper &name) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_getNumericField`. The ip handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusInf(dxfg_InstrumentProfile_getNumericField,
                                              static_cast<dxfg_instrument_profile_t *>(ip.get()), name.c_str());
}

// dxfg_InstrumentProfile_setNumericField
/* std::int32_t */ void setNumericField(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                        const StringLikeWrapper &name, double value) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_setNumericField`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfile_setNumericField,
                                       static_cast<dxfg_instrument_profile_t *>(ip.get()), name.c_str(), value);
}

// dxfg_InstrumentProfile_getDateField
std::int32_t getDateField(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                          const StringLikeWrapper &name) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_getDateField`. The ip handle is invalid");
    }

    return runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfile_getDateField,
                                              static_cast<dxfg_instrument_profile_t *>(ip.get()), name.c_str());
}

// dxfg_InstrumentProfile_setDateField
/* std::int32_t */ void setDateField(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip,
                                     const StringLikeWrapper &name, std::int32_t value) {
    if (!ip) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_InstrumentProfile_setNumericField`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_InstrumentProfile_setNumericField,
                                       static_cast<dxfg_instrument_profile_t *>(ip.get()), name.c_str(), value);
}

// dxfg_InstrumentProfile_getNonEmptyCustomFieldNames
/* dxfg_string_list* */ std::vector<std::string>
getNonEmptyCustomFieldNames(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw InvalidArgumentException("Unable to execute function `dxfg_InstrumentProfile_getNonEmptyCustomFieldNames`. "
                                    "The ip handle is invalid");
    }

    std::vector<std::string> result;
    auto graalStringList = internal::IsolatedStringList::toUnique(runGraalFunctionAndThrowIfNullptr(
        dxfg_InstrumentProfile_getNonEmptyCustomFieldNames, static_cast<dxfg_instrument_profile_t *>(ip.get())));

    const auto size = static_cast<dxfg_string_list *>(graalStringList.get())->size;
    const auto elements = static_cast<dxfg_string_list *>(graalStringList.get())->elements;

    result.reserve(size);

    for (auto i = 0; i < size; i++) {
        result.emplace_back(dxfcpp::toString(elements[i]));
    }

    result.shrink_to_fit();

    return result;
}

} // namespace isolated::ipf::IsolatedInstrumentProfile

namespace isolated::ipf::IsolatedInstrumentProfileList {
void release(/* dxfg_instrument_profile_list * */ void *list) {
    if (!list) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_CList_InstrumentProfile_wrapper_release`. The list is nullptr");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_CList_InstrumentProfile_release,
                                       static_cast<dxfg_instrument_profile_list *>(list));
}

void releaseWrapper(/* dxfg_instrument_profile_list * */ void *list) {
    if (!list) {
        throw InvalidArgumentException(
            "Unable to execute function `dxfg_CList_InstrumentProfile_wrapper_release`. The list is nullptr");
    }

    runGraalFunctionAndThrowIfMinusOne(dxfg_CList_InstrumentProfile_wrapper_release,
                                       static_cast<dxfg_instrument_profile_list *>(list));
}

std::unique_ptr<void, decltype(&releaseWrapper)> toUniqueWrapper(/* dxfg_instrument_profile_list * */ void *list) {
    return {list, releaseWrapper};
}

} // namespace isolated::ipf::IsolatedInstrumentProfileList

DXFCPP_END_NAMESPACE
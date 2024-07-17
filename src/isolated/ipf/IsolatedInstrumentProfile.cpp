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
        throw std::invalid_argument(
            "Unable to execute function `dxfg_InstrumentProfile_new2`. The ip handle is invalid");
    }

    return JavaObjectHandle<InstrumentProfile>(runGraalFunctionAndThrowIfNullptr(
        dxfg_InstrumentProfile_new2, static_cast<dxfg_instrument_profile_t *>(ip.get())));
}

// dxfg_InstrumentProfile_getType
/* const char* */ std::string getType(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw std::invalid_argument(
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
        throw std::invalid_argument(
            "Unable to execute function `dxfg_InstrumentProfile_setType`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_InstrumentProfile_setType,
                                           static_cast<dxfg_instrument_profile_t *>(ip.get()), value.c_str());
}

// dxfg_InstrumentProfile_getSymbol
/* const char* */ std::string
getSymbol(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw std::invalid_argument(
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
        throw std::invalid_argument(
            "Unable to execute function `dxfg_InstrumentProfile_setSymbol`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_InstrumentProfile_setSymbol,
                                           static_cast<dxfg_instrument_profile_t *>(ip.get()), value.c_str());
}

// dxfg_InstrumentProfile_getDescription
/* const char* */ std::string
getDescription(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw std::invalid_argument(
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
        throw std::invalid_argument(
            "Unable to execute function `dxfg_InstrumentProfile_setDescription`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_InstrumentProfile_setDescription,
                                           static_cast<dxfg_instrument_profile_t *>(ip.get()), value.c_str());
}

// dxfg_InstrumentProfile_getLocalSymbol
/* const char* */ std::string
getLocalSymbol(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw std::invalid_argument(
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
        throw std::invalid_argument(
            "Unable to execute function `dxfg_InstrumentProfile_setLocalSymbol`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_InstrumentProfile_setLocalSymbol,
                                           static_cast<dxfg_instrument_profile_t *>(ip.get()), value.c_str());
}

// dxfg_InstrumentProfile_getLocalDescription
/* const char* */ std::string
getLocalDescription(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw std::invalid_argument(
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
        throw std::invalid_argument(
            "Unable to execute function `dxfg_InstrumentProfile_setLocalDescription`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_InstrumentProfile_setLocalDescription,
                                           static_cast<dxfg_instrument_profile_t *>(ip.get()), value.c_str());
}

// dxfg_InstrumentProfile_getCountry
/* const char* */ std::string
getCountry(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw std::invalid_argument(
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
        throw std::invalid_argument(
            "Unable to execute function `dxfg_InstrumentProfile_setCountry`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_InstrumentProfile_setCountry,
                                           static_cast<dxfg_instrument_profile_t *>(ip.get()), value.c_str());
}

// dxfg_InstrumentProfile_getOPOL
/* const char* */ std::string getOPOL(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw std::invalid_argument(
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
        throw std::invalid_argument(
            "Unable to execute function `dxfg_InstrumentProfile_setOPOL`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_InstrumentProfile_setOPOL,
                                           static_cast<dxfg_instrument_profile_t *>(ip.get()), value.c_str());
}

// dxfg_InstrumentProfile_getExchangeData
/* const char* */ std::string
getExchangeData(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw std::invalid_argument(
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
        throw std::invalid_argument(
            "Unable to execute function `dxfg_InstrumentProfile_setExchangeData`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_InstrumentProfile_setExchangeData,
                                           static_cast<dxfg_instrument_profile_t *>(ip.get()), value.c_str());
}

// dxfg_InstrumentProfile_getExchanges
/* const char* */ std::string
getExchanges(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw std::invalid_argument(
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
        throw std::invalid_argument(
            "Unable to execute function `dxfg_InstrumentProfile_setExchanges`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_InstrumentProfile_setExchanges,
                                           static_cast<dxfg_instrument_profile_t *>(ip.get()), value.c_str());
}

// dxfg_InstrumentProfile_getCurrency
/* const char* */ std::string
getCurrency(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw std::invalid_argument(
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
        throw std::invalid_argument(
            "Unable to execute function `dxfg_InstrumentProfile_setCurrency`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_InstrumentProfile_setCurrency,
                                           static_cast<dxfg_instrument_profile_t *>(ip.get()), value.c_str());
}

// dxfg_InstrumentProfile_getBaseCurrency
/* const char* */ std::string
getBaseCurrency(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw std::invalid_argument(
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
        throw std::invalid_argument(
            "Unable to execute function `dxfg_InstrumentProfile_setBaseCurrency`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_InstrumentProfile_setBaseCurrency,
                                           static_cast<dxfg_instrument_profile_t *>(ip.get()), value.c_str());
}

// dxfg_InstrumentProfile_getCFI
/* const char* */ std::string getCFI(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw std::invalid_argument(
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
        throw std::invalid_argument(
            "Unable to execute function `dxfg_InstrumentProfile_setCFI`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_InstrumentProfile_setCFI,
                                           static_cast<dxfg_instrument_profile_t *>(ip.get()), value.c_str());
}

// dxfg_InstrumentProfile_getISIN
/* const char* */ std::string getISIN(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw std::invalid_argument(
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
        throw std::invalid_argument(
            "Unable to execute function `dxfg_InstrumentProfile_setISIN`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_InstrumentProfile_setISIN,
                                           static_cast<dxfg_instrument_profile_t *>(ip.get()), value.c_str());
}

// dxfg_InstrumentProfile_getSEDOL
/* const char* */ std::string getSEDOL(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw std::invalid_argument(
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
        throw std::invalid_argument(
            "Unable to execute function `dxfg_InstrumentProfile_setSEDOL`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_InstrumentProfile_setSEDOL,
                                           static_cast<dxfg_instrument_profile_t *>(ip.get()), value.c_str());
}

// dxfg_InstrumentProfile_getCUSIP
/* const char* */ std::string getCUSIP(/* dxfg_instrument_profile_t* */ const JavaObjectHandle<InstrumentProfile> &ip) {
    if (!ip) {
        throw std::invalid_argument(
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
        throw std::invalid_argument(
            "Unable to execute function `dxfg_InstrumentProfile_setCUSIP`. The ip handle is invalid");
    }

    runGraalFunctionAndThrowIfLessThanZero(dxfg_InstrumentProfile_setCUSIP,
                                           static_cast<dxfg_instrument_profile_t *>(ip.get()), value.c_str());
}

} // namespace isolated::ipf::IsolatedInstrumentProfile

DXFCPP_END_NAMESPACE
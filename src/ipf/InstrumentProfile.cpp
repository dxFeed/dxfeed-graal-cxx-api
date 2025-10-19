// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include <dxfg_api.h>

#include <dxfeed_graal_c_api/api.h>
#include <dxfeed_graal_cpp_api/api.hpp>

#include <cstring>
#include <memory>
#include <utf8.h>
#include <utility>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>

DXFCPP_BEGIN_NAMESPACE

InstrumentProfile::Ptr InstrumentProfile::create() {
    return createShared(isolated::ipf::IsolatedInstrumentProfile::create());
}

InstrumentProfile::Ptr InstrumentProfile::create(Ptr ip) {
    return createShared(isolated::ipf::IsolatedInstrumentProfile::create(ip->handle_));
}

std::string InstrumentProfile::getType() const {
    return isolated::ipf::IsolatedInstrumentProfile::getType(handle_);
}

void InstrumentProfile::setType(const StringLikeWrapper &type) const {
    isolated::ipf::IsolatedInstrumentProfile::setType(handle_, type);
}

std::string InstrumentProfile::getSymbol() const {
    return isolated::ipf::IsolatedInstrumentProfile::getSymbol(handle_);
}

void InstrumentProfile::setSymbol(const StringLikeWrapper &symbol) const {
    isolated::ipf::IsolatedInstrumentProfile::setSymbol(handle_, symbol);
}

std::string InstrumentProfile::getDescription() const {
    return isolated::ipf::IsolatedInstrumentProfile::getDescription(handle_);
}

void InstrumentProfile::setDescription(const StringLikeWrapper &description) const {
    isolated::ipf::IsolatedInstrumentProfile::setDescription(handle_, description);
}

std::string InstrumentProfile::getLocalSymbol() const {
    return isolated::ipf::IsolatedInstrumentProfile::getLocalSymbol(handle_);
}

void InstrumentProfile::setLocalSymbol(const StringLikeWrapper &localSymbol) const {
    isolated::ipf::IsolatedInstrumentProfile::setLocalSymbol(handle_, localSymbol);
}

std::string InstrumentProfile::getLocalDescription() const {
    return isolated::ipf::IsolatedInstrumentProfile::getLocalDescription(handle_);
}

void InstrumentProfile::setLocalDescription(const StringLikeWrapper &localDescription) const {
    isolated::ipf::IsolatedInstrumentProfile::setLocalDescription(handle_, localDescription);
}

std::string InstrumentProfile::getCountry() const {
    return isolated::ipf::IsolatedInstrumentProfile::getCountry(handle_);
}

void InstrumentProfile::setCountry(const StringLikeWrapper &country) const {
    isolated::ipf::IsolatedInstrumentProfile::setCountry(handle_, country);
}

std::string InstrumentProfile::getOPOL() const {
    return isolated::ipf::IsolatedInstrumentProfile::getOPOL(handle_);
}

void InstrumentProfile::setOPOL(const StringLikeWrapper &opol) const {
    isolated::ipf::IsolatedInstrumentProfile::setOPOL(handle_, opol);
}

std::string InstrumentProfile::getExchangeData() const {
    return isolated::ipf::IsolatedInstrumentProfile::getExchangeData(handle_);
}

void InstrumentProfile::setExchangeData(const StringLikeWrapper &exchangeData) const {
    isolated::ipf::IsolatedInstrumentProfile::setExchangeData(handle_, exchangeData);
}

std::string InstrumentProfile::getExchanges() const {
    return isolated::ipf::IsolatedInstrumentProfile::getExchanges(handle_);
}

void InstrumentProfile::setExchanges(const StringLikeWrapper &exchanges) const {
    isolated::ipf::IsolatedInstrumentProfile::setExchanges(handle_, exchanges);
}

std::string InstrumentProfile::getCurrency() const {
    return isolated::ipf::IsolatedInstrumentProfile::getCurrency(handle_);
}

void InstrumentProfile::setCurrency(const StringLikeWrapper &currency) const {
    isolated::ipf::IsolatedInstrumentProfile::setCurrency(handle_, currency);
}

std::string InstrumentProfile::getBaseCurrency() const {
    return isolated::ipf::IsolatedInstrumentProfile::getBaseCurrency(handle_);
}

void InstrumentProfile::setBaseCurrency(const StringLikeWrapper &baseCurrency) const {
    isolated::ipf::IsolatedInstrumentProfile::setBaseCurrency(handle_, baseCurrency);
}

std::string InstrumentProfile::getCFI() const {
    return isolated::ipf::IsolatedInstrumentProfile::getCFI(handle_);
}

void InstrumentProfile::setCFI(const StringLikeWrapper &cfi) const {
    isolated::ipf::IsolatedInstrumentProfile::setCFI(handle_, cfi);
}

std::string InstrumentProfile::getISIN() const {
    return isolated::ipf::IsolatedInstrumentProfile::getISIN(handle_);
}

void InstrumentProfile::setISIN(const StringLikeWrapper &isin) const {
    isolated::ipf::IsolatedInstrumentProfile::setISIN(handle_, isin);
}

std::string InstrumentProfile::getSEDOL() const {
    return isolated::ipf::IsolatedInstrumentProfile::getSEDOL(handle_);
}

void InstrumentProfile::setSEDOL(const StringLikeWrapper &sedol) const {
    isolated::ipf::IsolatedInstrumentProfile::setSEDOL(handle_, sedol);
}

std::string InstrumentProfile::getCUSIP() const {
    return isolated::ipf::IsolatedInstrumentProfile::getCUSIP(handle_);
}

void InstrumentProfile::setCUSIP(const StringLikeWrapper &cusip) const {
    isolated::ipf::IsolatedInstrumentProfile::setCUSIP(handle_, cusip);
}

std::int32_t InstrumentProfile::getICB() const {
    return isolated::ipf::IsolatedInstrumentProfile::getICB(handle_);
}

void InstrumentProfile::setICB(std::int32_t icb) const {
    isolated::ipf::IsolatedInstrumentProfile::setICB(handle_, icb);
}

std::int32_t InstrumentProfile::getSIC() const {
    return isolated::ipf::IsolatedInstrumentProfile::getSIC(handle_);
}

void InstrumentProfile::setSIC(std::int32_t sic) const {
    isolated::ipf::IsolatedInstrumentProfile::setSIC(handle_, sic);
}

double InstrumentProfile::getMultiplier() const {
    return isolated::ipf::IsolatedInstrumentProfile::getMultiplier(handle_);
}

void InstrumentProfile::setMultiplier(double multiplier) const {
    isolated::ipf::IsolatedInstrumentProfile::setMultiplier(handle_, multiplier);
}

std::string InstrumentProfile::getProduct() const {
    return isolated::ipf::IsolatedInstrumentProfile::getProduct(handle_);
}

void InstrumentProfile::setProduct(const StringLikeWrapper &product) const {
    isolated::ipf::IsolatedInstrumentProfile::setProduct(handle_, product);
}

std::string InstrumentProfile::getUnderlying() const {
    return isolated::ipf::IsolatedInstrumentProfile::getUnderlying(handle_);
}

void InstrumentProfile::setUnderlying(const StringLikeWrapper &underlying) const {
    isolated::ipf::IsolatedInstrumentProfile::setUnderlying(handle_, underlying);
}

double InstrumentProfile::getSPC() const {
    return isolated::ipf::IsolatedInstrumentProfile::getSPC(handle_);
}

void InstrumentProfile::setSPC(double spc) const {
    isolated::ipf::IsolatedInstrumentProfile::setSPC(handle_, spc);
}

std::string InstrumentProfile::getAdditionalUnderlyings() const {
    return isolated::ipf::IsolatedInstrumentProfile::getAdditionalUnderlyings(handle_);
}

void InstrumentProfile::setAdditionalUnderlyings(const StringLikeWrapper &additionalUnderlyings) const {
    isolated::ipf::IsolatedInstrumentProfile::setAdditionalUnderlyings(handle_, additionalUnderlyings);
}

std::string InstrumentProfile::getMMY() const {
    return isolated::ipf::IsolatedInstrumentProfile::getMMY(handle_);
}

void InstrumentProfile::setMMY(const StringLikeWrapper &mmy) const {
    isolated::ipf::IsolatedInstrumentProfile::setMMY(handle_, mmy);
}

std::int32_t InstrumentProfile::getExpiration() const {
    return isolated::ipf::IsolatedInstrumentProfile::getExpiration(handle_);
}

void InstrumentProfile::setExpiration(std::int32_t expiration) const {
    isolated::ipf::IsolatedInstrumentProfile::setExpiration(handle_, expiration);
}

std::int32_t InstrumentProfile::getLastTrade() const {
    return isolated::ipf::IsolatedInstrumentProfile::getLastTrade(handle_);
}

void InstrumentProfile::setLastTrade(std::int32_t lastTrade) const {
    isolated::ipf::IsolatedInstrumentProfile::setLastTrade(handle_, lastTrade);
}

double InstrumentProfile::getStrike() const {
    return isolated::ipf::IsolatedInstrumentProfile::getStrike(handle_);
}

void InstrumentProfile::setStrike(double strike) const {
    isolated::ipf::IsolatedInstrumentProfile::setStrike(handle_, strike);
}

std::string InstrumentProfile::getOptionType() const {
    return isolated::ipf::IsolatedInstrumentProfile::getOptionType(handle_);
}

void InstrumentProfile::setOptionType(const StringLikeWrapper &optionType) const {
    isolated::ipf::IsolatedInstrumentProfile::setOptionType(handle_, optionType);
}

std::string InstrumentProfile::getExpirationStyle() const {
    return isolated::ipf::IsolatedInstrumentProfile::getExpirationStyle(handle_);
}

void InstrumentProfile::setExpirationStyle(const StringLikeWrapper &expirationStyle) const {
    isolated::ipf::IsolatedInstrumentProfile::setExpirationStyle(handle_, expirationStyle);
}

std::string InstrumentProfile::getSettlementStyle() const {
    return isolated::ipf::IsolatedInstrumentProfile::getSettlementStyle(handle_);
}

void InstrumentProfile::setSettlementStyle(const StringLikeWrapper &settlementStyle) const {
    isolated::ipf::IsolatedInstrumentProfile::setSettlementStyle(handle_, settlementStyle);
}

std::string InstrumentProfile::getPriceIncrements() const {
    return isolated::ipf::IsolatedInstrumentProfile::getPriceIncrements(handle_);
}

void InstrumentProfile::setPriceIncrements(const StringLikeWrapper &priceIncrements) const {
    isolated::ipf::IsolatedInstrumentProfile::setPriceIncrements(handle_, priceIncrements);
}

std::string InstrumentProfile::getTradingHours() const {
    return isolated::ipf::IsolatedInstrumentProfile::getTradingHours(handle_);
}

void InstrumentProfile::setTradingHours(const StringLikeWrapper &tradingHours) const {
    isolated::ipf::IsolatedInstrumentProfile::setTradingHours(handle_, tradingHours);
}

std::string InstrumentProfile::getField(const StringLikeWrapper &name) const {
    return isolated::ipf::IsolatedInstrumentProfile::getField(handle_, name);
}

void InstrumentProfile::setField(const StringLikeWrapper &name, const StringLikeWrapper &value) const {
    isolated::ipf::IsolatedInstrumentProfile::setField(handle_, name, value);
}

double InstrumentProfile::getNumericField(const StringLikeWrapper &name) const {
    return isolated::ipf::IsolatedInstrumentProfile::getNumericField(handle_, name);
}

void InstrumentProfile::setNumericField(const StringLikeWrapper &name, double value) const {
    isolated::ipf::IsolatedInstrumentProfile::setNumericField(handle_, name, value);
}

std::int32_t InstrumentProfile::getDateField(const StringLikeWrapper &name) const {
    return isolated::ipf::IsolatedInstrumentProfile::getDateField(handle_, name);
}

void InstrumentProfile::setDateField(const StringLikeWrapper &name, std::int32_t value) const {
    isolated::ipf::IsolatedInstrumentProfile::setDateField(handle_, name, value);
}

std::vector<std::string> InstrumentProfile::getNonEmptyCustomFieldNames() const {
    return isolated::ipf::IsolatedInstrumentProfile::getNonEmptyCustomFieldNames(handle_);
}

std::string InstrumentProfile::toString() const {
    if (!handle_) {
        return "InstrumentProfile{<null>}";
    }

    return JavaObject::toString(handle_.get());
}

std::size_t InstrumentProfile::hashCode() const noexcept {
    if (!handle_) {
        return 0;
    }

    return JavaObject::hashCode(handle_.get());
}

bool InstrumentProfile::operator==(const InstrumentProfile &other) const {
    if (!handle_) {
        return !other.handle_;
    }

    if (!other.handle_) {
        return false;
    }

    return JavaObject::equals(handle_.get(), other.handle_.get()) == 0;
}

InstrumentProfile::~InstrumentProfile() noexcept {
}

InstrumentProfile::InstrumentProfile(LockExternalConstructionTag, JavaObjectHandle<InstrumentProfile> &&handle)
    : handle_(std::move(handle)) {
}

InstrumentProfile::Ptr InstrumentProfile::create(JavaObjectHandle<InstrumentProfile> &&handle) {
    return createShared(std::move(handle));
}

std::vector<std::shared_ptr<InstrumentProfile>> InstrumentProfile::List::fromGraal(void *graalList) {
    using ListType = dxfg_instrument_profile_list;
    using SizeType = decltype(ListType::size);

    if (!graalList) {
        return {};
    }

    std::vector<std::shared_ptr<InstrumentProfile>> result{};

    const auto list = static_cast<ListType *>(graalList);

    if (list->size <= 0 || list->elements == nullptr) {
        return result;
    }

    for (SizeType elementIndex = 0; elementIndex < list->size; elementIndex++) {
        if (list->elements[elementIndex]) {
            result.emplace_back(create(JavaObjectHandle<InstrumentProfile>(list->elements[elementIndex])));
        }
    }

    return result;
}

DXFCPP_END_NAMESPACE
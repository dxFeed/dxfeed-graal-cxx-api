// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#include "../../include/dxfeed_graal_cpp_api/ipf/InstrumentProfile.hpp"

#include "../isolated/ipf/IsolatedInstrumentProfile.hpp"

#include <dxfg_api.h>
#include <memory>
#include <utility>

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

void InstrumentProfile::setType(StringParam type) const {
    isolated::ipf::IsolatedInstrumentProfile::setType(handle_, type.view());
}

std::string InstrumentProfile::getSymbol() const {
    return isolated::ipf::IsolatedInstrumentProfile::getSymbol(handle_);
}

void InstrumentProfile::setSymbol(StringParam symbol) const {
    isolated::ipf::IsolatedInstrumentProfile::setSymbol(handle_, symbol.view());
}

std::string InstrumentProfile::getDescription() const {
    return isolated::ipf::IsolatedInstrumentProfile::getDescription(handle_);
}

void InstrumentProfile::setDescription(StringParam description) const {
    isolated::ipf::IsolatedInstrumentProfile::setDescription(handle_, description.view());
}

std::string InstrumentProfile::getLocalSymbol() const {
    return isolated::ipf::IsolatedInstrumentProfile::getLocalSymbol(handle_);
}

void InstrumentProfile::setLocalSymbol(StringParam localSymbol) const {
    isolated::ipf::IsolatedInstrumentProfile::setLocalSymbol(handle_, localSymbol.view());
}

std::string InstrumentProfile::getLocalDescription() const {
    return isolated::ipf::IsolatedInstrumentProfile::getLocalDescription(handle_);
}

void InstrumentProfile::setLocalDescription(StringParam localDescription) const {
    isolated::ipf::IsolatedInstrumentProfile::setLocalDescription(handle_, localDescription.view());
}

std::string InstrumentProfile::getCountry() const {
    return isolated::ipf::IsolatedInstrumentProfile::getCountry(handle_);
}

void InstrumentProfile::setCountry(StringParam country) const {
    isolated::ipf::IsolatedInstrumentProfile::setCountry(handle_, country.view());
}

std::string InstrumentProfile::getOPOL() const {
    return isolated::ipf::IsolatedInstrumentProfile::getOPOL(handle_);
}

void InstrumentProfile::setOPOL(StringParam opol) const {
    isolated::ipf::IsolatedInstrumentProfile::setOPOL(handle_, opol.view());
}

std::string InstrumentProfile::getExchangeData() const {
    return isolated::ipf::IsolatedInstrumentProfile::getExchangeData(handle_);
}

void InstrumentProfile::setExchangeData(StringParam exchangeData) const {
    isolated::ipf::IsolatedInstrumentProfile::setExchangeData(handle_, exchangeData.view());
}

std::string InstrumentProfile::getExchanges() const {
    return isolated::ipf::IsolatedInstrumentProfile::getExchanges(handle_);
}

void InstrumentProfile::setExchanges(StringParam exchanges) const {
    isolated::ipf::IsolatedInstrumentProfile::setExchanges(handle_, exchanges.view());
}

std::string InstrumentProfile::getCurrency() const {
    return isolated::ipf::IsolatedInstrumentProfile::getCurrency(handle_);
}

void InstrumentProfile::setCurrency(StringParam currency) const {
    isolated::ipf::IsolatedInstrumentProfile::setCurrency(handle_, currency.view());
}

std::string InstrumentProfile::getBaseCurrency() const {
    return isolated::ipf::IsolatedInstrumentProfile::getBaseCurrency(handle_);
}

void InstrumentProfile::setBaseCurrency(StringParam baseCurrency) const {
    isolated::ipf::IsolatedInstrumentProfile::setBaseCurrency(handle_, baseCurrency.view());
}

std::string InstrumentProfile::getCFI() const {
    return isolated::ipf::IsolatedInstrumentProfile::getCFI(handle_);
}

void InstrumentProfile::setCFI(StringParam cfi) const {
    isolated::ipf::IsolatedInstrumentProfile::setCFI(handle_, cfi.view());
}

std::string InstrumentProfile::getISIN() const {
    return isolated::ipf::IsolatedInstrumentProfile::getISIN(handle_);
}

void InstrumentProfile::setISIN(StringParam isin) const {
    isolated::ipf::IsolatedInstrumentProfile::setISIN(handle_, isin.view());
}

std::string InstrumentProfile::getSEDOL() const {
    return isolated::ipf::IsolatedInstrumentProfile::getSEDOL(handle_);
}

void InstrumentProfile::setSEDOL(StringParam sedol) const {
    isolated::ipf::IsolatedInstrumentProfile::setSEDOL(handle_, sedol.view());
}

std::string InstrumentProfile::getCUSIP() const {
    return isolated::ipf::IsolatedInstrumentProfile::getCUSIP(handle_);
}

void InstrumentProfile::setCUSIP(StringParam cusip) const {
    isolated::ipf::IsolatedInstrumentProfile::setCUSIP(handle_, cusip.view());
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

void InstrumentProfile::setProduct(StringParam product) const {
    isolated::ipf::IsolatedInstrumentProfile::setProduct(handle_, product.view());
}

std::string InstrumentProfile::getUnderlying() const {
    return isolated::ipf::IsolatedInstrumentProfile::getUnderlying(handle_);
}

void InstrumentProfile::setUnderlying(StringParam underlying) const {
    isolated::ipf::IsolatedInstrumentProfile::setUnderlying(handle_, underlying.view());
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

void InstrumentProfile::setAdditionalUnderlyings(StringParam additionalUnderlyings) const {
    isolated::ipf::IsolatedInstrumentProfile::setAdditionalUnderlyings(handle_, additionalUnderlyings.view());
}

std::string InstrumentProfile::getMMY() const {
    return isolated::ipf::IsolatedInstrumentProfile::getMMY(handle_);
}

void InstrumentProfile::setMMY(StringParam mmy) const {
    isolated::ipf::IsolatedInstrumentProfile::setMMY(handle_, mmy.view());
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

void InstrumentProfile::setOptionType(StringParam optionType) const {
    isolated::ipf::IsolatedInstrumentProfile::setOptionType(handle_, optionType.view());
}

std::string InstrumentProfile::getExpirationStyle() const {
    return isolated::ipf::IsolatedInstrumentProfile::getExpirationStyle(handle_);
}

void InstrumentProfile::setExpirationStyle(StringParam expirationStyle) const {
    isolated::ipf::IsolatedInstrumentProfile::setExpirationStyle(handle_, expirationStyle.view());
}

std::string InstrumentProfile::getSettlementStyle() const {
    return isolated::ipf::IsolatedInstrumentProfile::getSettlementStyle(handle_);
}

void InstrumentProfile::setSettlementStyle(StringParam settlementStyle) const {
    isolated::ipf::IsolatedInstrumentProfile::setSettlementStyle(handle_, settlementStyle.view());
}

std::string InstrumentProfile::getPriceIncrements() const {
    return isolated::ipf::IsolatedInstrumentProfile::getPriceIncrements(handle_);
}

void InstrumentProfile::setPriceIncrements(StringParam priceIncrements) const {
    isolated::ipf::IsolatedInstrumentProfile::setPriceIncrements(handle_, priceIncrements.view());
}

std::string InstrumentProfile::getTradingHours() const {
    return isolated::ipf::IsolatedInstrumentProfile::getTradingHours(handle_);
}

void InstrumentProfile::setTradingHours(StringParam tradingHours) const {
    isolated::ipf::IsolatedInstrumentProfile::setTradingHours(handle_, tradingHours.view());
}

std::string InstrumentProfile::getField(StringParam name) const {
    return isolated::ipf::IsolatedInstrumentProfile::getField(handle_, name.view());
}

void InstrumentProfile::setField(StringParam name, StringParam value) const {
    isolated::ipf::IsolatedInstrumentProfile::setField(handle_, name.view(), value.view());
}

double InstrumentProfile::getNumericField(StringParam name) const {
    return isolated::ipf::IsolatedInstrumentProfile::getNumericField(handle_, name.view());
}

void InstrumentProfile::setNumericField(StringParam name, double value) const {
    isolated::ipf::IsolatedInstrumentProfile::setNumericField(handle_, name.view(), value);
}

std::int32_t InstrumentProfile::getDateField(StringParam name) const {
    return isolated::ipf::IsolatedInstrumentProfile::getDateField(handle_, name.view());
}

void InstrumentProfile::setDateField(StringParam name, std::int32_t value) const {
    isolated::ipf::IsolatedInstrumentProfile::setDateField(handle_, name.view(), value);
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

    return JavaObject::equals(handle_.get(), other.handle_.get());
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
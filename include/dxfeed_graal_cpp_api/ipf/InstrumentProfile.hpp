// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

#include "../internal/Common.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace dxfcpp {

/**
 * Represents basic profile information about market instrument.
 * Please see <a href="http://www.dxfeed.com/downloads/documentation/dxFeed_Instrument_Profile_Format.pdf">Instrument
 * Profile Format documentation</a> for complete description.
 */
class DXFCPP_EXPORT InstrumentProfile final : public SharedEntity {
    struct Data {
        std::string type{};
        std::string symbol{};
        std::string description{};
        std::string localSymbol{};
        std::string localDescription{};
        std::string country{};
        std::string opol{};
        std::string exchangeData{};
        std::string exchanges{};
        std::string currency{};
        std::string baseCurrency{};
        std::string cfi{};
        std::string isin{};
        std::string sedol{};
        std::string cusip{};
        std::int32_t icb{};
        std::int32_t sic{};
        double multiplier = math::NaN;
        std::string product{};
        std::string underlying{};
        double spc = math::NaN;
        std::string additionalUnderlyings{};
        std::string mmy{};
        int32_t expiration{};
        int32_t lastTrade{};
        double strike = math::NaN;
        std::string optionType{};
        std::string expirationStyle{};
        std::string settlementStyle{};
        std::string priceIncrements{};
        std::string tradingHours{};
        std::unordered_map<std::string, std::string> customFields{};
    };

    Data data_{};

    void fillData(void *graalNative) noexcept;
    void fillGraalData(void *graalNative) const noexcept;
    static void freeGraalData(void *graalNative) noexcept;

    /**
     * Returns custom field value with a specified name.
     *
     * @param name The name of custom field.
     * @return The reference to custom field value with a specified name or std::nullopt
     */
    std::optional<std::reference_wrapper<const std::string>> getCustomField(const std::string &name) const noexcept {
        if (!data_.customFields.contains(name)) {
            return std::nullopt;
        }

        return std::cref(data_.customFields.at(name));
    }

    /**
     * Changes custom field value with a specified name.
     *
     * @param name The name of custom field.
     * @param value The custom field value.
     */
    void setCustomField(const std::string &name, const std::string &value) {
        data_.customFields[name] = value;
    }

  public:
    static std::shared_ptr<InstrumentProfile> fromGraal(void *graalNative) noexcept;

    /**
     * Allocates memory for the dxFeed Graal SDK structure (recursively if necessary).
     * Fills the dxFeed Graal SDK structure's fields by the data of the current entity (recursively if necessary).
     * Returns the pointer to the filled structure.
     *
     * @return The pointer to the filled dxFeed Graal SDK structure
     */
    void *toGraal() const noexcept;

    /**
     * Releases the memory occupied by the dxFeed Graal SDK structure (recursively if necessary).
     *
     * @param graalNative The pointer to the dxFeed Graal SDK structure.
     */
    static void freeGraal(void *graalNative) noexcept;

    /// The alias to a type of shared pointer to the InstrumentProfile object
    using Ptr = std::shared_ptr<InstrumentProfile>;

    /// The alias to a type of unique pointer to the InstrumentProfile object
    using Unique = std::unique_ptr<InstrumentProfile>;

    /**
     * Creates new instrument profile with default values.
     */
    InstrumentProfile() noexcept = default;

    /**
     * Returns type of instrument.
     * It takes precedence in conflict cases with other fields.
     * It is a mandatory field. It may not be empty.
     * Example: "STOCK", "FUTURE", "OPTION".
     *
     * @return type of instrument.
     */
    const std::string &getType() const & noexcept {
        return data_.type;
    }

    /**
     * Changes type of instrument.
     * It takes precedence in conflict cases with other fields.
     * It is a mandatory field. It may not be empty.
     * Example: "STOCK", "FUTURE", "OPTION".
     *
     * @param type The type of instrument.
     */
    void setType(const std::string &type) noexcept {
        data_.type = type;
    }

    /**
     * Changes type of instrument and returns the current instrument profile.
     *
     * @param type The type of instrument.
     * @return The current instrument profile.
     */
    InstrumentProfile &withType(const std::string &type) noexcept {
        InstrumentProfile::setType(type);

        return *this;
    }

    /**
     * Changes type of instrument and returns a shared pointer to the current instrument profile.
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<InstrumentProfile>(new
     * InstrumentProfile(...))` or `std::make_shared<InstrumentProfile>(...)`
     *
     * @param type The type of instrument.
     * @return A shared pointer to the current instrument profile.
     */
    InstrumentProfile::Ptr withTypeShared(const std::string &type) noexcept {
        InstrumentProfile::setType(type);

        return shared_from_this()->sharedAs<InstrumentProfile>();
    }

    /**
     * Returns identifier of instrument, preferable an international one in Latin alphabet.
     * It is a mandatory field. It may not be empty.
     * Example: "GOOG", "/YGM9", ".ZYEAD".
     *
     * @return The identifier of instrument.
     */
    const std::string &getSymbol() const & noexcept {
        return data_.symbol;
    }

    /**
     * Changes identifier of instrument, preferable an international one in Latin alphabet.
     * It is a mandatory field. It may not be empty.
     * Example: "GOOG", "/YGM9", ".ZYEAD".
     *
     * @param symbol The identifier of instrument.
     */
    void setSymbol(const std::string &symbol) noexcept {
        data_.symbol = symbol;
    }

    /**
     * Changes identifier of instrument, preferable an international one in Latin alphabet.
     * It is a mandatory field. It may not be empty.
     * Example: "GOOG", "/YGM9", ".ZYEAD".
     *
     * @param symbol The identifier of instrument.
     * @return The current instrument profile.
     */
    InstrumentProfile &withSymbol(const std::string &symbol) noexcept {
        InstrumentProfile::setSymbol(symbol);

        return *this;
    }

    /**
     * Changes identifier of instrument, preferable an international one in Latin alphabet.
     * It is a mandatory field. It may not be empty.
     * Example: "GOOG", "/YGM9", ".ZYEAD".
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<InstrumentProfile>(new
     * InstrumentProfile(...))` or `std::make_shared<InstrumentProfile>(...)`
     *
     * @param symbol The identifier of instrument.
     * @return A shared pointer to the current instrument profile.
     */
    InstrumentProfile::Ptr withSymbolShared(const std::string &symbol) noexcept {
        InstrumentProfile::setSymbol(symbol);

        return shared_from_this()->sharedAs<InstrumentProfile>();
    }

    /**
     * Returns the description of instrument, preferable an international one in Latin alphabet.
     * Example: "Google Inc.", "Mini Gold Futures,Jun-2009,ETH".
     *
     * @return The description of instrument.
     */
    const std::string &getDescription() const & noexcept {
        return data_.description;
    }

    /**
     * Changes description of instrument, preferable an international one in Latin alphabet.
     * Example: "Google Inc.", "Mini Gold Futures,Jun-2009,ETH".
     *
     * @param description The description of instrument.
     */
    void setDescription(const std::string &description) noexcept {
        data_.description = description;
    }

    /**
     * Changes description of instrument, preferable an international one in Latin alphabet.
     * Example: "Google Inc.", "Mini Gold Futures,Jun-2009,ETH".
     *
     * @param description The description of instrument.
     * @return The current instrument profile.
     */
    InstrumentProfile &withDescription(const std::string &description) noexcept {
        InstrumentProfile::setDescription(description);

        return *this;
    }

    /**
     * Changes description of instrument, preferable an international one in Latin alphabet.
     * Example: "Google Inc.", "Mini Gold Futures,Jun-2009,ETH".
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<InstrumentProfile>(new
     * InstrumentProfile(...))` or `std::make_shared<InstrumentProfile>(...)`
     *
     * @param description The description of instrument.
     * @return A shared pointer to the current instrument profile.
     */
    InstrumentProfile::Ptr withDescriptionShared(const std::string &description) noexcept {
        InstrumentProfile::setDescription(description);

        return shared_from_this()->sharedAs<InstrumentProfile>();
    }

    /**
     * Returns identifier of instrument in national language.
     * It shall be empty if same as @ref InstrumentProfile::getSymbol() "symbol".
     *
     * @return The identifier of instrument in national language.
     */
    const std::string &getLocalSymbol() const & noexcept {
        return data_.localSymbol;
    }

    /**
     * Changes identifier of instrument in national language.
     * It shall be empty if same as @ref InstrumentProfile::getSymbol() "symbol".
     *
     * @param localSymbol The identifier of instrument in national language.
     */
    void setLocalSymbol(const std::string &localSymbol) noexcept {
        data_.localSymbol = localSymbol;
    }

    /**
     * Changes identifier of instrument in national language.
     * It shall be empty if same as @ref InstrumentProfile::getSymbol() "symbol".
     *
     * @param localSymbol The identifier of instrument in national language.
     * @return The current instrument profile.
     */
    InstrumentProfile &withLocalSymbol(const std::string &localSymbol) noexcept {
        InstrumentProfile::setLocalSymbol(localSymbol);

        return *this;
    }

    /**
     * Changes identifier of instrument in national language.
     * It shall be empty if same as @ref InstrumentProfile::getSymbol() "symbol".
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<InstrumentProfile>(new
     * InstrumentProfile(...))` or `std::make_shared<InstrumentProfile>(...)`
     *
     * @param localSymbol The identifier of instrument in national language.
     * @return A shared pointer to the current instrument profile.
     */
    InstrumentProfile::Ptr withLocalSymbolShared(const std::string &localSymbol) noexcept {
        InstrumentProfile::setLocalSymbol(localSymbol);

        return shared_from_this()->sharedAs<InstrumentProfile>();
    }

    /**
     * Returns description of instrument in national language.
     * It shall be empty if same as @ref InstrumentProfile::getDescription() "description".
     *
     * @return The description of instrument in national language.
     */
    const std::string &getLocalDescription() const & noexcept {
        return data_.localDescription;
    }

    /**
     * Changes description of instrument in national language.
     * It shall be empty if same as @ref InstrumentProfile::getDescription() "description".
     *
     * @param localDescription The description of instrument in national language.
     */
    void setLocalDescription(const std::string &localDescription) noexcept {
        data_.localDescription = localDescription;
    }

    /**
     * Changes description of instrument in national language.
     * It shall be empty if same as @ref InstrumentProfile::getDescription() "description".
     *
     * @param localDescription The description of instrument in national language.
     * @return The current instrument profile.
     */
    InstrumentProfile &withLocalDescription(const std::string &localDescription) noexcept {
        InstrumentProfile::setLocalDescription(localDescription);

        return *this;
    }

    /**
     * Changes description of instrument in national language.
     * It shall be empty if same as @ref InstrumentProfile::getDescription() "description".
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<InstrumentProfile>(new
     * InstrumentProfile(...))` or `std::make_shared<InstrumentProfile>(...)`
     *
     * @param localDescription The description of instrument in national language.
     * @return A shared pointer to the current instrument profile.
     */
    InstrumentProfile::Ptr withLocalDescriptionShared(const std::string &localDescription) noexcept {
        InstrumentProfile::setLocalDescription(localDescription);

        return shared_from_this()->sharedAs<InstrumentProfile>();
    }

    /**
     * Returns country of origin (incorporation) of corresponding company or parent entity.
     * It shall use two-letter country code from ISO 3166-1 standard.
     * See <a href="http://en.wikipedia.org/wiki/ISO_3166-1">ISO 3166-1 on Wikipedia</a>.
     * Example: "US", "RU".
     *
     * @return The country of origin (incorporation) of corresponding company or parent entity.
     */
    const std::string &getCountry() const & noexcept {
        return data_.country;
    }

    /**
     * Changes country of origin (incorporation) of corresponding company or parent entity.
     * It shall use two-letter country code from ISO 3166-1 standard.
     * See <a href="http://en.wikipedia.org/wiki/ISO_3166-1">ISO 3166-1 on Wikipedia</a>.
     * Example: "US", "RU".
     *
     * @param country The country of origin (incorporation) of corresponding company or parent entity.
     */
    void setCountry(const std::string &country) noexcept {
        data_.country = country;
    }

    /**
     * Changes country of origin (incorporation) of corresponding company or parent entity.
     * It shall use two-letter country code from ISO 3166-1 standard.
     * See <a href="http://en.wikipedia.org/wiki/ISO_3166-1">ISO 3166-1 on Wikipedia</a>.
     * Example: "US", "RU".
     *
     * @param country The country of origin (incorporation) of corresponding company or parent entity.
     * @return The current instrument profile.
     */
    InstrumentProfile &withCountry(const std::string &country) noexcept {
        InstrumentProfile::setCountry(country);

        return *this;
    }

    /**
     * Changes country of origin (incorporation) of corresponding company or parent entity.
     * It shall use two-letter country code from ISO 3166-1 standard.
     * See <a href="http://en.wikipedia.org/wiki/ISO_3166-1">ISO 3166-1 on Wikipedia</a>.
     * Example: "US", "RU".
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<InstrumentProfile>(new
     * InstrumentProfile(...))` or `std::make_shared<InstrumentProfile>(...)`
     *
     * @param country The country of origin (incorporation) of corresponding company or parent entity.
     * @return A shared pointer to the current instrument profile.
     */
    InstrumentProfile::Ptr withCountryShared(const std::string &country) noexcept {
        InstrumentProfile::setCountry(country);

        return shared_from_this()->sharedAs<InstrumentProfile>();
    }

    /**
     * Returns official Place Of Listing, the organization that have listed this instrument.
     * Instruments with multiple listings shall use separate profiles for each listing.
     * It shall use Market Identifier Code (MIC) from ISO 10383 standard.
     * See <a href="http://en.wikipedia.org/wiki/ISO_10383">ISO 10383 on Wikipedia</a>
     * or <a href="http://www.iso15022.org/MIC/homepageMIC.htm">MIC homepage</a>.
     * Example: "XNAS", "RTSX"
     *
     * @return The official Place Of Listing, the organization that have listed this instrument.
     */
    const std::string &getOPOL() const & noexcept {
        return data_.opol;
    }

    /**
     * Changes official Place Of Listing, the organization that have listed this instrument.
     * Instruments with multiple listings shall use separate profiles for each listing.
     * It shall use Market Identifier Code (MIC) from ISO 10383 standard.
     * See <a href="http://en.wikipedia.org/wiki/ISO_10383">ISO 10383 on Wikipedia</a>
     * or <a href="http://www.iso15022.org/MIC/homepageMIC.htm">MIC homepage</a>.
     * Example: "XNAS", "RTSX"
     *
     * @param opol The official Place Of Listing, the organization that have listed this instrument.
     */
    void setOPOL(const std::string &opol) noexcept {
        data_.opol = opol;
    }

    /**
     * Changes official Place Of Listing, the organization that have listed this instrument.
     * Instruments with multiple listings shall use separate profiles for each listing.
     * It shall use Market Identifier Code (MIC) from ISO 10383 standard.
     * See <a href="http://en.wikipedia.org/wiki/ISO_10383">ISO 10383 on Wikipedia</a>
     * or <a href="http://www.iso15022.org/MIC/homepageMIC.htm">MIC homepage</a>.
     * Example: "XNAS", "RTSX"
     *
     * @param opol The official Place Of Listing, the organization that have listed this instrument.
     * @return The current instrument profile.
     */
    InstrumentProfile &withOPOL(const std::string &opol) noexcept {
        InstrumentProfile::setOPOL(opol);

        return *this;
    }

    /**
     * Changes official Place Of Listing, the organization that have listed this instrument.
     * Instruments with multiple listings shall use separate profiles for each listing.
     * It shall use Market Identifier Code (MIC) from ISO 10383 standard.
     * See <a href="http://en.wikipedia.org/wiki/ISO_10383">ISO 10383 on Wikipedia</a>
     * or <a href="http://www.iso15022.org/MIC/homepageMIC.htm">MIC homepage</a>.
     * Example: "XNAS", "RTSX"
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<InstrumentProfile>(new
     * InstrumentProfile(...))` or `std::make_shared<InstrumentProfile>(...)`
     *
     * @param opol The official Place Of Listing, the organization that have listed this instrument.
     * @return A shared pointer to the current instrument profile.
     */
    InstrumentProfile::Ptr withOPOLShared(const std::string &opol) noexcept {
        InstrumentProfile::setOPOL(opol);

        return shared_from_this()->sharedAs<InstrumentProfile>();
    }

    /**
     * Returns exchange-specific data required to properly identify instrument when communicating with exchange.
     * It uses exchange-specific format.
     *
     * @return The exchange-specific data required to properly identify instrument when communicating with exchange.
     */
    const std::string &getExchangeData() const & noexcept {
        return data_.exchangeData;
    }

    /**
     * Changes exchange-specific data required to properly identify instrument when communicating with exchange.
     * It uses exchange-specific format.
     *
     * @param exchangeData The exchange-specific data required to properly identify instrument when communicating with
     * exchange.
     */
    void setExchangeData(const std::string &exchangeData) noexcept {
        data_.exchangeData = exchangeData;
    }

    /**
     * Changes exchange-specific data required to properly identify instrument when communicating with exchange.
     * It uses exchange-specific format.
     *
     * @param exchangeData The exchange-specific data required to properly identify instrument when communicating with
     * exchange.
     * @return The current instrument profile.
     */
    InstrumentProfile &withExchangeData(const std::string &exchangeData) noexcept {
        InstrumentProfile::setExchangeData(exchangeData);

        return *this;
    }

    /**
     * Changes exchange-specific data required to properly identify instrument when communicating with exchange.
     * It uses exchange-specific format.
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<InstrumentProfile>(new
     * InstrumentProfile(...))` or `std::make_shared<InstrumentProfile>(...)`
     *
     * @param exchangeData The exchange-specific data required to properly identify instrument when communicating with
     * exchange.
     * @return A shared pointer to the current instrument profile.
     */
    InstrumentProfile::Ptr withExchangeDataShared(const std::string &exchangeData) noexcept {
        InstrumentProfile::setExchangeData(exchangeData);

        return shared_from_this()->sharedAs<InstrumentProfile>();
    }

    /**
     * Returns list of exchanges where instrument is quoted or traded.
     * Its shall use the following format:
     * ```
     *     <VALUE> ::= <empty> | <LIST>
     *     <LIST> ::= <MIC> | <MIC> <semicolon>
     *
     *     <LIST> the list shall be sorted by MIC.
     * ```
     * Example: "ARCX;CBSX ;XNAS;XNYS".
     *
     * @return The list of exchanges where instrument is quoted or traded.
     */
    const std::string &getExchanges() const & noexcept {
        return data_.exchanges;
    }

    /**
     * Changes list of exchanges where instrument is quoted or traded.
     * It shall use the following format:
     * ```
     *     <VALUE> ::= <empty> | <LIST>
     *     <LIST> ::= <MIC> | <MIC> <semicolon>
     *
     *     <LIST> the list shall be sorted by MIC.
     * ```
     * Example: "ARCX;CBSX ;XNAS;XNYS".
     *
     * @param exchanges The list of exchanges where instrument is quoted or traded.
     */
    void setExchanges(const std::string &exchanges) noexcept {
        data_.exchanges = exchanges;
    }

    /**
     * Changes list of exchanges where instrument is quoted or traded.
     * It shall use the following format:
     * ```
     *     <VALUE> ::= <empty> | <LIST>
     *     <LIST> ::= <MIC> | <MIC> <semicolon>
     *
     *     <LIST> the list shall be sorted by MIC.
     * ```
     * Example: "ARCX;CBSX ;XNAS;XNYS".
     *
     * @param exchanges The list of exchanges where instrument is quoted or traded.
     * @return The current instrument profile.
     */
    InstrumentProfile &withExchanges(const std::string &exchanges) noexcept {
        InstrumentProfile::setExchanges(exchanges);

        return *this;
    }

    /**
     * Changes list of exchanges where instrument is quoted or traded.
     * It shall use the following format:
     * ```
     *     <VALUE> ::= <empty> | <LIST>
     *     <LIST> ::= <MIC> | <MIC> <semicolon>
     *
     *     <LIST> the list shall be sorted by MIC.
     * ```
     * Example: "ARCX;CBSX ;XNAS;XNYS".
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<InstrumentProfile>(new
     * InstrumentProfile(...))` or `std::make_shared<InstrumentProfile>(...)`
     *
     * @param exchanges The list of exchanges where instrument is quoted or traded.
     * @return A shared pointer to the current instrument profile.
     */
    InstrumentProfile::Ptr withExchangesShared(const std::string &exchanges) noexcept {
        InstrumentProfile::setExchanges(exchanges);

        return shared_from_this()->sharedAs<InstrumentProfile>();
    }

    /**
     * Returns currency of quotation, pricing and trading.
     * It shall use three-letter currency code from ISO 4217 standard.
     * See <a href="http://en.wikipedia.org/wiki/ISO_4217">ISO 4217 on Wikipedia</a>.
     * Example: "USD", "RUB".
     *
     * @return Currency of quotation, pricing and trading.
     */
    const std::string &getCurrency() const & noexcept {
        return data_.currency;
    }

    /**
     * Changes currency of quotation, pricing and trading.
     * It shall use three-letter currency code from ISO 4217 standard.
     * See <a href="http://en.wikipedia.org/wiki/ISO_4217">ISO 4217 on Wikipedia</a>.
     * Example: "USD", "RUB".
     *
     * @param currency Currency of quotation, pricing and trading.
     */
    void setCurrency(const std::string &currency) noexcept {
        data_.currency = currency;
    }

    /**
     * Changes currency of quotation, pricing and trading.
     * It shall use three-letter currency code from ISO 4217 standard.
     * See <a href="http://en.wikipedia.org/wiki/ISO_4217">ISO 4217 on Wikipedia</a>.
     * Example: "USD", "RUB".
     *
     * @param currency Currency of quotation, pricing and trading.
     * @return The current instrument profile.
     */
    InstrumentProfile &withCurrency(const std::string &currency) noexcept {
        InstrumentProfile::setCurrency(currency);

        return *this;
    }

    /**
     * Changes currency of quotation, pricing and trading.
     * It shall use three-letter currency code from ISO 4217 standard.
     * See <a href="http://en.wikipedia.org/wiki/ISO_4217">ISO 4217 on Wikipedia</a>.
     * Example: "USD", "RUB".
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<InstrumentProfile>(new
     * InstrumentProfile(...))` or `std::make_shared<InstrumentProfile>(...)`
     *
     * @param currency Currency of quotation, pricing and trading.
     * @return A shared pointer to the current instrument profile.
     */
    InstrumentProfile::Ptr withCurrencyShared(const std::string &currency) noexcept {
        InstrumentProfile::setCurrency(currency);

        return shared_from_this()->sharedAs<InstrumentProfile>();
    }

    /**
     * Returns base currency of currency pair (FOREX instruments).
     * It shall use three-letter currency code similarly to @ref InstrumentProfile::getCurrency() "currency".
     *
     * @return The base currency of currency pair (FOREX instruments).
     */
    const std::string &getBaseCurrency() const & noexcept {
        return data_.baseCurrency;
    }

    /**
     * Changes base currency of currency pair (FOREX instruments).
     * It shall use three-letter currency code similarly to @ref InstrumentProfile::getCurrency() "currency".
     *
     * @param baseCurrency The base currency of currency pair (FOREX instruments).
     */
    void setBaseCurrency(const std::string &baseCurrency) noexcept {
        data_.baseCurrency = baseCurrency;
    }

    /**
     * Changes base currency of currency pair (FOREX instruments).
     * It shall use three-letter currency code similarly to @ref InstrumentProfile::getCurrency() "currency".
     *
     * @param baseCurrency The base currency of currency pair (FOREX instruments).
     * @return The current instrument profile.
     */
    InstrumentProfile &withBaseCurrency(const std::string &baseCurrency) noexcept {
        InstrumentProfile::setBaseCurrency(baseCurrency);

        return *this;
    }

    /**
     * Changes base currency of currency pair (FOREX instruments).
     * It shall use three-letter currency code similarly to @ref InstrumentProfile::getCurrency() "currency".
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<InstrumentProfile>(new
     * InstrumentProfile(...))` or `std::make_shared<InstrumentProfile>(...)`
     *
     * @param baseCurrency The base currency of currency pair (FOREX instruments).
     * @return A shared pointer to the current instrument profile.
     */
    InstrumentProfile::Ptr withBaseCurrencyShared(const std::string &baseCurrency) noexcept {
        InstrumentProfile::setBaseCurrency(baseCurrency);

        return shared_from_this()->sharedAs<InstrumentProfile>();
    }

    /**
     * Returns Classification of Financial Instruments code.
     * It is a mandatory field for OPTION instruments as it is the only way to distinguish Call/Put type,
     * American/European exercise, Cash/Physical delivery.
     * It shall use six-letter CFI code from ISO 10962 standard.
     * It is allowed to use 'X' extensively and to omit trailing letters (assumed to be 'X').
     * See <a href="http://en.wikipedia.org/wiki/ISO_10962">ISO 10962 on Wikipedia</a>.
     * Example: "ESNTPB", "ESXXXX", "ES" , "OPASPS".
     *
     * @return CFI code.
     */
    const std::string &getCFI() const & noexcept {
        return data_.cfi;
    }

    /**
     * Changes Classification of Financial Instruments code.
     * It is a mandatory field for OPTION instruments as it is the only way to distinguish Call/Put type,
     * American/European exercise, Cash/Physical delivery.
     * It shall use six-letter CFI code from ISO 10962 standard.
     * It is allowed to use 'X' extensively and to omit trailing letters (assumed to be 'X').
     * See <a href="http://en.wikipedia.org/wiki/ISO_10962">ISO 10962 on Wikipedia</a>.
     * Example: "ESNTPB", "ESXXXX", "ES" , "OPASPS".
     *
     * @param cfi CFI code.
     */
    void setCFI(const std::string &cfi) noexcept {
        data_.cfi = cfi;
    }

    /**
     * Changes Classification of Financial Instruments code.
     * It is a mandatory field for OPTION instruments as it is the only way to distinguish Call/Put type,
     * American/European exercise, Cash/Physical delivery.
     * It shall use six-letter CFI code from ISO 10962 standard.
     * It is allowed to use 'X' extensively and to omit trailing letters (assumed to be 'X').
     * See <a href="http://en.wikipedia.org/wiki/ISO_10962">ISO 10962 on Wikipedia</a>.
     * Example: "ESNTPB", "ESXXXX", "ES" , "OPASPS".
     *
     * @param cfi CFI code.
     * @return The current instrument profile.
     */
    InstrumentProfile &withCFI(const std::string &cfi) noexcept {
        InstrumentProfile::setCFI(cfi);

        return *this;
    }

    /**
     * Changes Classification of Financial Instruments code.
     * It is a mandatory field for OPTION instruments as it is the only way to distinguish Call/Put type,
     * American/European exercise, Cash/Physical delivery.
     * It shall use six-letter CFI code from ISO 10962 standard.
     * It is allowed to use 'X' extensively and to omit trailing letters (assumed to be 'X').
     * See <a href="http://en.wikipedia.org/wiki/ISO_10962">ISO 10962 on Wikipedia</a>.
     * Example: "ESNTPB", "ESXXXX", "ES" , "OPASPS".
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<InstrumentProfile>(new
     * InstrumentProfile(...))` or `std::make_shared<InstrumentProfile>(...)`
     *
     * @param cfi CFI code.
     * @return A shared pointer to the current instrument profile.
     */
    InstrumentProfile::Ptr withCFIShared(const std::string &cfi) noexcept {
        InstrumentProfile::setCFI(cfi);

        return shared_from_this()->sharedAs<InstrumentProfile>();
    }

    /**
     * Returns International Securities Identifying Number.
     * It shall use twelve-letter code from ISO 6166 standard.
     * See <a href="http://en.wikipedia.org/wiki/ISO_6166">ISO 6166 on Wikipedia</a>
     * or <a href="http://en.wikipedia.org/wiki/International_Securities_Identifying_Number">ISIN on Wikipedia</a>.
     * Example: "DE0007100000", "US38259P5089".
     *
     * @return International Securities Identifying Number.
     */
    const std::string &getISIN() const & noexcept {
        return data_.isin;
    }

    /**
     * Changes International Securities Identifying Number.
     * It shall use twelve-letter code from ISO 6166 standard.
     * See <a href="http://en.wikipedia.org/wiki/ISO_6166">ISO 6166 on Wikipedia</a>
     * or <a href="http://en.wikipedia.org/wiki/International_Securities_Identifying_Number">ISIN on Wikipedia</a>.
     * Example: "DE0007100000", "US38259P5089".
     *
     * @param isin International Securities Identifying Number.
     */
    void setISIN(const std::string &isin) noexcept {
        data_.isin = isin;
    }

    /**
     * Changes International Securities Identifying Number.
     * It shall use twelve-letter code from ISO 6166 standard.
     * See <a href="http://en.wikipedia.org/wiki/ISO_6166">ISO 6166 on Wikipedia</a>
     * or <a href="http://en.wikipedia.org/wiki/International_Securities_Identifying_Number">ISIN on Wikipedia</a>.
     * Example: "DE0007100000", "US38259P5089".
     *
     * @param isin International Securities Identifying Number.
     * @return The current instrument profile.
     */
    InstrumentProfile &withISIN(const std::string &isin) noexcept {
        InstrumentProfile::setISIN(isin);

        return *this;
    }

    /**
     * Changes International Securities Identifying Number.
     * It shall use twelve-letter code from ISO 6166 standard.
     * See <a href="http://en.wikipedia.org/wiki/ISO_6166">ISO 6166 on Wikipedia</a>
     * or <a href="http://en.wikipedia.org/wiki/International_Securities_Identifying_Number">ISIN on Wikipedia</a>.
     * Example: "DE0007100000", "US38259P5089".
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<InstrumentProfile>(new
     * InstrumentProfile(...))` or `std::make_shared<InstrumentProfile>(...)`
     *
     * @param isin International Securities Identifying Number.
     * @return A shared pointer to the current instrument profile.
     */
    InstrumentProfile::Ptr withISINShared(const std::string &isin) noexcept {
        InstrumentProfile::setISIN(isin);

        return shared_from_this()->sharedAs<InstrumentProfile>();
    }

    /**
     * Returns Stock Exchange Daily Official List.
     * It shall use seven-letter code assigned by London Stock Exchange.
     * See <a href="http://en.wikipedia.org/wiki/SEDOL">SEDOL on Wikipedia</a> or
     * <a href="http://www.londonstockexchange.com/en-gb/products/informationproducts/sedol/">SEDOL on LSE</a>.
     * Example: "2310967", "5766857".
     *
     * @return Stock Exchange Daily Official List.
     */
    const std::string &getSEDOL() const & noexcept {
        return data_.sedol;
    }

    /**
     * Changes Stock Exchange Daily Official List.
     * It shall use seven-letter code assigned by London Stock Exchange.
     * See <a href="http://en.wikipedia.org/wiki/SEDOL">SEDOL on Wikipedia</a> or
     * <a href="http://www.londonstockexchange.com/en-gb/products/informationproducts/sedol/">SEDOL on LSE</a>.
     * Example: "2310967", "5766857".
     *
     * @param sedol Stock Exchange Daily Official List.
     */
    void setSEDOL(const std::string &sedol) noexcept {
        data_.sedol = sedol;
    }

    /**
     * Changes Stock Exchange Daily Official List.
     * It shall use seven-letter code assigned by London Stock Exchange.
     * See <a href="http://en.wikipedia.org/wiki/SEDOL">SEDOL on Wikipedia</a> or
     * <a href="http://www.londonstockexchange.com/en-gb/products/informationproducts/sedol/">SEDOL on LSE</a>.
     * Example: "2310967", "5766857".
     *
     * @param sedol Stock Exchange Daily Official List.
     * @return The current instrument profile.
     */
    InstrumentProfile &withSEDOL(const std::string &sedol) noexcept {
        InstrumentProfile::setSEDOL(sedol);

        return *this;
    }

    /**
     * Changes Stock Exchange Daily Official List.
     * It shall use seven-letter code assigned by London Stock Exchange.
     * See <a href="http://en.wikipedia.org/wiki/SEDOL">SEDOL on Wikipedia</a> or
     * <a href="http://www.londonstockexchange.com/en-gb/products/informationproducts/sedol/">SEDOL on LSE</a>.
     * Example: "2310967", "5766857".
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<InstrumentProfile>(new
     * InstrumentProfile(...))` or `std::make_shared<InstrumentProfile>(...)`
     *
     * @param sedol Stock Exchange Daily Official List.
     * @return A shared pointer to the current instrument profile.
     */
    InstrumentProfile::Ptr withSEDOLShared(const std::string &sedol) noexcept {
        InstrumentProfile::setSEDOL(sedol);

        return shared_from_this()->sharedAs<InstrumentProfile>();
    }

    /**
     * Returns Committee on Uniform Security Identification Procedures code.
     * It shall use nine-letter code assigned by CUSIP Services Bureau.
     * See <a href="http://en.wikipedia.org/wiki/CUSIP">CUSIP on Wikipedia</a>.
     * Example: "38259P508".
     *
     * @return CUSIP code.
     */
    const std::string &getCUSIP() const & noexcept {
        return data_.cusip;
    }

    /**
     * Changes Committee on Uniform Security Identification Procedures code.
     * It shall use nine-letter code assigned by CUSIP Services Bureau.
     * See <a href="http://en.wikipedia.org/wiki/CUSIP">CUSIP on Wikipedia</a>.
     * Example: "38259P508".
     *
     * @param cusip CUSIP code.
     */
    void setCUSIP(const std::string &cusip) noexcept {
        data_.cusip = cusip;
    }

    /**
     * Changes Committee on Uniform Security Identification Procedures code.
     * It shall use nine-letter code assigned by CUSIP Services Bureau.
     * See <a href="http://en.wikipedia.org/wiki/CUSIP">CUSIP on Wikipedia</a>.
     * Example: "38259P508".
     *
     * @param cusip CUSIP code
     * @return The current instrument profile.
     */
    InstrumentProfile &withCUSIP(const std::string &cusip) noexcept {
        InstrumentProfile::setCUSIP(cusip);

        return *this;
    }

    /**
     * Changes Committee on Uniform Security Identification Procedures code.
     * It shall use nine-letter code assigned by CUSIP Services Bureau.
     * See <a href="http://en.wikipedia.org/wiki/CUSIP">CUSIP on Wikipedia</a>.
     * Example: "38259P508".
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<InstrumentProfile>(new
     * InstrumentProfile(...))` or `std::make_shared<InstrumentProfile>(...)`
     *
     * @param cusip CUSIP code
     * @return A shared pointer to the current instrument profile.
     */
    InstrumentProfile::Ptr withCUSIPShared(const std::string &cusip) noexcept {
        InstrumentProfile::setCUSIP(cusip);

        return shared_from_this()->sharedAs<InstrumentProfile>();
    }

    /**
     * Returns Industry Classification Benchmark.
     * It shall use four-digit number from ICB catalog.
     * See <a href="http://en.wikipedia.org/wiki/Industry_Classification_Benchmark">ICB on Wikipedia</a>
     * or <a href="http://www.icbenchmark.com/">ICB homepage</a>.
     * Example: "9535".
     *
     * @return Industry Classification Benchmark.
     */
    std::int32_t getICB() const noexcept {
        return data_.icb;
    }

    /**
     * Changes Industry Classification Benchmark.
     * It shall use four-digit number from ICB catalog.
     * See <a href="http://en.wikipedia.org/wiki/Industry_Classification_Benchmark">ICB on Wikipedia</a>
     * or <a href="http://www.icbenchmark.com/">ICB homepage</a>.
     * Example: "9535".
     *
     * @param icb Industry Classification Benchmark.
     */
    void setICB(std::int32_t icb) noexcept {
        data_.icb = icb;
    }

    /**
     * Changes Industry Classification Benchmark.
     * It shall use four-digit number from ICB catalog.
     * See <a href="http://en.wikipedia.org/wiki/Industry_Classification_Benchmark">ICB on Wikipedia</a>
     * or <a href="http://www.icbenchmark.com/">ICB homepage</a>.
     * Example: "9535".
     *
     * @param icb Industry Classification Benchmark.
     * @return The current instrument profile.
     */
    InstrumentProfile &withICB(std::int32_t icb) noexcept {
        InstrumentProfile::setICB(icb);

        return *this;
    }

    /**
     * Changes Industry Classification Benchmark.
     * It shall use four-digit number from ICB catalog.
     * See <a href="http://en.wikipedia.org/wiki/Industry_Classification_Benchmark">ICB on Wikipedia</a>
     * or <a href="http://www.icbenchmark.com/">ICB homepage</a>.
     * Example: "9535".
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<InstrumentProfile>(new
     * InstrumentProfile(...))` or `std::make_shared<InstrumentProfile>(...)`
     *
     * @param icb Industry Classification Benchmark.
     * @return A shared pointer to the current instrument profile.
     */
    InstrumentProfile::Ptr withICBShared(std::int32_t icb) noexcept {
        InstrumentProfile::setICB(icb);

        return shared_from_this()->sharedAs<InstrumentProfile>();
    }

    /**
     * Returns Standard Industrial Classification.
     * It shall use four-digit number from SIC catalog.
     * See <a href="http://en.wikipedia.org/wiki/Standard_Industrial_Classification">SIC on Wikipedia</a>
     * or <a href="https://www.osha.gov/pls/imis/sic_manual.html">SIC structure</a>.
     * Example: "7371".
     *
     * @return Standard Industrial Classification.
     */
    std::int32_t getSIC() const noexcept {
        return data_.sic;
    }

    /**
     * Changes Standard Industrial Classification.
     * It shall use four-digit number from SIC catalog.
     * See <a href="http://en.wikipedia.org/wiki/Standard_Industrial_Classification">SIC on Wikipedia</a>
     * or <a href="https://www.osha.gov/pls/imis/sic_manual.html">SIC structure</a>.
     * Example: "7371".
     *
     * @param sic Standard Industrial Classification.
     */
    void setSIC(std::int32_t sic) noexcept {
        data_.sic = sic;
    }

    /**
     * Changes Standard Industrial Classification.
     * It shall use four-digit number from SIC catalog.
     * See <a href="http://en.wikipedia.org/wiki/Standard_Industrial_Classification">SIC on Wikipedia</a>
     * or <a href="https://www.osha.gov/pls/imis/sic_manual.html">SIC structure</a>.
     * Example: "7371".
     *
     * @param sic Standard Industrial Classification.
     * @return The current instrument profile.
     */
    InstrumentProfile &withSIC(std::int32_t sic) noexcept {
        InstrumentProfile::setSIC(sic);

        return *this;
    }

    /**
     * Changes Standard Industrial Classification.
     * It shall use four-digit number from SIC catalog.
     * See <a href="http://en.wikipedia.org/wiki/Standard_Industrial_Classification">SIC on Wikipedia</a>
     * or <a href="https://www.osha.gov/pls/imis/sic_manual.html">SIC structure</a>.
     * Example: "7371".
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<InstrumentProfile>(new
     * InstrumentProfile(...))` or `std::make_shared<InstrumentProfile>(...)`
     *
     * @param sic Standard Industrial Classification.
     * @return A shared pointer to the current instrument profile.
     */
    InstrumentProfile::Ptr withSICShared(std::int32_t sic) noexcept {
        InstrumentProfile::setSIC(sic);

        return shared_from_this()->sharedAs<InstrumentProfile>();
    }

    /**
     * Returns market value multiplier.
     * Example: 100, 33.2.
     *
     * @return The market value multiplier.
     */
    double getMultiplier() const noexcept {
        return data_.multiplier;
    }

    /**
     * Changes market value multiplier.
     * Example: 100, 33.2.
     *
     * @param multiplier The market value multiplier.
     */
    void setMultiplier(double multiplier) noexcept {
        data_.multiplier = multiplier;
    }

    /**
     * Changes market value multiplier.
     * Example: 100, 33.2.
     *
     * @param multiplier The market value multiplier.
     * @return The current instrument profile.
     */
    InstrumentProfile &withMultiplier(double multiplier) noexcept {
        InstrumentProfile::setMultiplier(multiplier);

        return *this;
    }

    /**
     * Changes market value multiplier.
     * Example: 100, 33.2.
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<InstrumentProfile>(new
     * InstrumentProfile(...))` or `std::make_shared<InstrumentProfile>(...)`
     *
     * @param multiplier The market value multiplier.
     * @return A shared pointer to the current instrument profile.
     */
    InstrumentProfile::Ptr withMultiplierShared(double multiplier) noexcept {
        InstrumentProfile::setMultiplier(multiplier);

        return shared_from_this()->sharedAs<InstrumentProfile>();
    }

    /**
     * Returns product for futures and options on futures (underlying asset name).
     * Example: "/YG".
     *
     * @return The product for futures and options on futures (underlying asset name).
     */
    const std::string &getProduct() const & noexcept {
        return data_.product;
    }

    /**
     * Changes product for futures and options on futures (underlying asset name).
     * Example: "/YG".
     *
     * @param product The product for futures and options on futures (underlying asset name).
     */
    void setProduct(const std::string &product) noexcept {
        data_.product = product;
    }

    /**
     * Changes product for futures and options on futures (underlying asset name).
     * Example: "/YG".
     *
     * @param product The product for futures and options on futures (underlying asset name).
     * @return The current instrument profile.
     */
    InstrumentProfile &withProduct(const std::string &product) noexcept {
        InstrumentProfile::setProduct(product);

        return *this;
    }

    /**
     * Changes product for futures and options on futures (underlying asset name).
     * Example: "/YG".
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<InstrumentProfile>(new
     * InstrumentProfile(...))` or `std::make_shared<InstrumentProfile>(...)`
     *
     * @param product The product for futures and options on futures (underlying asset name).
     * @return A shared pointer to the current instrument profile.
     */
    InstrumentProfile::Ptr withProductShared(const std::string &product) noexcept {
        InstrumentProfile::setProduct(product);

        return shared_from_this()->sharedAs<InstrumentProfile>();
    }

    /**
     * Returns primary underlying symbol for options.
     * Example: "C", "/YGM9"
     *
     * @return The primary underlying symbol for options.
     */
    const std::string &getUnderlying() const & noexcept {
        return data_.underlying;
    }

    /**
     * Changes primary underlying symbol for options.
     * Example: "C", "/YGM9"
     *
     * @param underlying The primary underlying symbol for options.
     */
    void setUnderlying(const std::string &underlying) noexcept {
        data_.underlying = underlying;
    }

    /**
     * Changes primary underlying symbol for options.
     * Example: "C", "/YGM9"
     *
     * @param underlying The primary underlying symbol for options.
     * @return The current instrument profile.
     */
    InstrumentProfile &withUnderlying(const std::string &underlying) noexcept {
        InstrumentProfile::setUnderlying(underlying);

        return *this;
    }

    /**
     * Changes primary underlying symbol for options.
     * Example: "C", "/YGM9"
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<InstrumentProfile>(new
     * InstrumentProfile(...))` or `std::make_shared<InstrumentProfile>(...)`
     *
     * @param underlying The primary underlying symbol for options.
     * @return A shared pointer to the current instrument profile.
     */
    InstrumentProfile::Ptr withUnderlyingShared(const std::string &underlying) noexcept {
        InstrumentProfile::setUnderlying(underlying);

        return shared_from_this()->sharedAs<InstrumentProfile>();
    }

    /**
     * Returns shares per contract for options.
     * Example: 1, 100.
     *
     * @return shares per contract for options.
     */
    double getSPC() const noexcept {
        return data_.spc;
    }

    /**
     * Changes shares per contract for options.
     * Example: 1, 100.
     *
     * @param spc The shares per contract for options.
     */
    void setSPC(double spc) noexcept {
        data_.spc = spc;
    }

    /**
     * Changes shares per contract for options.
     * Example: 1, 100.
     *
     * @param spc The shares per contract for options.
     * @return The current instrument profile.
     */
    InstrumentProfile &withSPC(double spc) noexcept {
        InstrumentProfile::setSPC(spc);

        return *this;
    }

    /**
     * Changes shares per contract for options.
     * Example: 1, 100.
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<InstrumentProfile>(new
     * InstrumentProfile(...))` or `std::make_shared<InstrumentProfile>(...)`
     *
     * @param spc The shares per contract for options.
     * @return A shared pointer to the current instrument profile.
     */
    InstrumentProfile::Ptr withSPCShared(double spc) noexcept {
        InstrumentProfile::setSPC(spc);

        return shared_from_this()->sharedAs<InstrumentProfile>();
    }

    /**
     * Returns additional underlyings for options, including additional cash.
     * It shall use following format:
     * ```
     *     <VALUE> ::= <empty> | <LIST>
     *     <LIST> ::= <AU> | <AU> <semicolon> <space> <LIST>
     *     <AU> ::= <UNDERLYING> <space> <SPC>
     * the list shall be sorted by <UNDERLYING>.
     * ```
     * Example: "SE 50", "FIS 53; US$ 45.46".
     *
     * @return The additional underlyings for options, including additional cash.
     */
    const std::string &getAdditionalUnderlyings() const & noexcept {
        return data_.additionalUnderlyings;
    }

    /**
     * Changes additional underlyings for options, including additional cash.
     * It shall use following format:
     * ```
     *     <VALUE> ::= <empty> | <LIST>
     *     <LIST> ::= <AU> | <AU> <semicolon> <space> <LIST>
     *     <AU> ::= <UNDERLYING> <space> <SPC>
     * the list shall be sorted by <UNDERLYING>.
     * ```
     * Example: "SE 50", "FIS 53; US$ 45.46".
     *
     * @param additionalUnderlyings The additional underlyings for options, including additional cash.
     */
    void setAdditionalUnderlyings(const std::string &additionalUnderlyings) noexcept {
        data_.additionalUnderlyings = additionalUnderlyings;
    }

    /**
     * Changes additional underlyings for options, including additional cash.
     * It shall use following format:
     * ```
     *     <VALUE> ::= <empty> | <LIST>
     *     <LIST> ::= <AU> | <AU> <semicolon> <space> <LIST>
     *     <AU> ::= <UNDERLYING> <space> <SPC>
     * the list shall be sorted by <UNDERLYING>.
     * ```
     * Example: "SE 50", "FIS 53; US$ 45.46".
     *
     * @param additionalUnderlyings The additional underlyings for options, including additional cash.
     * @return The current instrument profile.
     */
    InstrumentProfile &withAdditionalUnderlyings(const std::string &additionalUnderlyings) noexcept {
        InstrumentProfile::setAdditionalUnderlyings(additionalUnderlyings);

        return *this;
    }

    /**
     * Changes additional underlyings for options, including additional cash.
     * It shall use following format:
     * ```
     *     <VALUE> ::= <empty> | <LIST>
     *     <LIST> ::= <AU> | <AU> <semicolon> <space> <LIST>
     *     <AU> ::= <UNDERLYING> <space> <SPC>
     * the list shall be sorted by <UNDERLYING>.
     * ```
     * Example: "SE 50", "FIS 53; US$ 45.46".
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<InstrumentProfile>(new
     * InstrumentProfile(...))` or `std::make_shared<InstrumentProfile>(...)`
     *
     * @param additionalUnderlyings The additional underlyings for options, including additional cash.
     * @return A shared pointer to the current instrument profile.
     */
    InstrumentProfile::Ptr withAdditionalUnderlyingsShared(const std::string &additionalUnderlyings) noexcept {
        InstrumentProfile::setAdditionalUnderlyings(additionalUnderlyings);

        return shared_from_this()->sharedAs<InstrumentProfile>();
    }

    /**
     * Returns maturity month-year as provided for corresponding FIX tag (200).
     * It can use several different formats depending on data source:
     * <ul>
     * <li>YYYYMM – if only year and month are specified
     * <li>YYYYMMDD – if full date is specified
     * <li>YYYYMMwN – if week number (within a month) is specified
     * </ul>
     *
     * @return The maturity month-year as provided for corresponding FIX tag (200).
     */
    const std::string &getMMY() const & noexcept {
        return data_.mmy;
    }

    /**
     * Changes maturity month-year as provided for corresponding FIX tag (200).
     * It can use several different formats depending on data source:
     * <ul>
     * <li>YYYYMM – if only year and month are specified
     * <li>YYYYMMDD – if full date is specified
     * <li>YYYYMMwN – if week number (within a month) is specified
     * </ul>
     *
     * @param mmy The maturity month-year as provided for corresponding FIX tag (200).
     */
    void setMMY(const std::string &mmy) noexcept {
        data_.mmy = mmy;
    }

    /**
     * Changes maturity month-year as provided for corresponding FIX tag (200).
     * It can use several different formats depending on data source:
     * <ul>
     * <li>YYYYMM – if only year and month are specified
     * <li>YYYYMMDD – if full date is specified
     * <li>YYYYMMwN – if week number (within a month) is specified
     * </ul>
     *
     * @param mmy The maturity month-year as provided for corresponding FIX tag (200).
     * @return The current instrument profile.
     */
    InstrumentProfile &withMMY(const std::string &mmy) noexcept {
        InstrumentProfile::setMMY(mmy);

        return *this;
    }

    /**
     * Changes maturity month-year as provided for corresponding FIX tag (200).
     * It can use several different formats depending on data source:
     * <ul>
     * <li>YYYYMM – if only year and month are specified
     * <li>YYYYMMDD – if full date is specified
     * <li>YYYYMMwN – if week number (within a month) is specified
     * </ul>
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<InstrumentProfile>(new
     * InstrumentProfile(...))` or `std::make_shared<InstrumentProfile>(...)`
     *
     * @param mmy The maturity month-year as provided for corresponding FIX tag (200).
     * @return A shared pointer to the current instrument profile.
     */
    InstrumentProfile::Ptr withMMYShared(const std::string &mmy) noexcept {
        InstrumentProfile::setMMY(mmy);

        return shared_from_this()->sharedAs<InstrumentProfile>();
    }

    /**
     * Returns day id of expiration.
     * Example: @ref day_util::#getDayIdByYearMonthDay() "dxfcpp::day_util::getDayIdByYearMonthDay"(20090117).
     *
     * @return The day id of expiration.
     */
    std::int32_t getExpiration() const noexcept {
        return data_.expiration;
    }

    /**
     * Changes day id of expiration.
     * Example: @ref day_util::#getDayIdByYearMonthDay() "dxfcpp::day_util::getDayIdByYearMonthDay"(20090117).
     *
     * @param expiration The day id of expiration.
     */
    void setExpiration(std::int32_t expiration) noexcept {
        data_.expiration = expiration;
    }

    /**
     * Changes day id of expiration.
     * Example: @ref day_util::#getDayIdByYearMonthDay() "dxfcpp::day_util::getDayIdByYearMonthDay"(20090117).
     *
     * @param expiration The day id of expiration.
     * @return The current instrument profile.
     */
    InstrumentProfile &withExpiration(std::int32_t expiration) noexcept {
        InstrumentProfile::setExpiration(expiration);

        return *this;
    }

    /**
     * Changes day id of expiration.
     * Example: @ref day_util::#getDayIdByYearMonthDay() "dxfcpp::day_util::getDayIdByYearMonthDay"(20090117).
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<InstrumentProfile>(new
     * InstrumentProfile(...))` or `std::make_shared<InstrumentProfile>(...)`
     *
     * @param expiration The day id of expiration.
     * @return A shared pointer to the current instrument profile.
     */
    InstrumentProfile::Ptr withExpirationShared(std::int32_t expiration) noexcept {
        InstrumentProfile::setExpiration(expiration);

        return shared_from_this()->sharedAs<InstrumentProfile>();
    }

    /**
     * Returns day id of last trading day.
     * Example: @ref day_util::#getDayIdByYearMonthDay() "dxfcpp::day_util::getDayIdByYearMonthDay"(20090116).
     *
     * @return The day id of last trading day.
     */
    std::int32_t getLastTrade() const noexcept {
        return data_.lastTrade;
    }

    /**
     * Changes day id of last trading day.
     * Example: @ref day_util::#getDayIdByYearMonthDay() "dxfcpp::day_util::getDayIdByYearMonthDay"(20090116).
     *
     * @param lastTrade The day id of last trading day.
     */
    void setLastTrade(std::int32_t lastTrade) noexcept {
        data_.lastTrade = lastTrade;
    }

    /**
     * Changes day id of last trading day.
     * Example: @ref day_util::#getDayIdByYearMonthDay() "dxfcpp::day_util::getDayIdByYearMonthDay"(20090116).
     *
     * @param lastTrade The day id of last trading day.
     * @return The current instrument profile.
     */
    InstrumentProfile &withLastTrade(std::int32_t lastTrade) noexcept {
        InstrumentProfile::setLastTrade(lastTrade);

        return *this;
    }

    /**
     * Changes day id of last trading day.
     * Example: @ref day_util::#getDayIdByYearMonthDay() "dxfcpp::day_util::getDayIdByYearMonthDay"(20090116).
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<InstrumentProfile>(new
     * InstrumentProfile(...))` or `std::make_shared<InstrumentProfile>(...)`
     *
     * @param lastTrade The day id of last trading day.
     * @return A shared pointer to the current instrument profile.
     */
    InstrumentProfile::Ptr withLastTradeShared(std::int32_t lastTrade) noexcept {
        InstrumentProfile::setLastTrade(lastTrade);

        return shared_from_this()->sharedAs<InstrumentProfile>();
    }

    /**
     * Returns strike price for options.
     * Example: 80, 22.5.
     *
     * @return The strike price for options.
     */
    double getStrike() const noexcept {
        return data_.strike;
    }

    /**
     * Changes strike price for options.
     * Example: 80, 22.5.
     *
     * @param strike The strike price for options.
     */
    void setStrike(double strike) noexcept {
        data_.strike = strike;
    }

    /**
     * Changes strike price for options.
     * Example: 80, 22.5.
     *
     * @param strike The strike price for options
     * @return The current instrument profile.
     */
    InstrumentProfile &withStrike(double strike) noexcept {
        InstrumentProfile::setStrike(strike);

        return *this;
    }

    /**
     * Changes strike price for options.
     * Example: 80, 22.5.
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<InstrumentProfile>(new
     * InstrumentProfile(...))` or `std::make_shared<InstrumentProfile>(...)`
     *
     * @param strike The strike price for options
     * @return A shared pointer to the current instrument profile.
     */
    InstrumentProfile::Ptr withStrikeShared(double strike) noexcept {
        InstrumentProfile::setStrike(strike);

        return shared_from_this()->sharedAs<InstrumentProfile>();
    }

    /**
     * Returns type of option.
     * It shall use one of following values:
     * <ul>
     * <li>STAN = Standard Options
     * <li>LEAP = Long-term Equity AnticiPation Securities
     * <li>SDO = Special Dated Options
     * <li>BINY = Binary Options
     * <li>FLEX = FLexible EXchange Options
     * <li>VSO = Variable Start Options
     * <li>RNGE = Range
     * </ul>
     *
     * @return The type of option.
     */
    const std::string &getOptionType() const & noexcept {
        return data_.optionType;
    }

    /**
     * Changes type of option.
     * It shall use one of following values:
     * <ul>
     * <li>STAN = Standard Options
     * <li>LEAP = Long-term Equity AnticiPation Securities
     * <li>SDO = Special Dated Options
     * <li>BINY = Binary Options
     * <li>FLEX = FLexible EXchange Options
     * <li>VSO = Variable Start Options
     * <li>RNGE = Range
     * </ul>
     *
     * @param optionType The type of option.
     */
    void setOptionType(const std::string &optionType) noexcept {
        data_.optionType = optionType;
    }

    /**
     * Changes type of option.
     * It shall use one of following values:
     * <ul>
     * <li>STAN = Standard Options
     * <li>LEAP = Long-term Equity AnticiPation Securities
     * <li>SDO = Special Dated Options
     * <li>BINY = Binary Options
     * <li>FLEX = FLexible EXchange Options
     * <li>VSO = Variable Start Options
     * <li>RNGE = Range
     * </ul>
     *
     * @param optionType The type of option.
     * @return The current instrument profile.
     */
    InstrumentProfile &withOptionType(const std::string &optionType) noexcept {
        InstrumentProfile::setOptionType(optionType);

        return *this;
    }

    /**
     * Changes type of option.
     * It shall use one of following values:
     * <ul>
     * <li>STAN = Standard Options
     * <li>LEAP = Long-term Equity AnticiPation Securities
     * <li>SDO = Special Dated Options
     * <li>BINY = Binary Options
     * <li>FLEX = FLexible EXchange Options
     * <li>VSO = Variable Start Options
     * <li>RNGE = Range
     * </ul>
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<InstrumentProfile>(new
     * InstrumentProfile(...))` or `std::make_shared<InstrumentProfile>(...)`
     *
     * @param optionType The type of option.
     * @return A shared pointer to the current instrument profile.
     */
    InstrumentProfile::Ptr withOptionTypeShared(const std::string &optionType) noexcept {
        InstrumentProfile::setOptionType(optionType);

        return shared_from_this()->sharedAs<InstrumentProfile>();
    }

    /**
     * Returns expiration cycle style, such as "Weeklys", "Quarterlys".
     *
     * @return The expiration cycle style.
     */
    const std::string &getExpirationStyle() const & noexcept {
        return data_.expirationStyle;
    }

    /**
     * Changes the expiration cycle style, such as "Weeklys", "Quarterlys".
     *
     * @param expirationStyle The expiration cycle style.
     */
    void setExpirationStyle(const std::string &expirationStyle) noexcept {
        data_.expirationStyle = expirationStyle;
    }

    /**
     * Changes the expiration cycle style, such as "Weeklys", "Quarterlys".
     *
     * @param expirationStyle The expiration cycle style.
     * @return The current instrument profile.
     */
    InstrumentProfile &withExpirationStyle(const std::string &expirationStyle) noexcept {
        InstrumentProfile::setExpirationStyle(expirationStyle);

        return *this;
    }

    /**
     * Changes the expiration cycle style, such as "Weeklys", "Quarterlys".
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<InstrumentProfile>(new
     * InstrumentProfile(...))` or `std::make_shared<InstrumentProfile>(...)`
     *
     * @param expirationStyle The expiration cycle style.
     * @return A shared pointer to the current instrument profile.
     */
    InstrumentProfile::Ptr withExpirationStyleShared(const std::string &expirationStyle) noexcept {
        InstrumentProfile::setExpirationStyle(expirationStyle);

        return shared_from_this()->sharedAs<InstrumentProfile>();
    }

    /**
     * Returns settlement price determination style, such as "Open", "Close".
     *
     * @return The settlement price determination style.
     */
    const std::string &getSettlementStyle() const & noexcept {
        return data_.settlementStyle;
    }

    /**
     * Changes settlement price determination style, such as "Open", "Close".
     *
     * @param settlementStyle The settlement price determination style.
     */
    void setSettlementStyle(const std::string &settlementStyle) noexcept {
        data_.settlementStyle = settlementStyle;
    }

    /**
     * Changes settlement price determination style, such as "Open", "Close".
     *
     * @param settlementStyle The settlement price determination style.
     * @return The current instrument profile.
     */
    InstrumentProfile &withSettlementStyle(const std::string &settlementStyle) noexcept {
        InstrumentProfile::setSettlementStyle(settlementStyle);

        return *this;
    }

    /**
     * Changes settlement price determination style, such as "Open", "Close".
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<InstrumentProfile>(new
     * InstrumentProfile(...))` or `std::make_shared<InstrumentProfile>(...)`
     *
     * @param settlementStyle The settlement price determination style.
     * @return A shared pointer to the current instrument profile.
     */
    InstrumentProfile::Ptr withSettlementStyleShared(const std::string &settlementStyle) noexcept {
        InstrumentProfile::setSettlementStyle(settlementStyle);

        return shared_from_this()->sharedAs<InstrumentProfile>();
    }

    /**
     * Returns minimum allowed price increments with corresponding price ranges.
     * It shall use following format:
     * ```
     *     <VALUE> ::= <empty> | <LIST>
     *     <LIST> ::= <INCREMENT> | <RANGE> <semicolon> <space> <LIST>
     *     <RANGE> ::= <INCREMENT> <space> <UPPER_LIMIT>
     * the list shall be sorted by <UPPER_LIMIT>.
     * ```
     * Example: "0.25", "0.01 3; 0.05".
     *
     * @return The minimum allowed price increments with corresponding price ranges.
     */
    const std::string &getPriceIncrements() const & noexcept {
        return data_.priceIncrements;
    }

    /**
     * Changes minimum allowed price increments with corresponding price ranges.
     * It shall use following format:
     * ```
     *     <VALUE> ::= <empty> | <LIST>
     *     <LIST> ::= <INCREMENT> | <RANGE> <semicolon> <space> <LIST>
     *     <RANGE> ::= <INCREMENT> <space> <UPPER_LIMIT>
     * the list shall be sorted by <UPPER_LIMIT>.
     * ```
     * Example: "0.25", "0.01 3; 0.05".
     *
     * @param priceIncrements The minimum allowed price increments with corresponding price ranges.
     */
    void setPriceIncrements(const std::string &priceIncrements) noexcept {
        data_.priceIncrements = priceIncrements;
    }

    /**
     * Changes minimum allowed price increments with corresponding price ranges.
     * It shall use following format:
     * ```
     *     <VALUE> ::= <empty> | <LIST>
     *     <LIST> ::= <INCREMENT> | <RANGE> <semicolon> <space> <LIST>
     *     <RANGE> ::= <INCREMENT> <space> <UPPER_LIMIT>
     * the list shall be sorted by <UPPER_LIMIT>.
     * ```
     * Example: "0.25", "0.01 3; 0.05".
     *
     * @param priceIncrements The minimum allowed price increments with corresponding price ranges.
     * @return The current instrument profile.
     */
    InstrumentProfile &withPriceIncrements(const std::string &priceIncrements) noexcept {
        InstrumentProfile::setPriceIncrements(priceIncrements);

        return *this;
    }

    /**
     * Changes minimum allowed price increments with corresponding price ranges.
     * It shall use following format:
     * ```
     *     <VALUE> ::= <empty> | <LIST>
     *     <LIST> ::= <INCREMENT> | <RANGE> <semicolon> <space> <LIST>
     *     <RANGE> ::= <INCREMENT> <space> <UPPER_LIMIT>
     * the list shall be sorted by <UPPER_LIMIT>.
     * ```
     * Example: "0.25", "0.01 3; 0.05".
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<InstrumentProfile>(new
     * InstrumentProfile(...))` or `std::make_shared<InstrumentProfile>(...)`
     *
     * @param priceIncrements The minimum allowed price increments with corresponding price ranges.
     * @return A shared pointer to the current instrument profile.
     */
    InstrumentProfile::Ptr withPriceIncrementsShared(const std::string &priceIncrements) noexcept {
        InstrumentProfile::setPriceIncrements(priceIncrements);

        return shared_from_this()->sharedAs<InstrumentProfile>();
    }

    /**
     * Returns trading hours specification.
     * See Schedule::getInstance().
     *
     * @return The trading hours specification.
     */
    const std::string &getTradingHours() const & noexcept {
        return data_.tradingHours;
    }

    /**
     * Changes trading hours specification.
     * See Schedule::getInstance().
     *
     * @param tradingHours The trading hours specification.
     */
    void setTradingHours(const std::string &tradingHours) noexcept {
        data_.tradingHours = tradingHours;
    }

    /**
     * Changes trading hours specification.
     * See Schedule::getInstance().
     *
     * @param tradingHours The trading hours specification.
     * @return The current instrument profile.
     */
    InstrumentProfile &withTradingHours(const std::string &tradingHours) noexcept {
        InstrumentProfile::setTradingHours(tradingHours);

        return *this;
    }

    /**
     * Changes trading hours specification.
     * See Schedule::getInstance().
     *
     * @warning Please do not use this method unless the object was created with `std::shared_ptr<InstrumentProfile>(new
     * InstrumentProfile(...))` or `std::make_shared<InstrumentProfile>(...)`
     *
     * @param tradingHours The trading hours specification.
     * @return A shared pointer to the current instrument profile.
     */
    InstrumentProfile::Ptr withTradingHoursShared(const std::string &tradingHours) noexcept {
        InstrumentProfile::setTradingHours(tradingHours);

        return shared_from_this()->sharedAs<InstrumentProfile>();
    }
};

} // namespace dxfcpp
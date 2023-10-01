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

    //    /**
    //     * Returns currency of quotation, pricing and trading.
    //     * It shall use three-letter currency code from ISO 4217 standard.
    //     * See <a href="http://en.wikipedia.org/wiki/ISO_4217">ISO 4217 on Wikipedia</a>.
    //     * Example: "USD", "RUB".
    //     * @return currency of quotation, pricing and trading.
    //     */
    //  public String getCurrency() {
    //        return currency;
    //    }
    //
    //    /**
    //     * Changes currency of quotation, pricing and trading.
    //     * It shall use three-letter currency code from ISO 4217 standard.
    //     * See <a href="http://en.wikipedia.org/wiki/ISO_4217">ISO 4217 on Wikipedia</a>.
    //     * Example: "USD", "RUB".
    //     * @param currency currency of quotation, pricing and trading.
    //     */
    //  public void setCurrency(String currency) {
    //        this.currency = currency == null || currency.isEmpty() ? "" : currency;
    //    }
    //
    //    /**
    //     * Returns base currency of currency pair (FOREX instruments).
    //     * It shall use three-letter currency code similarly to {@link #getCurrency currency}.
    //     * @return base currency of currency pair (FOREX instruments).
    //     */
    //  public String getBaseCurrency() {
    //        return baseCurrency;
    //    }
    //
    //    /**
    //     * Changes base currency of currency pair (FOREX instruments).
    //     * It shall use three-letter currency code similarly to {@link #setCurrency currency}.
    //     * @param baseCurrency base currency of currency pair (FOREX instruments).
    //     */
    //  public void setBaseCurrency(String baseCurrency) {
    //        this.baseCurrency = baseCurrency == null || baseCurrency.isEmpty() ? "" : baseCurrency;
    //    }
    //
    //    /**
    //     * Returns Classification of Financial Instruments code.
    //     * It is a mandatory field for OPTION instruments as it is the only way to distinguish Call/Put type,
    //     * American/European exercise, Cash/Physical delivery.
    //     * It shall use six-letter CFI code from ISO 10962 standard.
    //     * It is allowed to use 'X' extensively and to omit trailing letters (assumed to be 'X').
    //     * See <a href="http://en.wikipedia.org/wiki/ISO_10962">ISO 10962 on Wikipedia</a>.
    //     * Example: "ESNTPB", "ESXXXX", "ES" , "OPASPS".
    //     * @return CFI code.
    //     */
    //  public String getCFI() {
    //        return cfi;
    //    }
    //
    //    /**
    //     * Changes Classification of Financial Instruments code.
    //     * It is a mandatory field for OPTION instruments as it is the only way to distinguish Call/Put type,
    //     * American/European exercise, Cash/Physical delivery.
    //     * It shall use six-letter CFI code from ISO 10962 standard.
    //     * It is allowed to use 'X' extensively and to omit trailing letters (assumed to be 'X').
    //     * See <a href="http://en.wikipedia.org/wiki/ISO_10962">ISO 10962 on Wikipedia</a>.
    //     * Example: "ESNTPB", "ESXXXX", "ES" , "OPASPS".
    //     * @param cfi CFI code.
    //     */
    //  public void setCFI(String cfi) {
    //        this.cfi = cfi == null || cfi.isEmpty() ? "" : cfi;
    //    }
    //
    //    /**
    //     * Returns International Securities Identifying Number.
    //     * It shall use twelve-letter code from ISO 6166 standard.
    //     * See <a href="http://en.wikipedia.org/wiki/ISO_6166">ISO 6166 on Wikipedia</a>
    //     * or <a href="http://en.wikipedia.org/wiki/International_Securities_Identifying_Number">ISIN on
    //     Wikipedia</a>.
    //     * Example: "DE0007100000", "US38259P5089".
    //     * @return International Securities Identifying Number.
    //     */
    //  public String getISIN() {
    //        return isin;
    //    }
    //
    //    /**
    //     * Changes International Securities Identifying Number.
    //     * It shall use twelve-letter code from ISO 6166 standard.
    //     * See <a href="http://en.wikipedia.org/wiki/ISO_6166">ISO 6166 on Wikipedia</a>
    //     * or <a href="http://en.wikipedia.org/wiki/International_Securities_Identifying_Number">ISIN on
    //     Wikipedia</a>.
    //     * Example: "DE0007100000", "US38259P5089".
    //     * @param isin International Securities Identifying Number.
    //     */
    //  public void setISIN(String isin) {
    //        this.isin = isin == null || isin.isEmpty() ? "" : isin;
    //    }
    //
    //    /**
    //     * Returns Stock Exchange Daily Official List.
    //     * It shall use seven-letter code assigned by London Stock Exchange.
    //     * See <a href="http://en.wikipedia.org/wiki/SEDOL">SEDOL on Wikipedia</a> or
    //     * <a href="http://www.londonstockexchange.com/en-gb/products/informationproducts/sedol/">SEDOL on LSE</a>.
    //     * Example: "2310967", "5766857".
    //     * @return Stock Exchange Daily Official List.
    //     */
    //  public String getSEDOL() {
    //        return sedol;
    //    }
    //
    //    /**
    //     * Changes Stock Exchange Daily Official List.
    //     * It shall use seven-letter code assigned by London Stock Exchange.
    //     * See <a href="http://en.wikipedia.org/wiki/SEDOL">SEDOL on Wikipedia</a> or
    //     * <a href="http://www.londonstockexchange.com/en-gb/products/informationproducts/sedol/">SEDOL on LSE</a>.
    //     * Example: "2310967", "5766857".
    //     * @param sedol Stock Exchange Daily Official List.
    //     */
    //  public void setSEDOL(String sedol) {
    //        this.sedol = sedol == null || sedol.isEmpty() ? "" : sedol;
    //    }
    //
    //    /**
    //     * Returns Committee on Uniform Security Identification Procedures code.
    //     * It shall use nine-letter code assigned by CUSIP Services Bureau.
    //     * See <a href="http://en.wikipedia.org/wiki/CUSIP">CUSIP on Wikipedia</a>.
    //     * Example: "38259P508".
    //     * @return CUSIP code.
    //     */
    //  public String getCUSIP() {
    //        return cusip;
    //    }
    //
    //    /**
    //     * Changes Committee on Uniform Security Identification Procedures code.
    //     * It shall use nine-letter code assigned by CUSIP Services Bureau.
    //     * See <a href="http://en.wikipedia.org/wiki/CUSIP">CUSIP on Wikipedia</a>.
    //     * Example: "38259P508".
    //     * @param cusip CUSIP code.
    //     */
    //  public void setCUSIP(String cusip) {
    //        this.cusip = cusip == null || cusip.isEmpty() ? "" : cusip;
    //    }
    //
    //    /**
    //     * Returns Industry Classification Benchmark.
    //     * It shall use four-digit number from ICB catalog.
    //     * See <a href="http://en.wikipedia.org/wiki/Industry_Classification_Benchmark">ICB on Wikipedia</a>
    //     * or <a href="http://www.icbenchmark.com/">ICB homepage</a>.
    //     * Example: "9535".
    //     * @return Industry Classification Benchmark.
    //     */
    //  public int getICB() {
    //        return icb;
    //    }
    //
    //    /**
    //     * Changes Industry Classification Benchmark.
    //     * It shall use four-digit number from ICB catalog.
    //     * See <a href="http://en.wikipedia.org/wiki/Industry_Classification_Benchmark">ICB on Wikipedia</a>
    //     * or <a href="http://www.icbenchmark.com/">ICB homepage</a>.
    //     * Example: "9535".
    //     * @param icb Industry Classification Benchmark.
    //     */
    //  public void setICB(int icb) {
    //        this.icb = icb;
    //    }
    //

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

    //
    //    /**
    //     * Returns market value multiplier.
    //     * Example: 100, 33.2.
    //     * @return market value multiplier.
    //     */
    //  public double getMultiplier() {
    //        return multiplier;
    //    }
    //
    //    /**
    //     * Changes market value multiplier.
    //     * Example: 100, 33.2.
    //     * @param multiplier market value multiplier.
    //     */
    //  public void setMultiplier(double multiplier) {
    //        this.multiplier = multiplier;
    //    }
    //
    //    /**
    //     * Returns product for futures and options on futures (underlying asset name).
    //     * Example: "/YG".
    //     * @return product for futures and options on futures (underlying asset name).
    //     */
    //  public String getProduct() {
    //        return product;
    //    }
    //
    //    /**
    //     * Changes product for futures and options on futures (underlying asset name).
    //     * Example: "/YG".
    //     * @param product product for futures and options on futures (underlying asset name).
    //     */
    //  public void setProduct(String product) {
    //        this.product = product == null || product.isEmpty() ? "" : product;
    //    }
    //
    //    /**
    //     * Returns primary underlying symbol for options.
    //     * Example: "C", "/YGM9"
    //     * @return primary underlying symbol for options.
    //     */
    //  public String getUnderlying() {
    //        return underlying;
    //    }
    //
    //    /**
    //     * Changes primary underlying symbol for options.
    //     * Example: "C", "/YGM9"
    //     * @param underlying primary underlying symbol for options.
    //     */
    //  public void setUnderlying(String underlying) {
    //        this.underlying = underlying == null || underlying.isEmpty() ? "" : underlying;
    //    }
    //
    //    /**
    //     * Returns shares per contract for options.
    //     * Example: 1, 100.
    //     * @return shares per contract for options.
    //     */
    //  public double getSPC() {
    //        return spc;
    //    }
    //
    //    /**
    //     * Changes shares per contract for options.
    //     * Example: 1, 100.
    //     * @param spc shares per contract for options.
    //     */
    //  public void setSPC(double spc) {
    //        this.spc = spc;
    //    }
    //
    //    /**
    //     * Returns additional underlyings for options, including additional cash.
    //     * It shall use following format:
    //     * <pre>
    //     *     &lt;VALUE&gt; ::= &lt;empty&gt; | &lt;LIST&gt;
    //     *     &lt;LIST&gt; ::= &lt;AU&gt; | &lt;AU&gt; &lt;semicolon&gt; &lt;space&gt; &lt;LIST&gt;
    //     *     &lt;AU&gt; ::= &lt;UNDERLYING&gt; &lt;space&gt; &lt;SPC&gt; </pre>
    //     * the list shall be sorted by &lt;UNDERLYING&gt;.
    //     * Example: "SE 50", "FIS 53; US$ 45.46".
    //     * @return additional underlyings for options, including additional cash.
    //     */
    //  public String getAdditionalUnderlyings() {
    //        return additionalUnderlyings;
    //    }
    //
    //    /**
    //     * Changes additional underlyings for options, including additional cash.
    //     * It shall use following format:
    //     * <pre>
    //     *     &lt;VALUE&gt; ::= &lt;empty&gt; | &lt;LIST&gt;
    //     *     &lt;LIST&gt; ::= &lt;AU&gt; | &lt;AU&gt; &lt;semicolon&gt; &lt;space&gt; &lt;LIST&gt;
    //     *     &lt;AU&gt; ::= &lt;UNDERLYING&gt; &lt;space&gt; &lt;SPC&gt; </pre>
    //     * the list shall be sorted by &lt;UNDERLYING&gt;.
    //     * Example: "SE 50", "FIS 53; US$ 45.46".
    //     * @param additionalUnderlyings additional underlyings for options, including additional cash.
    //     */
    //  public void setAdditionalUnderlyings(String additionalUnderlyings) {
    //        this.additionalUnderlyings = additionalUnderlyings == null || additionalUnderlyings.isEmpty() ? "" :
    //        additionalUnderlyings;
    //    }
    //
    //    /**
    //     * Returns maturity month-year as provided for corresponding FIX tag (200).
    //     * It can use several different formats depending on data source:
    //     * <ul>
    //     * <li>YYYYMM – if only year and month are specified
    //     * <li>YYYYMMDD – if full date is specified
    //     * <li>YYYYMMwN – if week number (within a month) is specified
    //     * </ul>
    //     * @return  maturity month-year as provided for corresponding FIX tag (200).
    //     */
    //  public String getMMY() {
    //        return mmy;
    //    }
    //
    //    /**
    //     * Changes maturity month-year as provided for corresponding FIX tag (200).
    //     * It can use several different formats depending on data source:
    //     * <ul>
    //     * <li>YYYYMM – if only year and month are specified
    //     * <li>YYYYMMDD – if full date is specified
    //     * <li>YYYYMMwN – if week number (within a month) is specified
    //     * </ul>
    //     * @param mmy  maturity month-year as provided for corresponding FIX tag (200).
    //     */
    //  public void setMMY(String mmy) {
    //        this.mmy = mmy == null || mmy.isEmpty() ? "" : mmy;
    //    }
    //
    //    /**
    //     * Returns day id of expiration.
    //     * Example: {@link DayUtil#getDayIdByYearMonthDay DayUtil.getDayIdByYearMonthDay}(20090117).
    //     * @return day id of expiration.
    //     */
    //  public int getExpiration() {
    //        return expiration;
    //    }
    //
    //    /**
    //     * Changes day id of expiration.
    //     * Example: {@link DayUtil#getDayIdByYearMonthDay DayUtil.getDayIdByYearMonthDay}(20090117).
    //     * @param expiration day id of expiration.
    //     */
    //  public void setExpiration(int expiration) {
    //        this.expiration = expiration;
    //    }
    //
    //    /**
    //     * Returns day id of last trading day.
    //     * Example: {@link DayUtil#getDayIdByYearMonthDay DayUtil.getDayIdByYearMonthDay}(20090116).
    //     * @return day id of last trading day.
    //     */
    //  public int getLastTrade() {
    //        return lastTrade;
    //    }
    //
    //    /**
    //     * Changes day id of last trading day.
    //     * Example: {@link DayUtil#getDayIdByYearMonthDay DayUtil.getDayIdByYearMonthDay}(20090116).
    //     * @param lastTrade day id of last trading day.
    //     */
    //  public void setLastTrade(int lastTrade) {
    //        this.lastTrade = lastTrade;
    //    }
    //
    //    /**
    //     * Returns strike price for options.
    //     * Example: 80, 22.5.
    //     * @return strike price for options.
    //     */
    //  public double getStrike() {
    //        return strike;
    //    }
    //
    //    /**
    //     * Changes strike price for options.
    //     * Example: 80, 22.5.
    //     * @param strike strike price for options.
    //     */
    //  public void setStrike(double strike) {
    //        this.strike = strike;
    //    }
    //
    //    /**
    //     * Returns type of option.
    //     * It shall use one of following values:
    //     * <ul>
    //     * <li>STAN = Standard Options
    //     * <li>LEAP = Long-term Equity AnticiPation Securities
    //     * <li>SDO = Special Dated Options
    //     * <li>BINY = Binary Options
    //     * <li>FLEX = FLexible EXchange Options
    //     * <li>VSO = Variable Start Options
    //     * <li>RNGE = Range
    //     * </ul>
    //     * @return type of option.
    //     */
    //  public String getOptionType() {
    //        return optionType;
    //    }
    //
    //    /**
    //     * Changes type of option.
    //     * It shall use one of following values:
    //     * <ul>
    //     * <li>STAN = Standard Options
    //     * <li>LEAP = Long-term Equity AnticiPation Securities
    //     * <li>SDO = Special Dated Options
    //     * <li>BINY = Binary Options
    //     * <li>FLEX = FLexible EXchange Options
    //     * <li>VSO = Variable Start Options
    //     * <li>RNGE = Range
    //     * </ul>
    //     * @param optionType type of option.
    //     */
    //  public void setOptionType(String optionType) {
    //        this.optionType = optionType == null || optionType.isEmpty() ? "" : optionType;
    //    }
    //
    //    /**
    //     * Returns expiration cycle style, such as "Weeklys", "Quarterlys".
    //     * @return expiration cycle style.
    //     */
    //  public String getExpirationStyle() {
    //        return expirationStyle;
    //    }
    //
    //    /**
    //     * Returns expiration cycle style, such as "Weeklys", "Quarterlys".
    //     * @param expirationStyle expiration cycle style.
    //     */
    //  public void setExpirationStyle(String expirationStyle) {
    //        this.expirationStyle = expirationStyle == null || expirationStyle.isEmpty() ? "" : expirationStyle;
    //    }
    //
    //    /**
    //     * Returns settlement price determination style, such as "Open", "Close".
    //     * @return settlement price determination style.
    //     */
    //  public String getSettlementStyle() {
    //        return settlementStyle;
    //    }
    //
    //    /**
    //     * Changes settlement price determination style, such as "Open", "Close".
    //     * @param settlementStyle settlement price determination style.
    //     */
    //  public void setSettlementStyle(String settlementStyle) {
    //        this.settlementStyle = settlementStyle == null || settlementStyle.isEmpty() ? "" : settlementStyle;
    //    }
    //
    //    /**
    //     * Returns minimum allowed price increments with corresponding price ranges.
    //     * It shall use following format:
    //     * <pre>
    //     *     &lt;VALUE&gt; ::= &lt;empty&gt; | &lt;LIST&gt;
    //     *     &lt;LIST&gt; ::= &lt;INCREMENT&gt; | &lt;RANGE&gt; &lt;semicolon&gt; &lt;space&gt; &lt;LIST&gt;
    //     *     &lt;RANGE&gt; ::= &lt;INCREMENT&gt; &lt;space&gt; &lt;UPPER_LIMIT&gt; </pre>
    //     * the list shall be sorted by &lt;UPPER_LIMIT&gt;.
    //     * Example: "0.25", "0.01 3; 0.05".
    //     * @return minimum allowed price increments with corresponding price ranges.
    //     */
    //  public String getPriceIncrements() {
    //        return priceIncrements;
    //    }
    //
    //    /**
    //     * Changes minimum allowed price increments with corresponding price ranges.
    //     * It shall use following format:
    //     * <pre>
    //     *     &lt;VALUE&gt; ::= &lt;empty&gt; | &lt;LIST&gt;
    //     *     &lt;LIST&gt; ::= &lt;INCREMENT&gt; | &lt;RANGE&gt; &lt;semicolon&gt; &lt;space&gt; &lt;LIST&gt;
    //     *     &lt;RANGE&gt; ::= &lt;INCREMENT&gt; &lt;space&gt; &lt;UPPER_LIMIT&gt; </pre>
    //     * the list shall be sorted by &lt;UPPER_LIMIT&gt;.
    //     * Example: "0.25", "0.01 3; 0.05".
    //     * @param priceIncrements minimum allowed price increments with corresponding price ranges.
    //     */
    //  public void setPriceIncrements(String priceIncrements) {
    //        this.priceIncrements = priceIncrements == null || priceIncrements.isEmpty() ? "" : priceIncrements;
    //    }
    //
    //    /**
    //     * Returns trading hours specification.
    //     * See {@link Schedule#getInstance(String)}.
    //     * @return trading hours specification.
    //     */
    //  public String getTradingHours() {
    //        return tradingHours;
    //    }
    //
    //    /**
    //     * Changes trading hours specification.
    //     * See {@link Schedule#getInstance(String)}.
    //     * @param tradingHours trading hours specification.
    //     */
    //  public void setTradingHours(String tradingHours) {
    //        this.tradingHours = tradingHours == null || tradingHours.isEmpty() ? "" : tradingHours;
    //    }
};

} // namespace dxfcpp
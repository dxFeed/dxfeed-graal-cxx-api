// Copyright (c) 2024 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include "../internal/Common.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

DXFCPP_BEGIN_NAMESPACE

class IterableInstrumentProfile;
struct NonOwningInstrumentProfileIterator;
class InstrumentProfileReader;
class InstrumentProfileCollector;
struct Schedule;

/**
 * Represents basic profile information about market instrument.
 * Please see <a href="http://www.dxfeed.com/downloads/documentation/dxFeed_Instrument_Profile_Format.pdf">Instrument
 * Profile Format documentation</a> for complete description.
 */
struct DXFCPP_EXPORT InstrumentProfile final : public RequireMakeShared<InstrumentProfile> {
    friend IterableInstrumentProfile;
    friend NonOwningInstrumentProfileIterator;
    friend InstrumentProfileReader;
    friend InstrumentProfileCollector;
    friend Schedule;

    /// The alias to a type of shared pointer to the InstrumentProfile object
    using Ptr = std::shared_ptr<InstrumentProfile>;

    /// The alias to a type of unique pointer to the InstrumentProfile object
    using Unique = std::unique_ptr<InstrumentProfile>;

    /**
     * Creates an instrument profile with default values.
     */
    static Ptr create();

    /**
     * Creates an instrument profile as a copy of the specified instrument profile.
     * @param ip an instrument profile to copy.
     */
    static Ptr create(Ptr ip);

    /**
     * Returns type of instrument.
     * It takes precedence in conflict cases with other fields.
     * It is a mandatory field. It may not be empty.
     * Example: "STOCK", "FUTURE", "OPTION".
     *
     * @return type of instrument.
     */
    std::string getType() const;

    /**
     * Changes type of instrument.
     * It takes precedence in conflict cases with other fields.
     * It is a mandatory field. It may not be empty.
     * Example: "STOCK", "FUTURE", "OPTION".
     *
     * @param type The type of instrument.
     */
    void setType(const StringLikeWrapper &type) const;

    /**
     * Returns identifier of instrument, preferable an international one in Latin alphabet.
     * It is a mandatory field. It may not be empty.
     * Example: "GOOG", "/YGM9", ".ZYEAD".
     *
     * @return The identifier of instrument.
     */
    std::string getSymbol() const;

    /**
     * Changes identifier of instrument, preferable an international one in Latin alphabet.
     * It is a mandatory field. It may not be empty.
     * Example: "GOOG", "/YGM9", ".ZYEAD".
     *
     * @param symbol The identifier of instrument.
     */
    void setSymbol(const StringLikeWrapper &symbol) const;

    /**
     * Returns the description of instrument, preferable an international one in Latin alphabet.
     * Example: "Google Inc.", "Mini Gold Futures,Jun-2009,ETH".
     *
     * @return The description of instrument.
     */
    std::string getDescription() const;

    /**
     * Changes description of instrument, preferable an international one in Latin alphabet.
     * Example: "Google Inc.", "Mini Gold Futures,Jun-2009,ETH".
     *
     * @param description The description of instrument.
     */
    void setDescription(const StringLikeWrapper &description) const;

    /**
     * Returns identifier of instrument in national language.
     * It shall be empty if same as @ref InstrumentProfile::getSymbol() "symbol".
     *
     * @return The identifier of instrument in national language.
     */
    std::string getLocalSymbol() const;

    /**
     * Changes identifier of instrument in national language.
     * It shall be empty if same as @ref InstrumentProfile::getSymbol() "symbol".
     *
     * @param localSymbol The identifier of instrument in national language.
     */
    void setLocalSymbol(const StringLikeWrapper &localSymbol) const;

    /**
     * Returns description of instrument in national language.
     * It shall be empty if same as @ref InstrumentProfile::getDescription() "description".
     *
     * @return The description of instrument in national language.
     */
    std::string getLocalDescription() const;

    /**
     * Changes description of instrument in national language.
     * It shall be empty if same as @ref InstrumentProfile::getDescription() "description".
     *
     * @param localDescription The description of instrument in national language.
     */
    void setLocalDescription(const StringLikeWrapper &localDescription) const;

    /**
     * Returns country of origin (incorporation) of corresponding company or parent entity.
     * It shall use two-letter country code from ISO 3166-1 standard.
     * See <a href="http://en.wikipedia.org/wiki/ISO_3166-1">ISO 3166-1 on Wikipedia</a>.
     * Example: "US", "RU".
     *
     * @return The country of origin (incorporation) of corresponding company or parent entity.
     */
    std::string getCountry() const;

    /**
     * Changes country of origin (incorporation) of corresponding company or parent entity.
     * It shall use two-letter country code from ISO 3166-1 standard.
     * See <a href="http://en.wikipedia.org/wiki/ISO_3166-1">ISO 3166-1 on Wikipedia</a>.
     * Example: "US", "RU".
     *
     * @param country The country of origin (incorporation) of corresponding company or parent entity.
     */
    void setCountry(const StringLikeWrapper &country) const;

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
    std::string getOPOL() const;

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
    void setOPOL(const StringLikeWrapper &opol) const;

    /**
     * Returns exchange-specific data required to properly identify instrument when communicating with exchange.
     * It uses exchange-specific format.
     *
     * @return The exchange-specific data required to properly identify instrument when communicating with exchange.
     */
    std::string getExchangeData() const;

    /**
     * Changes exchange-specific data required to properly identify instrument when communicating with exchange.
     * It uses exchange-specific format.
     *
     * @param exchangeData The exchange-specific data required to properly identify instrument when communicating with
     * exchange.
     */
    void setExchangeData(const StringLikeWrapper &exchangeData) const;

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
    std::string getExchanges() const;

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
    void setExchanges(const StringLikeWrapper &exchanges) const;

    /**
     * Returns currency of quotation, pricing and trading.
     * It shall use three-letter currency code from ISO 4217 standard.
     * See <a href="http://en.wikipedia.org/wiki/ISO_4217">ISO 4217 on Wikipedia</a>.
     * Example: "USD", "RUB".
     *
     * @return Currency of quotation, pricing and trading.
     */
    std::string getCurrency() const;

    /**
     * Changes currency of quotation, pricing and trading.
     * It shall use three-letter currency code from ISO 4217 standard.
     * See <a href="http://en.wikipedia.org/wiki/ISO_4217">ISO 4217 on Wikipedia</a>.
     * Example: "USD", "RUB".
     *
     * @param currency Currency of quotation, pricing and trading.
     */
    void setCurrency(const StringLikeWrapper &currency) const;

    /**
     * Returns base currency of currency pair (FOREX instruments).
     * It shall use three-letter currency code similarly to @ref InstrumentProfile::getCurrency() "currency".
     *
     * @return The base currency of currency pair (FOREX instruments).
     */
    std::string getBaseCurrency() const;

    /**
     * Changes base currency of currency pair (FOREX instruments).
     * It shall use three-letter currency code similarly to @ref InstrumentProfile::getCurrency() "currency".
     *
     * @param baseCurrency The base currency of currency pair (FOREX instruments).
     */
    void setBaseCurrency(const StringLikeWrapper &baseCurrency) const;

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
    std::string getCFI() const;

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
    void setCFI(const StringLikeWrapper &cfi) const;

    /**
     * Returns International Securities Identifying Number.
     * It shall use twelve-letter code from ISO 6166 standard.
     * See <a href="http://en.wikipedia.org/wiki/ISO_6166">ISO 6166 on Wikipedia</a>
     * or <a href="http://en.wikipedia.org/wiki/International_Securities_Identifying_Number">ISIN on Wikipedia</a>.
     * Example: "DE0007100000", "US38259P5089".
     *
     * @return International Securities Identifying Number.
     */
    std::string getISIN() const;

    /**
     * Changes International Securities Identifying Number.
     * It shall use twelve-letter code from ISO 6166 standard.
     * See <a href="http://en.wikipedia.org/wiki/ISO_6166">ISO 6166 on Wikipedia</a>
     * or <a href="http://en.wikipedia.org/wiki/International_Securities_Identifying_Number">ISIN on Wikipedia</a>.
     * Example: "DE0007100000", "US38259P5089".
     *
     * @param isin International Securities Identifying Number.
     */
    void setISIN(const StringLikeWrapper &isin) const;

    /**
     * Returns Stock Exchange Daily Official List.
     * It shall use seven-letter code assigned by London Stock Exchange.
     * See <a href="http://en.wikipedia.org/wiki/SEDOL">SEDOL on Wikipedia</a> or
     * <a href="http://www.londonstockexchange.com/en-gb/products/informationproducts/sedol/">SEDOL on LSE</a>.
     * Example: "2310967", "5766857".
     *
     * @return Stock Exchange Daily Official List.
     */
    std::string getSEDOL() const;

    /**
     * Changes Stock Exchange Daily Official List.
     * It shall use seven-letter code assigned by London Stock Exchange.
     * See <a href="http://en.wikipedia.org/wiki/SEDOL">SEDOL on Wikipedia</a> or
     * <a href="http://www.londonstockexchange.com/en-gb/products/informationproducts/sedol/">SEDOL on LSE</a>.
     * Example: "2310967", "5766857".
     *
     * @param sedol Stock Exchange Daily Official List.
     */
    void setSEDOL(const StringLikeWrapper &sedol) const;

    /**
     * Returns Committee on Uniform Security Identification Procedures code.
     * It shall use nine-letter code assigned by CUSIP Services Bureau.
     * See <a href="http://en.wikipedia.org/wiki/CUSIP">CUSIP on Wikipedia</a>.
     * Example: "38259P508".
     *
     * @return CUSIP code.
     */
    std::string getCUSIP() const;

    /**
     * Changes Committee on Uniform Security Identification Procedures code.
     * It shall use nine-letter code assigned by CUSIP Services Bureau.
     * See <a href="http://en.wikipedia.org/wiki/CUSIP">CUSIP on Wikipedia</a>.
     * Example: "38259P508".
     *
     * @param cusip CUSIP code.
     */
    void setCUSIP(const StringLikeWrapper &cusip) const;

    /**
     * Returns Industry Classification Benchmark.
     * It shall use four-digit number from ICB catalog.
     * See <a href="http://en.wikipedia.org/wiki/Industry_Classification_Benchmark">ICB on Wikipedia</a>
     * or <a href="http://www.icbenchmark.com/">ICB homepage</a>.
     * Example: "9535".
     *
     * @return Industry Classification Benchmark.
     */
    std::int32_t getICB() const;

    /**
     * Changes Industry Classification Benchmark.
     * It shall use four-digit number from ICB catalog.
     * See <a href="http://en.wikipedia.org/wiki/Industry_Classification_Benchmark">ICB on Wikipedia</a>
     * or <a href="http://www.icbenchmark.com/">ICB homepage</a>.
     * Example: "9535".
     *
     * @param icb Industry Classification Benchmark.
     */
    void setICB(std::int32_t icb) const;

    /**
     * Returns Standard Industrial Classification.
     * It shall use four-digit number from SIC catalog.
     * See <a href="http://en.wikipedia.org/wiki/Standard_Industrial_Classification">SIC on Wikipedia</a>
     * or <a href="https://www.osha.gov/pls/imis/sic_manual.html">SIC structure</a>.
     * Example: "7371".
     *
     * @return Standard Industrial Classification.
     */
    std::int32_t getSIC() const;

    /**
     * Changes Standard Industrial Classification.
     * It shall use four-digit number from SIC catalog.
     * See <a href="http://en.wikipedia.org/wiki/Standard_Industrial_Classification">SIC on Wikipedia</a>
     * or <a href="https://www.osha.gov/pls/imis/sic_manual.html">SIC structure</a>.
     * Example: "7371".
     *
     * @param sic Standard Industrial Classification.
     */
    void setSIC(std::int32_t sic) const;

    /**
     * Returns market value multiplier.
     * Example: 100, 33.2.
     *
     * @return The market value multiplier.
     */
    double getMultiplier() const;

    /**
     * Changes market value multiplier.
     * Example: 100, 33.2.
     *
     * @param multiplier The market value multiplier.
     */
    void setMultiplier(double multiplier) const;

    /**
     * Returns product for futures and options on futures (underlying asset name).
     * Example: "/YG".
     *
     * @return The product for futures and options on futures (underlying asset name).
     */
    std::string getProduct() const;

    /**
     * Changes product for futures and options on futures (underlying asset name).
     * Example: "/YG".
     *
     * @param product The product for futures and options on futures (underlying asset name).
     */
    void setProduct(const StringLikeWrapper &product) const;

    /**
     * Returns primary underlying symbol for options.
     * Example: "C", "/YGM9"
     *
     * @return The primary underlying symbol for options.
     */
    std::string getUnderlying() const;

    /**
     * Changes primary underlying symbol for options.
     * Example: "C", "/YGM9"
     *
     * @param underlying The primary underlying symbol for options.
     */
    void setUnderlying(const StringLikeWrapper &underlying) const;

    /**
     * Returns shares per contract for options.
     * Example: 1, 100.
     *
     * @return shares per contract for options.
     */
    double getSPC() const;

    /**
     * Changes shares per contract for options.
     * Example: 1, 100.
     *
     * @param spc The shares per contract for options.
     */
    void setSPC(double spc) const;

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
    std::string getAdditionalUnderlyings() const;

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
    void setAdditionalUnderlyings(const StringLikeWrapper &additionalUnderlyings) const;

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
    std::string getMMY() const;

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
    void setMMY(const StringLikeWrapper &mmy) const;

    /**
     * Returns day id of expiration.
     * Example: @ref day_util::#getDayIdByYearMonthDay() "dxfcpp::day_util::getDayIdByYearMonthDay"(20090117).
     *
     * @return The day id of expiration.
     */
    std::int32_t getExpiration() const;

    /**
     * Changes day id of expiration.
     * Example: @ref day_util::#getDayIdByYearMonthDay() "dxfcpp::day_util::getDayIdByYearMonthDay"(20090117).
     *
     * @param expiration The day id of expiration.
     */
    void setExpiration(std::int32_t expiration) const;

    /**
     * Returns day id of last trading day.
     * Example: @ref day_util::#getDayIdByYearMonthDay() "dxfcpp::day_util::getDayIdByYearMonthDay"(20090116).
     *
     * @return The day id of last trading day.
     */
    std::int32_t getLastTrade() const;

    /**
     * Changes day id of last trading day.
     * Example: @ref day_util::#getDayIdByYearMonthDay() "dxfcpp::day_util::getDayIdByYearMonthDay"(20090116).
     *
     * @param lastTrade The day id of last trading day.
     */
    void setLastTrade(std::int32_t lastTrade) const;

    /**
     * Returns strike price for options.
     * Example: 80, 22.5.
     *
     * @return The strike price for options.
     */
    double getStrike() const;

    /**
     * Changes strike price for options.
     * Example: 80, 22.5.
     *
     * @param strike The strike price for options.
     */
    void setStrike(double strike) const;

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
    std::string getOptionType() const;

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
    void setOptionType(const StringLikeWrapper &optionType) const;

    /**
     * Returns expiration cycle style, such as "Weeklys", "Quarterlys".
     *
     * @return The expiration cycle style.
     */
    std::string getExpirationStyle() const;

    /**
     * Changes the expiration cycle style, such as "Weeklys", "Quarterlys".
     *
     * @param expirationStyle The expiration cycle style.
     */
    void setExpirationStyle(const StringLikeWrapper &expirationStyle) const;

    /**
     * Returns settlement price determination style, such as "Open", "Close".
     *
     * @return The settlement price determination style.
     */
    std::string getSettlementStyle() const;

    /**
     * Changes settlement price determination style, such as "Open", "Close".
     *
     * @param settlementStyle The settlement price determination style.
     */
    void setSettlementStyle(const StringLikeWrapper &settlementStyle) const;

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
    std::string getPriceIncrements() const;

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
    void setPriceIncrements(const StringLikeWrapper &priceIncrements) const;

    /**
     * Returns trading hours specification.
     * See Schedule::getInstance().
     *
     * @return The trading hours specification.
     */
    std::string getTradingHours() const;

    /**
     * Changes trading hours specification.
     * See Schedule::getInstance().
     *
     * @param tradingHours The trading hours specification.
     */
    void setTradingHours(const StringLikeWrapper &tradingHours) const;

    /**
     * Returns field value with a specified name.
     *
     * @param name name of field.
     * @return field value.
     */
    std::string getField(const StringLikeWrapper &name) const;

    /**
     * Changes field value with a specified name.
     *
     * @param name name of field.
     * @param value field value.
     */
    void setField(const StringLikeWrapper &name, const StringLikeWrapper &value) const;

    /**
     * Returns numeric field value with a specified name.
     *
     * @param name name of field.
     * @return field value.
     */
    double getNumericField(const StringLikeWrapper &name) const;

    /**
     * Changes numeric field value with a specified name.
     *
     * @param name name of field.
     * @param value field value.
     */
    void setNumericField(const StringLikeWrapper &name, double value) const;

    /**
     * Returns day id value for a date field with a specified name.
     *
     * @param name name of field.
     * @return day id value.
     */
    std::int32_t getDateField(const StringLikeWrapper &name) const;

    /**
     * Changes day id value for a date field with a specified name.
     *
     * @param name name of field.
     * @param value day id value.
     */
    void setDateField(const StringLikeWrapper &name, std::int32_t value) const;

    /**
     * Returns names of non-empty custom fields
     *
     * @return names of non-empty custom fields
     */
    std::vector<std::string> getNonEmptyCustomFieldNames() const;

    std::string toString() const override;

    friend std::ostream &operator<<(std::ostream &os, const InstrumentProfile &ip) {
        return os << ip.toString();
    }

    friend std::ostream &operator<<(std::ostream &os, const InstrumentProfile::Ptr &ip) {
        return os << ip->toString();
    }

    std::size_t hashCode() const;

    bool operator==(const InstrumentProfile &other) const;

    friend bool operator==(const InstrumentProfile::Ptr &ip1, const InstrumentProfile::Ptr &ip2) {
        if (ip1.get() == ip2.get()) {
            return true;
        }

        return *ip1.get() == *ip2.get();
    }

    ~InstrumentProfile() noexcept override;

    InstrumentProfile(const InstrumentProfile &) = delete;
    InstrumentProfile(InstrumentProfile &&) noexcept = delete;
    InstrumentProfile &operator=(const InstrumentProfile &) = delete;
    InstrumentProfile &operator=(const InstrumentProfile &&) noexcept = delete;

    explicit InstrumentProfile(LockExternalConstructionTag, JavaObjectHandle<InstrumentProfile> &&handle);

  private:
    JavaObjectHandle<InstrumentProfile> handle_;

    static Ptr create(JavaObjectHandle<InstrumentProfile> &&handle);

    struct List {
        /**
         * Creates a vector of objects of the current type and fills it with data from the the dxFeed Graal SDK list of
         * structures.
         *
         * @param list The pointer to the dxFeed Graal SDK list of structures.
         * @return The vector of objects of current type
         * @throws InvalidArgumentException
         */
        static std::vector<Ptr> fromGraal(void *list);
    };
};

DXFCPP_END_NAMESPACE

template <> struct std::hash<dxfcpp::InstrumentProfile> {
    std::size_t operator()(const dxfcpp::InstrumentProfile &t) const noexcept {
        return t.hashCode();
    }
};

template <> struct std::hash<dxfcpp::InstrumentProfile::Ptr> {
    std::size_t operator()(const dxfcpp::InstrumentProfile::Ptr &t) const noexcept {
        return t->hashCode();
    }
};

DXFCXX_DISABLE_MSC_WARNINGS_POP()
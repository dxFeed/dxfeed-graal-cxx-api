// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Common.hpp"
#include "../internal/Conf.hpp"

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
     * Returns the current instrument profile.
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
     * Returns a shared pointer to the current instrument profile.
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
     * Returns the current instrument profile.
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
     * Changes identifier of instrument, preferable an international one in Latin alphabet.
     * Returns a shared pointer to the current instrument profile.
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
};

} // namespace dxfcpp
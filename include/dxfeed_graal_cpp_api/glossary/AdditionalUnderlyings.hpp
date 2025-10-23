// Copyright (c) 2025 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../internal/Common.hpp"
#include "../internal/Conf.hpp"
#include "../internal/JavaObjectHandle.hpp"

#include <map>
#include <string>
#include <unordered_map>

DXFCPP_BEGIN_NAMESPACE

/**
 * Represents a set of additional underlyings for a given option. Each additional underlying
 * has an associated parameter called SPC (shares per contract) that specifies how many shares
 * of additional underlying are delivered during settlement of the option. In cases when
 * the option delivers additional cash, the SPC specifies an amount of how much cash is delivered.
 *
 * See AdditionalUnderlyings::getText() and AdditionalUnderlyings::getMap() for details about used formats and
 * representations.
 */
struct DXFCPP_EXPORT AdditionalUnderlyings : RequireMakeShared<AdditionalUnderlyings> {
    /// The alias to a type of shared pointer to the AdditionalUnderlyings object
    using Ptr = std::shared_ptr<AdditionalUnderlyings>;

    /// The alias to a type of unique pointer to the AdditionalUnderlyings object
    using Unique = std::unique_ptr<AdditionalUnderlyings>;

    private:
    mutable JavaObjectHandle<AdditionalUnderlyings> handle_;
    mutable std::mutex mtx_{};
    mutable bool initialized_{};
    std::function<JavaObjectHandle<AdditionalUnderlyings>()> initializer_;

    void init() const;

    static JavaObjectHandle<AdditionalUnderlyings>
    valueOfImpl(const std::vector<std::pair<const char *, double>> &mapLikeEntries);

    public:
    /**
     * Empty additional underlyings - it has empty text and empty map.
     */
    static const Ptr EMPTY;

    // lazy c-tor
    AdditionalUnderlyings(LockExternalConstructionTag,
                          std::function<JavaObjectHandle<AdditionalUnderlyings>()> initializer);

    AdditionalUnderlyings(LockExternalConstructionTag, JavaObjectHandle<AdditionalUnderlyings> &&handle);

    ~AdditionalUnderlyings() noexcept override;

    /**
     * Returns an instance of additional underlyings for specified textual representation.
     * See AdditionalUnderlyings::getText() for format specification.
     *
     * @param text The textual representation.
     * @throws JavaException("IllegalArgumentException") if text uses wrong format or contains invalid values.
     */
    static Ptr valueOf(const StringLikeWrapper &text);

    /**
     * Returns an instance of additional underlyings for specified internal representation.
     * See AdditionalUnderlyings::getMap() for details about internal representation.
     *
     * @tparam MapLikeType The type of `map`. `std::map<K, V>`, `std::unordered_map<K, V>`, `QMap<K, V>`,
     * `std::vector<std::pair<K, V>>` etc. excluding the type `std::initializer_list<std::pair<K, V>>`.
     * Where `K` is a string-like type and `V` is a type convertible to double.
     * @param map The map-like container.
     * @return An instance of additional underlyings.
     * @throws JavaException("IllegalArgumentException") if data contains invalid values.
     */
    template <typename MapLikeType>
        requires(!std::convertible_to<MapLikeType, StringLikeWrapper>)
    static Ptr valueOf(const MapLikeType &map);

    /**
     * Returns SPC for a specified underlying symbol or 0 is specified symbol is not found.
     * This method is equivalent to the expression "valueOf(text)->getSPC(symbol)" except it does not check the
     * correctness of format.
     *
     * @param text The text on which AdditionalUnderlying will be built.
     * @param symbol The underlying symbol.
     * @return SPC by the text and symbol.
     */
    static double getSPC(const StringLikeWrapper &text, const StringLikeWrapper &symbol);

    /**
     * Returns textual representation of additional underlyings in the format:
     * <pre>
     * TEXT ::= "" | LIST
     * LIST ::= AU | AU "; " LIST
     * AU ::= UNDERLYING " " SPC
     * </pre>
     * Where UNDERLYING is a symbol of an underlying instrument and SPC is a number of shares per contract of that
     * underlying. All additional underlyings are listed in the alphabetical order of the underlying symbol. In cases
     * when the option settles with additional cash, the underlying symbol will specify cash symbol and SPC will specify
     * the amount of cash.
     *
     * @return The textual representation of the additional underlyings.
     */
    std::string getText() const;

    /**
     * Returns internal representation of additional underlyings as a map from the underlying symbol to its SPC.
     *
     * @return The internal representation.
     */
    std::unordered_map<std::string, double> getMap() const;

    /**
     * Returns SPC for a specified underlying symbol or 0 is specified symbol is not found.
     *
     * @param symbol The underlying symbol.
     * @return The SPC for the symbol.
     */
    double getSPC(const StringLikeWrapper &symbol) const;

    /**
     * Returns `true` if this object is equal to `other` object
     *
     * @param other Another object
     * @return `true` if this object is equal to `other` object
     */
    bool operator==(const AdditionalUnderlyings &other) const;

    /**
     * Returns `true` if this object is equal to `other` object
     *
     * @param other Another object
     * @return `true` if this object is equal to `other` object
     */
    bool operator==(const AdditionalUnderlyings::Ptr &other) const {
        return *this == *other;
    }

    /**
     * @return A hash code value for this object.
     */
    std::size_t hashCode() const noexcept;

    /**
     * Returns a string representation of the current object.
     *
     * @return A string representation
     */
    std::string toString() const override;

    friend std::ostream &operator<<(std::ostream &os, const AdditionalUnderlyings &au) {
        return os << au.toString();
    }

    friend std::ostream &operator<<(std::ostream &os, const AdditionalUnderlyings::Ptr &au) {
        return os << au->toString();
    }
};

template <typename MapLikeType>
    requires(!std::convertible_to<MapLikeType, StringLikeWrapper>)
AdditionalUnderlyings::Ptr AdditionalUnderlyings::valueOf(const MapLikeType &map) {
    static_assert(MapLike<MapLikeType>, "AdditionalUnderlyings::valueOf(): argument must be a map-like container "
                                        "(e.g. std::map, std::unordered_map, QMap, or std::vector<std::pair<K, V>>)");

    using Elem = std::remove_cvref_t<decltype(*std::begin(map))>;
    using KeyType = std::remove_cvref_t<decltype(std::declval<Elem>().first)>;

    std::vector<std::pair<const char *, double>> mapLikeEntries{};
    mapLikeEntries.reserve(map.size());

    if constexpr (std::is_convertible_v<KeyType, std::string_view>) {
        for (const auto &[key, value] : map) {
            mapLikeEntries.emplace_back(std::string_view(key).data(), static_cast<double>(value));
        }

        return createShared(valueOfImpl(mapLikeEntries));
    } else if constexpr (requires { KeyType{}.toStdString(); }) {
        // QMap<QString, double>

        std::vector<std::string> keyStorage;
        keyStorage.reserve(map.size());

        for (const auto &[key, value] : map) {
            keyStorage.emplace_back(key.toStdString());
            mapLikeEntries.emplace_back(keyStorage.back().c_str(), static_cast<double>(value));
        }

        return createShared(valueOfImpl(mapLikeEntries));
    } else {
        static_assert(
            std::is_convertible_v<KeyType, std::string_view> || requires { KeyType{}.toStdString(); },
            "AdditionalUnderlyings::valueOf(): unsupported key type — must be convertible to std::string_view or "
            "provide .toStdString()");

        return {};
    }
}

DXFCPP_END_NAMESPACE

template <> struct std::hash<dxfcpp::AdditionalUnderlyings> {
    std::size_t operator()(const dxfcpp::AdditionalUnderlyings &au) const noexcept {
        return au.hashCode();
    }
};
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
    static const Ptr EMPTY;

    // lazy c-tor
    AdditionalUnderlyings(LockExternalConstructionTag,
                          std::function<JavaObjectHandle<AdditionalUnderlyings>()> initializer);

    AdditionalUnderlyings(LockExternalConstructionTag, JavaObjectHandle<AdditionalUnderlyings> &&handle);

    ~AdditionalUnderlyings() noexcept override;

    static Ptr valueOf(const StringLikeWrapper &value);

    template <typename MapLikeType>
    static Ptr valueOf(const MapLikeType &map) {
        static_assert(MapLike<MapLikeType>,
                      "AdditionalUnderlyings::valueOf(): argument must be a map-like container "
                      "(e.g. std::map, std::unordered_map, QMap, or std::vector<std::pair<...>>)");

        using Elem = std::remove_cvref_t<decltype(*std::begin(map))>;
        using KeyType = std::remove_cvref_t<decltype(std::declval<Elem>().first)>;

        std::vector<std::pair<const char *, double>> mapLikeEntries{};
        mapLikeEntries.reserve(map.size());

        // using KeyType = std::decay_t<decltype(std::declval<std::decay_t<decltype(*std::begin(map))>>().first)>;

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
        }

        return EMPTY;
    }

    static double getSPC(const StringLikeWrapper &text, const StringLikeWrapper &symbol);

    std::string getText() const;

    std::unordered_map<std::string, double> getMap() const;

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
    std::string toString() const;

    friend std::ostream &operator<<(std::ostream &os, const AdditionalUnderlyings &au) {
        return os << au.toString();
    }

    friend std::ostream &operator<<(std::ostream &os, const AdditionalUnderlyings::Ptr &au) {
        return os << au->toString();
    }
};

DXFCPP_END_NAMESPACE
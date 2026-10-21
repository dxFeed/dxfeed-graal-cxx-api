// Copyright (c) 2026 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../Conf.hpp"

DXFCXX_DISABLE_MSC_WARNINGS_PUSH(4251)

#include <array>
#include <cstddef>
#include <functional>
#include <span>
#include <stdexcept>
#include <utility>

DXFCPP_BEGIN_NAMESPACE

/**
 * A read-only map of constants by their keys.
 *
 * The entries are an array sorted by key at compile time (ConstantMap::sorted()), so a map of constants is
 * constant-initialized (`constinit`), does not allocate and has a trivial destructor: it can be used by the
 * initializers and the destructors of global variables. The keys are compared with `<` and `==`; a lookup is a binary
 * search.
 *
 * @tparam Key The type of the keys.
 * @tparam T The type of the constants.
 */
template <typename Key, typename T> class ConstantMap final {
    public:
    using key_type = Key;
    using mapped_type = std::reference_wrapper<const T>;
    using value_type = std::pair<Key, mapped_type>;
    using size_type = std::size_t;
    using const_iterator = typename std::span<const value_type>::iterator;
    using iterator = const_iterator;

    private:
    std::span<const value_type> entries_{};

    public:
    /**
     * Sorts the entries by key. A repeated key is an error, so in a constant expression it fails the compilation.
     *
     * @param unsorted The entries.
     * @return The entries sorted by key.
     */
    template <std::size_t N> static constexpr std::array<value_type, N> sorted(const value_type (&unsorted)[N]) {
        auto entries = std::to_array(unsorted);

        // An insertion sort: the maps are small, and the algorithms of some standard libraries (and std::swap of
        // std::variant in libstdc++ 11) are not constexpr.
        for (std::size_t i = 1; i < N; i++) {
            const auto entry = entries[i];
            auto j = i;

            for (; j > 0 && entry.first < entries[j - 1].first; j--) {
                entries[j] = entries[j - 1];
            }

            entries[j] = entry;
        }

        for (std::size_t i = 1; i < N; i++) {
            if (entries[i - 1].first == entries[i].first) {
                throw std::invalid_argument("The key is repeated");
            }
        }

        return entries;
    }

    /**
     * Creates the map of entries sorted by key.
     *
     * @param entries The entries sorted by key (see ConstantMap::sorted()). They must outlive the map.
     */
    constexpr explicit ConstantMap(std::span<const value_type> entries) noexcept : entries_{entries} {
    }

    [[nodiscard]] constexpr const_iterator begin() const noexcept {
        return entries_.begin();
    }

    [[nodiscard]] constexpr const_iterator end() const noexcept {
        return entries_.end();
    }

    [[nodiscard]] constexpr const_iterator cbegin() const noexcept {
        return begin();
    }

    [[nodiscard]] constexpr const_iterator cend() const noexcept {
        return end();
    }

    [[nodiscard]] constexpr size_type size() const noexcept {
        return entries_.size();
    }

    [[nodiscard]] constexpr bool empty() const noexcept {
        return entries_.empty();
    }

    /**
     * Finds the entry by key.
     *
     * @param key The key.
     * @return The iterator to the entry or end() if there is no such key.
     */
    [[nodiscard]] constexpr const_iterator find(const Key &key) const noexcept {
        auto first = begin();
        auto count = size();

        // std::lower_bound.
        while (count > 0) {
            const auto step = count / 2;

            if (const auto middle = first + static_cast<std::ptrdiff_t>(step); middle->first < key) {
                first = middle + 1;
                count -= step + 1;
            } else {
                count = step;
            }
        }

        return first != end() && first->first == key ? first : end();
    }

    /**
     * @param key The key.
     * @return `true` if the map has the key.
     */
    [[nodiscard]] constexpr bool contains(const Key &key) const noexcept {
        return find(key) != end();
    }

    /**
     * Returns the constant by key.
     *
     * @param key The key.
     * @return The reference to the constant.
     * @throws std::out_of_range if there is no such key.
     */
    [[nodiscard]] constexpr const mapped_type &at(const Key &key) const {
        const auto found = find(key);

        if (found == end()) {
            throw std::out_of_range("ConstantMap::at: no such key");
        }

        return found->second;
    }
};

DXFCPP_END_NAMESPACE

DXFCXX_DISABLE_MSC_WARNINGS_POP()

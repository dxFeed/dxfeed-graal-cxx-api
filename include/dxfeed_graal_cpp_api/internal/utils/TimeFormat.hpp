// Copyright (c) 2023 Devexperts LLC.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "../Conf.hpp"

#include <cstddef>
#include <cstdint>
#include <locale>
#include <optional>
#include <set>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace dxfcpp {

/**
 * Utility class for parsing and formatting dates and times in ISO-compatible format.
 *
 * TBD
 */
struct DXFCPP_EXPORT TimeFormat {
    template <typename Entry, std::size_t maxSize = 256>
        requires requires(Entry e) {
            { Entry{typename Entry::KeyType{}, typename Entry::ValueType{}} };
            { Entry::STUB } -> std::convertible_to<Entry>;
            { e.getKey() } -> std::convertible_to<typename Entry::KeyType>;
            { e.getValue() } -> std::convertible_to<typename Entry::ValueType>;
        }
    class Cache {
        std::vector<Entry> data_{maxSize, Entry::STUB};

      public:
        void add(const Entry &entry) noexcept {
            auto index = static_cast<std::size_t>(entry.getKey()) % maxSize;

            data_[index] = entry;
        }

        void add(const typename Entry::KeyType &key, const typename Entry::ValueType &value) {
            add({key, value});
        }

        void clear() noexcept {
            data_.swap({maxSize, Entry::STUB});
        }

        template <typename Key> const Entry &get(const Key &key) const noexcept {
            auto index = static_cast<std::size_t>(key) % maxSize;

            if (data_[index].getKey() == key) {
                return data_[index];
            }

            return Entry::STUB;
        }

        template <typename Key>
        std::optional<std::reference_wrapper<const typename Entry::ValueType>>
        getEntryValue(const Key &key) const noexcept {
            auto index = static_cast<std::size_t>(key) % maxSize;

            if (data_[index].getKey() == key) {
                return data_[index].getValue();
            }

            return std::nullopt;
        }
    };

  private:
    struct DXFCPP_EXPORT MinuteCacheEntry {
        using KeyType = std::int64_t;
        using ValueType = std::string;

        static const MinuteCacheEntry STUB;

      private:
        KeyType minute_{};
        ValueType template_{};

      public:
        MinuteCacheEntry(KeyType minute, ValueType minuteTemplate) noexcept
            : minute_{minute}, template_{std::move(minuteTemplate)} {
        }

        MinuteCacheEntry() noexcept : MinuteCacheEntry(std::numeric_limits<KeyType>::min(), "") {
        }

        [[nodiscard]] KeyType getKey() const noexcept {
            return minute_;
        }

        [[nodiscard]] const ValueType &getValue() const & noexcept {
            return template_;
        }

        [[nodiscard]] KeyType getMinute() const {
            return minute_;
        }

        [[nodiscard]] const ValueType &getTemplate() const & noexcept {
            return template_;
        }

        bool operator==(const MinuteCacheEntry &minuteCacheEntry) const noexcept {
            return minute_ == minuteCacheEntry.minute_ && template_ == minuteCacheEntry.template_;
        }
    };

    struct DXFCPP_EXPORT CacheEntry {
        using KeyType = std::int64_t;
        using ValueType = std::string;

        static const CacheEntry STUB;

      private:
        KeyType key_{};
        ValueType value_{};

      public:
        CacheEntry(KeyType key, ValueType value) noexcept : key_{key}, value_{std::move(value)} {
        }

        CacheEntry() noexcept : CacheEntry(std::numeric_limits<KeyType>::min(), "") {
        }

        [[nodiscard]] KeyType getKey() const noexcept {
            return key_;
        }

        [[nodiscard]] const ValueType &getValue() const & noexcept {
            return value_;
        }

        bool operator==(const CacheEntry &cacheEntry) const noexcept {
            return key_ == cacheEntry.key_ && value_ == cacheEntry.value_;
        }
    };

    class Format {};
};

} // namespace dxfcpp
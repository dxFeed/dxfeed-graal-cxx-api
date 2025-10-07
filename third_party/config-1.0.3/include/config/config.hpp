// Copyright (c) 2025 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <string>
#include <unordered_map>

#include "sources/ini.hpp"
#include "sources/env.hpp"

namespace org::ttldtor::config {

/**
 * Simple configuration class.
 * Receives various key and value sources as input. Mimics a key-value container (aka dictionary or map).
 */
class Config {
  std::map<std::string, std::string> data_{};

 public:
  using Iterator = std::map<std::string, std::string>::const_iterator;

  Config() = default;

  /**
   * Adds keys and values from the source.
   *
   * @tparam Source The source type.
   * @param source The properties' source.
   */
  template <typename Source>
  void addSource(Source&& source) {
    for (auto&& [key, value] : source) {
      data_[key] = value;
    }
  }

  /**
   * Returns an iterator to the beginning.
   *
   * @return An iterator to the beginning.
   */
  [[nodiscard]] Iterator begin() const noexcept {
    return data_.begin();
  }

  /**
   * Returns an iterator to the end.
   *
   * @return An iterator to the end
   */
  [[nodiscard]] Iterator end() const noexcept {
    return data_.end();
  }

  /**
   * Returns the collection of keys and values.
   *
   * @return The collection of keys and values.
   */
  [[nodiscard]] const std::map<std::string, std::string>& getData() const& noexcept {
    return data_;
  }

  /**
   * Checks whether the config is empty.
   *
   * @return `true` if the config is empty.
   */
  [[nodiscard]] bool empty() const noexcept {
    return data_.empty();
  }

  /**
   * Returns the number of elements.
   *
   * @return The number of elements.
   */
  [[nodiscard]] std::size_t size() const noexcept {
    return data_.size();
  }
};

}  // namespace org::ttldtor::config
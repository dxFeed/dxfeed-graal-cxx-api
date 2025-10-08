// Copyright (c) 2025 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <cstdlib>
#include <cstring>
#include <map>
#include <string>

#if defined(_WIN32)
#  include <windows.h>
#else
extern char** environ;
#endif

namespace org::ttldtor::config {

namespace detail {
inline std::string replaceAll(const std::string& str, const std::string& from, const std::string& to) {
  if (from.empty()) {
    return str;
  }

  std::string result;

  result.reserve(str.length());

  size_t lastPos = 0;
  size_t pos = 0;

  while ((pos = str.find(from, lastPos)) != std::string::npos) {
    result.append(str, lastPos, pos - lastPos);
    result.append(to);
    lastPos = pos + from.length();
  }
  result.append(str, lastPos, str.length() - lastPos);

  return result;
}
}  // namespace detail

/**
 * A class representing the environment variables source for configuration.
 * Loads all environment variables or specific ones with optional prefix filtering.
 * If an environment variable name contains "__", it will be replaced with ':' (the key section separator) in the
 * internal representation.
 */
class EnvSource {
  static constexpr auto SEPARATOR = "__";
  std::map<std::string, std::string> data_{};

  void parseAndAddEntry(const std::string& entry) {
    if (const auto pos = entry.find('='); pos != std::string::npos && pos > 0) {
      auto key = entry.substr(0, pos);

      key = detail::replaceAll(key, SEPARATOR, ":");

      data_[key] = entry.substr(pos + 1);
    }
  }

  void filterByPrefix(const std::string& prefix) {
    if (prefix.empty()) {
      return;
    }

    std::map<std::string, std::string> filtered{};

    for (const auto& [key, value] : data_) {
      if (key.starts_with(prefix)) {
        std::string newKey = key.substr(prefix.length());

        filtered[newKey] = value;
      }
    }

    data_ = std::move(filtered);
  }

  void loadAllEnvironmentVariables() {
#if defined(_WIN32)
    LPCH environmentStrings = GetEnvironmentStrings();

    if (!environmentStrings) {
      return;
    }

    LPCH current = environmentStrings;

    while (*current) {
      std::string entry(current);

      parseAndAddEntry(entry);

      current += entry.length() + 1;
    }
    FreeEnvironmentStrings(environmentStrings);
#else
    for (char** environmentPtr = environ; *environmentPtr != nullptr; ++environmentPtr) {
      std::string entry(*environmentPtr);

      parseAndAddEntry(entry);
    }
#endif
  }

 public:
  using Iterator = std::map<std::string, std::string>::const_iterator;

  /**
   * Constructs EnvSource loading all environment variables.
   */
  EnvSource() {
    loadAllEnvironmentVariables();
  }

  /**
   * Constructs EnvSource loading only variables with a specified prefix.
   * The prefix is removed from the key names.
   *
   * Example: prefix="APP_" will load "APP_PORT" as "PORT"
   *
   * @param prefix Prefix to filter environment variables
   */
  explicit EnvSource(const std::string& prefix) {
    loadAllEnvironmentVariables();
    filterByPrefix(prefix);
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
  [[nodiscard]] const std::map<std::string, std::string>& getData() const noexcept {
    return data_;
  }

  /**
   * Checks whether the source is empty.
   *
   * @return `true` if the source is empty.
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
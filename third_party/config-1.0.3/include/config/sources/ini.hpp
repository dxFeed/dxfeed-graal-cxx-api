// Copyright (c) 2025 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <algorithm>
#include <cctype>
#include <concepts>
#include <fstream>
#include <iostream>
#include <locale>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#if defined(_MSC_VER)
#  ifndef OTC_CONFIG_DISABLE_MSC_WARNINGS_PUSH
#    define OTC_CONFIG_DISABLE_MSC_WARNINGS_PUSH(...) __pragma(warning(push)) __pragma(warning(disable : __VA_ARGS__))
#    define OTC_CONFIG_DISABLE_MSC_WARNINGS_POP() __pragma(warning(pop))
#  endif
#else
#  ifndef OTC_CONFIG_DISABLE_MSC_WARNINGS_PUSH
#    define OTC_CONFIG_DISABLE_MSC_WARNINGS_PUSH(warnings)
#    define OTC_CONFIG_DISABLE_MSC_WARNINGS_POP()
#  endif
#endif

#if defined(__GNUC__) && !defined(__clang__) && !defined(__INTEL_COMPILER) && !defined(__NVCOMPILER)
#  ifndef OTC_CONFIG_DO_PRAGMA
#    define OTC_CONFIG_DO_PRAGMA(x) _Pragma(#x)
#  endif
#  ifndef OTC_CONFIG_DISABLE_GCC_WARNINGS_PUSH
#    define OTC_CONFIG_DISABLE_GCC_WARNINGS_PUSH(...) \
      OTC_CONFIG_DO_PRAGMA(GCC diagnostic push) OTC_CONFIG_DO_PRAGMA(GCC diagnostic ignored __VA_ARGS__)
#    define OTC_CONFIG_DISABLE_GCC_WARNINGS(...) OTC_CONFIG_DO_PRAGMA(GCC diagnostic ignored __VA_ARGS__)
#    define OTC_CONFIG_DISABLE_GCC_WARNINGS_POP() OTC_CONFIG_DO_PRAGMA(GCC diagnostic pop)
#  endif
#else
#  ifndef OTC_CONFIG_DISABLE_GCC_WARNINGS_PUSH
#    define OTC_CONFIG_DISABLE_GCC_WARNINGS_PUSH(warnings)
#    define OTC_CONFIG_DISABLE_GCC_WARNINGS(warnings)
#    define OTC_CONFIG_DISABLE_GCC_WARNINGS_POP()
#  endif
#endif

#if defined(__has_include)
#  if __has_include(<filesystem>)
#    include <filesystem>
#    if defined(__cpp_lib_filesystem)
#      define OTC_CONFIG_HAS_FILESYSTEM 1
namespace fs = std::filesystem;
#    elif defined(__cpp_lib_experimental_filesystem)
#      include <experimental/filesystem>
#      define OTC_CONFIG_HAS_FILESYSTEM 1
namespace fs = std::experimental::filesystem;
#    else
#      define OTC_CONFIG_HAS_FILESYSTEM 0
#    endif
#  else
#    define OTC_CONFIG_HAS_FILESYSTEM 0
#  endif
#else
#  define OTC_CONFIG_HAS_FILESYSTEM 0
#endif

#if OTC_CONFIG_HAS_FILESYSTEM == 0
#  ifdef _WIN32
#    include <windows.h>
#  endif
#endif

namespace org::ttldtor::config {

namespace ini {

namespace detail {

template <class CharT>
std::basic_ifstream<CharT> openInputFile(const std::string& path) {
#if OTC_CONFIG_HAS_FILESYSTEM
  return std::basic_ifstream<CharT>(fs::path(path));
#elif defined(_WIN32)
  const int wlen = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);

  if (wlen > 0) {
    std::wstring wpath(wlen, 0);

    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, &wpath[0], wlen);

    return std::basic_ifstream<CharT>(wpath);
  }
  return std::basic_ifstream<CharT>(path);
#else
  return std::basic_ifstream<CharT>(path);
#endif
}

template <class CharT>
std::basic_ofstream<CharT> openOutputFile(const std::string& path) {
#if OTC_CONFIG_HAS_FILESYSTEM
  return std::basic_ofstream<CharT>(fs::path(path));
#elif defined(_WIN32)
  int wlen = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);

  if (wlen > 0) {
    std::wstring wpath(wlen, 0);

    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, &wpath[0], wlen);

    return std::basic_ofstream<CharT>(wpath);
  }

  return std::basic_ofstream<CharT>(path);
#else
  return std::basic_ofstream<CharT>(path);
#endif
}

template <class CharT>
void trim(std::basic_string<CharT>& s, const std::locale& loc = std::locale{"C"}) {
  auto is_space = [&](CharT ch) {
    return std::isspace(ch, loc);
  };

  auto left = std::find_if_not(s.begin(), s.end(), is_space);
  auto right = std::find_if_not(s.rbegin(), s.rend(), is_space).base();

  s = (left < right) ? std::basic_string<CharT>(left, right) : std::basic_string<CharT>{};
}

template <class CharT>
void rtrimAt(std::basic_string<CharT>& s, CharT delimiter, const std::locale& loc = std::locale{"C"}) {
  if (auto pos = s.find(delimiter); pos != std::basic_string<CharT>::npos) {
    s.erase(pos);
    trim(s, loc);
  }
}

template <class CharT>
bool replaceAll(std::basic_string<CharT>& str, std::basic_string_view<CharT> from, std::basic_string_view<CharT> to) {
  if (from.empty())
    return false;

  bool changed = false;
  for (size_t pos = 0; (pos = str.find(from.data(), pos, from.length())) != std::basic_string<CharT>::npos;) {
    str.replace(pos, from.length(), to.data(), to.length());
    pos += to.length();
    changed = true;
  }

  return changed;
}

}  // namespace detail

template <typename CharT, typename T>
concept Parseable = requires(std::basic_istringstream<CharT> is, T& dst) {
  { is >> dst } -> std::same_as<std::basic_istream<CharT>&>;
};

template <typename CharT, Parseable<CharT> T>
bool extract(std::basic_string_view<CharT> value, T& dst) {
  std::basic_istringstream<CharT> is{std::basic_string<CharT>(value)};
  CharT c;

  return (is >> std::boolalpha >> dst) && !(is >> c);
}

template <typename CharT>
bool extract(std::basic_string_view<CharT> value, std::basic_string<CharT>& dst) {
  dst = value;

  return true;
}

template <typename CharT, typename T>
bool getValue(const std::map<std::basic_string<CharT>, std::basic_string<CharT>>& sec,
              std::basic_string_view<CharT> key, T& dst) {
  if (auto it = sec.find(std::basic_string<CharT>(key)); it != sec.end()) {
    return extract<CharT>(it->second, dst);
  }

  return false;
}

template <class CharT>
struct Format {
  CharT sectionStart = '[';
  CharT sectionEnd = ']';
  CharT assign = '=';
  CharT comment = ';';
  CharT comment2 = '#';
  CharT interpol = '$';
  CharT interpolStart = '{';
  CharT interpolSep = ':';
  CharT interpolEnd = '}';

  [[nodiscard]] constexpr auto localSymbol(std::basic_string_view<CharT> name) const {
    return std::basic_string<CharT>{interpol} + interpolStart + std::basic_string<CharT>{name} + interpolEnd;
  }

  [[nodiscard]] constexpr auto globalSymbol(std::basic_string_view<CharT> secName,
                                            std::basic_string_view<CharT> name) const {
    return localSymbol(std::basic_string<CharT>{secName} + interpolSep + std::basic_string<CharT>{name});
  }
};

template <class CharT>
class Ini {
 public:
  using String = std::basic_string<CharT>;
  using Section = std::map<String, String>;
  using Sections = std::map<String, Section>;

  Sections sections;
  std::vector<String> errors;
  Format<CharT> format;

  void generate(std::basic_ostream<CharT>& os) const {
    for (const auto& [name, section] : sections) {
      os << format.sectionStart << name << format.sectionEnd << '\n';

      for (const auto& [key, value] : section) {
        os << key << format.assign << value << '\n';
      }

      os << '\n';
    }
  }

  void parse(std::basic_istream<CharT>& is) {
    String currentSection;
    String line;

    while (std::getline(is, line)) {
      detail::trim(line);

      if (shouldSkipLine(line)) {
        continue;
      }

      parseLine(line, currentSection);
    }
  }

  bool parseFile(const std::string& utf8Path) {
    auto file = detail::openInputFile<CharT>(utf8Path);

    if (!file.is_open()) {
      errors.push_back("Cannot open file");

      return false;
    }

    parse(file);

    return true;
  }

  [[nodiscard]] bool generateFile(const std::string& utf8Path) const {
    auto file = detail::openOutputFile<CharT>(utf8Path);

    if (!file.is_open()) {
      return false;
    }

    generate(file);

    return true;
  }

  void interpolate(int maxDepth = 10) {
    interpolateLocalSymbols();
    interpolateGlobalSymbols(maxDepth);
  }

  void defaultSection(const Section& defaults) {
    for (auto& [_, section] : sections) {
      section.insert(defaults.begin(), defaults.end());
    }
  }
  OTC_CONFIG_DISABLE_MSC_WARNINGS_PUSH(4456)
  void stripTrailingComments() {
    for (auto& [_, section] : sections) {
      for (auto& [_, value] : section) {
        detail::rtrimAt(value, format.comment);
        detail::rtrimAt(value, format.comment2);
      }
    }
  }
  OTC_CONFIG_DISABLE_MSC_WARNINGS_POP()

  void clear() {
    sections.clear();
    errors.clear();
  }

 private:
  [[nodiscard]] bool shouldSkipLine(const String& line) const {
    return line.empty() || line.front() == format.comment || line.front() == format.comment2;
  }

  [[nodiscard]] bool isSectionLine(const String& line) const {
    return line.front() == format.sectionStart && line.back() == format.sectionEnd;
  }

  [[nodiscard]] std::optional<String> extractSectionName(const String& line) const {
    if (!isSectionLine(line))
      return std::nullopt;
    return line.substr(1, line.length() - 2);
  }

  struct KeyValue {
    String key;
    String value;
  };

  [[nodiscard]] std::optional<KeyValue> parseAssignment(const String& line) const {
    auto pos = line.find(format.assign);
    if (pos == String::npos || pos == 0)
      return std::nullopt;

    KeyValue kv{line.substr(0, pos), line.substr(pos + 1)};
    detail::trim(kv.key);
    detail::trim(kv.value);

    return kv;
  }

  void parseLine(const String& line, String& currentSection) {
    if (auto section = extractSectionName(line)) {
      currentSection = std::move(*section);

      return;
    }

    if (auto kv = parseAssignment(line)) {
      addKeyValue(currentSection, std::move(kv->key), std::move(kv->value), line);

      return;
    }

    errors.push_back(line);
  }

  void addKeyValue(const String& sectionName, String key, String value, const String& originalLine) {
    auto& sec = sections[sectionName];
    if (!sec.contains(key)) {
      sec[std::move(key)] = std::move(value);
    } else {
      errors.push_back(originalLine);
    }
  }

  void interpolateLocalSymbols() {
    for (auto& [secName, secData] : sections) {
      for (const auto& [key, _] : secData) {
        replaceInSection(secData, format.localSymbol(key), format.globalSymbol(secName, key));
      }
    }
  }

  void interpolateGlobalSymbols(int maxDepth) {
    for (int iter = 0; iter < maxDepth; ++iter) {
      if (!interpolateGlobalSymbolsOnce()) {
        break;
      }
    }
  }

  [[nodiscard]] bool interpolateGlobalSymbolsOnce() {
    bool changed = false;

    for (const auto& [secName, secData] : sections) {
      for (const auto& [key, keyValue] : secData) {
        changed |= replaceInAllSections(format.globalSymbol(secName, key), keyValue);
      }
    }

    return changed;
  }

  void replaceInSection(Section& section, std::basic_string_view<CharT> from, std::basic_string_view<CharT> to) {
    for (auto& [_, value] : section) {
      detail::replaceAll(value, from, to);
    }
  }

  [[nodiscard]] bool replaceInAllSections(std::basic_string_view<CharT> from, std::basic_string_view<CharT> to) {
    bool changed = false;

    for (auto& [_, section] : sections) {
      for (auto& [_, value] : section) {
        changed |= detail::replaceAll(value, from, to);
      }
    }

    return changed;
  }
};

}  // namespace ini

/**
 * A class representing the source of keys and values contained in an INI file.
 * Mimics a key-value container (aka dictionary or map).
 * Section names are prefixed to the key name.
 * The section name is separated from the key name by ':'.
 */
class IniSource {
  static constexpr char SEPARATOR = ':';

  std::map<std::string, std::string> data_{};

  template <typename CharT>
  static void stripCommentsAndFillData(ini::Ini<CharT>& ini, std::map<std::string, std::string>& data) {
    ini.stripTrailingComments();

    for (const auto& [sectionName, section] : ini.sections) {
      for (const auto& [key, value] : section) {
        std::string fullKey = sectionName;

        if (!fullKey.empty()) {
          fullKey += std::string(1, SEPARATOR);
        }

        fullKey += key;

        data[fullKey] = value;
      }
    }
  }

 public:
  using Iterator = std::map<std::string, std::string>::const_iterator;

  IniSource() = default;

  /**
   * Creates a source by parsing a string containing an ini file.
   *
   * @param iniString The ini-string.
   * @return The new source.
   */
  static IniSource fromString(const std::string& iniString) {
    ini::Ini<char> ini{};

    std::istringstream is{iniString};

    ini.parse(is);

    IniSource result{};

    stripCommentsAndFillData(ini, result.data_);

    return result;
  }

  /**
   * Creates a source by parsing an ini file.
   *
   * @param filePath File path.
   */
  explicit IniSource(const std::string& filePath) {
    ini::Ini<char> ini{};

    if (!ini.parseFile(filePath)) {
      return;
    }

    stripCommentsAndFillData(ini, data_);
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
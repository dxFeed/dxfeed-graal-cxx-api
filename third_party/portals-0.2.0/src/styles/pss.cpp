// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include <portals/style.hpp>

#include <charconv>

namespace portals {
namespace {
bool letter(char value) {
  return (value >= 'a' && value <= 'z') || (value >= 'A' && value <= 'Z') || value == '_';
}

bool digit(char value) {
  return value >= '0' && value <= '9';
}

class Parser {
public:
  explicit Parser(std::string_view source) : source_(source) {
  }

  StyleSheet parse() {
    StyleSheet sheet;

    skipSpace();

    while (position_ != source_.size()) {
      if (sheet.rules.size() == MAX_STYLE_RULES) {
        fail("Too many style rules");
      }

      auto selector = parseSelector();

      expect('{');

      StyleDeclarations declarations;

      while (peek() != '}') {
        parseDeclaration(declarations, selector.state);
      }

      expect('}');
      sheet.rules.push_back({std::move(selector), declarations});
    }

    return sheet;
  }

private:
  char peek() const {
    return position_ < source_.size() ? source_[position_] : '\0';
  }

  [[noreturn]] void fail(std::string_view message) const {
    throw StyleError{TreeError::INVALID_ARGUMENT, line_, column_, message};
  }

  void advance() {
    if (peek() == '\n') {
      ++line_;
      column_ = 1;
    } else {
      ++column_;
    }

    ++position_;
  }

  void skipSpace() {
    for (;;) {
      while (peek() == ' ' || peek() == '\t' || peek() == '\r' || peek() == '\n') {
        advance();
      }

      if (source_.substr(position_, 2) != "/*") {
        return;
      }

      advance();
      advance();

      while (source_.substr(position_, 2) != "*/") {
        if (position_ == source_.size()) {
          fail("Unterminated comment");
        }

        advance();
      }

      advance();
      advance();
    }
  }

  void expect(char expected) {
    if (peek() != expected) {
      fail("Expected punctuation");
    }

    advance();
    skipSpace();
  }

  std::string_view identifier() {
    if (!letter(peek())) {
      fail("Expected ASCII identifier");
    }

    const auto begin = position_;

    while (letter(peek()) || digit(peek()) || peek() == '-') {
      advance();
    }

    return source_.substr(begin, position_ - begin);
  }

  StyleState parseState() {
    const auto location = StyleError{TreeError::INVALID_ARGUMENT, line_, column_, "Unknown presentation state"};
    const auto name = identifier();

    if (name == "normal") {
      return StyleState::NORMAL;
    }

    if (name == "focused") {
      return StyleState::FOCUSED;
    }

    if (name == "disabled") {
      return StyleState::DISABLED;
    }

    if (name == "selected") {
      return StyleState::SELECTED;
    }

    if (name == "header") {
      return StyleState::HEADER;
    }

    if (name == "caret") {
      return StyleState::CARET;
    }

    throw location;
  }

  StyleSelector parseSelector() {
    StyleSelector selector;
    bool present{};

    if (peek() == '*') {
      advance();
      present = true;
    } else if (letter(peek())) {
      selector.type = identifier();
      present = true;
    }

    while (peek() == '.' || peek() == '#' || peek() == ':') {
      const auto marker = peek();

      advance();
      present = true;

      if (marker == '.') {
        selector.classes.emplace_back(identifier());
      } else if (marker == '#') {
        if (!selector.id.empty()) {
          fail("Duplicate selector id");
        }

        selector.id = identifier();
      } else {
        if (selector.state != StyleState::ANY) {
          fail("Duplicate selector state");
        }

        selector.state = parseState();
      }
    }

    if (!present) {
      fail("Expected selector");
    }

    skipSpace();

    return selector;
  }

  std::string_view value() {
    const auto begin = position_;

    while (peek() != ';' && peek() != '}' && peek() != '\0' && peek() != ' ' && peek() != '\t' && peek() != '\r' &&
           peek() != '\n') {
      advance();
    }

    return source_.substr(begin, position_ - begin);
  }

  static RgbColor parseColor(std::string_view text, StyleError location) {
    unsigned number{};

    if (text.size() != 7 || text.front() != '#') {
      throw location;
    }

    const auto result = std::from_chars(text.data() + 1, text.data() + text.size(), number, 16);

    if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) {
      throw location;
    }

    return {static_cast<std::uint8_t>(number >> 16), static_cast<std::uint8_t>(number >> 8),
            static_cast<std::uint8_t>(number)};
  }

  static bool parseBoolean(std::string_view text, StyleError location) {
    if (text == "true") {
      return true;
    }

    if (text == "false") {
      return false;
    }

    throw location;
  }

  void parseDeclaration(StyleDeclarations &declarations, StyleState state) {
    const auto propertyLocation = StyleError{TreeError::INVALID_ARGUMENT, line_, column_, "Unknown property"};
    const auto name = identifier();

    skipSpace();
    expect(':');

    auto location = StyleError{TreeError::INVALID_ARGUMENT, line_, column_, "Invalid property value"};
    const auto text = value();

    if (name == "foreground") {
      declarations.foreground = parseColor(text, location);
    } else if (name == "background") {
      declarations.background = parseColor(text, location);
    } else if (name == "bold") {
      declarations.bold = parseBoolean(text, location);
    } else if (name == "italic") {
      declarations.italic = parseBoolean(text, location);
    } else if (name == "underline") {
      declarations.underline = parseBoolean(text, location);
    } else if (name == "min-width") {
      std::int32_t width{};
      const auto parsed = std::from_chars(text.data(), text.data() + text.size(), width);

      if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size() || width < 0 ||
          state != StyleState::ANY) {
        location.message = "Expected nonnegative state-independent cell width";

        throw location;
      }

      declarations.minWidth = width;
    } else {
      throw propertyLocation;
    }

    skipSpace();
    expect(';');
  }

  std::string_view source_;
  std::size_t position_{};
  std::size_t line_{1};
  std::size_t column_{1};
};
} // namespace

std::expected<StyleSheet, StyleError> parsePss(std::string_view source) {
  if (source.size() > MAX_PSS_BYTES) {
    return std::unexpected(StyleError{TreeError::INVALID_ARGUMENT, 1, 1, "PSS source exceeds byte budget"});
  }

  try {
    return Parser(source).parse();
  } catch (const StyleError &error) {
    return std::unexpected(error);
  } catch (const std::bad_alloc &) {
    return std::unexpected(StyleError{TreeError::NO_MEMORY, 1, 1, "PSS allocation failed"});
  }
}
} // namespace portals

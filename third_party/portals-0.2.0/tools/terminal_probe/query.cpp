// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include "query.hpp"

#include <charconv>

namespace portals::probe {
namespace {
constexpr std::size_t SEQUENCE_LIMIT = 64;

bool parameters(std::string_view text, bool cursor) {
  std::size_t count = 0;

  while (!text.empty()) {
    const auto separator = text.find(';');
    const auto part = text.substr(0, separator);
    unsigned value = 0;
    const auto result = std::from_chars(part.data(), part.data() + part.size(), value);

    if (part.empty() || result.ec != std::errc{} || result.ptr != part.data() + part.size() || (cursor && value == 0)) {
      return false;
    }

    ++count;

    if (separator == std::string_view::npos) {
      break;
    }

    text.remove_prefix(separator + 1);

    if (text.empty()) {
      return false;
    }
  }

  return cursor ? count == 2 : count != 0;
}
} // namespace

QueryCollector::QueryCollector(Time deadline, std::size_t byte_limit) : deadline_(deadline), byte_limit_(byte_limit) {
}

void QueryCollector::feed(std::string_view bytes, Time now) {
  if (finished_ || now >= deadline_ || received_ >= byte_limit_) {
    return;
  }

  for (const char byte : bytes) {
    if (received_ == byte_limit_) {
      limited_ = true;
      finish();

      break;
    }

    ++received_;

    if (byte == '\x1b') {
      unmatched_ += pending_;
      pending_ = byte;

      continue;
    }

    if (pending_.empty()) {
      unmatched_ += byte;

      continue;
    }

    pending_ += byte;

    if (pending_.size() == 2 && byte == '[') {
      continue;
    }

    const auto value = static_cast<unsigned char>(byte);
    const bool final = pending_.size() > 2 && value >= 0x40 && value <= 0x7e;
    const bool parameter = value >= 0x30 && value <= 0x3f;

    if (final) {
      const std::string_view body(pending_.data() + 2, pending_.size() - 3);

      if (byte == 'c' && !da_ && body.starts_with('?') && parameters(body.substr(1), false)) {
        da_ = pending_;
      } else if (byte == 'R' && !cpr_ && parameters(body, true)) {
        cpr_ = pending_;
      } else {
        unmatched_ += pending_;
      }

      pending_.clear();
    } else if (pending_.size() == 2 || !parameter || pending_.size() >= SEQUENCE_LIMIT) {
      unmatched_ += pending_;
      pending_.clear();
    }
  }

  if (received_ == byte_limit_) {
    limited_ = true;
    finish();
  }
}

void QueryCollector::finish() {
  unmatched_ += pending_;
  pending_.clear();
  finished_ = true;
}

bool QueryCollector::active(Time now) const {
  return !finished_ && now < deadline_ && received_ < byte_limit_ && !(da_ && cpr_);
}

const std::optional<std::string> &QueryCollector::deviceAttributes() const {
  return da_;
}

const std::optional<std::string> &QueryCollector::cursorPosition() const {
  return cpr_;
}

const std::string &QueryCollector::unmatched() const {
  return unmatched_;
}

std::size_t QueryCollector::received() const {
  return received_;
}

bool QueryCollector::limited() const {
  return limited_;
}

std::string escapeBytes(std::string_view bytes) {
  constexpr std::string_view HEX = "0123456789abcdef";
  std::string result = "\"";

  for (const unsigned char byte : bytes) {
    if (byte == '"' || byte == '\\') {
      result += '\\';
      result += static_cast<char>(byte);
    } else if (byte >= 0x20 && byte <= 0x7e) {
      result += static_cast<char>(byte);
    } else {
      result += "\\x";
      result += HEX[byte >> 4];
      result += HEX[byte & 0x0f];
    }
  }

  result += '"';

  return result;
}
} // namespace portals::probe

// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include <portals/diff.hpp>

#include <array>
#include <charconv>
#include <iterator>
#include <limits>
#include <new>
#include <stdexcept>

namespace portals {
namespace {
constexpr std::size_t MAX_DECIMAL_DIGITS = std::numeric_limits<std::uint32_t>::digits10 + 1;

// Each sequence is emitted as a unit so sizing and output use identical bytes.
constexpr std::string_view CANCEL_SEQUENCE = "\x18";
constexpr std::string_view FRAME_BASELINE = "\x1b[0m\x1b[?6l\x1b[r\x1b[4l\x1b[?7l";
constexpr std::string_view CLEAR_SCREEN = "\x1b[2J";
constexpr std::string_view FRAME_FINISH = "\x1b[0m\x1b[1;1H\x1b[?7h";
constexpr std::string_view SGR_RESET_PREFIX = "\x1b[0";
constexpr std::string_view SGR_BOLD = ";1";
constexpr std::string_view SGR_ITALIC = ";3";
constexpr std::string_view SGR_UNDERLINE = ";4";
constexpr std::string_view SGR_RGB_FOREGROUND = ";38;2;";
constexpr std::string_view SGR_RGB_BACKGROUND = ";48;2;";

// Count before allocating: oversized frames never build an oversized temporary.
class Encoding {
public:
  explicit Encoding(std::size_t limit, std::string *bytes = nullptr) : limit_(limit), bytes_(bytes) {
  }

  void append(std::string_view value) {
    if (value.size() > limit_ - size_) {
      throw DiffError::OUTPUT_LIMIT;
    }

    size_ += value.size();

    if (bytes_) {
      bytes_->append(value);
    }
  }

  void number(std::uint32_t value) {
    char digits[MAX_DECIMAL_DIGITS];

    const auto result = std::to_chars(std::begin(digits), std::end(digits), value);

    append({digits, static_cast<std::size_t>(result.ptr - digits)});
  }

  std::size_t size() const noexcept {
    return size_;
  }

private:
  std::size_t limit_;
  std::size_t size_{};

  std::string *bytes_;
};

// Intern IDs belong to different buffers and cannot establish semantic equality.
// Compare resolved glyph/style values and the continuation role instead.
bool equalCell(const CellView &left, const CellView &right) noexcept {
  return left.glyph == right.glyph && left.style == right.style && left.cell.columns == right.cell.columns &&
         (left.cell.kind == CellKind::CONTINUATION) == (right.cell.kind == CellKind::CONTINUATION);
}

bool equalRow(const CellBuffer &previous, const CellBuffer &next, std::int32_t y) noexcept {
  for (std::int32_t x = 0; x < next.size().width; ++x) {
    if (!equalCell(*previous.at({x, y}), *next.at({x, y}))) {
      return false;
    }
  }

  return true;
}

void color(Encoding &out, const RgbColor &rgb, std::string_view selector) {
  out.append(selector);
  out.number(rgb.red);
  out.append(";");
  out.number(rgb.green);
  out.append(";");
  out.number(rgb.blue);
}

void ansiColor(Encoding &out, RgbColor rgb, bool background) {
  constexpr std::array<RgbColor, 16> ANSI_PALETTE{{{0, 0, 0},
                                                   {170, 0, 0},
                                                   {0, 170, 0},
                                                   {170, 85, 0},
                                                   {0, 0, 170},
                                                   {170, 0, 170},
                                                   {0, 170, 170},
                                                   {170, 170, 170},
                                                   {85, 85, 85},
                                                   {255, 85, 85},
                                                   {85, 255, 85},
                                                   {255, 255, 85},
                                                   {85, 85, 255},
                                                   {255, 85, 255},
                                                   {85, 255, 255},
                                                   {255, 255, 255}}};
  constexpr std::uint32_t BASIC_FOREGROUND = 30;
  constexpr std::uint32_t BRIGHT_FOREGROUND = 90;
  constexpr std::uint32_t BACKGROUND_OFFSET = 10;
  constexpr std::size_t BASIC_COLORS = 8;
  auto distance = [](RgbColor left, RgbColor right) {
    const auto red = static_cast<int>(left.red) - right.red;
    const auto green = static_cast<int>(left.green) - right.green;
    const auto blue = static_cast<int>(left.blue) - right.blue;

    return red * red + green * green + blue * blue;
  };
  std::size_t nearest{};

  for (std::size_t index = 1; index < ANSI_PALETTE.size(); ++index) {
    if (distance(rgb, ANSI_PALETTE[index]) < distance(rgb, ANSI_PALETTE[nearest])) {
      nearest = index;
    }
  }

  const auto base = nearest < BASIC_COLORS ? BASIC_FOREGROUND : BRIGHT_FOREGROUND;

  out.append(";");
  out.number(base + static_cast<std::uint32_t>(nearest % BASIC_COLORS) + (background ? BACKGROUND_OFFSET : 0));
}

void style(Encoding &out, const CellStyle &value, ColorMode colorMode) {
  out.append(SGR_RESET_PREFIX);

  if (value.bold) {
    out.append(SGR_BOLD);
  }

  if (value.italic) {
    out.append(SGR_ITALIC);
  }

  if (value.underline) {
    out.append(SGR_UNDERLINE);
  }

  if (value.foreground) {
    if (colorMode == ColorMode::ANSI16) {
      ansiColor(out, *value.foreground, false);
    } else {
      color(out, *value.foreground, SGR_RGB_FOREGROUND);
    }
  }

  if (value.background) {
    if (colorMode == ColorMode::ANSI16) {
      ansiColor(out, *value.background, true);
    } else {
      color(out, *value.background, SGR_RGB_BACKGROUND);
    }
  }

  out.append("m");
}

void encode(Encoding &out, const CellBuffer *previous, const CellBuffer &next, ColorMode colorMode) {
  const bool full = !previous || previous->size() != next.size() || previous->policy() != next.policy();
  bool started = false;
  CellStyle current;

  // A full transaction establishes a parser boundary for our CSI/UTF-8 grammar.
  if (full) {
    out.append(CANCEL_SEQUENCE);
  }

  const auto begin = [&] {
    out.append(FRAME_BASELINE);
    started = true;

    if (full) {
      out.append(CLEAR_SCREEN);
    }
  };

  if (full) {
    begin();
  }

  // A changed row is emitted completely, with wrapping disabled, so overwriting a
  // wide glyph also clears its previous footprint without splitting grapheme bytes.
  for (std::int32_t y = 0; y < next.size().height; ++y) {
    if (!full && equalRow(*previous, next, y)) {
      continue;
    }

    if (!started) {
      begin();
    }

    out.append("\x1b[");
    out.number(static_cast<std::uint32_t>(y) + 1);
    out.append(";1H");

    for (std::int32_t x = 0; x < next.size().width; ++x) {
      const auto cell = *next.at({x, y});

      if (cell.cell.kind == CellKind::CONTINUATION) {
        continue;
      }

      if (cell.style != current) {
        style(out, cell.style, colorMode);
        current = cell.style;
      }

      out.append(cell.glyph);
    }
  }

  if (started) {
    out.append(FRAME_FINISH);
  }
}
} // namespace

std::expected<std::string, DiffError> encodeDiff(const CellBuffer *previous, const CellBuffer &next,
                                                 std::size_t max_bytes, ColorMode colorMode) {
  try {
    Encoding measure(max_bytes);

    encode(measure, previous, next, colorMode);

    // MSVC Debug allocates a proxy even for empty strings. The pointer constructor
    // can propagate that failure; the default constructor is noexcept.
    std::string bytes("");

    bytes.reserve(measure.size());

    Encoding output(max_bytes, &bytes);

    encode(output, previous, next, colorMode);

    return bytes;
  } catch (DiffError error) {
    return std::unexpected(error);
  } catch (const std::bad_alloc &) {
    return std::unexpected(DiffError::ALLOCATION_FAILURE);
  } catch (const std::length_error &) {
    return std::unexpected(DiffError::ALLOCATION_FAILURE);
  }
}
} // namespace portals

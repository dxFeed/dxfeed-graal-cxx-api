// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#include <portals/text.hpp>

#include <utf8proc.h>

#include <algorithm>
#include <limits>
#include <utility>

namespace portals {
namespace {
constexpr utf8proc_int32_t TEXT_SELECTOR = 0xfe0e;
constexpr utf8proc_int32_t EMOJI_SELECTOR = 0xfe0f;
constexpr utf8proc_int32_t ZERO_WIDTH_JOINER = 0x200d;
constexpr utf8proc_int32_t KEYCAP_MARK = 0x20e3;
constexpr unsigned UNSUPPORTED_SCALAR_WIDTH = 3;

// Grapheme boundaries come from utf8proc; terminal display support is a separate policy.
// Preserve unsupported clusters as logical text and replace only their displayed footprint.
struct ClusterWidth {
  unsigned scalar_cells{};
  unsigned regional_indicators{};
  bool pictographic{};
  bool wide_pictographic{};
  bool emoji_sequence{};
  bool text_style{};
  bool unsafe{};
  bool keycap_base{};
  bool keycap{};
  bool emoji_format{};

  void add(utf8proc_int32_t cp, WidthPolicy policy, bool first) noexcept {
    const auto *property = utf8proc_get_property(cp);
    auto width = utf8proc_charwidth(cp);
    const bool pictograph = property->boundclass == UTF8PROC_BOUNDCLASS_EXTENDED_PICTOGRAPHIC;
    const bool tag = cp >= 0xe0020 && cp <= 0xe007f;
    const bool modifier = cp >= 0x1f3fb && cp <= 0x1f3ff;
    const bool format_in_emoji = cp == ZERO_WIDTH_JOINER || tag;

    unsafe = unsafe || property->category == UTF8PROC_CATEGORY_CC || property->category == UTF8PROC_CATEGORY_CN ||
             property->category == UTF8PROC_CATEGORY_ZL || property->category == UTF8PROC_CATEGORY_ZP ||
             (property->category == UTF8PROC_CATEGORY_CF && !format_in_emoji);
    pictographic = pictographic || pictograph;
    wide_pictographic = wide_pictographic || (pictograph && width == 2);
    emoji_sequence = emoji_sequence || cp == EMOJI_SELECTOR || cp == ZERO_WIDTH_JOINER || tag || modifier;
    text_style = text_style || cp == TEXT_SELECTOR;
    regional_indicators += property->boundclass == UTF8PROC_BOUNDCLASS_REGIONAL_INDICATOR ? 1u : 0u;

    if (first) {
      keycap_base = (cp >= '0' && cp <= '9') || cp == '#' || cp == '*';
    }

    if (!first && cp != EMOJI_SELECTOR && cp != KEYCAP_MARK) {
      keycap_base = false;
    }

    keycap = keycap || (keycap_base && cp == KEYCAP_MARK);
    emoji_format = emoji_format || format_in_emoji;

    if (width == 1 && policy.ambiguous == AmbiguousWidth::WIDE && utf8proc_charwidth_ambiguous(cp)) {
      width = 2;
    }

    // Saturation suffices: ordinary clusters wider than two cells are unsupported.
    scalar_cells = std::min(UNSUPPORTED_SCALAR_WIDTH, scalar_cells + static_cast<unsigned>(std::max(0, width)));
  }

  Grapheme finish(std::size_t begin, std::size_t end, WidthPolicy policy) const noexcept {
    const bool emoji =
      keycap || regional_indicators != 0 || (pictographic && !text_style && (wide_pictographic || emoji_sequence));
    const auto cells = emoji ? static_cast<unsigned>(policy.emoji) : scalar_cells;
    const bool unsupported = unsafe || (emoji_format && !emoji) || cells == 0 || cells > 2;

    return {begin, end, static_cast<std::uint8_t>(unsupported ? 1 : cells),
            unsupported ? TextDisplay::UNSUPPORTED : TextDisplay::PRINTABLE};
  }
};
} // namespace

GraphemeText &GraphemeText::operator=(const GraphemeText &other) {
  if (this != &other) {
    GraphemeText copy(other);

    *this = std::move(copy);
  }

  return *this;
}

// Member initialization may allocate MSVC iterator proxies. Do not touch the source
// until both empty containers exist; the following assignment itself allocates nothing.
GraphemeText::GraphemeText(GraphemeText &&other) {
  *this = std::move(other);
}

GraphemeText &GraphemeText::operator=(GraphemeText &&other) noexcept {
  if (this != &other) {
    bytes_ = std::move(other.bytes_);
    graphemes_ = std::move(other.graphemes_);
    policy_ = other.policy_;
    cells_ = std::exchange(other.cells_, 0);
    other.bytes_.clear();
    other.graphemes_.clear();
  }

  return *this;
}

std::expected<GraphemeText, TextError> GraphemeText::from(std::string_view bytes, WidthPolicy policy,
                                                          TextLimits limits) {
  if ((policy.ambiguous != AmbiguousWidth::NARROW && policy.ambiguous != AmbiguousWidth::WIDE) ||
      (policy.emoji != EmojiWidth::ONE && policy.emoji != EmojiWidth::TWO)) {
    return std::unexpected(TextError{TextErrorKind::INVALID_POLICY, 0});
  }

  if (bytes.size() > limits.max_input_bytes || bytes.size() > static_cast<std::size_t>(PTRDIFF_MAX)) {
    return std::unexpected(TextError{TextErrorKind::INPUT_LIMIT, 0});
  }

  GraphemeText result;

  result.policy_ = policy;

  const auto max_cells = std::min(limits.max_cells, static_cast<std::size_t>(INT32_MAX));
  std::size_t begin = 0;
  std::size_t offset = 0;
  utf8proc_int32_t previous = 0;
  utf8proc_int32_t state = 0;
  ClusterWidth cluster;

  // Commit metadata only for a complete cluster. The result stays local until all input
  // is validated, so a later decoding or limit error never exposes a partial text value.
  const auto append = [&](std::size_t end) -> std::expected<void, TextError> {
    if (result.graphemes_.size() >= limits.max_graphemes) {
      return std::unexpected(TextError{TextErrorKind::GRAPHEME_LIMIT, begin});
    }

    const auto grapheme = cluster.finish(begin, end, policy);

    if (grapheme.cells > max_cells - static_cast<std::size_t>(result.cells_)) {
      return std::unexpected(TextError{TextErrorKind::CELL_LIMIT, begin});
    }

    result.graphemes_.push_back(grapheme);
    result.cells_ += grapheme.cells;

    return {};
  };

  while (offset < bytes.size()) {
    utf8proc_int32_t cp = 0;
    const auto length = utf8proc_iterate(reinterpret_cast<const utf8proc_uint8_t *>(bytes.data() + offset),
                                         static_cast<utf8proc_ssize_t>(bytes.size() - offset), &cp);

    if (length < 0) {
      return std::unexpected(TextError{TextErrorKind::INVALID_UTF8, offset});
    }

    if (offset != 0 && utf8proc_grapheme_break_stateful(previous, cp, &state)) {
      const auto appended = append(offset);

      if (!appended) {
        return std::unexpected(appended.error());
      }

      begin = offset;
      cluster = {};
    }

    const auto next = offset + static_cast<std::size_t>(length);

    if (next - begin > limits.max_cluster_bytes) {
      return std::unexpected(TextError{TextErrorKind::CLUSTER_LIMIT, begin});
    }

    cluster.add(cp, policy, offset == begin);
    previous = cp;
    offset = next;
  }

  if (!bytes.empty()) {
    const auto appended = append(offset);

    if (!appended) {
      return std::unexpected(appended.error());
    }
  }

  result.bytes_ = std::string(bytes);

  return result;
}

std::string_view GraphemeText::bytes() const noexcept {
  return bytes_;
}

std::span<const Grapheme> GraphemeText::graphemes() const noexcept {
  return graphemes_;
}

std::int32_t GraphemeText::measure() const noexcept {
  return cells_;
}

WidthPolicy GraphemeText::policy() const noexcept {
  return policy_;
}

std::expected<std::size_t, TextError> GraphemeText::nextCursor(std::size_t byte_offset) const noexcept {
  if (byte_offset == bytes_.size()) {
    return byte_offset;
  }

  const auto found = std::ranges::lower_bound(graphemes_, byte_offset, {}, &Grapheme::byte_begin);

  if (found == graphemes_.end() || found->byte_begin != byte_offset) {
    return std::unexpected(TextError{TextErrorKind::NOT_BOUNDARY, byte_offset});
  }

  return found->byte_end;
}

std::expected<std::size_t, TextError> GraphemeText::previousCursor(std::size_t byte_offset) const noexcept {
  if (byte_offset == 0) {
    return 0;
  }

  const auto found = std::ranges::lower_bound(graphemes_, byte_offset, {}, &Grapheme::byte_end);

  if (found == graphemes_.end() || found->byte_end != byte_offset) {
    return std::unexpected(TextError{TextErrorKind::NOT_BOUNDARY, byte_offset});
  }

  return found->byte_begin;
}

TextClip GraphemeText::clipPrefix(std::int32_t max_cells) const noexcept {
  TextClip result;

  max_cells = std::max(0, max_cells);

  for (const auto &grapheme : graphemes_) {
    if (grapheme.cells > max_cells - result.cells) {
      break;
    }

    result.byte_end = grapheme.byte_end;
    ++result.graphemes;
    result.cells += grapheme.cells;
  }

  return result;
}

} // namespace portals

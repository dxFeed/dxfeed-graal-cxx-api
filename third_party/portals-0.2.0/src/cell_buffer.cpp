// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#include <portals/cell_buffer.hpp>

#include <algorithm>
#include <limits>
#include <new>
#include <utility>

namespace portals {
namespace {
bool validPolicy(WidthPolicy policy) noexcept {
  return (policy.ambiguous == AmbiguousWidth::NARROW || policy.ambiguous == AmbiguousWidth::WIDE) &&
         (policy.emoji == EmojiWidth::ONE || policy.emoji == EmojiWidth::TWO);
}

std::expected<std::size_t, CellBufferError> checkedCells(CellSize size, CellBufferLimits limits) noexcept {
  const auto columns = static_cast<std::size_t>(size.width);
  const auto rows = static_cast<std::size_t>(size.height);

  if (columns > limits.max_columns) {
    return std::unexpected(CellBufferError::COLUMN_LIMIT);
  }

  if (rows > limits.max_rows) {
    return std::unexpected(CellBufferError::ROW_LIMIT);
  }

  if (rows != 0 && columns > limits.max_cells / rows) {
    return std::unexpected(CellBufferError::CELL_LIMIT);
  }

  return rows * columns;
}

bool addAllocation(std::size_t &total, std::size_t count, std::size_t element_bytes) noexcept {
  const auto allocation_limit = static_cast<std::size_t>(std::numeric_limits<std::ptrdiff_t>::max());

  if (count > allocation_limit / element_bytes) {
    return false;
  }

  const auto bytes = count * element_bytes;

  if (bytes > std::numeric_limits<std::size_t>::max() - total) {
    return false;
  }

  total += bytes;

  return true;
}
} // namespace

CellBuffer::CellBuffer(CellBuffer &&other) noexcept {
  swap(other);
}

CellBuffer &CellBuffer::operator=(CellBuffer &&other) noexcept {
  if (this != &other) {
    CellBuffer replacement(std::move(other));

    swap(replacement);
  }

  return *this;
}

std::expected<CellBuffer, CellBufferError> CellBuffer::from(CellSize size, WidthPolicy policy,
                                                            CellBufferLimits limits) {
  const auto coordinate_limit = static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max());
  const auto id_count_limit = static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max());
  std::size_t maximum_bytes = sizeof(CellBuffer);

  if (limits.max_columns > coordinate_limit || limits.max_rows > coordinate_limit || limits.max_glyphs == 0 ||
      limits.max_glyphs > id_count_limit || limits.max_glyph_bytes == 0 || limits.max_styles == 0 ||
      limits.max_styles > id_count_limit || !addAllocation(maximum_bytes, limits.max_cells, sizeof(Cell)) ||
      !addAllocation(maximum_bytes, limits.max_glyphs, sizeof(GlyphRecord)) ||
      !addAllocation(maximum_bytes, limits.max_glyph_bytes, sizeof(char)) ||
      !addAllocation(maximum_bytes, limits.max_styles, sizeof(CellStyle))) {
    return std::unexpected(CellBufferError::INVALID_LIMITS);
  }

  if (!validPolicy(policy)) {
    return std::unexpected(CellBufferError::INVALID_POLICY);
  }

  size = size.normalized();

  const auto cell_count = checkedCells(size, limits);

  if (!cell_count) {
    return std::unexpected(cell_count.error());
  }

  try {
    CellBuffer result;

    result.size_ = size;
    result.policy_ = policy;
    result.limits_ = limits;
    result.cell_count_ = *cell_count;

    if (*cell_count != 0) {
      result.cells_ = std::make_unique<Cell[]>(*cell_count);
    }

    result.glyphs_ = std::make_unique<GlyphRecord[]>(limits.max_glyphs);
    result.bytes_ = std::make_unique_for_overwrite<char[]>(limits.max_glyph_bytes);
    result.styles_ = std::make_unique<CellStyle[]>(limits.max_styles);

    result.clear();

    return result;
  } catch (const std::bad_alloc &) {
    return std::unexpected(CellBufferError::ALLOCATION_FAILURE);
  }
}

CellSize CellBuffer::size() const noexcept {
  return size_;
}

WidthPolicy CellBuffer::policy() const noexcept {
  return policy_;
}

CellBufferLimits CellBuffer::limits() const noexcept {
  return limits_;
}

std::span<const Cell> CellBuffer::cells() const noexcept {
  return {cells_.get(), cell_count_};
}

bool CellBuffer::contains(CellPoint position) const noexcept {
  return position.x >= 0 && position.y >= 0 && position.x < size_.width && position.y < size_.height;
}

std::size_t CellBuffer::index(CellPoint position) const noexcept {
  return static_cast<std::size_t>(position.y) * static_cast<std::size_t>(size_.width) +
         static_cast<std::size_t>(position.x);
}

std::expected<CellView, CellBufferError> CellBuffer::at(CellPoint position) const noexcept {
  if (!contains(position)) {
    return std::unexpected(CellBufferError::OUT_OF_BOUNDS);
  }

  const auto cell = cells_[index(position)];
  const auto &glyph = glyphs_[cell.glyph.value];

  return CellView{cell, {bytes_.get() + glyph.offset, glyph.bytes}, styles_[cell.style.value]};
}

// Replacing either half removes the complete old glyph, even if its other half lies
// outside the new write. A continuation must never survive without its matching lead.
void CellBuffer::clearGlyph(std::size_t cell_index) noexcept {
  const auto old = cells_[cell_index];

  if (old.kind == CellKind::CONTINUATION) {
    cells_[cell_index - 1] = {};
  } else if (old.columns == 2) {
    cells_[cell_index + 1] = {};
  }

  cells_[cell_index] = {};
}

std::expected<void, CellBufferError> CellBuffer::put(CellPoint position, const GraphemeText &text,
                                                     std::size_t grapheme_index, CellStyle style) noexcept {
  if (!contains(position)) {
    return std::unexpected(CellBufferError::OUT_OF_BOUNDS);
  }

  if (grapheme_index >= text.graphemes().size()) {
    return std::unexpected(CellBufferError::INVALID_GRAPHEME);
  }

  if (text.policy() != policy_) {
    return std::unexpected(CellBufferError::POLICY_MISMATCH);
  }

  const auto &grapheme = text.graphemes()[grapheme_index];

  if (grapheme.cells == 2 && position.x == size_.width - 1) {
    return std::unexpected(CellBufferError::WIDE_AT_EDGE);
  }

  const auto glyph_bytes = grapheme.display == TextDisplay::UNSUPPORTED
                             ? std::string_view("?")
                             : text.bytes().substr(grapheme.byte_begin, grapheme.byte_end - grapheme.byte_begin);
  std::size_t glyph_id = 0;

  for (; glyph_id < glyph_count_; ++glyph_id) {
    const auto &candidate = glyphs_[glyph_id];

    if (candidate.columns == grapheme.cells &&
        std::string_view(bytes_.get() + candidate.offset, candidate.bytes) == glyph_bytes) {
      break;
    }
  }

  std::size_t style_id = 0;

  for (; style_id < style_count_; ++style_id) {
    if (styles_[style_id] == style) {
      break;
    }
  }

  if (glyph_id == glyph_count_) {
    if (glyph_count_ == limits_.max_glyphs) {
      return std::unexpected(CellBufferError::GLYPH_LIMIT);
    }

    if (glyph_bytes.size() > limits_.max_glyph_bytes - byte_count_) {
      return std::unexpected(CellBufferError::GLYPH_BYTE_LIMIT);
    }
  }

  if (style_id == style_count_ && style_count_ == limits_.max_styles) {
    return std::unexpected(CellBufferError::STYLE_LIMIT);
  }

  // All coordinate, policy and pool-capacity checks precede this commit point.
  // From here onward every operation uses preallocated storage and cannot fail.
  if (glyph_id == glyph_count_) {
    std::ranges::copy(glyph_bytes, bytes_.get() + byte_count_);
    glyphs_[glyph_id] = {byte_count_, glyph_bytes.size(), grapheme.cells};
    byte_count_ += glyph_bytes.size();
    ++glyph_count_;
  }

  if (style_id == style_count_) {
    styles_[style_id] = style;
    ++style_count_;
  }

  const auto destination = index(position);

  clearGlyph(destination);

  if (grapheme.cells == 2) {
    clearGlyph(destination + 1);
  }

  const GlyphId glyph{static_cast<std::uint32_t>(glyph_id)};
  const StyleId interned_style{static_cast<std::uint32_t>(style_id)};

  cells_[destination] = {glyph, interned_style, CellKind::LEAD, grapheme.cells};

  if (grapheme.cells == 2) {
    cells_[destination + 1] = {glyph, interned_style, CellKind::CONTINUATION, 0};
  }

  return {};
}

void CellBuffer::clear() noexcept {
  for (std::size_t i = 0; i < cell_count_; ++i) {
    cells_[i] = {};
  }

  if (glyphs_) {
    glyphs_[0] = {0, 1, 1};
    bytes_[0] = ' ';
    styles_[0] = {};
    glyph_count_ = 1;
    byte_count_ = 1;
    style_count_ = 1;
  }
}

std::expected<void, CellBufferError> CellBuffer::resize(CellSize size) {
  auto replacement = from(size, policy_, limits_);

  if (!replacement) {
    return std::unexpected(replacement.error());
  }

  swap(*replacement);

  return {};
}

CellBufferMemory CellBuffer::memory() const noexcept {
  const auto glyph_capacity = glyphs_ ? limits_.max_glyphs : 0;
  const auto byte_capacity = bytes_ ? limits_.max_glyph_bytes : 0;
  const auto style_capacity = styles_ ? limits_.max_styles : 0;
  const auto cell_bytes = cell_count_ * sizeof(Cell);
  const auto record_bytes = glyph_capacity * sizeof(GlyphRecord);
  const auto style_bytes = style_capacity * sizeof(CellStyle);

  return {sizeof(CellBuffer), cell_bytes,  record_bytes,
          byte_capacity,      style_bytes, sizeof(CellBuffer) + cell_bytes + record_bytes + byte_capacity + style_bytes,
          glyph_count_,       byte_count_, style_count_};
}

void CellBuffer::swap(CellBuffer &other) noexcept {
  using std::swap;
  swap(size_, other.size_);
  swap(policy_, other.policy_);
  swap(limits_, other.limits_);
  swap(cells_, other.cells_);
  swap(glyphs_, other.glyphs_);
  swap(bytes_, other.bytes_);
  swap(styles_, other.styles_);
  swap(cell_count_, other.cell_count_);
  swap(glyph_count_, other.glyph_count_);
  swap(byte_count_, other.byte_count_);
  swap(style_count_, other.style_count_);
}

} // namespace portals

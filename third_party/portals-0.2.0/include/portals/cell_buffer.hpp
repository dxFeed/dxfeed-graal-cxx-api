// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <portals/geometry.hpp>
#include <portals/text.hpp>

#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <optional>
#include <span>
#include <string_view>

namespace portals {

/** @brief Owning RGB color; terminal encoding belongs to the output adapter. */
struct RgbColor {
  /** @brief Red component. */
  std::uint8_t red{};
  /** @brief Green component. */
  std::uint8_t green{};
  /** @brief Blue component. */
  std::uint8_t blue{};
  /** @brief Compare all components.
   * @param other Value whose corresponding fields are compared.
   * @return True when every compared field is equal; false otherwise.
   */
  bool operator==(const RgbColor &other) const noexcept = default;
};

/** @brief Small owning paint value, independent of themes and text metrics. */
struct CellStyle {
  /** @brief Foreground RGB, or the terminal default when absent. */
  std::optional<RgbColor> foreground;
  /** @brief Background RGB, or the terminal default when absent. */
  std::optional<RgbColor> background;
  /** @brief Request bold rendering. */
  bool bold{};
  /** @brief Request italic rendering. */
  bool italic{};
  /** @brief Request underlined rendering. */
  bool underline{};
  /** @brief Compare the complete value.
   * @param other Value whose corresponding fields are compared.
   * @return True when every compared field is equal; false otherwise.
   */
  bool operator==(const CellStyle &other) const noexcept = default;
};

/** @brief Output-only glyph index scoped to one buffer between clear/resize/move. */
struct GlyphId {
  /** @brief Local index; zero identifies the blank glyph. Never external authority. */
  std::uint32_t value{};
  /** @brief Compare local indices; equality across buffers has no semantic meaning.
   * @param other Value whose corresponding fields are compared.
   * @return True when every compared field is equal; false otherwise.
   */
  bool operator==(const GlyphId &other) const noexcept = default;
};

/** @brief Output-only style index scoped to one buffer between clear/resize/move. */
struct StyleId {
  /** @brief Local index; zero identifies the default style. */
  std::uint32_t value{};
  /** @brief Compare local indices; equality across buffers has no semantic meaning.
   * @param other Value whose corresponding fields are compared.
   * @return True when every compared field is equal; false otherwise.
   */
  bool operator==(const StyleId &other) const noexcept = default;
};

/** @brief A cell's role in a complete display grapheme. */
enum class CellKind {
  /** @brief Default-style ASCII space, occupying one cell. */
  BLANK,
  /** @brief First cell of a written one- or two-cell grapheme. */
  LEAD,
  /** @brief Second cell of the immediately preceding two-cell lead. */
  CONTINUATION
};

/** @brief Owning metadata; IDs may only be interpreted within the source buffer. */
struct Cell {
  /** @brief Interned display grapheme, shared by both halves of a wide glyph. */
  GlyphId glyph;
  /** @brief Interned style, shared by both halves of a wide glyph. */
  StyleId style;
  /** @brief Blank, lead or continuation role. */
  CellKind kind{CellKind::BLANK};
  /** @brief Display width: one for blank/narrow, two for wide lead, zero for continuation. */
  std::uint8_t columns{1};
  /** @brief Compare metadata, including local IDs.
   * @param other Value whose corresponding fields are compared.
   * @return True when every compared field is equal; false otherwise.
   */
  bool operator==(const Cell &other) const noexcept = default;
};

/** @brief A synchronous borrowed display glyph plus owning metadata and style. */
struct CellView {
  /** @brief Copied cell metadata with source-buffer-local IDs. */
  Cell cell;
  /** @brief Borrowed display UTF-8; continuation resolves the same bytes as its lead. */
  std::string_view glyph;
  /** @brief Independent style value. */
  CellStyle style;
};

/** @brief Owning caps; creation allocates all intern pool capacity, including defaults. */
struct CellBufferLimits {
  /** @brief Maximum normalized columns, bounded additionally by INT32_MAX. */
  std::size_t max_columns{4096};
  /** @brief Maximum normalized rows, bounded additionally by INT32_MAX. */
  std::size_t max_rows{4096};
  /** @brief Maximum cells in the checked rows-times-columns product. */
  std::size_t max_cells{4 * 1024 * 1024};
  /** @brief Maximum interned display glyphs, including the blank glyph. Must be positive. */
  std::size_t max_glyphs{65536};
  /** @brief Maximum interned UTF-8 bytes, including one blank byte. Must be positive. */
  std::size_t max_glyph_bytes{1024 * 1024};
  /** @brief Maximum interned styles, including the default style. Must be positive. */
  std::size_t max_styles{4096};
  /** @brief Compare all resource caps.
   * @param other Value whose corresponding fields are compared.
   * @return True when every compared field is equal; false otherwise.
   */
  bool operator==(const CellBufferLimits &other) const noexcept = default;
};

/** @brief Observable, owning buffer failure; no partial write or resize is returned. */
enum class CellBufferError {
  /** @brief Invalid caps, ID representation or allocation-size arithmetic. */
  INVALID_LIMITS,
  /** @brief An unrecognized width policy value. */
  INVALID_POLICY,
  /** @brief Normalized columns exceed the cap. */
  COLUMN_LIMIT,
  /** @brief Normalized rows exceed the cap. */
  ROW_LIMIT,
  /** @brief Checked cell product exceeds the cap or representation. */
  CELL_LIMIT,
  /** @brief A position lies outside the buffer. */
  OUT_OF_BOUNDS,
  /** @brief A grapheme index lies outside the validated text. */
  INVALID_GRAPHEME,
  /** @brief The text's complete captured width policy differs from this buffer. */
  POLICY_MISMATCH,
  /** @brief The whole wide glyph cannot fit at the right edge. */
  WIDE_AT_EDGE,
  /** @brief The glyph record pool is full. */
  GLYPH_LIMIT,
  /** @brief The glyph byte pool cannot hold the new complete glyph. */
  GLYPH_BYTE_LIMIT,
  /** @brief The style pool is full. */
  STYLE_LIMIT,
  /** @brief Storage allocation failed; the original buffer remains unchanged. */
  ALLOCATION_FAILURE
};

/** @brief Measured C++ storage accounting, excluding allocator overhead and process RSS. */
struct CellBufferMemory {
  /** @brief sizeof(CellBuffer), including inline metadata and unique_ptr owners. */
  std::size_t object_bytes{};
  /** @brief Allocated cell array bytes (exact element count times sizeof(Cell)). */
  std::size_t cell_storage_bytes{};
  /** @brief Allocated glyph-record array bytes, including unused slots. */
  std::size_t glyph_record_storage_bytes{};
  /** @brief Allocated UTF-8 byte array capacity. */
  std::size_t glyph_byte_storage_bytes{};
  /** @brief Allocated style array bytes, including unused slots. */
  std::size_t style_storage_bytes{};
  /** @brief Sum of object and allocated array bytes; not RSS or peak resize memory. */
  std::size_t total_bytes{};
  /** @brief Current interned glyph count, including the blank when storage exists. */
  std::size_t glyph_count{};
  /** @brief Current used UTF-8 bytes, including the blank when storage exists. */
  std::size_t glyph_bytes{};
  /** @brief Current interned style count, including the default when storage exists. */
  std::size_t style_count{};
  /** @brief Compare accounting and counts.
   * @param other Value whose corresponding fields are compared.
   * @return True when every compared field is equal; false otherwise.
   */
  bool operator==(const CellBufferMemory &other) const noexcept = default;
};

/**
 * @brief Owning bounded cell framebuffer and immutable-width-policy intern pools.
 *
 * No I/O, callbacks, worker threads or global storage. Any thread may own a
 * buffer; the UI owner mutates it with caller synchronization. Concurrent reads
 * of an unchanged buffer are safe. IDs are local output metadata, never accepted
 * as lookup/write authority. put copies complete display bytes and style.
 * Successful put/clear/resize/assignment invalidates borrowed views logically;
 * failures preserve views, cells, intern counts and accounting. Moving invalidates
 * source views and leaves a distinct source empty, with zero allocated storage;
 * it can be cleared, resized, assigned and queried. Self-move is a no-op.
 * No table eviction occurs: even overwritten entries remain until clear/resize.
 */
class CellBuffer {
public:
  /** @brief Transfer storage and leave a distinct source empty; no allocation or errors.
   * @param other Sole storage owner to consume; it becomes empty with default policy and limits.
   */
  CellBuffer(CellBuffer &&other) noexcept;
  /** @brief Transfer storage, invalidate both objects' views and empty a distinct source.
   * @param other Storage owner to consume; self-move preserves the buffer.
   * @return This buffer after replacement; a distinct source has empty storage.
   */
  CellBuffer &operator=(CellBuffer &&other) noexcept;
  /** @brief Buffers have one owner; copying is disabled. */
  CellBuffer(const CellBuffer &) = delete;
  /** @brief Buffers have one owner; copy assignment is disabled. */
  CellBuffer &operator=(const CellBuffer &) = delete;

  /**
   * @brief Allocate a blank buffer and fixed-capacity glyph/byte/style pools.
   * @param size Owning cell extents; negative components normalize to zero.
   * @param policy Owning complete width policy, captured for all future writes.
   * @param limits Owning caps, validated before allocation and multiplication.
   * @return Independent buffer or explicit limit/policy/allocation error.
   * @note Any thread; allocation failures are returned, never partial results.
   */
  [[nodiscard]] static std::expected<CellBuffer, CellBufferError> from(CellSize size, WidthPolicy policy = {},
                                                                       CellBufferLimits limits = {});
  /** @brief Return owning normalized extents; no errors or allocation.
   * @return The current nonnegative width and height in terminal cells.
   */
  [[nodiscard]] CellSize size() const noexcept;
  /** @brief Return the owning captured width policy; no errors or allocation.
   * @return An independent copy of the policy accepted by put().
   */
  [[nodiscard]] WidthPolicy policy() const noexcept;
  /** @brief Return the owning resource caps; no errors or allocation.
   * @return An independent copy of the caps used by creation and resize.
   */
  [[nodiscard]] CellBufferLimits limits() const noexcept;
  /** @brief Borrow row-major metadata until successful mutation/move/destruction; no errors.
   * @return Borrowed row-major cell records; interpret IDs only through this buffer.
   */
  [[nodiscard]] std::span<const Cell> cells() const noexcept;
  /**
   * @brief Resolve by coordinate; no API accepts arbitrary glyph/style IDs.
   * @param position Owning coordinate, borrowed storage is never retained.
   * @return Borrowed display bytes and copied metadata/style, or OUT_OF_BOUNDS.
   * @note No allocation; view expires on successful mutation/move/destruction.
   */
  [[nodiscard]] std::expected<CellView, CellBufferError> at(CellPoint position) const noexcept;
  /**
   * @brief Atomically write one complete grapheme, clearing every intersected old glyph.
   * @param position Owning first-cell coordinate.
   * @param text Validated text, borrowed synchronously; display bytes are copied.
   * @param grapheme_index Index in text.graphemes(), not a byte offset.
   * @param style Owning paint value, interned by equality.
   * @return Success or coordinate/index/policy/edge/pool error; failure changes nothing.
   * @note No allocation. Unsupported text displays ASCII '?'. Touching an old
   * continuation clears its lead too, even outside the new glyph's footprint.
   */
  [[nodiscard]] std::expected<void, CellBufferError> put(CellPoint position, const GraphemeText &text,
                                                         std::size_t grapheme_index, CellStyle style = {}) noexcept;
  /** @brief Reset to default blanks and reset intern counts; retain capacity, no errors/allocation. */
  void clear() noexcept;
  /**
   * @brief Allocate a new blank extent with the same policy/caps; discard old contents on success.
   * @param size Owning extents; negative components normalize to zero.
   * @return Success or extent/allocation error; failure preserves all old storage.
   * @note New and old storage coexist during allocation; transient peak can exceed memory().
   */
  [[nodiscard]] std::expected<void, CellBufferError> resize(CellSize size);
  /** @brief Return measured owned storage/counts, excluding allocator overhead; no errors/allocation.
   * @return Current owned capacities and used counts; neither peak allocation nor process memory.
   */
  [[nodiscard]] CellBufferMemory memory() const noexcept;

private:
  struct GlyphRecord {
    std::size_t offset{};
    std::size_t bytes{};
    std::uint8_t columns{};
  };

  CellBuffer() = default;
  void swap(CellBuffer &other) noexcept;
  [[nodiscard]] bool contains(CellPoint position) const noexcept;
  [[nodiscard]] std::size_t index(CellPoint position) const noexcept;
  void clearGlyph(std::size_t cell_index) noexcept;

  CellSize size_{};
  WidthPolicy policy_{};
  CellBufferLimits limits_{};
  std::unique_ptr<Cell[]> cells_;
  std::unique_ptr<GlyphRecord[]> glyphs_;
  std::unique_ptr<char[]> bytes_;
  std::unique_ptr<CellStyle[]> styles_;
  std::size_t cell_count_{};
  std::size_t glyph_count_{};
  std::size_t byte_count_{};
  std::size_t style_count_{};
};

} // namespace portals

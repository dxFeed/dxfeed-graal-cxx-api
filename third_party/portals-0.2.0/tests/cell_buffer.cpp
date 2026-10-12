// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#include <portals/cell_buffer.hpp>

#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <string>
#include <utility>
#include <vector>

namespace {
int array_failure_countdown = -1;
std::size_t live_arrays = 0;
int failures = 0;
} // namespace

// Exercise allocation failure without requesting enormous or platform-specific allocations.
void *operator new[](std::size_t bytes) {
  if (array_failure_countdown == 0) {
    array_failure_countdown = -1;
    throw std::bad_alloc();
  }

  if (array_failure_countdown > 0) {
    --array_failure_countdown;
  }

  if (auto *memory = std::malloc(bytes == 0 ? 1 : bytes)) {
    ++live_arrays;

    return memory;
  }

  throw std::bad_alloc();
}

void operator delete[](void *memory) noexcept {
  if (memory) {
    --live_arrays;
    std::free(memory);
  }
}

void operator delete[](void *memory, std::size_t) noexcept {
  ::operator delete[](memory);
}

namespace {
using namespace portals;

void check(bool condition, const char *message) {
  if (!condition) {
    if (failures < 20) {
      std::cerr << message << '\n';
    }

    ++failures;
  }
}

std::string utf8(std::u8string_view text) {
  return {reinterpret_cast<const char *>(text.data()), text.size()};
}

CellBufferLimits smallLimits() {
  return {32, 16, 256, 64, 1024, 16};
}

CellBuffer makeBuffer(CellSize size, CellBufferLimits limits = smallLimits(), WidthPolicy policy = {}) {
  auto buffer = CellBuffer::from(size, policy, limits);

  if (!buffer) {
    std::cerr << "fixture buffer construction failed\n";
    std::exit(2);
  }

  return std::move(*buffer);
}

GraphemeText makeText(std::string_view bytes, WidthPolicy policy = {}) {
  auto text = GraphemeText::from(bytes, policy);

  if (!text) {
    std::cerr << "fixture text construction failed\n";
    std::exit(2);
  }

  return std::move(*text);
}

struct SnapshotCell {
  Cell cell;
  std::string glyph;
  CellStyle style;
  bool operator==(const SnapshotCell &) const = default;
};

std::vector<SnapshotCell> snapshot(const CellBuffer &buffer) {
  std::vector<SnapshotCell> result;

  for (std::int32_t y = 0; y < buffer.size().height; ++y) {
    for (std::int32_t x = 0; x < buffer.size().width; ++x) {
      const auto view = buffer.at({x, y});

      check(view.has_value(), "all cell coordinates resolve");

      if (view) {
        result.push_back({view->cell, std::string(view->glyph), view->style});
      }
    }
  }

  return result;
}

void invariant(const CellBuffer &buffer) {
  const auto count = static_cast<std::size_t>(buffer.size().width) * static_cast<std::size_t>(buffer.size().height);
  const auto memory = buffer.memory();
  const auto limits = buffer.limits();

  check(buffer.cells().size() == count, "normalized dimensions match row-major storage");
  check(memory.glyph_count <= limits.max_glyphs && memory.glyph_bytes <= limits.max_glyph_bytes &&
          memory.style_count <= limits.max_styles,
        "intern usage is bounded");
  check(memory.cell_storage_bytes == count * sizeof(Cell), "cell allocation has exact element accounting");
  check(memory.object_bytes == sizeof(CellBuffer) &&
          memory.total_bytes == memory.object_bytes + memory.cell_storage_bytes + memory.glyph_record_storage_bytes +
                                  memory.glyph_byte_storage_bytes + memory.style_storage_bytes,
        "memory total measures object plus allocated arrays");

  for (std::int32_t y = 0; y < buffer.size().height; ++y) {
    for (std::int32_t x = 0; x < buffer.size().width; ++x) {
      const auto view = buffer.at({x, y});
      const auto &cell = view->cell;
      const auto offset =
        static_cast<std::size_t>(y) * static_cast<std::size_t>(buffer.size().width) + static_cast<std::size_t>(x);

      check(cell == buffer.cells()[offset], "span and coordinate lookup agree");
      check(cell.glyph.value < memory.glyph_count && cell.style.value < memory.style_count,
            "every live ID resolves within its local pool");
      check(!view->glyph.empty(), "every cell resolves nonempty display bytes");

      if (cell.kind == CellKind::BLANK) {
        check(cell == Cell{} && view->glyph == " " && view->style == CellStyle{}, "blank resolves default values");
      } else if (cell.kind == CellKind::CONTINUATION) {
        check(x > 0 && cell.columns == 0, "continuation never starts a row");

        if (x > 0) {
          const auto lead = buffer.at({x - 1, y});

          check(lead->cell.kind == CellKind::LEAD && lead->cell.columns == 2 && lead->cell.glyph == cell.glyph &&
                  lead->cell.style == cell.style && lead->glyph == view->glyph && lead->style == view->style,
                "continuation always matches the immediately preceding wide lead");
        }
      } else {
        check(cell.columns == 1 || cell.columns == 2, "lead width is one or two");

        if (cell.columns == 2) {
          check(x + 1 < buffer.size().width, "wide lead never ends a row");

          if (x + 1 < buffer.size().width) {
            const auto continuation = buffer.at({x + 1, y});

            check(continuation->cell.kind == CellKind::CONTINUATION && continuation->cell.glyph == cell.glyph &&
                    continuation->cell.style == cell.style,
                  "wide lead has its matching continuation");
          }
        }
      }
    }
  }
}

void overwriteAndLifetime() {
  auto buffer = makeBuffer({6, 2});
  auto wide = makeText(utf8(u8"界"));
  const auto narrow = makeText("x");
  CellStyle colored;

  colored.foreground = RgbColor{10, 20, 30};
  colored.background = RgbColor{40, 50, 60};
  colored.bold = true;
  check(buffer.put({0, 0}, wide, 0, colored).has_value(), "wide write succeeds");

  const auto lead = buffer.at({0, 0});

  check(lead->glyph == utf8(u8"界") && lead->style == colored && lead->cell.columns == 2,
        "wide semantic bytes and style are stored");
  check(buffer.put({1, 0}, narrow, 0).has_value(), "write over second half succeeds");
  check(buffer.at({0, 0})->glyph == " " && buffer.at({0, 0})->cell.kind == CellKind::BLANK &&
          buffer.at({1, 0})->glyph == "x",
        "overwriting continuation clears the entire older glyph");
  check(buffer.put({0, 0}, wide, 0, colored) && buffer.put({2, 0}, wide, 0, colored) && buffer.put({1, 0}, wide, 0),
        "wide write can intersect two older wide clusters");
  check(buffer.at({0, 0})->glyph == " " && buffer.at({3, 0})->glyph == " " && buffer.at({1, 0})->cell.columns == 2 &&
          buffer.at({2, 0})->cell.kind == CellKind::CONTINUATION,
        "both complete intersected clusters are cleared");
  check(buffer.put({1, 0}, narrow, 0).has_value() && buffer.at({2, 0})->glyph == " ",
        "overwriting wide lead clears its continuation");

  {
    const auto temporary = makeText(utf8(u8"e\u0301👩‍💻"));

    check(buffer.put({0, 1}, temporary, 0, colored) && buffer.put({1, 1}, temporary, 1, colored),
          "write by grapheme index from multicluster source");
  }

  wide = makeText("replaced");
  check(buffer.at({0, 1})->glyph == utf8(u8"e\u0301") && buffer.at({1, 1})->glyph == utf8(u8"👩‍💻") &&
          buffer.at({2, 1})->style == colored,
        "buffer owns bytes and styles after text destruction/replacement");

  const auto controls = makeText("\x1b\t\r\n");

  check(buffer.put({5, 1}, controls, 2).has_value() && buffer.at({5, 1})->glyph == "?" &&
          buffer.at({5, 1})->cell.columns == 1,
        "unsupported whole logical cluster becomes a safe one-cell placeholder");
  invariant(buffer);

  const auto allocation = buffer.memory();

  buffer.clear();
  invariant(buffer);
  check(buffer.memory().glyph_count == 1 && buffer.memory().glyph_bytes == 1 && buffer.memory().style_count == 1 &&
          buffer.memory().total_bytes == allocation.total_bytes,
        "clear resets pools while retaining exactly accounted capacity");
  check(buffer.resize({2, 1}).has_value(), "resize smaller succeeds");
  invariant(buffer);
  check(buffer.memory().glyph_count == 1 && buffer.at({1, 0})->glyph == " ", "resize discards all old IDs and content");
  check(buffer.resize({-1, 3}).has_value() && buffer.size() == CellSize{0, 3} && buffer.cells().empty(),
        "negative extents normalize to zero");
  buffer.clear();
  invariant(buffer);
  check(buffer.resize({3, 2}).has_value(), "resize after empty succeeds");
  invariant(buffer);
}

void failuresAreAtomic() {
  const auto narrow = makeText("a");
  const auto wide = makeText(utf8(u8"界"));
  auto buffer = makeBuffer({3, 2});

  check(buffer.put({1, 0}, wide, 0).has_value(), "seed wide glyph at edge");

  const auto before = snapshot(buffer);
  const auto memory = buffer.memory();
  const auto borrowed = buffer.at({1, 0})->glyph;

  auto reject = [&](auto result, CellBufferError error) {
    check(!result && result.error() == error, "failure category is explicit");
    check(snapshot(buffer) == before && buffer.memory() == memory && borrowed == utf8(u8"界"),
          "failed operation preserves cells, pool counts, allocation and borrowed bytes");
    invariant(buffer);
  };

  reject(buffer.put({2, 0}, wide, 0), CellBufferError::WIDE_AT_EDGE);
  reject(buffer.put({-1, 0}, narrow, 0), CellBufferError::OUT_OF_BOUNDS);
  reject(buffer.put({3, 0}, narrow, 0), CellBufferError::OUT_OF_BOUNDS);
  reject(buffer.put({0, 2}, narrow, 0), CellBufferError::OUT_OF_BOUNDS);
  reject(buffer.put({0, -1}, narrow, 0), CellBufferError::OUT_OF_BOUNDS);
  reject(buffer.put({0, 0}, narrow, 1), CellBufferError::INVALID_GRAPHEME);
  reject(buffer.put({0, 0}, makeText(""), 0), CellBufferError::INVALID_GRAPHEME);

  WidthPolicy changed;

  changed.revision = 1;
  reject(buffer.put({0, 0}, makeText("a", changed), 0), CellBufferError::POLICY_MISMATCH);
  changed = {};
  changed.emoji = EmojiWidth::ONE;
  reject(buffer.put({0, 0}, makeText("a", changed), 0), CellBufferError::POLICY_MISMATCH);
  reject(buffer.resize({33, 2}), CellBufferError::COLUMN_LIMIT);
  reject(buffer.resize({3, 17}), CellBufferError::ROW_LIMIT);
  reject(buffer.resize({32, 16}), CellBufferError::CELL_LIMIT);
  check(!buffer.at({-1, 0}) && !buffer.at({0, 2}), "invalid coordinate lookup returns error");

  for (int allocation_index = 0; allocation_index < 4; ++allocation_index) {
    const auto old_live_arrays = live_arrays;

    array_failure_countdown = allocation_index;
    reject(buffer.resize({4, 3}), CellBufferError::ALLOCATION_FAILURE);
    check(live_arrays == old_live_arrays && array_failure_countdown == -1,
          "partial resize allocations are released on each possible allocation failure");
  }
}

void poolLimits() {
  const auto a = makeText("a");
  const auto b = makeText("b");
  auto limits = smallLimits();

  limits.max_glyphs = 2;

  auto glyphs = makeBuffer({2, 1}, limits);

  check(glyphs.put({0, 0}, a, 0).has_value(), "new glyph fits last record");

  const auto glyph_snapshot = snapshot(glyphs);
  const auto glyph_memory = glyphs.memory();
  const auto glyph_error = glyphs.put({0, 0}, b, 0);

  check(!glyph_error && glyph_error.error() == CellBufferError::GLYPH_LIMIT && snapshot(glyphs) == glyph_snapshot &&
          glyphs.memory() == glyph_memory,
        "full glyph pool refuses new glyph without changing state");

  for (int i = 0; i < 100; ++i) {
    check(glyphs.put({i % 2, 0}, a, 0).has_value(), "intern hit still succeeds with full glyph pool");
  }

  check(glyphs.memory().glyph_count == 2 && glyphs.memory().glyph_bytes == 2,
        "repeated equal glyph writes do not grow pools");
  glyphs.clear();
  check(glyphs.put({0, 0}, b, 0).has_value(), "clear reclaims all old glyph slots");
  invariant(glyphs);

  limits = smallLimits();
  limits.max_glyph_bytes = 3;

  auto bytes = makeBuffer({2, 1}, limits);

  check(bytes.put({0, 0}, a, 0).has_value(), "first byte fits");

  const auto byte_snapshot = snapshot(bytes);
  const auto byte_memory = bytes.memory();
  const auto too_long = bytes.put({0, 0}, makeText(utf8(u8"e\u0301")), 0);

  check(!too_long && too_long.error() == CellBufferError::GLYPH_BYTE_LIMIT && snapshot(bytes) == byte_snapshot &&
          bytes.memory() == byte_memory,
        "whole multibyte glyph is rejected atomically by byte cap");
  check(bytes.put({1, 0}, b, 0).has_value() && bytes.memory().glyph_bytes == 3, "exact byte cap succeeds");
  invariant(bytes);

  limits = smallLimits();
  limits.max_styles = 2;

  auto styles = makeBuffer({2, 1}, limits);
  CellStyle first;

  first.italic = true;

  CellStyle second;

  second.underline = true;
  check(styles.put({0, 0}, a, 0, first).has_value(), "style fits last slot");

  const auto style_snapshot = snapshot(styles);
  const auto style_memory = styles.memory();
  const auto too_many_styles = styles.put({1, 0}, b, 0, second);

  check(!too_many_styles && too_many_styles.error() == CellBufferError::STYLE_LIMIT &&
          snapshot(styles) == style_snapshot && styles.memory() == style_memory,
        "style rejection also prevents installation of a new glyph");
  check(styles.put({1, 0}, b, 0, first).has_value() && styles.memory().style_count == 2,
        "equal style hits succeed at capacity");
  invariant(styles);
}

void configurationAndMoves() {
  auto limits = smallLimits();

  for (int option = 0; option < 10; ++option) {
    auto invalid = limits;

    switch (option) {
    case 0:
      invalid.max_glyphs = 0;

      break;
    case 1:
      invalid.max_glyph_bytes = 0;

      break;
    case 2:
      invalid.max_styles = 0;

      break;
    case 3:
      invalid.max_rows = std::numeric_limits<std::size_t>::max();

      break;
    case 4:
      invalid.max_cells = std::numeric_limits<std::size_t>::max();

      break;
    case 5:
      invalid.max_glyphs = std::numeric_limits<std::size_t>::max();

      break;
    case 6:
      invalid.max_glyph_bytes = std::numeric_limits<std::size_t>::max();

      break;
    case 7:
      invalid.max_columns = std::numeric_limits<std::size_t>::max();

      break;
    case 8:
      invalid.max_styles = std::numeric_limits<std::size_t>::max();

      break;
    case 9:
      invalid.max_cells = static_cast<std::size_t>(std::numeric_limits<std::ptrdiff_t>::max()) / sizeof(Cell);
      invalid.max_glyph_bytes = static_cast<std::size_t>(std::numeric_limits<std::ptrdiff_t>::max());

      break;
    }

    const auto result = CellBuffer::from({0, 0}, {}, invalid);

    check(!result && result.error() == CellBufferError::INVALID_LIMITS, "invalid caps refused before allocation");
  }

  WidthPolicy invalid_policy;

  invalid_policy.emoji = static_cast<EmojiWidth>(3);

  const auto invalid = CellBuffer::from({0, 0}, invalid_policy, limits);

  check(!invalid && invalid.error() == CellBufferError::INVALID_POLICY, "invalid width policy refused");
  invalid_policy = {};
  invalid_policy.ambiguous = static_cast<AmbiguousWidth>(7);

  const auto invalid_ambiguous = CellBuffer::from({0, 0}, invalid_policy, limits);

  check(!invalid_ambiguous && invalid_ambiguous.error() == CellBufferError::INVALID_POLICY,
        "invalid ambiguous policy refused");
  limits.max_columns = 0;
  limits.max_rows = 0;
  limits.max_cells = 0;

  auto empty = makeBuffer({-7, -9}, limits);

  invariant(empty);
  check(empty.size() == CellSize{} && empty.memory().glyph_count == 1, "zero cell caps permit empty valid buffer");

  WidthPolicy custom_policy;

  custom_policy.ambiguous = AmbiguousWidth::WIDE;
  custom_policy.emoji = EmojiWidth::ONE;
  custom_policy.revision = 77;

  auto custom = makeBuffer({3, 1}, smallLimits(), custom_policy);
  const auto ambiguous = makeText(utf8(u8"·"), custom_policy);
  const auto emoji = makeText(utf8(u8"😀"), custom_policy);

  check(custom.policy() == custom_policy && custom.put({0, 0}, ambiguous, 0) && custom.put({2, 0}, emoji, 0) &&
          custom.at({0, 0})->cell.columns == 2 && custom.at({2, 0})->cell.columns == 1,
        "nondefault complete width policy is retained and applied to cells");
  invariant(custom);

  auto original = makeBuffer({3, 1});

  check(original.put({1, 0}, makeText(utf8(u8"界")), 0).has_value(), "move fixture wide glyph");

  const auto original_snapshot = snapshot(original);
  const auto original_memory = original.memory();
  auto moved = std::move(original);

  check(snapshot(moved) == original_snapshot && moved.memory() == original_memory, "move transfers coherent ownership");
  check(original.size() == CellSize{} && original.cells().empty() && original.memory().glyph_count == 0 &&
          original.memory().total_bytes == sizeof(CellBuffer),
        "moved-from buffer has no dangling storage or live IDs");
  original.clear();
  invariant(original);
  check(original.resize({1, 1}).has_value(), "moved-from buffer is reusable by resize");
  invariant(original);

  auto &self = moved;

  moved = std::move(self);
  check(snapshot(moved) == original_snapshot, "self-move preserves buffer");
  original = std::move(moved);
  check(snapshot(original) == original_snapshot && moved.cells().empty(),
        "move assignment replaces old coherent state");
  invariant(original);
}

struct OracleCell {
  std::string glyph{" "};
  CellStyle style{};
  CellKind kind{CellKind::BLANK};
  std::uint8_t columns{1};
};

void boundedProperty() {
  const std::vector<GraphemeText> texts = {makeText("a"),          makeText("b"),          makeText(utf8(u8"e\u0301")),
                                           makeText(utf8(u8"界")), makeText(utf8(u8"😀")), makeText("\t"),
                                           makeText(" ")};
  auto buffer = makeBuffer({8, 3});
  std::vector<OracleCell> oracle(24);
  std::uint32_t random = 0x12345678;
  auto next = [&]() {
    random = random * 1664525u + 1013904223u;

    return random;
  };

  for (int iteration = 0; iteration < 12000; ++iteration) {
    if (iteration % 137 == 0) {
      const CellSize size{static_cast<std::int32_t>(next() % 9), static_cast<std::int32_t>(next() % 4)};

      check(buffer.resize(size).has_value(), "bounded property resize succeeds");
      oracle.assign(static_cast<std::size_t>(size.width) * static_cast<std::size_t>(size.height), {});
    } else if (iteration % 53 == 0) {
      buffer.clear();
      oracle.assign(oracle.size(), {});
    } else {
      const auto &text = texts[next() % texts.size()];
      const auto width = text.graphemes()[0].cells;
      const CellPoint position{static_cast<std::int32_t>(next() % 11) - 1, static_cast<std::int32_t>(next() % 6) - 1};
      CellStyle style;

      style.bold = next() % 2 == 0;
      style.underline = next() % 3 == 0;

      const bool in_bounds =
        position.x >= 0 && position.y >= 0 && position.x < buffer.size().width && position.y < buffer.size().height;
      const bool fits = in_bounds && position.x + width <= buffer.size().width;
      const auto before = snapshot(buffer);
      const auto memory = buffer.memory();
      const auto result = buffer.put(position, text, 0, style);

      check(result.has_value() == fits, "property write acceptance matches independent bounds");

      if (!fits) {
        check(snapshot(buffer) == before && buffer.memory() == memory, "property failed write is completely atomic");
      } else {
        const auto row = static_cast<std::size_t>(position.y) * static_cast<std::size_t>(buffer.size().width);
        const auto start = row + static_cast<std::size_t>(position.x);
        const auto end = start + width;
        auto old = oracle;

        // Remove old clusters by their full footprint, rather than following production IDs.
        for (std::size_t i = row; i < row + static_cast<std::size_t>(buffer.size().width); ++i) {
          if (old[i].kind == CellKind::CONTINUATION) {
            continue;
          }

          const auto old_end = i + old[i].columns;

          if (i < end && start < old_end) {
            for (auto erased = i; erased < old_end; ++erased) {
              oracle[erased] = {};
            }
          }
        }

        const auto bytes =
          text.graphemes()[0].display == TextDisplay::UNSUPPORTED ? std::string("?") : std::string(text.bytes());

        oracle[start] = {bytes, style, CellKind::LEAD, width};

        if (width == 2) {
          oracle[start + 1] = {bytes, style, CellKind::CONTINUATION, 0};
        }
      }
    }

    invariant(buffer);

    const auto actual = snapshot(buffer);

    check(actual.size() == oracle.size(), "oracle and buffer extents agree");

    for (std::size_t i = 0; i < actual.size() && i < oracle.size(); ++i) {
      check(actual[i].glyph == oracle[i].glyph && actual[i].style == oracle[i].style &&
              actual[i].cell.kind == oracle[i].kind && actual[i].cell.columns == oracle[i].columns,
            "all cells match independent semantic whole-footprint oracle");
    }
  }

  std::cout << "Checked 12000 bounded resize/clear/overwrite transitions\n";
}
} // namespace

int main() {
  overwriteAndLifetime();
  failuresAreAtomic();
  poolLimits();
  configurationAndMoves();
  boundedProperty();
  check(live_arrays == 0, "all owned arrays are released at test completion");

  if (failures != 0) {
    std::cerr << failures << " failed cell-buffer checks\n";

    return 1;
  }

  const auto buffer = makeBuffer({3, 2});
  const auto memory = buffer.memory();

  std::cout << "Measured payload: object=" << memory.object_bytes << " cells=" << memory.cell_storage_bytes
            << " glyph_records=" << memory.glyph_record_storage_bytes
            << " glyph_capacity=" << memory.glyph_byte_storage_bytes << " styles=" << memory.style_storage_bytes
            << " total=" << memory.total_bytes << " bytes (not RSS)\n";

  return 0;
}

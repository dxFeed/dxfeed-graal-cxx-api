// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#include <portals/render.hpp>

#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <string>
#include <utility>
#include <vector>

namespace {
bool fail_next_scalar_allocation = false;
} // namespace

// Keep array allocation separate so failure targets renderer temporary storage.
void *operator new(std::size_t bytes) {
  if (fail_next_scalar_allocation) {
    fail_next_scalar_allocation = false;
    throw std::bad_alloc();
  }

  if (auto *memory = std::malloc(bytes == 0 ? 1 : bytes)) {
    return memory;
  }

  throw std::bad_alloc();
}

void *operator new[](std::size_t bytes) {
  if (auto *memory = std::malloc(bytes == 0 ? 1 : bytes)) {
    return memory;
  }

  throw std::bad_alloc();
}

void operator delete(void *memory) noexcept {
  std::free(memory);
}

void operator delete(void *memory, std::size_t) noexcept {
  ::operator delete(memory);
}

void operator delete[](void *memory) noexcept {
  std::free(memory);
}

void operator delete[](void *memory, std::size_t) noexcept {
  ::operator delete[](memory);
}

// Route nothrow temporary buffers through the same replacement allocation pair.
void *operator new(std::size_t bytes, const std::nothrow_t &) noexcept {
  try {
    return ::operator new(bytes);
  } catch (...) {
    return nullptr;
  }
}

void *operator new[](std::size_t bytes, const std::nothrow_t &) noexcept {
  try {
    return ::operator new[](bytes);
  } catch (...) {
    return nullptr;
  }
}

void operator delete(void *memory, const std::nothrow_t &) noexcept {
  ::operator delete(memory);
}

void operator delete[](void *memory, const std::nothrow_t &) noexcept {
  ::operator delete[](memory);
}

namespace {
using namespace portals;
int failures = 0;

void check(bool condition, const char *message) {
  if (!condition) {
    std::cerr << message << '\n';
    ++failures;
  }
}

std::string utf8(std::u8string_view bytes) {
  return {reinterpret_cast<const char *>(bytes.data()), bytes.size()};
}

GraphemeText text(std::string_view bytes, WidthPolicy policy = {}) {
  auto result = GraphemeText::from(bytes, policy);

  if (!result) {
    std::exit(2);
  }

  return std::move(*result);
}

CellRect rect(std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height) {
  auto result = CellRect::from({x, y}, {width, height});

  if (!result) {
    std::exit(2);
  }

  return *result;
}

RenderLimits limits() {
  RenderLimits result;

  result.buffer = {32, 16, 256, 64, 1024, 16};

  return result;
}

CellBuffer render(CellSize size, std::span<const RenderCommand> commands, WidthPolicy policy = {}) {
  auto result = renderFullFrame(size, commands, policy, limits());

  if (!result) {
    std::cerr << "fixture render failed\n";
    std::exit(2);
  }

  return std::move(*result);
}

struct SnapshotCell {
  std::string glyph;
  CellStyle style;
  CellKind kind;
  std::uint8_t columns;
  bool operator==(const SnapshotCell &) const = default;
};

std::vector<SnapshotCell> snapshot(const CellBuffer &buffer) {
  std::vector<SnapshotCell> result;

  for (std::int32_t y = 0; y < buffer.size().height; ++y) {
    for (std::int32_t x = 0; x < buffer.size().width; ++x) {
      const auto view = buffer.at({x, y});

      result.push_back({std::string(view->glyph), view->style, view->cell.kind, view->cell.columns});
    }
  }

  return result;
}

void cell(const CellBuffer &buffer, CellPoint position, std::string_view glyph, CellKind kind = CellKind::LEAD,
          std::uint8_t columns = 1, CellStyle style = {}) {
  const auto view = buffer.at(position);

  check(view && view->glyph == glyph && view->style == style && view->cell.kind == kind &&
          view->cell.columns == columns,
        "semantic cell snapshot mismatch");
}

void emptyAndOrder() {
  const auto empty = render({3, 1}, {});

  cell(empty, {0, 0}, " ", CellKind::BLANK);
  cell(empty, {1, 0}, " ", CellKind::BLANK);
  cell(empty, {2, 0}, " ", CellKind::BLANK);
  check(render({-1, 3}, {}).cells().empty(), "negative extent normalizes empty");

  const auto base = text("abcdef");
  const auto upper = text("XY");
  const auto low = text("L");
  const auto tie = text("T");
  CellStyle bold;

  bold.bold = true;

  const auto clip = rect(0, 0, 6, 1);
  const std::array<RenderCommand, 4> commands{
    TextCommand{{1, 0}, upper, clip, bold, 20}, TextCommand{{0, 0}, base, clip, {}, 10},
    TextCommand{{0, 0}, low, clip, {}, -1}, TextCommand{{2, 0}, tie, clip, {}, 20}};
  const auto result = render({6, 1}, commands);

  cell(result, {0, 0}, "a");
  cell(result, {1, 0}, "X", CellKind::LEAD, 1, bold);
  cell(result, {2, 0}, "T");
  cell(result, {3, 0}, "d");
  cell(result, {4, 0}, "e");
  cell(result, {5, 0}, "f");
  check(snapshot(result) == snapshot(render({6, 1}, commands)), "replay semantic determinism");
}

void wholeGraphemeClipping() {
  const auto value = text(utf8(u8"A界e\u0301Z"));
  const std::array<RenderCommand, 1> clipped{TextCommand{{-1, 0}, value, rect(1, 0, 2, 1)}};
  const auto result = render({4, 1}, clipped);

  cell(result, {0, 0}, " ", CellKind::BLANK);
  cell(result, {1, 0}, " ", CellKind::BLANK);
  cell(result, {2, 0}, utf8(u8"e\u0301"));
  cell(result, {3, 0}, " ", CellKind::BLANK);

  const auto wide = text(utf8(u8"界"));
  const std::array<RenderCommand, 1> right_edge{TextCommand{{2, 0}, wide, rect(0, 0, 3, 1)}};

  cell(render({3, 1}, right_edge), {2, 0}, " ", CellKind::BLANK);

  const std::array<RenderCommand, 1> fits{TextCommand{{1, 0}, wide, rect(0, 0, 3, 1)}};
  const auto fitted = render({3, 1}, fits);

  cell(fitted, {1, 0}, utf8(u8"界"), CellKind::LEAD, 2);
  cell(fitted, {2, 0}, utf8(u8"界"), CellKind::CONTINUATION, 0);

  const auto narrow = text("x");
  const std::array<RenderCommand, 2> collision{TextCommand{{0, 0}, wide, rect(0, 0, 3, 1)},
                                               TextCommand{{1, 0}, narrow, rect(1, 0, 1, 1)}};
  const auto cleaned = render({3, 1}, collision);

  cell(cleaned, {0, 0}, " ", CellKind::BLANK);
  cell(cleaned, {1, 0}, "x");
  cell(cleaned, {2, 0}, " ", CellKind::BLANK);
}

void fillControlsAndOwnership() {
  CellStyle style;

  style.background = RgbColor{12, 34, 56};

  const std::array<RenderCommand, 1> fill{FillCommand{rect(-1, -1, 4, 3), rect(1, 0, 2, 2), style}};
  const auto result = render({4, 2}, fill);

  for (std::int32_t y = 0; y < 2; ++y) {
    cell(result, {0, y}, " ", CellKind::BLANK);
    cell(result, {1, y}, " ", CellKind::LEAD, 1, style);
    cell(result, {2, y}, " ", CellKind::LEAD, 1, style);
    cell(result, {3, y}, " ", CellKind::BLANK);
  }

  const auto controls = text("\x1b\n\t");
  const std::array<RenderCommand, 1> command{TextCommand{{0, 0}, controls, rect(0, 0, 3, 1)}};
  const auto safe = render({3, 1}, command);

  cell(safe, {0, 0}, "?");
  cell(safe, {1, 0}, "?");
  cell(safe, {2, 0}, "?");

  const auto owned = [] {
    auto temporary = text(utf8(u8"e\u0301"));
    const std::array<RenderCommand, 1> borrowed{TextCommand{{0, 0}, temporary, rect(0, 0, 1, 1)}};

    return render({1, 1}, borrowed);
  }();

  cell(owned, {0, 0}, utf8(u8"e\u0301"));
}

void errorKind(const std::expected<CellBuffer, RenderError> &result, RenderErrorKind kind, const char *message) {
  check(!result && result.error().kind == kind, message);
}

void matchingWidthPolicies() {
  WidthPolicy policy;

  policy.ambiguous = AmbiguousWidth::WIDE;
  policy.emoji = EmojiWidth::ONE;
  policy.revision = 7;

  const auto value = text(utf8(u8"·🙂A"), policy);
  const std::array<RenderCommand, 1> command{TextCommand{{0, 0}, value, rect(0, 0, 4, 1)}};
  const auto result = render({4, 1}, command, policy);

  check(result.policy() == policy, "frame captures complete matching policy");
  cell(result, {0, 0}, utf8(u8"·"), CellKind::LEAD, 2);
  cell(result, {1, 0}, utf8(u8"·"), CellKind::CONTINUATION, 0);
  cell(result, {2, 0}, utf8(u8"🙂"));
  cell(result, {3, 0}, "A");

  const auto wide = text(utf8(u8"界界"));
  const auto clip = rect(0, 0, 4, 1);
  CellStyle style;

  style.underline = true;

  const std::array<RenderCommand, 2> fill_collision{TextCommand{{0, 0}, wide, clip},
                                                    FillCommand{rect(1, 0, 2, 1), clip, style}};
  const auto filled = render({4, 1}, fill_collision);

  cell(filled, {0, 0}, " ", CellKind::BLANK);
  cell(filled, {1, 0}, " ", CellKind::LEAD, 1, style);
  cell(filled, {2, 0}, " ", CellKind::LEAD, 1, style);
  cell(filled, {3, 0}, " ", CellKind::BLANK);
}

void boundsAndPolicy() {
  const auto value = text("ab");
  const std::array<RenderCommand, 1> commands{TextCommand{{0, 0}, value, rect(0, 0, 2, 1)}};
  auto options = limits();

  options.max_commands = 0;
  errorKind(renderFullFrame({2, 1}, commands, {}, options), RenderErrorKind::COMMAND_LIMIT, "command cap is explicit");
  options = limits();
  options.max_text_bytes = 1;
  errorKind(renderFullFrame({2, 1}, commands, {}, options), RenderErrorKind::TEXT_BYTE_LIMIT,
            "aggregate text byte cap is explicit");
  options = limits();
  options.max_work = 1;
  errorKind(renderFullFrame({2, 1}, commands, {}, options), RenderErrorKind::WORK_LIMIT, "text work cap is explicit");

  const std::array<RenderCommand, 1> fill{FillCommand{rect(-10, -10, 20, 20), rect(0, 0, 2, 1)}};

  errorKind(renderFullFrame({2, 1}, fill, {}, options), RenderErrorKind::WORK_LIMIT, "fill work counts clipped cells");
  options.max_work = 2;
  check(renderFullFrame({2, 1}, fill, {}, options).has_value(), "exact work cap is allowed");

  WidthPolicy changed;

  changed.revision = 1;
  errorKind(renderFullFrame({2, 1}, commands, changed, limits()), RenderErrorKind::POLICY_MISMATCH,
            "complete text policy must match even for offscreen commands");
  options = limits();
  options.max_text_bytes = 3;

  const std::array<RenderCommand, 2> repeated{commands[0], commands[0]};

  errorKind(renderFullFrame({2, 1}, repeated, {}, options), RenderErrorKind::TEXT_BYTE_LIMIT,
            "repeated runs consume aggregate original byte budget");
  options = limits();
  options.max_work = 0;

  const std::array<RenderCommand, 1> empty_fill{FillCommand{rect(0, 0, 0, 16), rect(0, 0, 0, 16)}};

  check(renderFullFrame({0, 16}, empty_fill, {}, options).has_value(), "empty fill costs no work");

  const std::array<RenderCommand, 1> offscreen{TextCommand{{-20, 0}, value, rect(0, 0, 2, 1)}};

  errorKind(renderFullFrame({2, 1}, offscreen, {}, options), RenderErrorKind::WORK_LIMIT,
            "offscreen text retains bounded input work");
  options = limits();
  options.buffer.max_columns = 1;

  const auto buffer_error = renderFullFrame({2, 1}, {}, {}, options);

  check(!buffer_error && buffer_error.error().kind == RenderErrorKind::BUFFER_ERROR &&
          buffer_error.error().buffer_error == CellBufferError::COLUMN_LIMIT,
        "buffer error detail is preserved");
  options = limits();
  options.buffer.max_glyphs = 1;

  const auto pool_error = renderFullFrame({2, 1}, commands, {}, options);

  check(!pool_error && pool_error.error().kind == RenderErrorKind::BUFFER_ERROR &&
          pool_error.error().buffer_error == CellBufferError::GLYPH_LIMIT,
        "intern pool exhaustion returns no partial frame");

  constexpr auto MINIMUM = std::numeric_limits<std::int32_t>::min();
  constexpr auto MAXIMUM = std::numeric_limits<std::int32_t>::max();
  const std::array<RenderCommand, 3> extremes{TextCommand{{MINIMUM, 0}, value, rect(0, 0, 2, 1)},
                                              TextCommand{{MAXIMUM, 0}, value, rect(0, 0, 2, 1)},
                                              TextCommand{{0, MINIMUM}, value, rect(0, 0, 2, 1)}};
  const auto untouched = render({2, 1}, extremes);

  cell(untouched, {0, 0}, " ", CellKind::BLANK);
  cell(untouched, {1, 0}, " ", CellKind::BLANK);
}

void temporaryAllocationFailure() {
  const auto value = text("x");
  const std::array<RenderCommand, 1> commands{TextCommand{{0, 0}, value, rect(0, 0, 1, 1)}};

  fail_next_scalar_allocation = true;

  const auto result = renderFullFrame({1, 1}, commands, {}, limits());

  errorKind(result, RenderErrorKind::ALLOCATION_FAILURE, "temporary allocation failure returns no frame");
  check(!fail_next_scalar_allocation, "failure injection reached renderer temporary allocation");
  cell(render({1, 1}, commands), {0, 0}, "x");
}

void asciiOracleProperty() {
  const auto value = text("abcd");
  CellStyle bold;

  bold.bold = true;

  const auto clip = rect(0, 0, 5, 2);

  for (std::int32_t seed = 0; seed < 60; ++seed) {
    std::vector<RenderCommand> commands;

    for (std::int32_t i = 0; i < 6; ++i) {
      commands.emplace_back(TextCommand{
        {(seed * 3 + i * 2) % 9 - 3, i % 2}, value, clip, i % 2 == 0 ? bold : CellStyle{}, (seed + i) % 5 - 2});
    }

    std::vector<SnapshotCell> expected(10, {" ", {}, CellKind::BLANK, 1});

    for (std::int32_t z = -2; z <= 2; ++z) {
      for (std::int32_t i = 0; i < 6; ++i) {
        if ((seed + i) % 5 - 2 != z) {
          continue;
        }

        for (std::int32_t offset = 0; offset < 4; ++offset) {
          const auto x = (seed * 3 + i * 2) % 9 - 3 + offset;

          if (x >= 0 && x < 5) {
            expected[static_cast<std::size_t>((i % 2) * 5 + x)] = {std::string(1, "abcd"[offset]),
                                                                   i % 2 == 0 ? bold : CellStyle{}, CellKind::LEAD, 1};
          }
        }
      }
    }

    check(snapshot(render({5, 2}, commands)) == expected, "independent ASCII oracle property");
  }
}
} // namespace

int main() {
  emptyAndOrder();
  wholeGraphemeClipping();
  fillControlsAndOwnership();
  matchingWidthPolicies();
  boundsAndPolicy();
  temporaryAllocationFailure();
  asciiOracleProperty();

  return failures == 0 ? 0 : 1;
}

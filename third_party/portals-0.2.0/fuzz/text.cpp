// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include <portals/text.hpp>

#include <algorithm>
#include <array>
#include <cstdlib>

namespace {
using namespace portals;
constexpr std::size_t MAX_INPUT_BYTES = 4096;

void require(bool condition) {
  if (!condition) {
    std::abort();
  }
}

void verify(const GraphemeText &text) {
  const auto clusters = text.graphemes();
  std::size_t byteEnd = 0;
  std::int32_t cells = 0;
  std::array<bool, MAX_INPUT_BYTES + 1> boundaries{};

  for (const auto &cluster : clusters) {
    require(cluster.byte_begin == byteEnd && cluster.byte_end > byteEnd);
    require(cluster.byte_end <= text.bytes().size());
    require(cluster.cells == 1 || cluster.cells == 2);
    require(text.nextCursor(cluster.byte_begin) == cluster.byte_end);
    require(text.previousCursor(cluster.byte_end) == cluster.byte_begin);
    boundaries[cluster.byte_begin] = true;
    byteEnd = cluster.byte_end;
    cells += cluster.cells;
  }

  require(byteEnd == text.bytes().size() && cells == text.measure());
  require(text.previousCursor(0) == 0);
  require(text.nextCursor(byteEnd) == byteEnd);
  boundaries[byteEnd] = true;

  // Probe every byte, including interior UTF-8 and combining-cluster offsets.
  for (std::size_t offset = 0; offset <= byteEnd; ++offset) {
    require(text.nextCursor(offset).has_value() == boundaries[offset]);
    require(text.previousCursor(offset).has_value() == boundaries[offset]);
  }

  const std::array budgets{-1, 0, 1, 2, cells / 2, cells, cells + 1};

  for (const auto budget : budgets) {
    TextClip expected;

    // A simple linear oracle does not reuse the implementation's search path.
    for (const auto &cluster : clusters) {
      if (expected.cells + cluster.cells > std::max(budget, 0)) {
        break;
      }

      expected.byte_end = cluster.byte_end;
      ++expected.graphemes;
      expected.cells += cluster.cells;
    }

    const auto actual = text.clipPrefix(budget);

    require(actual.byte_end == expected.byte_end && actual.graphemes == expected.graphemes &&
            actual.cells == expected.cells);
  }
}
} // namespace

// The libFuzzer entry point has an externally required C symbol name.
// NOLINTNEXTLINE(readability-identifier-naming)
extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size) {
  if (size > MAX_INPUT_BYTES) {
    return 0;
  }

  const std::string_view bytes(reinterpret_cast<const char *>(data), size);
  WidthPolicy policy;

  if (size) {
    policy.ambiguous = data[0] & 1 ? AmbiguousWidth::WIDE : AmbiguousWidth::NARROW;
    policy.emoji = data[0] & 2 ? EmojiWidth::TWO : EmojiWidth::ONE;
  }

  const auto text = GraphemeText::from(bytes, policy);

  if (!text) {
    require(text.error().byte_offset <= size);

    return 0;
  }

  require(text->bytes() == bytes && text->policy() == policy);
  verify(*text);

  TextLimits limits;

  limits.max_input_bytes = size;
  limits.max_cluster_bytes = size ? data[0] : 0;
  limits.max_graphemes = size ? data[size - 1] : 0;
  limits.max_cells = size ? data[size / 2] : 0;

  const auto bounded = GraphemeText::from(bytes, policy, limits);

  if (bounded) {
    require(bounded->bytes() == bytes);
    require(bounded->graphemes().size() <= limits.max_graphemes);
    require(static_cast<std::size_t>(bounded->measure()) <= limits.max_cells);
    verify(*bounded);
  } else {
    require(bounded.error().byte_offset <= size);
  }

  return 0;
}

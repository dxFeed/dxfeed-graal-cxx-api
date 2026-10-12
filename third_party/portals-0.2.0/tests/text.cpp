// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#include <portals/text.hpp>

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace allocation_test {
thread_local std::ptrdiff_t remaining{-1};
} // namespace allocation_test

void *operator new(std::size_t size) {
  if (allocation_test::remaining == 0) {
    throw std::bad_alloc();
  }

  if (allocation_test::remaining > 0) {
    --allocation_test::remaining;
  }

  if (void *value = std::malloc(size ? size : 1)) {
    return value;
  }

  throw std::bad_alloc();
}

void operator delete(void *value) noexcept {
  std::free(value);
}

void operator delete(void *value, std::size_t) noexcept {
  std::free(value);
}

void *operator new[](std::size_t size) {
  return ::operator new(size);
}

void operator delete[](void *value) noexcept {
  ::operator delete(value);
}

void operator delete[](void *value, std::size_t) noexcept {
  ::operator delete(value);
}

void *operator new(std::size_t size, const std::nothrow_t &) noexcept {
  try {
    return ::operator new(size);
  } catch (...) {
    return nullptr;
  }
}

void operator delete(void *value, const std::nothrow_t &) noexcept {
  ::operator delete(value);
}

void *operator new[](std::size_t size, const std::nothrow_t &tag) noexcept {
  return ::operator new(size, tag);
}

void operator delete[](void *value, const std::nothrow_t &) noexcept {
  ::operator delete(value);
}

namespace {
int failures = 0;

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

void appendUtf8(std::string &text, std::uint32_t cp) {
  if (cp <= 0x7f) {
    text += static_cast<char>(cp);
  } else if (cp <= 0x7ff) {
    text += static_cast<char>(0xc0 | (cp >> 6));
    text += static_cast<char>(0x80 | (cp & 0x3f));
  } else if (cp <= 0xffff) {
    text += static_cast<char>(0xe0 | (cp >> 12));
    text += static_cast<char>(0x80 | ((cp >> 6) & 0x3f));
    text += static_cast<char>(0x80 | (cp & 0x3f));
  } else {
    text += static_cast<char>(0xf0 | (cp >> 18));
    text += static_cast<char>(0x80 | ((cp >> 12) & 0x3f));
    text += static_cast<char>(0x80 | ((cp >> 6) & 0x3f));
    text += static_cast<char>(0x80 | (cp & 0x3f));
  }
}

void fixtures() {
  using namespace portals;
  const auto empty = GraphemeText::from("");

  check(empty && empty->measure() == 0 && empty->graphemes().empty(), "empty text");
  check(empty && empty->nextCursor(0) == 0 && empty->previousCursor(0) == 0, "empty cursor endpoints");

  struct Fixture {
    std::u8string_view bytes;
    std::size_t count;
    std::int32_t cells;
  };

  const Fixture examples[] = {{u8"Hello", 5, 5},       {u8"Привет", 6, 6}, {u8"e\u0301", 1, 1}, {u8"界", 1, 2},
                              {u8"👩‍💻", 1, 2}, {u8"👍🏽", 1, 2},   {u8"1️⃣", 1, 2},       {u8"🇺🇦", 1, 2},
                              {u8"🇦🇧🇨", 2, 4},         {u8"❤", 1, 1},      {u8"❤️", 1, 2},       {u8"❤︎", 1, 1},
                              {u8"😀", 1, 2},          {u8"·α", 2, 2}};

  for (const auto &example : examples) {
    const auto text = GraphemeText::from(utf8(example.bytes));

    check(text && text->graphemes().size() == example.count && text->measure() == example.cells,
          "semantic width and segmentation fixture");

    if (!text) {
      continue;
    }

    std::size_t previous = 0;
    std::int32_t cells = 0;

    for (const auto &grapheme : text->graphemes()) {
      check(grapheme.byte_begin == previous && grapheme.byte_end > previous, "ordered contiguous coverage");
      check(text->nextCursor(previous) == grapheme.byte_end, "forward cursor at boundary");
      check(text->previousCursor(grapheme.byte_end) == previous, "reverse cursor at boundary");
      previous = grapheme.byte_end;
      cells += grapheme.cells;
    }

    check(previous == text->bytes().size() && cells == text->measure(), "coverage and measured width");

    for (std::int32_t budget = -1; budget <= text->measure() + 1; ++budget) {
      const auto clip = text->clipPrefix(budget);

      check(clip.cells <= (budget < 0 ? 0 : budget) && text->nextCursor(clip.byte_end).has_value(),
            "clipping preserves boundaries and budget");
      check(clip.graphemes <= text->graphemes().size(), "clip count bounded");

      if (clip.graphemes < text->graphemes().size()) {
        check(clip.cells + text->graphemes()[clip.graphemes].cells > (budget < 0 ? 0 : budget),
              "clip is the maximal fitting prefix");
      }
    }

    for (std::size_t byte = 0; byte <= text->bytes().size(); ++byte) {
      bool boundary = byte == 0;

      for (const auto &grapheme : text->graphemes()) {
        boundary = boundary || byte == grapheme.byte_end;
      }

      check(text->nextCursor(byte).has_value() == boundary && text->previousCursor(byte).has_value() == boundary,
            "cursor refuses inside scalar or cluster");
    }

    const auto past_end = text->nextCursor(text->bytes().size() + 1);

    check(!past_end && past_end.error().kind == TextErrorKind::NOT_BOUNDARY, "past-end cursor explicit error");
    check(text->nextCursor(text->bytes().size()) == text->bytes().size() && text->previousCursor(0) == 0,
          "cursor endpoints are idempotent");
  }

  WidthPolicy wide;

  wide.ambiguous = AmbiguousWidth::WIDE;
  wide.revision = 7;

  const auto ambiguous = GraphemeText::from(utf8(u8"·α"), wide);

  check(ambiguous && ambiguous->measure() == 4 && ambiguous->policy() == wide, "ambiguous policy captured");

  WidthPolicy narrow_emoji;

  narrow_emoji.emoji = EmojiWidth::ONE;

  for (auto bytes : {u8"😀", u8"👩‍💻", u8"👍🏽", u8"❤️", u8"1️⃣", u8"🇺🇦"}) {
    const auto text = GraphemeText::from(utf8(bytes), narrow_emoji);

    check(text && text->measure() == 1, "emoji one-cell policy includes single emoji");
  }

  for (auto bytes : {u8"\u0301", u8"\r\n", u8"\t", u8"\u001b", u8"\u0085", u8"\u2028", u8"\u202e"}) {
    const auto text = GraphemeText::from(utf8(bytes));

    check(text && text->graphemes().size() == 1 && text->measure() == 1 &&
            text->graphemes()[0].display == TextDisplay::UNSUPPORTED,
          "control or standalone mark becomes whole-cluster placeholder");
  }

  const auto nul = GraphemeText::from(std::string("a\0b", 3));

  check(nul && nul->bytes().size() == 3 && nul->graphemes().size() == 3 &&
          nul->graphemes()[1].display == TextDisplay::UNSUPPORTED,
        "embedded NUL does not terminate scan");

  const auto cjk = GraphemeText::from(utf8(u8"界a"));

  check(cjk && cjk->clipPrefix(1).byte_end == 0 && cjk->clipPrefix(2).byte_end == 3 && cjk->clipPrefix(3).byte_end == 4,
        "wide cluster never partially clipped");

  std::string source = utf8(u8"é");
  auto owned = GraphemeText::from(source);

  source.clear();
  check(owned && owned->bytes() == utf8(u8"é"), "text owns original bytes");

  if (owned) {
    auto moved = std::move(*owned);

    check(moved.measure() == 1 && owned->measure() == 0 && owned->bytes().empty() && owned->graphemes().empty(),
          "move preserves destination and resets source");
  }
}

void errors() {
  using namespace portals;
  const std::vector<std::string> invalid = {"\xc0\xaf", "\xed\xa0\x80", "\x80", "\xe2\x82", "\xf4\x90\x80\x80"};

  for (const auto &bytes : invalid) {
    const auto text = GraphemeText::from("ok" + bytes);

    check(!text && text.error().kind == TextErrorKind::INVALID_UTF8 && text.error().byte_offset == 2,
          "invalid UTF-8 reports offending sequence byte offset");
  }

  TextLimits limits;

  limits.max_input_bytes = 1;

  auto text = GraphemeText::from("ab", {}, limits);

  check(!text && text.error().kind == TextErrorKind::INPUT_LIMIT, "input byte cap");
  limits = {};
  limits.max_cluster_bytes = 2;
  text = GraphemeText::from(utf8(u8"xé"), {}, limits);
  check(!text && text.error().kind == TextErrorKind::CLUSTER_LIMIT && text.error().byte_offset == 1,
        "cluster limit reports start and does not truncate");
  limits = {};
  limits.max_graphemes = 1;
  text = GraphemeText::from("ab", {}, limits);
  check(!text && text.error().kind == TextErrorKind::GRAPHEME_LIMIT && text.error().byte_offset == 1,
        "grapheme count cap");
  limits = {};
  limits.max_cells = 2;
  text = GraphemeText::from(utf8(u8"a界"), {}, limits);
  check(!text && text.error().kind == TextErrorKind::CELL_LIMIT && text.error().byte_offset == 1,
        "checked cell accumulation");
  limits = {0, 0, 0, 0};
  check(GraphemeText::from("", {}, limits).has_value(), "zero limits permit empty input");

  WidthPolicy invalid_policy;

  invalid_policy.emoji = static_cast<EmojiWidth>(3);
  text = GraphemeText::from("", invalid_policy);
  check(!text && text.error().kind == TextErrorKind::INVALID_POLICY, "policy enum validation");

  std::string long_cluster = "e";

  for (int i = 0; i < 1000; ++i) {
    long_cluster += utf8(u8"́");
  }

  text = GraphemeText::from(long_cluster);
  check(text && text->graphemes().size() == 1 && text->measure() == 1, "long bounded cluster");
}

void officialCorpus(const char *path) {
  std::ifstream input(path);

  check(input.good(), "Unicode conformance fixture opened");

  std::string line;
  std::size_t cases = 0;

  while (std::getline(input, line)) {
    line = line.substr(0, line.find('#'));

    std::istringstream tokens(line);
    std::string token;
    std::string bytes;
    std::vector<std::size_t> boundaries;

    while (tokens >> token) {
      if (token == "\xc3\xb7") {
        boundaries.push_back(bytes.size());
      } else if (token != "\xc3\x97") {
        appendUtf8(bytes, static_cast<std::uint32_t>(std::stoul(token, nullptr, 16)));
      }
    }

    if (boundaries.empty()) {
      continue;
    }

    ++cases;

    const auto text = portals::GraphemeText::from(bytes);

    check(text.has_value(), "corpus accepts all valid UTF-8 including controls");

    if (!text) {
      continue;
    }

    std::vector<std::size_t> actual{0};

    for (const auto &grapheme : text->graphemes()) {
      actual.push_back(grapheme.byte_end);
    }

    if (actual != boundaries) {
      std::cerr << "Unicode corpus case " << cases << " differs\n";
      check(false, "official extended grapheme boundaries");
    }
  }

  constexpr std::size_t EXPECTED_UNICODE18_CASES = 853;

  check(cases == EXPECTED_UNICODE18_CASES, "full official pinned corpus, no filtering");
  std::cout << "Unicode 18 corpus: " << cases << " cases\n";
}

void allocationFailures() {
  using portals::GraphemeText;
  const std::string bytes = "A sufficiently long text with multiple graphemes";
  const auto reference = GraphemeText::from(bytes);
  constexpr std::ptrdiff_t PROBE_LIMIT = 64;
  bool constructed{};
  bool moved{};
  bool copied{};

  for (std::ptrdiff_t budget = 0; budget < PROBE_LIMIT; ++budget) {
    allocation_test::remaining = budget;

    try {
      const auto result = GraphemeText::from(bytes);

      allocation_test::remaining = -1;
      check(result && result->bytes() == bytes, "construction succeeds without a partial value");
      constructed = true;

      break;
    } catch (const std::bad_alloc &) {
      allocation_test::remaining = -1;
    }
  }

  for (std::ptrdiff_t budget = 0; budget < PROBE_LIMIT; ++budget) {
    auto source = *reference;

    allocation_test::remaining = budget;

    try {
      GraphemeText result(std::move(source));

      allocation_test::remaining = -1;
      check(result.bytes() == bytes && result.measure() == reference->measure(), "move retains bytes and metadata");
      check(source.bytes().empty() && source.graphemes().empty() && source.measure() == 0,
            "successful move empties the source");
      moved = true;

      break;
    } catch (const std::bad_alloc &) {
      allocation_test::remaining = -1;
      check(source.bytes() == bytes && source.measure() == reference->measure(),
            "failed move construction preserves the source");
    }
  }

  for (std::ptrdiff_t budget = 0; budget < PROBE_LIMIT; ++budget) {
    auto destination = *GraphemeText::from("old");

    allocation_test::remaining = budget;

    try {
      destination = *reference;

      allocation_test::remaining = -1;
      check(destination.bytes() == bytes && destination.measure() == reference->measure(),
            "copy assignment replaces all text metadata");
      copied = true;

      break;
    } catch (const std::bad_alloc &) {
      allocation_test::remaining = -1;
      check(destination.bytes() == "old" && destination.measure() == 3, "failed copy leaves destination unchanged");
    }
  }

  check(constructed && moved && copied, "all text operations succeed after bounded allocation probes");

  auto source = *reference;
  auto destination = *GraphemeText::from("old");

  allocation_test::remaining = 0;
  destination = std::move(source);
  allocation_test::remaining = -1;
  check(destination.bytes() == bytes && source.bytes().empty(), "move assignment allocates nothing");
}
} // namespace

int main(int argc, char **argv) {
  fixtures();
  errors();
  allocationFailures();
  check(argc == 2, "fixture path argument");

  if (argc == 2) {
    officialCorpus(argv[1]);
  }

  return failures == 0 ? 0 : 1;
}

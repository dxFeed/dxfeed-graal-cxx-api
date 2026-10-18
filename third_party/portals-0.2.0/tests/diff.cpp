// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include "support/vt_oracle.hpp"
#include <portals/diff.hpp>
#include <portals/render.hpp>

#include <array>
#include <cstdlib>
#include <iostream>
#include <new>
#include <string>
#include <vector>

namespace {
bool fail_next_allocation = false;
std::size_t minimum_failure_bytes = 0;
} // namespace

void *operator new(std::size_t bytes) {
  if (fail_next_allocation && bytes >= minimum_failure_bytes) {
    fail_next_allocation = false;
    throw std::bad_alloc();
  }

  if (auto *memory = std::malloc(bytes == 0 ? 1 : bytes)) {
    return memory;
  }

  throw std::bad_alloc();
}

// Keep array storage on a matching allocator/deallocator pair on MSVC as well.
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

void check(bool value, const char *message) {
  if (!value) {
    std::cerr << message << '\n';
    ++failures;
  }
}

std::string utf8(std::u8string_view value) {
  return {reinterpret_cast<const char *>(value.data()), value.size()};
}

CellBuffer buffer(CellSize size = {8, 3}, WidthPolicy policy = {}) {
  return std::move(*CellBuffer::from(size, policy, {32, 16, 256, 64, 512, 32}));
}

void put(CellBuffer &frame, int x, int y, const std::string &value, CellStyle style = {}) {
  auto text = GraphemeText::from(value, frame.policy());

  if (!text || !frame.put({x, y}, *text, 0, style)) {
    std::exit(2);
  }
}

void transition(const CellBuffer &old, const CellBuffer &next) {
  test_support::VtOracle terminal(next.size().width, next.size().height);
  auto initial = encodeDiff(nullptr, old);
  auto delta = encodeDiff(&old, next);

  check(initial.has_value() && delta.has_value(), "bounded ordinary frames encode");

  if (!initial || !delta) {
    return;
  }

  terminal.feed(*initial);
  terminal.feed(*delta);
  check(terminal.equals(next), "independent VT interpreter reaches target cells/styles/wide roles");
  check(terminal.neutral(), "completed transaction leaves home/default style/wrap enabled");
  check(terminal.scrolls() == 0, "last-column and lower-right output must not scroll");
}
} // namespace

int main() {
  // Verify oracle behavior independently of the encoder before trusting it.
  test_support::VtOracle known(2, 1);

  known.feed("abX");
  check(known.scrolls() == 1 && known.at(0, 0).glyph == "X", "oracle detects delayed wrap/scroll");

  test_support::VtOracle unicode(3, 1);

  unicode.feed("\x1b[?7l\x1b[1;38;2;1;2;3m" + utf8(u8"界é"));
  check(unicode.at(0, 0).width == 2 && unicode.at(1, 0).tail && unicode.at(2, 0).glyph == utf8(u8"é") &&
          unicode.at(0, 0).style.bold && unicode.at(0, 0).style.foreground == RgbColor{1, 2, 3},
        "oracle independently decodes SGR/UTF-8/combining/wide");

  auto old = buffer();
  auto next = buffer();
  CellStyle rich{{RgbColor{255, 7, 100}}, {RgbColor{1, 2, 255}}, true, true, true};

  put(old, 6, 2, utf8(u8"界"), rich);
  put(next, 7, 2, "Z");
  put(next, 1, 0, utf8(u8"é"), rich);
  put(next, 3, 1, utf8(u8"😀"), rich);
  transition(old, next);
  transition(next, old);

  auto initial = encodeDiff(nullptr, old);

  check(initial && !initial->empty(), "initial frame must emit a complete repaint");
  check(encodeDiff(&old, old, 0)->empty(), "unchanged frame has zero output even with zero cap");
  check(!encodeDiff(nullptr, old, initial->size() - 1), "one byte below exact output cap rejects");
  check(encodeDiff(nullptr, old, initial->size()) == initial, "exact output cap succeeds");

  fail_next_allocation = true;

  const auto allocation_failed = encodeDiff(nullptr, old);

  check(!allocation_failed && allocation_failed.error() == DiffError::ALLOCATION_FAILURE,
        "output allocation failure is explicit and preserves input");
  check(encodeDiff(nullptr, old) == initial, "failed allocation leaves the source frame unchanged");
  check(!fail_next_allocation, "first allocation failure was actually injected");
  minimum_failure_bytes = initial->size();
  fail_next_allocation = true;

  const auto payload_allocation_failed = encodeDiff(nullptr, old);

  check(!fail_next_allocation, "payload allocation failure was actually injected");
  check(!payload_allocation_failed && payload_allocation_failed.error() == DiffError::ALLOCATION_FAILURE,
        "output storage allocation failure is explicit, independently of debug bookkeeping");
  minimum_failure_bytes = 0;

  auto reordered = buffer();

  put(reordered, 0, 0, "temporary");
  put(reordered, 0, 0, " ");
  put(reordered, 6, 2, utf8(u8"界"), rich);
  check(encodeDiff(&old, reordered)->empty(), "cross-buffer local IDs and blank/space distinction are irrelevant");

  std::uint32_t random = 0x12345678;
  const auto next_random = [&] {
    random = random * 1664525u + 1013904223u;

    return random;
  };
  const std::array<std::string, 6> alphabet{" ", "A", "z", utf8(u8"é"), utf8(u8"界"), utf8(u8"😀")};

  for (int sample = 0; sample < 150; ++sample) {
    auto a = buffer();
    auto b = buffer();

    for (auto *frame : {&a, &b}) {
      for (int write = 0; write < 24; ++write) {
        const auto glyph = next_random() % alphabet.size();
        const auto x = static_cast<int>(next_random() % (glyph >= 4 ? 7 : 8));
        const auto y = static_cast<int>(next_random() % 3);

        put(*frame, x, y, alphabet[glyph], next_random() % 2 ? rich : CellStyle{});
      }
    }

    transition(a, b);
  }

  for (CellSize resized : {CellSize{4, 1}, CellSize{12, 5}}) {
    auto before_resize = buffer();

    put(before_resize, 2, 0, "q", rich);

    auto after_resize = buffer(resized);

    put(after_resize, resized.width - 2, resized.height - 1, utf8(u8"界"), rich);

    test_support::VtOracle terminal(8, 3);

    terminal.feed(*encodeDiff(nullptr, before_resize));
    terminal.resize(resized.width, resized.height);
    terminal.feed(*encodeDiff(&before_resize, after_resize));
    check(terminal.equals(after_resize) && terminal.neutral(), "grow/shrink repaint clears retained terminal contents");
    check(encodeDiff(&after_resize, after_resize)->empty(), "resized frame becomes unchanged after completion");
  }

  WidthPolicy revised;

  revised.revision = 1;

  auto policy_before = buffer();
  auto policy_after = buffer({8, 3}, revised);
  auto policy_bytes = encodeDiff(&policy_before, policy_after);

  check(policy_bytes && !policy_bytes->empty(), "policy invalidation repaints even equal display cells");

  test_support::VtOracle policy_terminal(8, 3);

  policy_terminal.feed("stale");
  policy_terminal.feed(*policy_bytes);
  check(policy_terminal.equals(policy_after) && policy_terminal.neutral(),
        "policy repaint restores all viewport cells");
  check(encodeDiff(&policy_after, policy_after)->empty(), "unchanged revised policy emits nothing");
  // Full renderer remains a separately implemented desired-frame oracle.
  std::vector<RenderCommand> scene;
  const auto clip = *CellRect::from({0, 0}, {8, 3});
  const auto scene_text = GraphemeText::from(utf8(u8"a界é"));

  scene.push_back(TextCommand{{2, 2}, *scene_text, clip, rich, 0});

  auto rendered = renderFullFrame({8, 3}, scene);

  if (!rendered) {
    return 2;
  }

  transition(old, *rendered);

  for (CellSize size : {CellSize{0, 0}, CellSize{1, 1}, CellSize{8, 1}}) {
    auto frame = buffer(size);
    test_support::VtOracle terminal(size.width, size.height);

    terminal.feed(*encodeDiff(nullptr, frame));
    check(terminal.equals(frame) && terminal.neutral(), "empty/narrow/full-row surfaces are safe");
  }

  return failures == 0 ? 0 : 1;
}

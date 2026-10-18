// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#include <portals/input.hpp>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace {
using namespace portals;
int failures = 0;
std::size_t partition_runs = 0;

void check(bool value, const char *message) {
  if (!value) {
    if (failures < 30) {
      std::cerr << message << '\n';
    }

    ++failures;
  }
}

InputEvent text(char32_t scalar, bool alt = false) {
  return TextInputEvent{scalar, {false, alt, false, false}};
}

InputEvent key(InputKey identity, InputModifiers mods = {}) {
  return KeyEvent{identity, 0, mods};
}

struct Run {
  std::vector<InputEvent> events;
  std::size_t consumed{};
  InputStatus status{};
  bool operator==(const Run &) const = default;
};

bool feed(InputDecoder &decoder, std::string_view bytes, InputTime now, Run &run) {
  std::size_t offset = 0;
  std::size_t pulls = 0;

  do {
    if (++pulls > bytes.size() * 3 + 20) {
      check(false, "bounded pull progress");

      return false;
    }

    auto step = decoder.next(bytes.substr(offset), now);

    check(step.has_value(), "valid feed schedule accepted");

    if (!step) {
      return false;
    }

    check(step->consumed <= bytes.size() - offset, "exact consumed prefix in range");
    offset += step->consumed;
    run.consumed += step->consumed;
    run.status = step->status;

    if (step->event) {
      if (const auto *issue = std::get_if<InputIssue>(&*step->event)) {
        check(issue->begin <= issue->end && issue->end <= run.consumed, "issue range covers consumed bytes only");
      }

      run.events.push_back(std::move(*step->event));
    }

    if (step->status == InputStatus::FAILED) {
      return false;
    }

    if (step->status == InputStatus::NEED_INPUT) {
      check(offset == bytes.size(), "need_input consumes complete available suffix");

      return true;
    }
  } while (true);
}

void finish(InputDecoder &decoder, InputTime now, Run &run) {
  for (unsigned tries = 0; tries < 5; ++tries) {
    auto step = decoder.finish(now);

    check(step.has_value(), "finish accepts monotonic clock");

    if (!step) {
      return;
    }

    check(step->consumed == 0, "finish consumes no bytes");
    run.status = step->status;

    if (step->event) {
      run.events.push_back(std::move(*step->event));
    }

    if (step->status == InputStatus::FINISHED || step->status == InputStatus::FAILED) {
      return;
    }
  }

  check(false, "finish drains bounded pending events");
}

Run decode(std::string_view bytes, const std::vector<std::size_t> &ends = {}, InputOptions options = {},
           bool probes = false) {
  auto decoder = *InputDecoder::create(options);
  Run result;

  if (probes) {
    check(decoder.expectReplies({true, true, 42, 0, 100}).has_value(), "probe window armed");
  }

  std::size_t start = 0;

  for (auto end : ends) {
    if (!feed(decoder, bytes.substr(start, end - start), 0, result)) {
      return result;
    }

    start = end;
  }

  if (feed(decoder, bytes.substr(start), 0, result)) {
    finish(decoder, 0, result);
  }

  check(result.consumed == bytes.size(), "all input accounted exactly once");

  return result;
}

void golden(std::string_view bytes, std::vector<InputEvent> expected, InputOptions options = {}, bool probes = false) {
  const auto actual = decode(bytes, {}, options, probes);

  check(actual.events == expected, "semantic golden event sequence");

  if (actual.events != expected && failures < 30) {
    std::cerr << "  fixture bytes=" << bytes.size() << " actual=" << actual.events.size()
              << " expected=" << expected.size() << '\n';
  }
}

void semanticFixtures() {
  for (unsigned byte = 0; byte < 32; ++byte) {
    const std::string bytes(1, static_cast<char>(byte));

    if (byte == 8 || byte == 9 || byte == 13 || byte == 27) {
      const auto identity = byte == 8    ? InputKey::BACKSPACE
                            : byte == 9  ? InputKey::TAB
                            : byte == 13 ? InputKey::ENTER
                                         : InputKey::ESCAPE;

      golden(bytes, {key(identity)});
    } else {
      golden(bytes, {KeyEvent{InputKey::CHARACTER, static_cast<char32_t>(byte + 0x40), {false, false, true, false}}});
    }
  }

  golden("a\r\t\x7f\b",
         {text(U'a'), key(InputKey::ENTER), key(InputKey::TAB), key(InputKey::BACKSPACE), key(InputKey::BACKSPACE)});
  golden(std::string("\0\x01\x1c\n", 4), {KeyEvent{InputKey::CHARACTER, U'@', {false, false, true, false}},
                                          KeyEvent{InputKey::CHARACTER, U'A', {false, false, true, false}},
                                          KeyEvent{InputKey::CHARACTER, U'\\', {false, false, true, false}},
                                          KeyEvent{InputKey::CHARACTER, U'J', {false, false, true, false}}});
  golden("\xc2\xa2\xe7\x95\x8c\xf0\x9f\x98\x80", {text(U'¢'), text(U'界'), text(U'😀')});
  golden("e\xcc\x81", {text(U'e'), text(U'\u0301')});
  golden("\xc2\x80\xdf\xbf\xe0\xa0\x80\xed\x9f\xbf\xee\x80\x80\xf4\x8f\xbf\xbf",
         {text(0x80), text(0x7ff), text(0x800), text(0xd7ff), text(0xe000), text(0x10ffff)});
  golden("\x1b"
         "x\x1b\xc3\xa9",
         {text(U'x', true), text(U'é', true)});
  golden("\x1b\x1b", {key(InputKey::ESCAPE), key(InputKey::ESCAPE)});
  golden("\x1b[A\x1bOB\x1b[1;6C\x1b[24~\x1b[Z",
         {key(InputKey::UP), key(InputKey::DOWN), key(InputKey::RIGHT, {true, false, true, false}), key(InputKey::F12),
          key(InputKey::TAB, {true, false, false, false})});

  const std::pair<std::string, InputKey> keys[] = {
    {"A", InputKey::UP},  {"B", InputKey::DOWN}, {"C", InputKey::RIGHT}, {"D", InputKey::LEFT}, {"H", InputKey::HOME},
    {"F", InputKey::END}, {"P", InputKey::F1},   {"Q", InputKey::F2},    {"R", InputKey::F3},   {"S", InputKey::F4}};

  for (const auto &[suffix, identity] : keys) {
    golden("\x1b[" + suffix, {key(identity)});
    golden("\x1bO" + suffix, {key(identity)});

    for (unsigned mod = 1; mod <= 8; ++mod) {
      golden("\x1b[1;" + std::to_string(mod) + suffix,
             {key(identity, {(mod & 1) == 0, ((mod - 1) & 2) != 0, ((mod - 1) & 4) != 0, false})});
    }
  }

  const std::pair<unsigned, InputKey> tilde[] = {
    {1, InputKey::HOME},    {2, InputKey::INSERT},    {3, InputKey::DELETE_KEY}, {4, InputKey::END},
    {5, InputKey::PAGE_UP}, {6, InputKey::PAGE_DOWN}, {7, InputKey::HOME},       {8, InputKey::END},
    {11, InputKey::F1},     {12, InputKey::F2},       {13, InputKey::F3},        {14, InputKey::F4},
    {15, InputKey::F5},     {17, InputKey::F6},       {18, InputKey::F7},        {19, InputKey::F8},
    {20, InputKey::F9},     {21, InputKey::F10},      {23, InputKey::F11},       {24, InputKey::F12}};

  for (auto [number, identity] : tilde) {
    golden("\x1b[" + std::to_string(number) + ";3~", {key(identity, {false, true, false, false})});
  }

  golden("\x80", {InputIssue{InputIssueKind::INVALID_UTF8, 0, 1}});
  golden("\xe2\x82", {InputIssue{InputIssueKind::TRUNCATED, 0, 2}});
  golden("\xe2\x1b[A", {InputIssue{InputIssueKind::INVALID_UTF8, 0, 1}, key(InputKey::UP)});
  golden("\xed\xa0\x80\xf4\x90\x80\x80\xc0\xaf",
         {InputIssue{InputIssueKind::INVALID_UTF8, 0, 3}, InputIssue{InputIssueKind::INVALID_UTF8, 3, 7},
          InputIssue{InputIssueKind::INVALID_UTF8, 7, 8}, InputIssue{InputIssueKind::INVALID_UTF8, 8, 9}});
  golden("\x1b[1;;2A", {InputIssue{InputIssueKind::INVALID_SEQUENCE, 0, 7}});
  golden("\x1b[1;A", {InputIssue{InputIssueKind::INVALID_SEQUENCE, 0, 5}});
  golden("\x1b[1:2A", {InputIssue{InputIssueKind::INVALID_SEQUENCE, 0, 6}});
  golden("\x1b[1 q", {InputIssue{InputIssueKind::UNKNOWN_SEQUENCE, 0, 5}});
  golden("\x1b[1 2Aq", {InputIssue{InputIssueKind::INVALID_SEQUENCE, 0, 5}, text(U'q')});
  golden("\x1b[\x01"
         "a",
         {InputIssue{InputIssueKind::INVALID_SEQUENCE, 0, 3}, text(U'a')});
  golden("\x1b[4294967296~", {InputIssue{InputIssueKind::NUMERIC_OVERFLOW, 0, 13}});
  golden("\x1b[00000000001~", {InputIssue{InputIssueKind::PARAMETER_LIMIT, 0, 14}});
  golden("\x1b[1;9A", {InputIssue{InputIssueKind::UNKNOWN_SEQUENCE, 0, 6}});
  golden("\x9b"
         "A",
         {InputIssue{InputIssueKind::INVALID_UTF8, 0, 1}, text(U'A')});
}

void mouseAndProbes() {
  golden("\x1b[<0;1;2M\x1b[<0;1;2m\x1b[<35;9;8M\x1b[<92;3;4M",
         {MouseInputEvent{0, 1, MouseButton::LEFT, MouseAction::PRESS, {}},
          MouseInputEvent{0, 1, MouseButton::LEFT, MouseAction::RELEASE, {}},
          MouseInputEvent{8, 7, MouseButton::NO_BUTTON, MouseAction::MOVE, {}},
          MouseInputEvent{2, 3, MouseButton::WHEEL_UP, MouseAction::SCROLL, {true, true, true, false}}});

  for (unsigned code = 64; code < 68; ++code) {
    golden("\x1b[<" + std::to_string(code) + ";1;1M",
           {MouseInputEvent{0,
                            0,
                            static_cast<MouseButton>(static_cast<unsigned>(MouseButton::WHEEL_UP) + code - 64),
                            MouseAction::SCROLL,
                            {}}});
  }

  for (const std::string seq : {"\x1b[<0;0;1M", "\x1b[<128;1;1M", "\x1b[<64;1;1m", "\x1b[<96;1;1M", "\x1b[<3;1;1M",
                                "\x1b[<0;2147483649;1M", "\x1b[<1;1M"}) {
    golden(seq, {InputIssue{InputIssueKind::INVALID_SEQUENCE, 0, seq.size()}});
  }

  golden("\x1b[<2;2147483648;1M",
         {MouseInputEvent{std::numeric_limits<std::int32_t>::max(), 0, MouseButton::RIGHT, MouseAction::PRESS, {}}});
  golden("\x1b[?1;2c\x1b[4;5R",
         {InputProbeCandidate{InputProbeKind::PRIMARY_DEVICE_ATTRIBUTES, 42, {1, 2}, 2},
          InputProbeCandidate{InputProbeKind::CURSOR_POSITION, 42, {4, 5}, 2}},
         {}, true);
  golden("\x1b[?4294967295c", {InputProbeCandidate{InputProbeKind::PRIMARY_DEVICE_ATTRIBUTES, 42, {4294967295U}, 1}},
         {}, true);
  golden("\x1b[1;2R", {key(InputKey::F3, {true, false, false, false})});
  golden("\x1b[1;2R\x1b[1;2R",
         {InputProbeCandidate{InputProbeKind::CURSOR_POSITION, 42, {1, 2}, 2},
          key(InputKey::F3, {true, false, false, false})},
         {}, true);
  golden("\x1b[?1c\x1b[?1c",
         {InputProbeCandidate{InputProbeKind::PRIMARY_DEVICE_ATTRIBUTES, 42, {1}, 1},
          InputIssue{InputIssueKind::UNKNOWN_SEQUENCE, 5, 10}},
         {}, true);
  golden("\x1b[0;1R", {InputIssue{InputIssueKind::UNKNOWN_SEQUENCE, 0, 6}}, {}, true);

  InputOptions disabled;

  disabled.sgr_mouse = false;
  disabled.bracketed_paste = false;
  golden("\x1b[<0;1;1M", {InputIssue{InputIssueKind::UNKNOWN_SEQUENCE, 0, 9}}, disabled);
  golden("\x1b[200~", {InputIssue{InputIssueKind::UNKNOWN_SEQUENCE, 0, 6}}, disabled);
}

void pasteAndLimits() {
  const std::string begin = "\x1b[200~";
  const std::string end = "\x1b[201~";

  golden(begin + end, {PasteInputEvent{}});

  std::string hostile = "\x1b[A\x1b[<0;1;1M\x1b[?1c\x1b]52;hello\a\x1b[200~\xff";

  hostile.append("\0\r\t\n", 4);
  golden(begin + hostile + end + "x", {PasteInputEvent{hostile}, text(U'x')});
  golden(begin + "\x1b[20\x1b\x1b[201x" + end, {PasteInputEvent{"\x1b[20\x1b\x1b[201x"}});
  // The legacy delimiter is not authenticated: a forged end really closes paste.
  golden(begin + "a" + end + "\x1b[A", {PasteInputEvent{"a"}, key(InputKey::UP)});

  InputOptions small;

  small.max_paste_bytes = 3;
  golden(begin + "abc" + end, {PasteInputEvent{"abc"}}, small);
  golden(begin + "abcd\x1b[A" + end + "z", {InputIssue{InputIssueKind::PASTE_LIMIT, 0, 10}, text(U'z')}, small);
  golden(begin + "ab\x1b[20x\x1b[A" + end + "z", {InputIssue{InputIssueKind::PASTE_LIMIT, 0, 13}, text(U'z')}, small);
  small.max_paste_bytes = 0;
  golden(begin + end, {PasteInputEvent{}}, small);
  golden(begin + "a" + end, {InputIssue{InputIssueKind::PASTE_LIMIT, 0, 7}}, small);
  golden(begin + "abc", {InputIssue{InputIssueKind::TRUNCATED, 0, 9}});

  for (const std::string intro : {"\x1b]", "\x1bP", "\x1bX", "\x1b^", "\x1b_"}) {
    golden(intro + "\x1b[A\x1b[200~hidden\x1b\\x", {InputIssue{InputIssueKind::UNKNOWN_SEQUENCE, 0, 2}, text(U'x')});
  }

  golden("\x1b]hidden\ax", {InputIssue{InputIssueKind::UNKNOWN_SEQUENCE, 0, 2}, text(U'x')});
  golden("\x1bPhidden\ax\x1b\\z", {InputIssue{InputIssueKind::UNKNOWN_SEQUENCE, 0, 2}, text(U'z')});
  small = {};
  small.max_sequence_bytes = 6;
  golden("\x1b[11~", {key(InputKey::F1)}, small);
  golden("\x1b[1;2A", {key(InputKey::UP, {true, false, false, false})}, small);
  golden("\x1b[01;2Aq", {InputIssue{InputIssueKind::SEQUENCE_LIMIT, 0, 7}, text(U'q')}, small);
  golden("\x1b[00001;2Aq", {InputIssue{InputIssueKind::SEQUENCE_LIMIT, 0, 7}, text(U'q')}, small);
  small.max_parameters = 1;
  golden("\x1b[1;2A", {InputIssue{InputIssueKind::PARAMETER_LIMIT, 0, 6}}, small);
  small.max_digits = 1;
  golden("\x1b[11~", {InputIssue{InputIssueKind::PARAMETER_LIMIT, 0, 5}}, small);

  for (std::size_t length : {63, 64, 65}) {
    const auto sequence = "\x1b[" + std::string(length - 3, ' ') + "q";

    golden(sequence,
           {InputIssue{length == 65 ? InputIssueKind::SEQUENCE_LIMIT : InputIssueKind::UNKNOWN_SEQUENCE, 0, length}});
  }

  std::string parameters = "\x1b[?0";

  for (unsigned i = 1; i < 16; ++i) {
    parameters += ";0";
  }

  golden(parameters + "c", {InputProbeCandidate{InputProbeKind::PRIMARY_DEVICE_ATTRIBUTES, 42, {}, 16}}, {}, true);
  golden(parameters + ";0c", {InputIssue{InputIssueKind::PARAMETER_LIMIT, 0, parameters.size() + 3}}, {}, true);
}

void timing() {
  auto decoder = *InputDecoder::create();
  Run run;

  feed(decoder, "\x1b", 10, run);
  check(decoder.deadline() == 40, "ESC absolute deadline exposed");
  feed(decoder, "", 39, run);
  check(run.events.empty(), "ESC does not expire early");
  feed(decoder, "x", 40, run);
  check(run.events == std::vector<InputEvent>{key(InputKey::ESCAPE), text(U'x')}, "deadline equality expires first");

  auto backwards = decoder.next("bad", 39);

  check(!backwards && backwards.error() == InputError::BACKWARD_TIME, "backward time rejected without input");
  check(!decoder.deadline(), "ground has no timer");
  decoder.reset();
  run = {};
  feed(decoder, "\x1b\xc3", 0, run);
  feed(decoder, "\xa9", 29, run);
  check(run.events == std::vector<InputEvent>{text(U'é', true)}, "Alt scalar within ESC window");
  decoder.reset();
  run = {};
  feed(decoder, "\x1b\xc3", 0, run);
  feed(decoder, "\xa9", 30, run);
  check(run.events == std::vector<InputEvent>{key(InputKey::ESCAPE), InputIssue{InputIssueKind::TRUNCATED, 1, 2},
                                              InputIssue{InputIssueKind::INVALID_UTF8, 2, 3}},
        "Alt timeout drains Escape and truncated tail before suffix");
  decoder.reset();
  run = {};
  feed(decoder, "\x1b\xc3", 0, run);
  finish(decoder, 0, run);
  check(run.events == std::vector<InputEvent>{key(InputKey::ESCAPE), InputIssue{InputIssueKind::TRUNCATED, 1, 2}},
        "EOF Alt tail drains Escape and truncated scalar");

  auto closed = decoder.next("x", 0);

  check(!closed && closed.error() == InputError::CLOSED, "EOF closes feed");

  auto again = decoder.finish(0);

  check(again && again->status == InputStatus::FINISHED && !again->event, "EOF idempotent after drain");

  for (const std::string prefix : {"\x1b[12", "\x1bO", "\x1b]abc", "\x1b[200~abc"}) {
    decoder.reset();
    run = {};
    feed(decoder, prefix, 0, run);

    const auto deadline = *decoder.deadline();

    feed(decoder, "", deadline - 1, run);

    const auto step = decoder.next("\x1b[A", deadline);

    check(step && step->consumed == 0 && step->status == InputStatus::FAILED && step->event &&
            std::get<InputIssue>(*step->event).kind == InputIssueKind::TIMEOUT,
          "unfinished control deadline fails before suffix consumption");
    check(!decoder.next("x", deadline), "failed stream requires explicit reset");
    decoder.reset();
    run = {};
    feed(decoder, "x", 0, run);
    check(run.events == std::vector<InputEvent>{text(U'x')}, "reset creates independent session");
  }

  decoder.reset();
  run = {};
  feed(decoder, "\x1b[", 0, run);
  feed(decoder, "1", 999, run);
  check(decoder.deadline() == 1000, "CSI bytes never extend deadline");
  decoder.reset();
  run = {};
  check(decoder.expectReplies({true, true, 8, 10, 20}).has_value(), "explicit probe window");
  feed(decoder, "\x1b[1;2R", 20, run);
  check(run.events == std::vector<InputEvent>{key(InputKey::F3, {true, false, false, false})},
        "late CPR uses key table");
  decoder.reset();
  run = {};
  check(decoder.expectReplies({true, true, 8, 10, 20}).has_value(), "future probe window armed");
  feed(decoder, "\x1b[", 9, run);
  feed(decoder, "1;2R", 10, run);
  check(run.events == std::vector<InputEvent>{key(InputKey::F3, {true, false, false, false})},
        "reply start must also belong to expected window");
  decoder.reset();
  run = {};
  feed(decoder, "\x1b[", 0, run);
  check(!decoder.expectReplies({true, true, 1, 0, 100}), "window cannot change inside candidate");
  decoder.reset();
  run = {};

  const auto max_time = std::numeric_limits<InputTime>::max();

  feed(decoder, "\x1b", max_time - 1, run);
  check(decoder.deadline() == max_time, "deadline addition saturates without wrap");
  feed(decoder, "", max_time, run);
  check(run.events == std::vector<InputEvent>{key(InputKey::ESCAPE)}, "saturated deadline expires at maximum time");
  decoder.reset();
  run = {};
  feed(decoder,
       "\x1b"
       "x",
       max_time, run);
  check(run.events == std::vector<InputEvent>{key(InputKey::ESCAPE), text(U'x')},
        "new saturated deadline expires within a single feed");

  InputOptions shorter;

  shorter.sequence_timeout = 10;
  decoder = *InputDecoder::create(shorter);
  run = {};
  feed(decoder, "\x1b", 0, run);

  const auto step = decoder.next("[A", 20);

  check(step && step->consumed == 1 && step->status == InputStatus::FAILED && step->event &&
          std::get<InputIssue>(*step->event).kind == InputIssueKind::TIMEOUT,
        "shorter sequence interval expires immediately on introducer transition");
  shorter = {};
  shorter.max_paste_bytes = 1;
  decoder = *InputDecoder::create(shorter);
  run = {};
  feed(decoder, "\x1b[200~ab", 0, run);
  feed(decoder, "\x1b[A", 29999, run);
  check(decoder.deadline() == 30000 && run.events.size() == 1, "oversize paste discard keeps original deadline");

  const auto expired_paste = decoder.next("\x1b[201~q", 30000);

  check(expired_paste && expired_paste->status == InputStatus::FAILED && expired_paste->consumed == 0,
        "discarded paste cannot recover after deadline through input suffix");

  const auto scheduled = [](bool bytewise) {
    auto scheduled_decoder = *InputDecoder::create();
    Run events;
    const std::pair<std::string_view, InputTime> batches[] = {
      {"\x1b\xc3", 0}, {"\xa9\x1b[", 29}, {"1;2A\x1b", 30}, {"\xc3\xa9", 60}};

    for (auto [bytes, time] : batches) {
      if (bytewise) {
        for (std::size_t i = 0; i < bytes.size(); ++i) {
          feed(scheduled_decoder, bytes.substr(i, 1), time, events);
        }
      } else {
        feed(scheduled_decoder, bytes, time, events);
      }
    }

    finish(scheduled_decoder, 60, events);

    return events;
  };

  check(scheduled(false) == scheduled(true), "same timed arrival batches preserve chunk invariance across deadlines");
}

std::uint32_t randomNext(std::uint32_t &seed) {
  seed = seed * 1664525U + 1013904223U;

  return seed;
}

void partitions(std::string_view bytes, InputOptions options = {}, bool probes = false) {
  const auto expected = decode(bytes, {}, options, probes);

  if (bytes.size() <= 12 && !bytes.empty()) {
    const auto possibilities = std::size_t{1} << (bytes.size() - 1);

    for (std::size_t mask = 0; mask < possibilities; ++mask) {
      std::vector<std::size_t> ends;

      for (std::size_t i = 1; i < bytes.size(); ++i) {
        if (mask & (std::size_t{1} << (i - 1))) {
          ends.push_back(i);
        }
      }

      check(decode(bytes, ends, options, probes) == expected,
            "all short nonempty partitions preserve events and accounting");
      ++partition_runs;
    }
  } else {
    for (std::size_t split = 1; split < bytes.size(); ++split) {
      check(decode(bytes, {split}, options, probes) == expected, "every long single split preserves events");
      ++partition_runs;
    }
  }

  for (std::size_t chunk : {1, 2, 3, 5, 8, 17}) {
    std::vector<std::size_t> ends;

    for (std::size_t end = chunk; end < bytes.size(); end += chunk) {
      ends.push_back(end);
    }

    check(decode(bytes, ends, options, probes) == expected, "fixed chunks preserve events");
    ++partition_runs;
  }

  for (std::size_t budget : {1, 2, 7}) {
    auto bounded = options;

    bounded.work_bytes = budget;
    check(decode(bytes, {}, bounded, probes) == expected, "one-event output and small work budgets preserve semantics");
  }
}

void partitionAndFuzz() {
  for (const std::string bytes :
       {"a\r\t", "\x1b\x1b", "\x1b\xc3\xa9", "\xe2\x1b[A", "\x1b[1;2A", "\x1b[<0;1;1M", "\x1b[?1;2c", "\x1b[1;2R",
        "\x1b[1;;A", "\x1b[200~\x1b[201~", "\x1b]a\x1b\\x", "\xf4\x90\x80\x80", "\x1b[1 2Aq",
        "\x1b[200~x\x1b[20\x1b\x1b[201~z", "\x1b[200~\x1b[?1c\x1b[<0;1;1M\x1b]52;bad\a\x1b[201~"}) {
    partitions(bytes);
    partitions(bytes, {}, true);
  }

  InputOptions small;

  small.max_paste_bytes = 3;
  small.max_sequence_bytes = 6;
  partitions("\x1b[200~abcdef\x1b[A\x1b[201~q", small);
  partitions("\x1b[000000000000Aq", small);
  partitions("\x1b]" + std::string(512, 'x') + "\x1b\\q", small);
  golden("\x1b]" + std::string(1024 * 1024, 'x') + "\x1b\\q",
         {InputIssue{InputIssueKind::UNKNOWN_SEQUENCE, 0, 2}, text(U'q')}, small);
  golden("\x1b[200~" + std::string(1024 * 1024, 'x') + "\x1b[201~q",
         {InputIssue{InputIssueKind::PASTE_LIMIT, 0, 10}, text(U'q')}, small);

  std::uint32_t seed = 0x7a113;

  for (unsigned iteration = 0; iteration < 600; ++iteration) {
    std::string bytes;
    const auto length = randomNext(seed) % 96;

    for (unsigned i = 0; i < length; ++i) {
      bytes += static_cast<char>(randomNext(seed) >> 24);
    }

    if (iteration % 3 == 0) {
      bytes = "\x1b[200~" + bytes + "\x1b[201~q";
    } else if (iteration % 3 == 1) {
      bytes = "\x1b[" + bytes + "Aq";
    }

    const auto expected = decode(bytes, {}, small, true);
    std::vector<std::size_t> ends;

    for (std::size_t end = 1; end < bytes.size(); ++end) {
      if (randomNext(seed) % 3 == 0) {
        ends.push_back(end);
      }
    }

    check(decode(bytes, ends, small, true) == expected, "seeded malformed stream has chunk-invariant recovery");
    ++partition_runs;
  }
}

void optionsAndOwnership() {
  InputOptions options;

  options.work_bytes = 0;
  check(!InputDecoder::create(options), "zero work budget rejected");
  options = {};
  options.max_sequence_bytes = 65;
  check(!InputDecoder::create(options), "scratch cap validated");
  options = {};
  options.max_parameters = 17;
  check(!InputDecoder::create(options), "parameter storage cap validated");
  options = {};
  options.escape_timeout = 0;
  check(!InputDecoder::create(options), "zero ESC interval rejected");

  auto decoder = *InputDecoder::create();
  std::string source = "\x1b[200~abc\x1b[201~";
  auto result = decoder.next(source, 0);

  source.assign(source.size(), 'x');
  decoder.reset();
  check(result && result->event && std::get<PasteInputEvent>(*result->event).bytes == "abc",
        "event owns payload independent of source and reset");
  check(!decoder.expectReplies({true, true, 0, 1, 1}), "empty probe window rejected");

  KeyEvent native{InputKey::CHARACTER, U'é', {true, false, false, true}, KeyAction::RELEASE};

  check(native.action == KeyAction::RELEASE && native.modifiers.super,
        "event values can represent native backend data");
}
} // namespace

int main() {
  semanticFixtures();
  mouseAndProbes();
  pasteAndLimits();
  timing();
  partitionAndFuzz();
  optionsAndOwnership();
  std::cout << "Input partition/fuzz runs: " << partition_runs << "; failures: " << failures << '\n';

  return failures == 0 ? 0 : 1;
}

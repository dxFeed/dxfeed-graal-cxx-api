// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include <portals/input.hpp>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <source_location>
#include <string_view>
#include <vector>

namespace {
using namespace portals;
constexpr std::size_t MAX_INPUT_BYTES = 4096;
constexpr std::size_t PULLS_PER_BYTE = 3;
constexpr std::size_t DRAIN_ALLOWANCE = 16;
constexpr std::size_t MIXED_CHUNK_BYTES = 37;
constexpr InputTime ARRIVAL_TIME = 1;
constexpr InputTime EXPIRED_TIME = 100000;

void require(bool condition, std::source_location location = std::source_location::current()) {
  if (!condition) {
    // Report the failed invariant without allocating during the failure path.
    std::fprintf(stderr, "Input fuzz invariant failed at %s:%u\n", location.file_name(), location.line());
    std::abort();
  }
}

struct Decoded {
  std::vector<InputEvent> events;
  InputStatus status{};

  bool operator==(const Decoded &) const = default;
};

Decoded decode(std::string_view bytes, std::size_t chunk, InputOptions options, InputTime finishTime) {
  auto decoder = InputDecoder::create(options);

  require(decoder.has_value());
  require(decoder->expectReplies({true, true, 1, 0, EXPIRED_TIME}).has_value());

  std::vector<InputEvent> events;
  std::size_t consumed = 0;

  // Identical clocks and reply windows isolate chunking from timeout semantics.
  for (std::size_t begin = 0; begin < bytes.size(); begin += chunk) {
    const auto part = bytes.substr(begin, chunk);
    std::size_t offset = 0;
    std::size_t pulls = 0;

    while (true) {
      require(++pulls <= part.size() * PULLS_PER_BYTE + DRAIN_ALLOWANCE);

      auto step = decoder->next(part.substr(offset), ARRIVAL_TIME);

      require(step.has_value());
      require(step->consumed <= part.size() - offset);
      offset += step->consumed;
      consumed += step->consumed;

      if (step->event) {
        if (const auto *issue = std::get_if<InputIssue>(&*step->event)) {
          require(issue->begin <= issue->end && issue->end <= consumed);
        }

        events.push_back(std::move(*step->event));
      }

      require(step->status != InputStatus::FAILED && step->status != InputStatus::FINISHED);

      if (step->status == InputStatus::NEED_INPUT) {
        require(offset == part.size());

        break;
      }
    }
  }

  require(consumed == bytes.size());

  for (std::size_t pulls = 0; pulls < DRAIN_ALLOWANCE; ++pulls) {
    auto step = decoder->finish(finishTime);

    require(step.has_value() && step->consumed == 0);

    if (step->event) {
      events.push_back(std::move(*step->event));
    }

    if (step->status == InputStatus::FINISHED || step->status == InputStatus::FAILED) {
      if (step->status == InputStatus::FAILED) {
        // Unterminated control/paste data must fail closed when its deadline expires.
        require(step->event.has_value());

        const auto *issue = std::get_if<InputIssue>(&*step->event);

        require(issue && issue->kind == InputIssueKind::TIMEOUT);
        require(issue->begin <= issue->end && issue->end <= consumed);
      }

      const auto rejected = decoder->next({}, finishTime);

      require(!rejected && rejected.error() == InputError::CLOSED);

      // Events must remain independently owned after the source stream is reset.
      decoder->reset();

      const auto reset = decoder->next({}, 0);

      require(reset && reset->status == InputStatus::NEED_INPUT && !reset->event && reset->consumed == 0);

      return {std::move(events), step->status};
    }
  }

  std::abort();
}
} // namespace

// The libFuzzer entry point has an externally required C symbol name.
// NOLINTNEXTLINE(readability-identifier-naming)
extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size) {
  if (size > MAX_INPUT_BYTES) {
    return 0;
  }

  const std::string_view bytes(reinterpret_cast<const char *>(data), size);
  InputOptions options;

  options.max_paste_bytes = size ? data[0] : 0;
  options.work_bytes = size ? 1 + data[size - 1] : 1;

  if (size > 2) {
    constexpr std::size_t MIN_SEQUENCE_BYTES = 3;
    constexpr std::size_t SEQUENCE_LIMIT_VARIANTS = 62;
    constexpr std::size_t PARAMETER_LIMIT_VARIANTS = 16;

    options.max_sequence_bytes = MIN_SEQUENCE_BYTES + data[1] % SEQUENCE_LIMIT_VARIANTS;
    options.max_parameters = 1 + data[2] % PARAMETER_LIMIT_VARIANTS;
  }

  const auto finishTime = size && (data[0] & 1) ? EXPIRED_TIME : ARRIVAL_TIME;
  const auto whole = decode(bytes, std::max<std::size_t>(size, 1), options, finishTime);

  require(whole == decode(bytes, 1, options, finishTime));
  require(whole == decode(bytes, MIXED_CHUNK_BYTES, options, finishTime));

  return 0;
}

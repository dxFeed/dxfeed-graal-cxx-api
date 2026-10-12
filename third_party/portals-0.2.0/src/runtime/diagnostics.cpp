// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include <algorithm>
#include <array>
#include <format>
#include <limits>
#include <portals/diagnostics.hpp>
#include <stdexcept>

namespace portals {
namespace {
constexpr auto MAX_ID = std::numeric_limits<std::uint64_t>::max();

std::uint64_t issue(std::atomic<std::uint64_t> &counter) noexcept {
  auto value = counter.load(std::memory_order_relaxed);

  while (value != MAX_ID) {
    if (counter.compare_exchange_weak(value, value + 1, std::memory_order_relaxed)) {
      return value + 1;
    }
  }

  return 0;
}

constexpr std::array EVENT_NAMES{"input",
                                 "queue_accepted",
                                 "queue_rejected",
                                 "queue_dispatch",
                                 "queue_replaced",
                                 "queue_cancelled",
                                 "target",
                                 "focus",
                                 "action",
                                 "handler_started",
                                 "handler_returned",
                                 "property",
                                 "invalidation",
                                 "frame_requested",
                                 "frame_not_requested",
                                 "frame_submitted",
                                 "output",
                                 "completed",
                                 "queue_returned"};
constexpr std::array REASON_NAMES{"none",
                                  "queue_full",
                                  "replaced",
                                  "cancelled",
                                  "no_target",
                                  "target_disabled",
                                  "stale_target",
                                  "unbound_action",
                                  "unhandled",
                                  "handler_failed",
                                  "value_unchanged",
                                  "value_changed",
                                  "outcome_unknown",
                                  "frame_not_requested",
                                  "diff_empty",
                                  "output_failed",
                                  "output_accepted",
                                  "output_pending"};
constexpr std::array STATE_NAMES{"disabled", "enabled", "identity_exhausted"};
constexpr std::array INPUT_NAMES{"none", "key", "text", "paste", "mouse", "other"};

template <class Enum, std::size_t N> std::string_view name(Enum value, const std::array<const char *, N> &names) {
  const auto index = static_cast<std::size_t>(value);

  return index < N ? names[index] : "unknown";
}
} // namespace

thread_local DiagnosticScope *DiagnosticScope::active_{};

std::expected<void, DiagnosticError> Diagnostics::enable(DiagnosticOptions options) {
  std::lock_guard lock(mutex_);

  if (!options.capacity || state_.load() == DiagnosticState::IDENTITY_EXHAUSTED) {
    return std::unexpected(DiagnosticError::INVALID_ARGUMENT);
  }

  try {
    std::vector<DiagnosticRecord> replacement(options.capacity);
    const auto period = issue(identity_);

    if (!period) {
      state_.store(DiagnosticState::IDENTITY_EXHAUSTED);

      return std::unexpected(DiagnosticError::INVALID_ARGUMENT);
    }

    ring_.swap(replacement);
    options_ = options;
    start_ = count_ = 0;
    begin_ = sequence_ == MAX_ID ? MAX_ID : sequence_ + 1;
    end_ = overflow_ = 0;
    contention_.store(0);
    saturated_.store(false);
    period_.store(period);
    // Producers may discover identity exhaustion while this control-plane reset holds
    // the ring lock. Never overwrite that terminal state with a fresh enabled flag.
    auto previousState = state_.load();

    if (previousState == DiagnosticState::IDENTITY_EXHAUSTED ||
        !state_.compare_exchange_strong(previousState, DiagnosticState::ENABLED)) {
      return std::unexpected(DiagnosticError::INVALID_ARGUMENT);
    }

    return {};
  } catch (const std::bad_alloc &) {
    return std::unexpected(DiagnosticError::NO_MEMORY);
  } catch (const std::length_error &) {
    return std::unexpected(DiagnosticError::INVALID_ARGUMENT);
  }
}

void Diagnostics::disable() noexcept {
  std::lock_guard lock(mutex_);

  if (state_.load() == DiagnosticState::ENABLED) {
    state_.store(DiagnosticState::DISABLED);
    end_ = sequence_;
  }
}

bool Diagnostics::enabled() const noexcept {
  return state_.load(std::memory_order_relaxed) == DiagnosticState::ENABLED;
}

std::uint64_t Diagnostics::nextIdentity() noexcept {
  if (!enabled()) {
    return 0;
  }

  const auto id = issue(identity_);

  if (!id) {
    state_.store(DiagnosticState::IDENTITY_EXHAUSTED);
  }

  return id;
}

DiagnosticContext Diagnostics::current() const noexcept {
  if (!enabled()) {
    return {};
  }

  for (auto scope = DiagnosticScope::active_; scope; scope = scope->previous_) {
    if (scope->diagnostics_ == this) {
      return scope->context_;
    }
  }

  return {.period = period_.load()};
}

DiagnosticContext Diagnostics::beginInput(DiagnosticInput input) noexcept {
  if (!enabled()) {
    return {};
  }

  const auto parent = current();
  DiagnosticContext context{nextIdentity(), parent.correlationId, period_.load()};

  record({.context = context,
          .event = DiagnosticEvent::INPUT,
          .input = input,
          .incomplete = parent.correlationId && parent.period != context.period});

  return context;
}

void Diagnostics::lostContention() noexcept {
  if (const auto count = issue(contention_); !count || count == MAX_ID) {
    saturated_.store(true);
  }
}

void Diagnostics::record(DiagnosticRecord record) noexcept {
  if (!enabled()) {
    return;
  }

  if (record.event == DiagnosticEvent::FRAME_REQUESTED) {
    for (auto scope = DiagnosticScope::active_; scope; scope = scope->previous_) {
      if (scope->diagnostics_ == this) {
        scope->frame_requested_ = true;
      }
    }
  }

  std::unique_lock lock(mutex_, std::try_to_lock);

  if (!lock.owns_lock()) {
    lostContention();

    return;
  }

  if (!enabled()) {
    return;
  }

  if (sequence_ == MAX_ID) {
    state_.store(DiagnosticState::IDENTITY_EXHAUSTED);
    end_ = sequence_;

    return;
  }

  record.sequence = ++sequence_;
  record.incomplete |= !record.context.correlationId || record.context.period != period_.load();

  if (count_ == ring_.size()) {
    ring_[start_] = record;
    start_ = (start_ + 1) % ring_.size();

    if (overflow_ != MAX_ID) {
      ++overflow_;

      if (overflow_ == MAX_ID) {
        saturated_.store(true);
      }
    } else {
      saturated_.store(true);
    }
  } else {
    ring_[(start_ + count_) % ring_.size()] = record;
    ++count_;
  }
}

void Diagnostics::note(DiagnosticEvent event, DiagnosticReason reason, DiagnosticNodeId node) noexcept {
  if (!enabled()) {
    return;
  }

  if (event == DiagnosticEvent::PROPERTY) {
    for (auto scope = DiagnosticScope::active_; scope; scope = scope->previous_) {
      if (scope->diagnostics_ == this) {
        scope->changed_ |= reason == DiagnosticReason::VALUE_CHANGED;
        scope->unchanged_ |= reason == DiagnosticReason::VALUE_UNCHANGED;
      }
    }
  }

  record({.context = current(), .stableNodeId = node, .event = event, .reason = reason});
}

DiagnosticSnapshot Diagnostics::snapshot() const {
  std::unique_lock lock(mutex_);
  DiagnosticSnapshot result;

  result.options = options_;
  result.state = state_.load();
  result.period = period_.load();
  result.periodBegin = begin_;
  result.periodEnd = result.state == DiagnosticState::IDENTITY_EXHAUSTED ? sequence_ : end_;
  result.records.reserve(count_);

  for (std::size_t index = 0; index < count_; ++index) {
    result.records.push_back(ring_[(start_ + index) % ring_.size()]);
  }

  result.droppedOverflow = overflow_;
  result.droppedContention = contention_.load();
  result.countersSaturated = saturated_.load();
  lock.unlock();

  result.incomplete = result.droppedOverflow || result.droppedContention || result.countersSaturated ||
                      result.state == DiagnosticState::IDENTITY_EXHAUSTED;

  // Resolve outstanding queue/output edges once, then propagate missing evidence by
  // correlation. The two bounded passes take quadratic time and allocate no side index.
  for (auto &record : result.records) {
    if (record.event == DiagnosticEvent::FRAME_REQUESTED) {
      record.incomplete |= !std::ranges::any_of(result.records, [&](const auto &output) {
        return output.context.frameId == record.context.frameId && output.event == DiagnosticEvent::OUTPUT &&
               (output.reason == DiagnosticReason::OUTPUT_ACCEPTED || output.reason == DiagnosticReason::DIFF_EMPTY ||
                output.reason == DiagnosticReason::OUTPUT_FAILED);
      });
    }

    if (record.event == DiagnosticEvent::QUEUE_ACCEPTED) {
      record.incomplete |= !std::ranges::any_of(result.records, [&](const auto &delivery) {
        return delivery.context.publicationId == record.context.publicationId &&
               (delivery.event == DiagnosticEvent::QUEUE_RETURNED ||
                delivery.event == DiagnosticEvent::QUEUE_CANCELLED);
      });
    }
  }

  // Any global loss taints every retained chain; exact per-chain attribution is not promised.
  for (auto &record : result.records) {
    const auto id = record.context.correlationId;
    bool began{};
    bool ended{};
    bool incomplete = result.incomplete;

    for (const auto &candidate : result.records) {
      if (candidate.context.correlationId != id) {
        continue;
      }

      began |= candidate.event == DiagnosticEvent::INPUT;
      ended |= candidate.event == DiagnosticEvent::COMPLETED || candidate.event == DiagnosticEvent::QUEUE_RETURNED ||
               candidate.event == DiagnosticEvent::QUEUE_REJECTED ||
               candidate.event == DiagnosticEvent::QUEUE_CANCELLED;
      incomplete |= candidate.incomplete;
    }

    record.incomplete |= incomplete || !began || !ended;
  }

  result.incomplete |= std::ranges::any_of(result.records, [](const auto &record) {
    return record.incomplete;
  });

  // A copy prvalue avoids the potentially allocating noexcept vector move in MSVC Debug.
  // Guaranteed return-value elision preserves bad_alloc propagation without relying on NRVO.
  return DiagnosticSnapshot(result);
}

DiagnosticScope::DiagnosticScope(Diagnostics *diagnostics, DiagnosticContext context) noexcept {
  if (diagnostics && diagnostics->enabled()) {
    diagnostics_ = diagnostics;
    context_ = context;
    previous_ = active_;
    active_ = this;
  }
}

DiagnosticScope::~DiagnosticScope() {
  if (diagnostics_) {
    active_ = previous_;
  }
}

DiagnosticReason DiagnosticScope::outcome() const noexcept {
  if (changed_) {
    return DiagnosticReason::VALUE_CHANGED;
  }

  return unchanged_ ? DiagnosticReason::VALUE_UNCHANGED : DiagnosticReason::OUTCOME_UNKNOWN;
}

bool DiagnosticScope::frameRequested() const noexcept {
  return frame_requested_;
}

std::string exportDiagnostics(const DiagnosticSnapshot &snapshot) {
  std::string text = std::format("PORTALS-DIAGNOSTICS {}\n", snapshot.schemaVersion);

  text +=
    std::format("state={} capacity={} period={} period_begin={} period_end={} retained={}..{} "
                "dropped_overflow={} dropped_contention={} counters_saturated={} incomplete={}\n",
                name(snapshot.state, STATE_NAMES), snapshot.options.capacity, snapshot.period, snapshot.periodBegin,
                snapshot.periodEnd, snapshot.records.empty() ? 0 : snapshot.records.front().sequence,
                snapshot.records.empty() ? 0 : snapshot.records.back().sequence, snapshot.droppedOverflow,
                snapshot.droppedContention, snapshot.countersSaturated, snapshot.incomplete);

  for (const auto &record : snapshot.records) {
    const auto &context = record.context;
    const auto &node = record.stableNodeId;
    const bool queueEvent =
      record.event == DiagnosticEvent::QUEUE_ACCEPTED || record.event == DiagnosticEvent::QUEUE_DISPATCH ||
      record.event == DiagnosticEvent::QUEUE_REPLACED || record.event == DiagnosticEvent::QUEUE_CANCELLED ||
      record.event == DiagnosticEvent::QUEUE_REJECTED || record.event == DiagnosticEvent::QUEUE_RETURNED;

    text += std::format(
      "seq={} correlationId={} parentCorrelationId={} period={} publicationId={} "
      "relatedPublicationId={} frameId={} stableNodeId={}:{}:{} event={} reason={} input={} action={} "
      "queue={} incomplete={}\n",
      record.sequence, context.correlationId, context.parentCorrelationId, context.period, context.publicationId,
      record.relatedPublicationId, context.frameId, node.session, node.slot, node.generation,
      name(record.event, EVENT_NAMES), name(record.reason, REASON_NAMES), name(record.input, INPUT_NAMES),
      record.semanticAction, queueEvent ? (record.controlQueue ? "control" : "data") : "none", record.incomplete);
  }

  return text;
}

std::expected<void, DiagnosticError> validateDiagnosticVersion(std::string_view text) noexcept {
  if (!text.starts_with("PORTALS-DIAGNOSTICS 1\n")) {
    return std::unexpected(DiagnosticError::UNSUPPORTED_VERSION);
  }

  return {};
}
} // namespace portals

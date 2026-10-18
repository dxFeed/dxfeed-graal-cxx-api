// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include <portals/scheduler.hpp>

#include <algorithm>
#include <condition_variable>
#include <limits>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <thread>
#include <vector>

namespace portals {
namespace {
void increment(std::uint64_t &value, std::uint64_t amount = 1) {
  value += std::min(amount, std::numeric_limits<std::uint64_t>::max() - value);
}

SchedulerTime steadyTime(void *) noexcept {
  return static_cast<SchedulerTime>(
    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}
} // namespace

namespace detail {
struct SchedulerRegistration {
  std::uint64_t id{};
  std::uint64_t scope{};

  std::function<void(std::shared_ptr<const void>)> callback;

  SchedulerLane lane{};
  SchedulerOverflow overflow{};

  bool timer{};

  SchedulerTime deadline{};

  bool active{true};
  DiagnosticContext diagnostic{};
};

struct SchedulerPublication {
  std::uint64_t id{};
  std::shared_ptr<const void> value;
  SchedulerTime queued_at{};
  DiagnosticContext diagnostic{};
};

struct SchedulerQueue {
  std::vector<SchedulerPublication> values;
  SchedulerQueueMetrics metrics;
  std::size_t capacity{};
};

struct SchedulerState {
  explicit SchedulerState(SchedulerOptions limits, SchedulerClock service, Scheduler::ErrorSink error_sink,
                          void *context)
      : options(limits), clock(service), sink(error_sink), sink_context(context) {
    if (!clock.read) {
      clock.read = steadyTime;
    }

    data.capacity = options.data_capacity;
    control.capacity = options.control_capacity;
    data.values.reserve(data.capacity);
    control.values.reserve(control.capacity);
    registrations.reserve(options.subscription_capacity);
  }

  bool onOwner() const {
    return owner == std::this_thread::get_id();
  }

  SchedulerTime now() const {
    return clock.read(clock.context);
  }

  auto find(std::uint64_t id) {
    return std::ranges::find_if(registrations, [id](const auto &entry) {
      return entry->id == id;
    });
  }

  std::expected<std::uint64_t, SchedulerError> nextIdentity() {
    if (next_id == std::numeric_limits<std::uint64_t>::max()) {
      return std::unexpected(SchedulerError::IDENTITY_EXHAUSTED);
    }

    return ++next_id;
  }

  void tracePublication(const SchedulerPublication &publication, DiagnosticEvent event, DiagnosticReason reason,
                        bool controlLane = false) noexcept {
    if (options.diagnostics && options.diagnostics->enabled()) {
      options.diagnostics->record(
        {.context = publication.diagnostic, .event = event, .reason = reason, .controlQueue = controlLane});
    }
  }

  void discard(SchedulerQueue &queue, std::uint64_t id) {
    for (;;) {
      SchedulerPublication removed;

      {
        std::lock_guard lock(mutex);
        auto found = std::ranges::find_if(queue.values, [id](const auto &entry) {
          return entry.id == id;
        });

        if (found == queue.values.end()) {
          return;
        }

        removed = std::move(*found);
        queue.values.erase(found);
        increment(queue.metrics.cancelled);
        queue.metrics.pending = queue.values.size();
      }

      tracePublication(removed, DiagnosticEvent::QUEUE_CANCELLED, DiagnosticReason::CANCELLED, &queue == &control);

      // User-supplied snapshot deleters are never called while holding the lock.
    }
  }

  void disconnect(std::uint64_t id) {
    std::shared_ptr<SchedulerRegistration> removed;

    {
      std::lock_guard lock(mutex);
      auto found = find(id);

      if (found == registrations.end()) {
        return;
      }

      removed = std::move(*found);
      registrations.erase(found);
    }

    if (removed->timer && options.diagnostics && options.diagnostics->enabled()) {
      options.diagnostics->record({.context = removed->diagnostic,
                                   .event = DiagnosticEvent::QUEUE_CANCELLED,
                                   .reason = DiagnosticReason::CANCELLED});
    }

    discard(data, id);
    discard(control, id);

    // The dispatcher may retain removed until its active invocation returns.
  }

  void closeScope(std::uint64_t id) {
    // Disable the entire scope before releasing any user storage. A destructor may reenter
    // publication while individual registrations are being reclaimed, but cannot revive delivery.
    {
      std::lock_guard lock(mutex);

      for (const auto &entry : registrations) {
        if (entry->scope == id) {
          entry->active = false;
        }
      }
    }

    for (;;) {
      std::uint64_t target{};

      {
        std::lock_guard lock(mutex);
        auto found = std::ranges::find_if(registrations, [id](const auto &entry) {
          return entry->scope == id;
        });

        if (found == registrations.end()) {
          return;
        }

        target = (*found)->id;
      }

      disconnect(target);
    }
  }

  void reclaim() {
    // Detach all owners under the mutex, then destroy them on UI after unlock. Snapshot
    // deleters and callback captures can reenter without observing half-cleared queue bookkeeping.
    std::vector<std::shared_ptr<SchedulerRegistration>> removed;
    std::vector<SchedulerPublication> old_data;
    std::vector<SchedulerPublication> old_control;

    {
      std::lock_guard lock(mutex);

      removed.swap(registrations);
      old_data.swap(data.values);
      old_control.swap(control.values);
      increment(data.metrics.cancelled, old_data.size());
      increment(control.metrics.cancelled, old_control.size());
      data.metrics.pending = control.metrics.pending = 0;
    }

    if (options.diagnostics && options.diagnostics->enabled()) {
      for (const auto &registration : removed) {
        if (registration->timer) {
          options.diagnostics->record({.context = registration->diagnostic,
                                       .event = DiagnosticEvent::QUEUE_CANCELLED,
                                       .reason = DiagnosticReason::CANCELLED});
        }
      }
    }

    for (const auto &publication : old_data) {
      tracePublication(publication, DiagnosticEvent::QUEUE_CANCELLED, DiagnosticReason::CANCELLED);
    }

    for (const auto &publication : old_control) {
      tracePublication(publication, DiagnosticEvent::QUEUE_CANCELLED, DiagnosticReason::CANCELLED, true);
    }
  }

  std::optional<SchedulerTime> nextTimer() const {
    std::optional<SchedulerTime> result;

    for (const auto &entry : registrations) {
      if (entry->active && entry->timer && (!result || entry->deadline < *result)) {
        result = entry->deadline;
      }
    }

    return result;
  }

  SchedulerOptions options;
  SchedulerClock clock;
  Scheduler::ErrorSink sink;

  void *sink_context;

  const std::thread::id owner{std::this_thread::get_id()};

  mutable std::mutex mutex;

  std::condition_variable condition;
  SchedulerQueue data;
  SchedulerQueue control;
  std::vector<std::shared_ptr<SchedulerRegistration>> registrations;
  SchedulerMetrics totals;
  std::uint64_t next_id{};

  bool stopped{};
  bool wake_pending{};
  bool dispatching{}; // UI-only; never read by producers.
};

std::expected<PublishResult, SchedulerError> schedulerPublish(const std::shared_ptr<SchedulerState> &state,
                                                              std::uint64_t id, std::shared_ptr<const void> value) {
  if (!state) {
    return std::unexpected(SchedulerError::CLOSED);
  }

  DiagnosticContext diagnostic;
  auto *recorder = state->options.diagnostics.get();

  if (recorder && recorder->enabled()) {
    diagnostic = recorder->current();
    diagnostic.publicationId = recorder->nextIdentity();
  }

  // value also holds any displaced snapshot until after unlock.
  std::unique_lock lock(state->mutex);

  if (state->stopped) {
    if (recorder && recorder->enabled()) {
      recorder->record(
        {.context = diagnostic, .event = DiagnosticEvent::QUEUE_REJECTED, .reason = DiagnosticReason::CANCELLED});
    }

    return std::unexpected(SchedulerError::CLOSED);
  }

  auto found = state->find(id);

  if (found == state->registrations.end() || !(*found)->active) {
    if (recorder && recorder->enabled()) {
      recorder->record(
        {.context = diagnostic, .event = DiagnosticEvent::QUEUE_REJECTED, .reason = DiagnosticReason::CANCELLED});
    }

    return std::unexpected(SchedulerError::CLOSED);
  }

  if (!value) {
    return std::unexpected(SchedulerError::INVALID_ARGUMENT);
  }

  const auto &registration = **found;
  auto &queue = registration.lane == SchedulerLane::CONTROL ? state->control : state->data;
  PublishResult result = PublishResult::QUEUED;
  auto pending = queue.values.end();

  if (registration.overflow == SchedulerOverflow::REPLACE_LATEST) {
    pending = std::ranges::find_if(queue.values, [id](const auto &entry) {
      return entry.id == id;
    });
  }

  // Replacement preserves this sender's FIFO position, including when the queue is full;
  // only its latency origin changes to the newly accepted snapshot.
  if (pending != queue.values.end()) {
    if (recorder && recorder->enabled()) {
      recorder->record({.context = pending->diagnostic,
                        .event = DiagnosticEvent::QUEUE_CANCELLED,
                        .reason = DiagnosticReason::REPLACED});
      recorder->record({.context = diagnostic,
                        .relatedPublicationId = pending->diagnostic.publicationId,
                        .event = DiagnosticEvent::QUEUE_REPLACED,
                        .reason = DiagnosticReason::REPLACED});
    }

    pending->diagnostic = diagnostic;
    pending->value.swap(value);
    pending->queued_at = state->now();
    increment(queue.metrics.replaced);
    result = PublishResult::REPLACED;
  } else {
    if (queue.values.size() == queue.capacity) {
      increment(queue.metrics.rejected);

      if (recorder && recorder->enabled()) {
        recorder->record({.context = diagnostic,
                          .event = DiagnosticEvent::QUEUE_REJECTED,
                          .reason = DiagnosticReason::QUEUE_FULL,
                          .controlQueue = registration.lane == SchedulerLane::CONTROL});
      }

      return std::unexpected(SchedulerError::WOULD_BLOCK);
    }

    queue.values.push_back({id, std::move(value), state->now(), diagnostic});
    queue.metrics.pending = queue.values.size();
    queue.metrics.high_water = std::max(queue.metrics.high_water, queue.metrics.pending);
  }

  if (recorder && recorder->enabled()) {
    recorder->record({.context = diagnostic,
                      .event = DiagnosticEvent::QUEUE_ACCEPTED,
                      .controlQueue = registration.lane == SchedulerLane::CONTROL});
  }

  increment(queue.metrics.accepted);
  lock.unlock();
  state->condition.notify_one();

  return result;
}

void schedulerWake(const std::shared_ptr<SchedulerState> &state) noexcept {
  if (state) {
    {
      std::lock_guard lock(state->mutex);

      state->wake_pending = true;
    }

    state->condition.notify_one();
  }
}

void schedulerRequestStop(const std::shared_ptr<SchedulerState> &state) noexcept {
  if (state) {
    {
      std::lock_guard lock(state->mutex);

      state->stopped = true;
    }

    state->condition.notify_one();
  }
}
} // namespace detail

Subscription::Subscription(std::shared_ptr<detail::SchedulerState> state, std::uint64_t id)
    : state_(std::move(state)), id_(id) {
}

Subscription::~Subscription() {
  if (!disconnect()) {
    std::terminate();
  }
}

Subscription::Subscription(Subscription &&other) noexcept
    : state_(std::move(other.state_)), id_(std::exchange(other.id_, 0)) {
}

Subscription &Subscription::operator=(Subscription &&other) noexcept {
  if (this != &other) {
    if (!disconnect()) {
      std::terminate();
    }

    state_ = std::move(other.state_);
    id_ = std::exchange(other.id_, 0);
  }

  return *this;
}

std::expected<void, SchedulerError> Subscription::disconnect() {
  if (!state_ || !id_) {
    return {};
  }

  if (!state_->onOwner()) {
    return std::unexpected(SchedulerError::WRONG_THREAD);
  }

  state_->disconnect(std::exchange(id_, 0));

  return {};
}

UiScope::UiScope(std::shared_ptr<detail::SchedulerState> state, std::uint64_t id) : state_(std::move(state)), id_(id) {
}

UiScope::~UiScope() {
  if (!close()) {
    std::terminate();
  }
}

UiScope::UiScope(UiScope &&other) noexcept : state_(std::move(other.state_)), id_(std::exchange(other.id_, 0)) {
}

UiScope &UiScope::operator=(UiScope &&other) noexcept {
  if (this != &other) {
    if (!close()) {
      std::terminate();
    }

    state_ = std::move(other.state_);
    id_ = std::exchange(other.id_, 0);
  }

  return *this;
}

std::expected<void, SchedulerError> UiScope::close() {
  if (!state_ || !id_) {
    return {};
  }

  if (!state_->onOwner()) {
    return std::unexpected(SchedulerError::WRONG_THREAD);
  }

  state_->closeScope(std::exchange(id_, 0));

  return {};
}

Scheduler::Scheduler(std::shared_ptr<detail::SchedulerState> state) : state_(std::move(state)) {
}

Scheduler::~Scheduler() {
  if (!state_->onOwner() || state_->dispatching) {
    std::terminate();
  }

  (void)stop();
}

std::expected<std::unique_ptr<Scheduler>, SchedulerError>
Scheduler::create(SchedulerOptions options, SchedulerClock clock, ErrorSink sink, void *sink_context) {
  if (!options.data_capacity || !options.control_capacity || !options.data_budget || !options.control_budget ||
      !options.timer_budget || !options.subscription_capacity) {
    return std::unexpected(SchedulerError::INVALID_ARGUMENT);
  }

  try {
    auto state = std::make_shared<detail::SchedulerState>(options, clock, sink, sink_context);

    return std::unique_ptr<Scheduler>(new Scheduler(std::move(state)));
  } catch (const std::bad_alloc &) {
    return std::unexpected(SchedulerError::NO_MEMORY);
  } catch (const std::length_error &) {
    return std::unexpected(SchedulerError::INVALID_ARGUMENT);
  }
}

std::expected<UiScope, SchedulerError> Scheduler::makeScope() {
  if (!state_->onOwner()) {
    return std::unexpected(SchedulerError::WRONG_THREAD);
  }

  std::lock_guard lock(state_->mutex);

  if (state_->stopped) {
    return std::unexpected(SchedulerError::CLOSED);
  }

  auto id = state_->nextIdentity();

  if (!id) {
    return std::unexpected(id.error());
  }

  return UiScope(state_, *id);
}

std::expected<std::uint64_t, SchedulerError> Scheduler::add(const UiScope &scope,
                                                            std::function<void(std::shared_ptr<const void>)> callback,
                                                            SchedulerLane lane, SchedulerOverflow overflow, bool timer,
                                                            SchedulerTime deadline) {
  if (!state_->onOwner()) {
    return std::unexpected(SchedulerError::WRONG_THREAD);
  }

  if (scope.state_ != state_) {
    return std::unexpected(SchedulerError::WRONG_SCHEDULER);
  }

  if (!scope.id_) {
    return std::unexpected(SchedulerError::CLOSED);
  }

  if ((lane != SchedulerLane::DATA && lane != SchedulerLane::CONTROL) ||
      (overflow != SchedulerOverflow::REJECT && overflow != SchedulerOverflow::REPLACE_LATEST) ||
      (lane == SchedulerLane::CONTROL && overflow == SchedulerOverflow::REPLACE_LATEST)) {
    return std::unexpected(SchedulerError::INVALID_ARGUMENT);
  }

  auto registration = std::make_shared<detail::SchedulerRegistration>();

  registration->callback = std::move(callback);
  registration->scope = scope.id_;
  registration->lane = lane;
  registration->overflow = overflow;
  registration->timer = timer;
  registration->deadline = deadline;

  if (timer && state_->options.diagnostics && state_->options.diagnostics->enabled()) {
    registration->diagnostic = state_->options.diagnostics->current();
    registration->diagnostic.publicationId = state_->options.diagnostics->nextIdentity();
  }

  std::lock_guard lock(state_->mutex);

  if (state_->stopped) {
    return std::unexpected(SchedulerError::CLOSED);
  }

  if (state_->registrations.size() == state_->options.subscription_capacity) {
    return std::unexpected(SchedulerError::WOULD_BLOCK);
  }

  auto id = state_->nextIdentity();

  if (!id) {
    return std::unexpected(id.error());
  }

  registration->id = *id;

  if (timer && state_->options.diagnostics && state_->options.diagnostics->enabled()) {
    state_->options.diagnostics->record(
      {.context = registration->diagnostic, .event = DiagnosticEvent::QUEUE_ACCEPTED});
  }
  state_->registrations.push_back(std::move(registration));

  return *id;
}

std::expected<SchedulerDispatch, SchedulerError> Scheduler::dispatch() {
  if (!state_->onOwner()) {
    return std::unexpected(SchedulerError::WRONG_THREAD);
  }

  if (state_->dispatching) {
    return std::unexpected(SchedulerError::REENTRANT_DISPATCH);
  }

  state_->dispatching = true;

  struct DispatchGuard {
    bool &active;

    ~DispatchGuard() {
      active = false;
    }
  } guard{state_->dispatching};

  SchedulerDispatch result;
  auto invoke = [&](const std::shared_ptr<detail::SchedulerRegistration> &registration,
                    std::shared_ptr<const void> value) {
    try {
      registration->callback(std::move(value));

      if (auto *recorder = diagnostics(); recorder && recorder->enabled()) {
        recorder->record({.context = recorder->current(),
                          .event = DiagnosticEvent::QUEUE_RETURNED,
                          .reason = DiagnosticReason::OUTCOME_UNKNOWN,
                          .controlQueue = registration->lane == SchedulerLane::CONTROL});
      }
    } catch (...) {
      if (auto *recorder = diagnostics(); recorder && recorder->enabled()) {
        recorder->note(DiagnosticEvent::HANDLER_RETURNED, DiagnosticReason::HANDLER_FAILED);
        recorder->record({.context = recorder->current(),
                          .event = DiagnosticEvent::QUEUE_RETURNED,
                          .reason = DiagnosticReason::HANDLER_FAILED,
                          .controlQueue = registration->lane == SchedulerLane::CONTROL});
      }

      state_->disconnect(registration->id);
      ++result.callback_failures;
      {
        std::lock_guard lock(state_->mutex);

        increment(state_->totals.callback_failures);
      }

      if (state_->sink) {
        state_->sink(state_->sink_context, SchedulerError::CALLBACK_FAILED);
      }
    }
  };
  // Selecting a strong registration under the mutex is callback admission. Invocation and
  // owner release happen outside it, so self-disconnect/stop cannot destroy active captures.
  auto drain = [&](detail::SchedulerQueue &queue, std::size_t budget, std::size_t &count) {
    for (std::size_t step = 0; step < budget; ++step) {
      detail::SchedulerPublication publication;

      std::shared_ptr<detail::SchedulerRegistration> registration;

      {
        std::lock_guard lock(state_->mutex);

        if (state_->stopped || queue.values.empty()) {
          break;
        }

        publication = std::move(queue.values.front());
        queue.values.erase(queue.values.begin());
        queue.metrics.pending = queue.values.size();

        auto found = state_->find(publication.id);

        if (found != state_->registrations.end() && (*found)->active) {
          registration = *found;

          const auto now = state_->now();

          queue.metrics.last_latency = now >= publication.queued_at ? now - publication.queued_at : 0;
          queue.metrics.max_latency = std::max(queue.metrics.max_latency, queue.metrics.last_latency);
          increment(queue.metrics.total_latency, queue.metrics.last_latency);
          increment(queue.metrics.delivered);
        } else {
          increment(queue.metrics.cancelled);
        }
      }

      if (registration) {
        ++count;
        DiagnosticScope context(diagnostics(), publication.diagnostic);

        state_->tracePublication(publication, DiagnosticEvent::QUEUE_DISPATCH, DiagnosticReason::NONE,
                                 registration->lane == SchedulerLane::CONTROL);
        invoke(registration, std::move(publication.value));
      } else {
        state_->tracePublication(publication, DiagnosticEvent::QUEUE_CANCELLED, DiagnosticReason::CANCELLED,
                                 &queue == &state_->control);
      }
    }
  };
  drain(state_->control, state_->options.control_budget, result.control);

  // The registration vector preserves insertion order. Strict deadline comparison therefore
  // selects the earliest timer and breaks equal-deadline ties by registration order. Removing
  // it before invocation makes the timer one-shot even when its callback reenters registration.
  for (std::size_t step = 0; step < state_->options.timer_budget; ++step) {
    std::shared_ptr<detail::SchedulerRegistration> registration;

    {
      std::lock_guard lock(state_->mutex);

      if (state_->stopped) {
        break;
      }

      const auto now = state_->now();
      auto earliest = state_->registrations.end();

      for (auto it = state_->registrations.begin(); it != state_->registrations.end(); ++it) {
        if ((*it)->active && (*it)->timer && (*it)->deadline <= now &&
            (earliest == state_->registrations.end() || (*it)->deadline < (*earliest)->deadline)) {
          earliest = it;
        }
      }

      if (earliest == state_->registrations.end()) {
        break;
      }

      registration = std::move(*earliest);
      state_->registrations.erase(earliest);
      increment(state_->totals.timer_callbacks);
    }

    ++result.timers;
    DiagnosticScope context(diagnostics(), registration->diagnostic);

    if (auto *recorder = diagnostics(); recorder && recorder->enabled()) {
      recorder->note(DiagnosticEvent::QUEUE_DISPATCH);
    }

    invoke(registration, {});
  }

  drain(state_->data, state_->options.data_budget, result.data);
  {
    std::lock_guard lock(state_->mutex);

    increment(state_->totals.cycles);
    result.stopped = state_->stopped;
  }

  if (result.stopped) {
    state_->reclaim();
  }

  return result;
}

std::expected<SchedulerWake, SchedulerError> Scheduler::waitFor(std::chrono::milliseconds timeout) {
  if (!state_->onOwner()) {
    return std::unexpected(SchedulerError::WRONG_THREAD);
  }

  if (state_->dispatching) {
    return std::unexpected(SchedulerError::REENTRANT_DISPATCH);
  }

  if (timeout.count() < 0) {
    return std::unexpected(SchedulerError::INVALID_ARGUMENT);
  }

  // Clamp before addition to avoid overflowing the native steady-clock deadline.
  const auto start = std::chrono::steady_clock::now();
  const auto remaining =
    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::time_point::max() - start);

  const auto deadline = start + std::min(timeout, remaining);
  std::unique_lock lock(state_->mutex);

  for (;;) {
    if (state_->stopped) {
      return SchedulerWake::STOPPED;
    }

    const auto next = state_->nextTimer();
    const auto now = state_->now();

    if (state_->wake_pending || !state_->control.values.empty() || !state_->data.values.empty() ||
        (next && *next <= now)) {
      state_->wake_pending = false;

      return SchedulerWake::READY;
    }

    auto native_now = std::chrono::steady_clock::now();

    if (native_now >= deadline) {
      return SchedulerWake::TIMEOUT;
    }

    // The injected clock determines readiness; the native steady clock bounds real waiting.
    // Recheck both after wakeups, including spurious wakes and explicit fake-clock advances.
    auto until = deadline;

    if (next) {
      auto maximum = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - native_now).count();

      if (*next - now <= static_cast<SchedulerTime>(maximum)) {
        until = native_now + std::chrono::milliseconds(*next - now);
      }
    }

    state_->condition.wait_until(lock, until);
  }
}

void Scheduler::requestStop() noexcept {
  detail::schedulerRequestStop(state_);
}

std::expected<void, SchedulerError> Scheduler::stop() {
  if (!state_->onOwner()) {
    return std::unexpected(SchedulerError::WRONG_THREAD);
  }

  requestStop();
  state_->reclaim();

  return {};
}

Diagnostics *Scheduler::diagnostics() const noexcept {
  return state_->options.diagnostics.get();
}

SchedulerMetrics Scheduler::metrics() const {
  std::lock_guard lock(state_->mutex);
  auto result = state_->totals;

  result.data = state_->data.metrics;
  result.control = state_->control.metrics;

  return result;
}
} // namespace portals

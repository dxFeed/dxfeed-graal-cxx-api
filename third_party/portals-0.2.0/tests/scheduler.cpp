// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include <portals/scheduler.hpp>

#include <atomic>
#include <chrono>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {
int failures{};
using namespace portals;
using namespace std::chrono_literals;

void check(bool condition, const char *message) {
  if (!condition) {
    std::cerr << message << '\n';
    ++failures;
  }
}

void boundedFairness() {
  using namespace portals;
  auto scheduler =
    Scheduler::create({.data_capacity = 2, .control_capacity = 3, .data_budget = 1, .control_budget = 3});
  auto scope = (*scheduler)->makeScope();
  std::vector<int> controls;
  int data{};
  auto updates = (*scheduler)->connect<int>(*scope, [&](auto value) {
    data = *value;
  });
  auto input = (*scheduler)
                 ->connect<int>(
                   *scope,
                   [&](auto value) {
                     controls.push_back(*value);
                   },
                   SchedulerLane::CONTROL);

  check(updates->publisher.publish(std::make_shared<const int>(1)).has_value(), "first publication accepted");
  check(updates->publisher.publish(std::make_shared<const int>(2)).has_value(), "second publication accepted");
  check(updates->publisher.publish(std::make_shared<const int>(3)).error() == SchedulerError::WOULD_BLOCK,
        "full data queue rejects publication");

  for (int event : {10, 20, 30}) {
    check(input->publisher.publish(std::make_shared<const int>(event)).has_value(), "control has separate capacity");
  }

  auto cycle = (*scheduler)->dispatch();

  check(cycle && cycle->control == 3 && cycle->data == 1, "cycle respects independent budgets");
  check(controls == std::vector<int>({10, 20, 30}) && data == 1, "keyboard resize quit beat data flood");

  auto metrics = (*scheduler)->metrics();

  check(metrics.data.high_water == 2 && metrics.data.rejected == 1 && metrics.data.pending == 1,
        "queue metrics expose capacity and rejection");
}

struct ManualClock {
  SchedulerTime time{};

  SchedulerClock service() {
    return {[](void *context) noexcept {
              return static_cast<ManualClock *>(context)->time;
            },
            this};
  }
};

void sustainedFlood() {
  auto scheduler = Scheduler::create(
    {.data_capacity = 4, .control_capacity = 3, .data_budget = 2, .control_budget = 1, .timer_budget = 1});
  auto scope = (*scheduler)->makeScope();
  std::vector<int> seen;
  auto updates = (*scheduler)->connect<int>(*scope, [](auto) {
  });
  auto controls = (*scheduler)
                    ->connect<int>(
                      *scope,
                      [&](auto value) {
                        seen.push_back(*value);
                      },
                      SchedulerLane::CONTROL);
  std::vector<Subscription> timers;

  for (int event : {10, 20, 30}) {
    check(controls->publisher.publish(std::make_shared<const int>(event)).has_value(), "queue control event");
    timers.push_back(std::move(*(*scheduler)->at(*scope, 0, [] {
    })));
  }

  check(controls->publisher.publish(std::make_shared<const int>(40)).error() == SchedulerError::WOULD_BLOCK,
        "control overflow explicitly requests backpressure");

  for (int cycle = 0; cycle < 3; ++cycle) {
    for (int publication = 0; publication < 100; ++publication) {
      (void)updates->publisher.publish(std::make_shared<const int>(publication));
    }

    auto result = (*scheduler)->dispatch();

    check(result && result->control == 1 && result->timers == 1 && result->data == 2,
          "continuous flood cannot steal control or timer budget");
    check(seen.size() == static_cast<std::size_t>(cycle + 1), "FIFO control bound is ceil(capacity / budget) cycles");
  }

  check(seen == std::vector<int>({10, 20, 30}), "keyboard resize quit survive continuous flood in order");
  check((*scheduler)->metrics().data.high_water == 4, "continuous flood stays at configured capacity");
}

void replacementAndLatency() {
  ManualClock clock;
  auto scheduler = Scheduler::create({.data_capacity = 2}, clock.service());
  auto scope = (*scheduler)->makeScope();
  std::vector<int> values;
  auto first = (*scheduler)
                 ->connect<int>(
                   *scope,
                   [&](auto value) {
                     values.push_back(*value);
                   },
                   SchedulerLane::DATA, SchedulerOverflow::REPLACE_LATEST);
  auto second = (*scheduler)->connect<int>(*scope, [&](auto value) {
    values.push_back(*value);
  });

  check(first->publisher.publish(std::make_shared<const int>(1)) == PublishResult::QUEUED, "initial snapshot queued");
  clock.time = 3;
  check(second->publisher.publish(std::make_shared<const int>(2)) == PublishResult::QUEUED,
        "independent sender queued");
  clock.time = 5;
  check(first->publisher.publish(std::make_shared<const int>(3)) == PublishResult::REPLACED,
        "replace works at full capacity");
  clock.time = 10;
  check((*scheduler)->dispatch().has_value(), "dispatch coalesced values");
  check(values == std::vector<int>({3, 2}), "replacement stays in sender FIFO slot without replacing another sender");

  auto metrics = (*scheduler)->metrics().data;

  check(metrics.accepted == 3 && metrics.replaced == 1 && metrics.delivered == 2 && metrics.high_water == 2,
        "replacement counted separately from delivery");
  check(metrics.last_latency == 7 && metrics.max_latency == 7 && metrics.total_latency == 12,
        "latency starts at most recent replacement and uses injected clock");
  check(first->publisher.publish({}).error() == SchedulerError::INVALID_ARGUMENT, "null snapshot rejected");
  check((*scheduler)
            ->connect<int>(
              *scope,
              [](auto) {
              },
              SchedulerLane::CONTROL, SchedulerOverflow::REPLACE_LATEST)
            .error() == SchedulerError::INVALID_ARGUMENT,
        "control cannot silently coalesce domain events");
}

struct Capture {
  int &destroyed;
  std::thread::id &destroy_thread;

  ~Capture() {
    ++destroyed;
    destroy_thread = std::this_thread::get_id();
  }
};

void cancellationAndSelfClose() {
  auto scheduler = Scheduler::create();
  auto scope = (*scheduler)->makeScope();
  int destroyed{};
  int called{};
  std::thread::id destroyed_on;
  auto context = std::make_shared<Capture>(destroyed, destroyed_on);
  auto connection = (*scheduler)->connect<int>(*scope, [context, &scope, &destroyed, &called, &scheduler](auto) {
    ++called;
    check(scope->close().has_value(), "self-close succeeds without join");
    check(destroyed == 0, "active callback capture survives self-close");
    check((*scheduler)->dispatch().error() == SchedulerError::REENTRANT_DISPATCH, "nested dispatch rejected");
    check((*scheduler)->waitFor(0ms).error() == SchedulerError::REENTRANT_DISPATCH, "nested blocking wait rejected");
  });

  context.reset();
  check(connection->publisher.publish(std::make_shared<const int>(1)).has_value(), "first callback queued");
  check(connection->publisher.publish(std::make_shared<const int>(2)).has_value(), "late callback queued");
  check((*scheduler)->dispatch()->data == 1, "self-close prevents second queued invocation");
  check(called == 1 && destroyed == 1 && destroyed_on == std::this_thread::get_id(),
        "capture reclaimed on UI after invocation");
  check(connection->publisher.publish(std::make_shared<const int>(3)).error() == SchedulerError::CLOSED,
        "publication after self-close is closed");
  check(connection->subscription.disconnect().has_value() && scope->close().has_value(),
        "arbitrary repeated cancellation is safe");
  check((*scheduler)->metrics().data.cancelled == 1, "discarded queued callback counted");

  auto another_scope = (*scheduler)->makeScope();
  auto cancelled = (*scheduler)->connect<int>(*another_scope, [&](auto) {
    ++called;
  });
  auto survivor = (*scheduler)->connect<int>(*another_scope, [&](auto) {
    ++called;
  });

  (void)cancelled->publisher.publish(std::make_shared<const int>(1));
  (void)survivor->publisher.publish(std::make_shared<const int>(1));
  cancelled->subscription = {};
  check((*scheduler)->dispatch()->data == 1 && called == 2, "destroying one token preserves other subscription");
  *another_scope = {};
  check(survivor->publisher.publish(std::make_shared<const int>(1)).error() == SchedulerError::CLOSED,
        "scope destroyed before subscription suppresses late results");
}

void reentrantDeleters() {
  auto scheduler = Scheduler::create();
  auto scope = (*scheduler)->makeScope();
  int values{};
  auto destination = (*scheduler)->connect<int>(*scope, [&](auto value) {
    values += *value;
  });
  auto replacement = (*scheduler)
                       ->connect<int>(
                         *scope,
                         [](auto) {
                         },
                         SchedulerLane::DATA, SchedulerOverflow::REPLACE_LATEST);
  int deletes{};
  auto payload = std::shared_ptr<const int>(new int(1), [&](const int *value) {
    delete value;
    ++deletes;
    check(destination->publisher.publish(std::make_shared<const int>(7)).has_value(),
          "deleter can publish without locked mutex");
  });

  (void)replacement->publisher.publish(std::move(payload));
  (void)replacement->publisher.publish(std::make_shared<const int>(2));
  check(deletes == 1, "replace releases old snapshot outside mutex");
  (void)(*scheduler)->dispatch();
  check(values == 7, "reentrant publication reaches UI");

  auto closing_scope = (*scheduler)->makeScope();
  int forbidden{};
  auto context = std::shared_ptr<int>(new int(0), [&](int *value) {
    delete value;
    check((*scheduler)->dispatch().has_value(), "capture destructor may dispatch outside an active dispatch");
  });
  auto first = (*scheduler)->connect<int>(*closing_scope, [context](auto) {
  });
  auto second = (*scheduler)->connect<int>(*closing_scope, [&](auto) {
    ++forbidden;
  });

  context.reset();
  (void)second->publisher.publish(std::make_shared<const int>(1));
  check(closing_scope->close().has_value(), "close whole scope with reentrant capture destruction");
  check(forbidden == 0, "all scope registrations disconnect before any capture is reclaimed");
  check((*scheduler)->metrics().data.cancelled == 1,
        "reentrant dispatch counts inactive queued publication as cancelled");

  auto stopping_payload = std::shared_ptr<const int>(new int(1), [&](const int *value) {
    delete value;
    check(destination->publisher.publish(std::make_shared<const int>(1)).error() == SchedulerError::CLOSED,
          "stop releases payload outside lock into closed channel");
  });

  (void)replacement->publisher.publish(std::move(stopping_payload));
  check((*scheduler)->stop().has_value(), "stop permits reentrant snapshot deleter");
}

void timersAndStop() {
  ManualClock clock;
  auto scheduler = Scheduler::create({.timer_budget = 1}, clock.service());
  auto scope = (*scheduler)->makeScope();
  std::vector<int> fired;
  auto later = (*scheduler)->at(*scope, 20, [&] {
    fired.push_back(20);
  });
  auto earlier = (*scheduler)->at(*scope, 10, [&] {
    fired.push_back(10);
  });
  auto cancelled = (*scheduler)->at(*scope, 5, [&] {
    fired.push_back(5);
  });

  check(cancelled->disconnect().has_value(), "timer cancelled before deadline");
  clock.time = 9;
  check((*scheduler)->dispatch()->timers == 0 && (*scheduler)->waitFor(0ms) == SchedulerWake::TIMEOUT,
        "timer waits until deadline without sleeps");
  clock.time = 20;
  check((*scheduler)->waitFor(0ms) == SchedulerWake::READY, "due timer makes scheduler ready");
  check((*scheduler)->dispatch()->timers == 1 && fired == std::vector<int>({10}),
        "earliest due timer first within budget");
  check((*scheduler)->dispatch()->timers == 1 && fired == std::vector<int>({10, 20}),
        "debt preserved for next dispatch");
  check((*scheduler)->dispatch()->timers == 0, "one-shot timer cannot fire twice");

  auto stopping = (*scheduler)->at(*scope, 20, [&] {
    check((*scheduler)->stop().has_value(), "self-stop succeeds");
  });
  auto suppressed = (*scheduler)->at(*scope, 20, [&] {
    fired.push_back(99);
  });

  check((*scheduler)->dispatch()->stopped && fired.size() == 2, "stop prevents following timer");
  check((*scheduler)->waitFor(0ms) == SchedulerWake::STOPPED, "stopped wait returns immediately");
  check((*scheduler)->metrics().timer_callbacks == 3, "timer invocation metrics are observable");
}

struct AllocationFailure {
  AllocationFailure() = default;

  AllocationFailure(const AllocationFailure &) {
    throw std::bad_alloc();
  }

  void operator()(std::shared_ptr<const int>) const {
  }
};

void errorsAndAffinity() {
  check(Scheduler::create({.data_capacity = 0}).error() == SchedulerError::INVALID_ARGUMENT, "zero capacity rejected");
  check(Scheduler::create({.data_capacity = std::numeric_limits<std::size_t>::max()}).error() ==
          SchedulerError::INVALID_ARGUMENT,
        "impossible vector capacity rejected without overflow");

  int failures_reported{};
  auto scheduler = Scheduler::create(
    {.subscription_capacity = 1}, {},
    [](void *context, SchedulerError error) noexcept {
      if (error == SchedulerError::CALLBACK_FAILED) {
        ++*static_cast<int *>(context);
      }
    },
    &failures_reported);
  auto scope = (*scheduler)->makeScope();
  AllocationFailure callable;

  check((*scheduler)->connect<int>(*scope, callable).error() == SchedulerError::NO_MEMORY,
        "callback ownership allocation failure translated without registration");

  int calls{};
  auto throwing = (*scheduler)->connect<int>(*scope, [&](auto) {
    ++calls;
    throw std::runtime_error("application callback");
  });

  check((*scheduler)
            ->at(*scope, 0,
                 [] {
                 })
            .error() == SchedulerError::WOULD_BLOCK,
        "timers share bounded registration capacity");
  (void)throwing->publisher.publish(std::make_shared<const int>(1));
  (void)throwing->publisher.publish(std::make_shared<const int>(2));

  auto result = (*scheduler)->dispatch();

  check(result->callback_failures == 1 && failures_reported == 1 && calls == 1,
        "exception reports once and disables queued callbacks");

  auto recovered = (*scheduler)->connect<int>(*scope, [](auto) {
  });

  check(recovered.has_value(), "failed callback releases registration capacity");
  check(throwing->publisher.publish(std::make_shared<const int>(3)).error() == SchedulerError::CLOSED,
        "late publication cannot target a newly allocated registration");

  auto other = Scheduler::create();
  auto other_scope = (*other)->makeScope();

  check((*scheduler)
            ->connect<int>(*other_scope,
                           [](auto) {
                           })
            .error() == SchedulerError::WRONG_SCHEDULER,
        "foreign scope is not accepted");
  check((*scheduler)->waitFor(-1ms).error() == SchedulerError::INVALID_ARGUMENT, "negative wait rejected");

  bool foreign_rejected{};
  std::thread worker([&] {
    foreign_rejected = (*scheduler)->dispatch().error() == SchedulerError::WRONG_THREAD &&
                       (*scheduler)->makeScope().error() == SchedulerError::WRONG_THREAD &&
                       (*scheduler)->stop().error() == SchedulerError::WRONG_THREAD &&
                       (*scheduler)->waitFor(0ms).error() == SchedulerError::WRONG_THREAD &&
                       scope->close().error() == SchedulerError::WRONG_THREAD &&
                       recovered->subscription.disconnect().error() == SchedulerError::WRONG_THREAD;
  });

  worker.join();
  check(foreign_rejected, "UI mutations reject worker thread without changing lifetime");
}

void wakeAndProducerLifetime() {
  Publisher<int> late;
  int destroyed{};
  std::thread::id destroyed_on;

  {
    std::atomic<bool> waiting{};

    SchedulerClock clock{[](void *context) noexcept {
                           static_cast<std::atomic<bool> *>(context)->store(true, std::memory_order_release);

                           return static_cast<SchedulerTime>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                                               std::chrono::steady_clock::now().time_since_epoch())
                                                               .count());
                         },
                         &waiting};

    auto scheduler = Scheduler::create({.data_capacity = 1, .control_capacity = 1}, clock);
    auto scope = (*scheduler)->makeScope();
    auto capture = std::make_shared<Capture>(destroyed, destroyed_on);
    int value{};
    auto connection = (*scheduler)->connect<int>(*scope, [capture, &value](auto next) {
      value = *next;
    });

    capture.reset();
    late = connection->publisher;

    std::thread producer([sender = late, &waiting] {
      const auto deadline = std::chrono::steady_clock::now() + 1s;

      while (!waiting.load(std::memory_order_acquire) && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::yield();
      }

      (void)sender.publish(std::make_shared<const int>(42));
    });
    auto wake = (*scheduler)->waitFor(1000ms);

    producer.join();
    check(wake == SchedulerWake::READY, "worker publication wakes finite wait without lost notification");
    (void)(*scheduler)->dispatch();
    check(value == 42, "worker data applied only on UI dispatch");
    late.wake();
    check((*scheduler)->waitFor(0ms) == SchedulerWake::READY && (*scheduler)->waitFor(0ms) == SchedulerWake::TIMEOUT,
          "explicit wake is a one-shot signal without a publication");

    auto control = (*scheduler)
                     ->connect<int>(
                       *scope,
                       [](auto) {
                       },
                       SchedulerLane::CONTROL);

    (void)control->publisher.publish(std::make_shared<const int>(1));
    (void)late.publish(std::make_shared<const int>(1));

    std::thread stopper([sender = late] {
      sender.requestStop();
    });

    stopper.join();
    check((*scheduler)->waitFor(0ms) == SchedulerWake::STOPPED, "stop bypasses full control and data queues");

    auto stopped = (*scheduler)->dispatch();

    check(stopped->stopped && stopped->control == 0 && stopped->data == 0, "stop begins no queued callbacks");
    check(destroyed == 1 && destroyed_on == std::this_thread::get_id(), "worker stop reclaims callback only on UI");
  }

  check(late.publish(std::make_shared<const int>(99)).error() == SchedulerError::CLOSED,
        "producer surviving scheduler is safely closed");
  late.requestStop();
  late.wake();
}
} // namespace

int main() {
  boundedFairness();
  sustainedFlood();
  replacementAndLatency();
  cancellationAndSelfClose();
  reentrantDeleters();
  timersAndStop();
  errorsAndAffinity();
  wakeAndProducerLifetime();

  return failures == 0 ? 0 : 1;
}

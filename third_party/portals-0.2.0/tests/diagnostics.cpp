// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include <portals/diagnostics.hpp>
#include <portals/scheduler.hpp>

#include <array>
#include <barrier>
#include <cstdlib>
#include <new>
#include <stdexcept>
#include <string_view>
#include <thread>

namespace {
thread_local bool rejectAllocations{};
thread_local std::ptrdiff_t allocationsUntilFailure{-1};

void check(bool condition) {
  if (!condition) {
    throw std::runtime_error("diagnostics contract failed");
  }
}
} // namespace

void *operator new(std::size_t size) {
  if (!rejectAllocations && allocationsUntilFailure != 0) {
    if (allocationsUntilFailure > 0) {
      --allocationsUntilFailure;
    }

    if (void *result = std::malloc(size ? size : 1)) {
      return result;
    }
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

namespace {
void inFlightPublication() {
  using namespace portals;

  for (const bool failSecond : {false, true}) {
    auto recorder = std::make_shared<Diagnostics>();
    auto scheduler = Scheduler::create({.control_capacity = 2, .diagnostics = recorder}).value();
    auto owner = scheduler->makeScope().value();
    bool inFlightIncomplete{};
    std::uint64_t secondPublication{};
    auto first = scheduler
                   ->connect<int>(
                     owner,
                     [](auto) {
                     },
                     SchedulerLane::CONTROL)
                   .value();
    auto second = scheduler
                    ->connect<int>(
                      owner,
                      [&](auto) {
                        secondPublication = recorder->current().publicationId;
                        inFlightIncomplete = recorder->snapshot().incomplete;

                        if (failSecond) {
                          throw std::runtime_error("synthetic handler failure");
                        }
                      },
                      SchedulerLane::CONTROL)
                    .value();

    recorder->enable({64}).value();

    {
      DiagnosticScope action(recorder.get(), recorder->beginInput(DiagnosticInput::KEY));

      (void)first.publisher.publish(std::make_shared<const int>(1)).value();
      (void)second.publisher.publish(std::make_shared<const int>(2)).value();
    }

    const auto dispatched = scheduler->dispatch().value();
    const auto after = recorder->snapshot();

    check(inFlightIncomplete);
    check(!after.incomplete);
    check(dispatched.callback_failures == static_cast<std::size_t>(failSecond));

    bool terminalFound{};

    for (const auto &record : after.records) {
      if (record.context.publicationId == secondPublication && record.event == DiagnosticEvent::QUEUE_RETURNED) {
        terminalFound = true;
        check(record.reason == (failSecond ? DiagnosticReason::HANDLER_FAILED : DiagnosticReason::OUTCOME_UNKNOWN));
      }
    }

    check(terminalFound);
  }
}

void snapshotAllocationFailures() {
  using namespace portals;
  constexpr std::ptrdiff_t MAX_ALLOCATION_ATTEMPTS = 16;
  std::size_t observedFailures{};

  for (const bool populated : {false, true}) {
    Diagnostics recorder;

    recorder.enable({16}).value();

    if (populated) {
      DiagnosticScope scope(&recorder, recorder.beginInput(DiagnosticInput::KEY));

      recorder.note(DiagnosticEvent::COMPLETED);
    }

    const auto expected = exportDiagnostics(recorder.snapshot());
    bool completedSweep{};

    // Exercise every allocation, including Debug iterator proxies and the returned owner.
    // The failpoint stays active through return-value materialization, not just reserve().
    for (std::ptrdiff_t failAfter = 0; failAfter < MAX_ALLOCATION_ATTEMPTS; ++failAfter) {
      allocationsUntilFailure = failAfter;

      try {
        const auto snapshot = recorder.snapshot();

        allocationsUntilFailure = -1;
        check(exportDiagnostics(snapshot) == expected);
        completedSweep = true;
      } catch (const std::bad_alloc &) {
        allocationsUntilFailure = -1;
        ++observedFailures;
      }

      check(recorder.enabled());
      check(exportDiagnostics(recorder.snapshot()) == expected);

      if (completedSweep) {
        break;
      }
    }

    check(completedSweep);
  }

  check(observedFailures > 0);
}
} // namespace

int main(int argc, char **argv) {
  if (argc == 2 && std::string_view(argv[1]) == "--inflight") {
    inFlightPublication();

    return 0;
  }

  if (argc == 2 && std::string_view(argv[1]) == "--snapshot-allocation") {
    snapshotAllocationFailures();

    return 0;
  }

  using namespace portals;
  Diagnostics recorder;

  rejectAllocations = true;
  recorder.note(DiagnosticEvent::INPUT);
  check(!recorder.beginInput(DiagnosticInput::TEXT).correlationId);
  check(!recorder.nextIdentity());
  rejectAllocations = false;
  check(recorder.snapshot().records.empty());
  check(!recorder.enable({0}));
  check(recorder.enable({1}).has_value());

  const auto first = recorder.beginInput(DiagnosticInput::KEY);

  {
    DiagnosticScope scope(&recorder, first);

    rejectAllocations = true;
    recorder.note(DiagnosticEvent::PROPERTY, DiagnosticReason::VALUE_UNCHANGED);
    recorder.note(DiagnosticEvent::COMPLETED);
    rejectAllocations = false;
    check(scope.outcome() == DiagnosticReason::VALUE_UNCHANGED);
  }

  const auto overwritten = recorder.snapshot();

  check(overwritten.records.size() == 1 && overwritten.droppedOverflow == 2);
  check(overwritten.incomplete && overwritten.records.front().sequence == 3);
  check(recorder.current().correlationId == 0);
  recorder.disable();

  const auto stopped = recorder.snapshot();

  check(stopped.state == DiagnosticState::DISABLED && stopped.periodEnd == 3);
  check(recorder.enable({16}).has_value());

  {
    DiagnosticScope scope(&recorder, first);

    recorder.note(DiagnosticEvent::COMPLETED);
  }

  check(recorder.snapshot().incomplete);
  check(recorder.enable({16}).has_value());

  {
    DiagnosticScope staleParent(&recorder, first);
    DiagnosticScope newChild(&recorder, recorder.beginInput(DiagnosticInput::OTHER));

    recorder.note(DiagnosticEvent::COMPLETED);
  }

  check(recorder.snapshot().incomplete);
  check(recorder.enable({16}).has_value());

  const auto second = recorder.beginInput(DiagnosticInput::KEY);

  check(second.correlationId > first.correlationId && second.period != first.period);

  {
    DiagnosticScope parent(&recorder, second);
    const auto child = recorder.beginInput(DiagnosticInput::OTHER);

    check(child.parentCorrelationId == second.correlationId);

    {
      DiagnosticScope nested(&recorder, child);

      check(recorder.current().correlationId == child.correlationId);
      recorder.note(DiagnosticEvent::COMPLETED);
    }

    check(recorder.current().correlationId == second.correlationId);
    recorder.note(DiagnosticEvent::COMPLETED);
  }

  const auto complete = recorder.snapshot();
  const auto exported = exportDiagnostics(complete);

  check(!complete.incomplete);
  check(validateDiagnosticVersion(exported).has_value());
  check(!validateDiagnosticVersion("PORTALS-DIAGNOSTICS 999\n"));
  check(!validateDiagnosticVersion("PORTALS-TRACE 1\n"));
  check(recorder.enable({64}).has_value());

  constexpr std::size_t THREADS = 8;
  constexpr std::size_t WRITES = 2000;
  std::barrier ready(static_cast<std::ptrdiff_t>(THREADS + 1));
  std::array<std::thread, THREADS> workers;

  for (auto &worker : workers) {
    worker = std::thread([&] {
      ready.arrive_and_wait();

      for (std::size_t index = 0; index < WRITES; ++index) {
        recorder.record({.context = second, .event = DiagnosticEvent::PROPERTY});
      }
    });
  }

  ready.arrive_and_wait();

  for (std::size_t index = 0; index < 50; ++index) {
    check(recorder.snapshot().records.size() <= 64);
  }

  for (auto &worker : workers) {
    worker.join();
  }

  const auto concurrent = recorder.snapshot();

  check(concurrent.records.size() <= 64);
  check(concurrent.records.size() + concurrent.droppedOverflow + concurrent.droppedContention == THREADS * WRITES);
  check(concurrent.incomplete);

  auto timedRecorder = std::make_shared<Diagnostics>();
  SchedulerTime fakeNow{};
  auto scheduler = Scheduler::create({.diagnostics = timedRecorder}, {[](void *context) noexcept {
                                                                        return *static_cast<SchedulerTime *>(context);
                                                                      },
                                                                      &fakeNow})
                     .value();
  auto owner = scheduler->makeScope().value();
  Subscription timer;
  std::uint64_t expectedCorrelation{};
  int timerCalls{};

  check(timedRecorder->enable({32}).has_value());

  {
    const auto token = timedRecorder->beginInput(DiagnosticInput::OTHER);
    DiagnosticScope scope(timedRecorder.get(), token);

    expectedCorrelation = token.correlationId;
    timer = scheduler
              ->at(owner, 5,
                   [&] {
                     ++timerCalls;
                     check(timedRecorder->current().correlationId == expectedCorrelation);
                   })
              .value();
  }

  check(scheduler->dispatch().has_value() && timerCalls == 0);
  fakeNow = 5;
  check(scheduler->dispatch().has_value() && timerCalls == 1);
  check(!timedRecorder->snapshot().incomplete);

  {
    DiagnosticScope scope(timedRecorder.get(), timedRecorder->beginInput(DiagnosticInput::OTHER));
    auto cancelledTimer = scheduler
                            ->at(owner, 10,
                                 [&] {
                                   ++timerCalls;
                                 })
                            .value();

    check(cancelledTimer.disconnect().has_value());
  }

  fakeNow = 10;
  check(scheduler->dispatch().has_value() && timerCalls == 1);
  check(!timedRecorder->snapshot().incomplete);
}

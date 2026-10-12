// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include "../examples/gallery/gallery.hpp"
#include "support/test_terminal.hpp"
#include <algorithm>
#include <fstream>
#include <portals/diagnostics.hpp>
#include <source_location>

namespace {
void check(bool condition, std::source_location location = std::source_location::current()) {
  if (!condition) {
    throw std::runtime_error("diagnostics flow check failed at line " + std::to_string(location.line()));
  }
}
} // namespace

int main(int argc, char **argv) {
  using namespace portals;
  using gallery::take;
  auto diagnostics = std::make_shared<Diagnostics>();
  auto scheduler = take(Scheduler::create({.data_capacity = 1, .control_capacity = 1, .diagnostics = diagnostics}));
  auto tree = take(WidgetTree::create(*scheduler));
  auto root = take(tree->root());
  auto button = take(tree->emplace<Button>(root, gallery::text("Run")));
  auto router = take(EventRouter::create(*tree));
  auto model = take(Property<int>::create(*tree, button, 7, Invalidation::PAINT));
  int calls{};
  auto binding = take(bindButton(*router, button, [&] {
    ++calls;
    check(!take(model->set(7)).changed);
  }));
  take(router->setFocus(button));
  take(tree->consumeFrameRequest());
  test_support::FakeClock clock;
  test_support::TestTerminal terminal(clock, {8, 1});
  FrameWriter writer(terminal, {}, diagnostics);
  auto render = [&] {
    std::optional<CellBuffer> buffer;
    take(tree->frame([&] {
      auto viewport = take(CellRect::from({}, {8, 1}));
      take(tree->arrange(root, viewport, viewport));
      take(tree->arrange(button, viewport, viewport));
      std::vector<RenderCommand> commands;
      gallery::SurfaceSink sink(take(tree->inspect(button)).geometry, commands);
      take(tree->paint(button, sink));
      buffer.emplace(take(renderFullFrame({8, 1}, commands)));
    }));
    take(writer.submit(std::move(*buffer), tree->diagnosticFrame()));
    check(!writer.flush(TerminalDeadline::max()).error);
  };
  render();
  take(tree->consumeFrameRequest());

  auto before = terminal.snapshot();
  take(router->dispatch(KeyEvent{InputKey::ENTER}));
  check(calls == 1 && take(tree->consumeFrameRequest()) == Invalidation::NONE);
  check(diagnostics->snapshot().records.empty());
  calls = 0;
  take(diagnostics->enable({128}));
  auto input = take(scheduler->makeScope());
  auto channel = take(scheduler->connect<InputEvent>(
    input,
    [&](auto event) {
      take(router->dispatch(*event));
    },
    SchedulerLane::CONTROL));
  {
    DiagnosticScope scope(diagnostics.get(), diagnostics->beginInput(DiagnosticInput::KEY));
    take(channel.publisher.publish(std::make_shared<const InputEvent>(KeyEvent{InputKey::ENTER})));
  }
  take(scheduler->dispatch());
  check(take(tree->consumeFrameRequest()) == Invalidation::NONE);
  check(calls == 1);
  check(terminal.snapshot() == before);
  auto snapshot = diagnostics->snapshot();

  check(!snapshot.incomplete);
  auto exported = exportDiagnostics(snapshot);
  check(exported.find("value_unchanged") != std::string::npos);
  check(exported.find("frame_not_requested") != std::string::npos);
  check(exported.find("handler_returned") != std::string::npos);
  check(exported.find("queue_dispatch") != std::string::npos);

  if (argc == 2) {
    // This finite headless fixture owns no active native terminal. Applications export only
    // after restoring their terminal, and must observe open/write/close errors independently.
    std::ofstream file(argv[1], std::ios::binary | std::ios::trunc);

    file << exported;
    file.close();

    if (!file) {
      return 2;
    }
  }

  auto send = [&](InputEvent event = KeyEvent{InputKey::ENTER}) {
    DiagnosticScope scope(diagnostics.get(), diagnostics->beginInput(DiagnosticInput::KEY));

    return channel.publisher.publish(std::make_shared<const InputEvent>(std::move(event)));
  };
  auto has = [&](DiagnosticReason reason) {
    const auto trace = diagnostics->snapshot();

    return std::ranges::any_of(trace.records, [=](const auto &record) {
      return record.reason == reason;
    });
  };

  binding.disconnect();
  binding = take(bindButton(*router, button, [&] {
    ++calls;
    take(model->set(8));
    take(setControlText(*tree, button, gallery::text("Changed")));
  }));
  take(diagnostics->enable({128}));
  take(send());
  take(scheduler->dispatch());
  check(take(tree->consumeFrameRequest()) != Invalidation::NONE);

  const auto frame = tree->diagnosticFrame();

  check(frame.frameId && frame.correlationId);
  render();
  check(terminal.snapshot() != before);
  check(has(DiagnosticReason::VALUE_CHANGED) && has(DiagnosticReason::OUTPUT_ACCEPTED));
  check(!diagnostics->snapshot().incomplete);
  take(tree->consumeFrameRequest());

  // Two real input causes can share one frame without an unbounded list on the tree.
  binding.disconnect();
  binding = take(bindButton(*router, button, [&] {
    take(model->set(take(model->snapshot())->value + 1));
  }));
  take(diagnostics->enable({128}));
  take(send());
  take(scheduler->dispatch());
  take(send());
  take(scheduler->dispatch());
  take(tree->consumeFrameRequest());

  const auto mergedFrame = tree->diagnosticFrame().frameId;

  render();
  check(has(DiagnosticReason::DIFF_EMPTY));
  const auto merged = diagnostics->snapshot();
  std::vector<std::uint64_t> causes;

  for (const auto &record : merged.records) {
    if (record.event == DiagnosticEvent::FRAME_REQUESTED) {
      check(record.context.frameId == mergedFrame);
      causes.push_back(record.context.correlationId);
    }
  }

  check(causes.size() == 2 && causes.front() != causes.back() && !merged.incomplete);
  take(tree->consumeFrameRequest());

  // A handler with no property evidence has an unknown outcome even if no frame was requested.
  binding.disconnect();
  binding = take(bindButton(*router, button, [&] {
    ++calls;
  }));
  take(diagnostics->enable({128}));
  take(send());
  take(scheduler->dispatch());
  check(has(DiagnosticReason::OUTCOME_UNKNOWN));
  check(!has(DiagnosticReason::VALUE_UNCHANGED));
  take(send(PasteInputEvent{"PRIVATE_CLIPBOARD_123"}));
  take(scheduler->dispatch());
  check(has(DiagnosticReason::UNHANDLED));
  check(exportDiagnostics(diagnostics->snapshot()).find("PRIVATE_CLIPBOARD_123") == std::string::npos);
  check(exportDiagnostics(diagnostics->snapshot()).find("Changed") == std::string::npos);

  // Queue rejection is an observed outcome; admitting diagnostics cannot change queue metrics.
  take(diagnostics->enable({128}));
  take(send());
  const auto rejected = send();

  check(!rejected && rejected.error() == SchedulerError::WOULD_BLOCK);
  check(has(DiagnosticReason::QUEUE_FULL));
  take(scheduler->dispatch());
  check(scheduler->metrics().control.rejected == 1);

  take(diagnostics->enable({128}));
  take(router->invoke({}, {Action::ACTIVATE}));
  check(has(DiagnosticReason::NO_TARGET));
  take(tree->setEnabled(button, false));
  take(router->invoke(button, {Action::ACTIVATE}));
  check(has(DiagnosticReason::TARGET_DISABLED));
  take(tree->setEnabled(button, true));
  binding.disconnect();
  take(router->invoke(button, {Action::ACTIVATE}));
  check(has(DiagnosticReason::UNBOUND_ACTION));

  binding = take(bindButton(*router, button, [&] {
    take(model->set(99));
    throw std::runtime_error("PRIVATE_EXCEPTION_123");
  }));
  take(diagnostics->enable({128}));
  const auto failed = router->invoke(button, {Action::ACTIVATE});

  check(!failed && failed.error() == TreeError::CALLBACK_FAILED);
  check(take(model->snapshot())->value == 99);
  check(has(DiagnosticReason::HANDLER_FAILED) && has(DiagnosticReason::VALUE_CHANGED));
  check(exportDiagnostics(diagnostics->snapshot()).find("PRIVATE_EXCEPTION_123") == std::string::npos);

  // Replacement keeps a bounded direct link to the displaced publication.
  auto data = take(scheduler->connect<int>(
    input,
    [](auto) {
    },
    SchedulerLane::DATA, SchedulerOverflow::REPLACE_LATEST));
  take(diagnostics->enable({128}));
  {
    DiagnosticScope scope(diagnostics.get(), diagnostics->beginInput(DiagnosticInput::OTHER));

    take(data.publisher.publish(std::make_shared<const int>(1)));
    take(data.publisher.publish(std::make_shared<const int>(2)));
  }
  take(scheduler->dispatch());
  const auto replaced = diagnostics->snapshot();
  const auto replacement = std::ranges::find_if(replaced.records, [](const auto &record) {
    return record.event == DiagnosticEvent::QUEUE_REPLACED;
  });

  check(replacement != replaced.records.end() && replacement->relatedPublicationId);
  check(replacement->relatedPublicationId != replacement->context.publicationId);
  check(scheduler->metrics().data.replaced == 1);

  auto late = take(tree->connect<int>(button, [&](Button &, auto) {
    ++calls;
  }));
  take(diagnostics->enable({128}));
  const auto callsBeforeRemoval = calls;
  const auto oldId = button.id();
  {
    DiagnosticScope scope(diagnostics.get(), diagnostics->beginInput(DiagnosticInput::OTHER));

    take(late.publisher.publish(std::make_shared<const int>(3)));
  }
  take(tree->remove(button));
  auto newButton = take(tree->emplace<Button>(root, gallery::text("New")));

  check(newButton.id() != oldId);
  take(scheduler->dispatch());
  check(calls == callsBeforeRemoval && has(DiagnosticReason::CANCELLED));
  check(!router->invoke(button, {Action::ACTIVATE}));
  check(has(DiagnosticReason::STALE_TARGET));

  // A real transport failure occurs after an accepted frame, never as a fictitious rollback.
  test_support::TestTerminal broken(clock, {8, 1});
  FrameWriter brokenWriter(broken, {}, diagnostics);
  take(diagnostics->enable({128}));
  {
    DiagnosticScope scope(diagnostics.get(), diagnostics->beginInput(DiagnosticInput::OTHER));
    auto token = diagnostics->current();

    token.frameId = diagnostics->nextIdentity();
    take(brokenWriter.submit(take(renderFullFrame({8, 1}, {})), token));
    broken.failAfter(1);
    check(brokenWriter.flush(TerminalDeadline::max()).error.has_value());
  }
  check(has(DiagnosticReason::OUTPUT_FAILED));

  // A submit observed while disabled cannot inherit a previous frame's correlation on flush.
  test_support::TestTerminal resumedTerminal(clock, {8, 1});
  FrameWriter resumedWriter(resumedTerminal, {}, diagnostics);

  diagnostics->disable();
  take(resumedWriter.submit(take(renderFullFrame({8, 1}, {}))));
  take(diagnostics->enable({128}));
  check(!resumedWriter.flush(TerminalDeadline::max()).error);
  const auto resumed = diagnostics->snapshot();

  check(resumed.incomplete && resumed.records.size() == 1);
  check(resumed.records.front().context.correlationId == 0);

  // A publication started while disabled must not be reported as a complete recorded action.
  diagnostics->disable();
  take(channel.publisher.publish(std::make_shared<const InputEvent>(KeyEvent{InputKey::ENTER})));
  take(diagnostics->enable({128}));
  take(scheduler->dispatch());
  check(diagnostics->snapshot().incomplete);
}

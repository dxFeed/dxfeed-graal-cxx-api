// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include "../examples/gallery/common.hpp"
#include "../examples/replay/terminal.hpp"
#include "support/vt_oracle.hpp"
#include <cassert>
#include <portals/controls.hpp>
#include <source_location>

namespace {
using namespace portals;
using example::take;

void check(bool condition, std::source_location where = std::source_location::current()) {
  if (!condition) {
    throw std::runtime_error("application check line " + std::to_string(where.line()));
  }
}

class Scene : public ApplicationScene {
public:
  explicit Scene(Application &app) : label(take(app.tree().emplace<Label>(app.root(), example::text("initial")))) {
  }

  std::expected<bool, TreeError> input(Application &app, const InputEvent &event) override {
    events.push_back(event);

    if (stopOnInput) {
      app.requestStop();
    } else if (change) {
      take(setControlText(app.tree(), label, example::text("changed")));
    } else {
      take(app.tree().invalidate(label, Invalidation::PAINT));
    }

    return true;
  }

  std::expected<void, TreeError> prepare(Application &app, CellSize) override {
    if (throwPrepare) {
      throw std::runtime_error("hook");
    }

    if (prepareChanges && !events.empty()) {
      take(setControlText(app.tree(), label, example::text("prepared")));
    }

    return {};
  }

  void stop(Application &) override {
    stopped = true;
  }

  Node<Label> label;
  std::vector<InputEvent> events;
  bool change{}, prepareChanges{}, stopped{}, stopOnInput{}, throwPrepare{};
};

class OutputTerminal : public example::ReplayTerminal {
public:
  using ReplayTerminal::ReplayTerminal;
  std::size_t delays{}, calls{}, submitted{};
  bool fail{}, failClose{};

  std::expected<void, TerminalError> submit(std::string bytes) override {
    submitted = bytes.size();

    return ReplayTerminal::submit(std::move(bytes));
  }

  TerminalWrite flush(TerminalDeadline deadline, std::stop_token stop) override {
    ++calls;

    if (fail) {
      return {TerminalStatus::DEADLINE, 0, submitted, TerminalError{TerminalErrorKind::OUTPUT_UNCERTAIN, 995}};
    }

    if (calls <= delays) {
      return {TerminalStatus::DEADLINE, 0, submitted, {}};
    }

    return ReplayTerminal::flush(deadline, stop);
  }

  std::expected<void, TerminalError> close() noexcept override {
    (void)ReplayTerminal::close();

    if (failClose) {
      return std::unexpected(TerminalError{TerminalErrorKind::SYSTEM, 17});
    }

    return {};
  }
};

void idleAndUnchanged() {
  example::ReplayTerminal idle({}, 5000);

  auto app = take(Application::create({}, idle.clock()));
  Scene scene(*app);

  const auto result = take(app->run(scene, idle));
  check(result.frames == 1 && result.bytes && idle.closed && scene.stopped);
  example::ReplayTerminal unchanged({{100, "x", {}}}, 5000);

  auto second = take(Application::create({}, unchanged.clock()));
  Scene same(*second);

  const auto repeated = take(second->run(same, unchanged));
  check(repeated.frames == 2 && repeated.bytes == result.bytes && unchanged.output == idle.output);

  auto rerun = second->run(same, unchanged);
  check(!rerun && std::get<ApplicationFailure>(rerun.error().cause) == ApplicationFailure::ALREADY_RUN);

  auto invalidNative = take(Application::create({}, idle.clock()));
  ApplicationScene empty;
  check(!invalidNative->run(empty));
}

void queueAndDecoder() {
  ApplicationOptions options;
  options.scheduler.control_capacity = 1;
  options.scheduler.control_budget = 1;
  options.inputBudget = 2;
  example::ReplayTerminal terminal({{50, "abcdef\x1b", {}}, {150, "\xe7\x95", {}}, {150, "\x8c", {}}}, 250);

  auto app = take(Application::create(options, terminal.clock()));
  Scene scene(*app);

  const auto result = take(app->run(scene, terminal));
  check(result.inputs == 8 && scene.events.size() == 8);
  std::u32string text;

  for (const auto &event : scene.events) {
    if (const auto *value = std::get_if<TextInputEvent>(&event)) {
      text += value->scalar;
    }
  }

  check(text == U"abcdef\u754c");
  check(std::get<KeyEvent>(scene.events[6]).key == InputKey::ESCAPE);
  check(app->scheduler().metrics().control.high_water == 1 && app->scheduler().metrics().control.rejected > 0);
}

void nativeSemanticBatch() {
  struct SemanticTerminal : example::ReplayTerminal {
    SemanticTerminal() : ReplayTerminal({{50, "ready", CellSize{32, 4}}}, 200) {
    }

    TerminalRead read(TerminalDeadline deadline, std::stop_token stop) override {
      auto batch = ReplayTerminal::read(deadline, stop);

      if (!batch.bytes.empty()) {
        batch.bytes.clear();
        batch.events = {KeyEvent{InputKey::ENTER}, TextInputEvent{U'界'}, KeyEvent{InputKey::TAB}};
      }

      return batch;
    }
  } terminal;

  ApplicationOptions options;
  options.scheduler.control_capacity = options.scheduler.control_budget = 1;
  auto app = take(Application::create(options, terminal.clock()));
  Scene scene(*app);
  const auto result = take(app->run(scene, terminal));

  check(result.inputs == 3 && scene.events.size() == 3);
  check(std::get<TextInputEvent>(scene.events[1]).scalar == U'界');
}

void decoderDeadline() {
  ApplicationOptions options;
  options.inputSlice = std::chrono::milliseconds(1000);
  example::ReplayTerminal terminal({{10, "\x1b", {}}}, 2000);

  auto app = take(Application::create(options, terminal.clock()));

  struct DeadlineScene : Scene {
    using Scene::Scene;
    std::uint64_t delivered{};

    std::expected<bool, TreeError> input(Application &app, const InputEvent &event) override {
      delivered = app.now();
      app.requestStop();

      return Scene::input(app, event);
    }
  } scene(*app);

  take(app->run(scene, terminal));
  check(scene.delivered == 40 && scene.events.size() == 1);
}

void eofCutoffRetainsFrameCap() {
  // EOF freezes this fake clock before the next frame deadline. Accepted input
  // still applies, but shutdown does not bypass the cap to repaint dirty state.
  for (const auto delays : {std::size_t{0}, std::size_t{5}}) {
    OutputTerminal terminal({{20, "xyz", {}}}, 25);
    terminal.delays = delays;
    ApplicationOptions options;
    options.scheduler.control_capacity = options.scheduler.control_budget = 1;
    options.inputBudget = 1;
    auto app = take(Application::create(options, terminal.clock()));
    Scene scene(*app);
    scene.change = true;
    const auto result = take(app->run(scene, terminal));
    check(result.inputs == 3 && result.frames == 1 && result.iterations < 16);
    take(app->tree().read(scene.label, [](const Label &label) {
      check(label.text().bytes() == "changed");
    }));
    check(app->now() == 25 && terminal.closed && scene.stopped);
    check(take(app->tree().consumeFrameRequest()) == Invalidation::LAYOUT);
    test_support::VtOracle oracle(80, 24);
    oracle.feed(terminal.output);
    const auto pixels = oracle.snapshot();
    check(pixels.cells[0].glyph == "i" && pixels.cells[6].glyph == "l");
  }
}

void outputAndCleanup() {
  OutputTerminal terminal({{20, "x", {}}}, 300);
  terminal.delays = 5;
  auto app = take(Application::create({}, terminal.clock()));
  Scene scene(*app);
  scene.change = true;
  const auto result = take(app->run(scene, terminal));
  check(result.frames == 2 && terminal.calls >= 7);
  test_support::VtOracle oracle(80, 24);
  oracle.feed(terminal.output);

  const auto pixels = oracle.snapshot();
  check(pixels.cells[0].glyph == "c" && pixels.cells[6].glyph == "d");

  OutputTerminal failed;
  failed.fail = failed.failClose = true;
  auto failingApp = take(Application::create({}, failed.clock()));
  Scene failingScene(*failingApp);

  const auto failure = failingApp->run(failingScene, failed);
  check(!failure && failure.error().phase == ApplicationPhase::FLUSH && failed.closed && failingScene.stopped);
  check(std::get<TerminalError>(failure.error().cause).kind == TerminalErrorKind::OUTPUT_UNCERTAIN);
  check(failure.error().restoration && failure.error().restoration->native_code == 17);
  check(failed.calls == 1);

  example::ReplayTerminal throwing;
  auto throwApp = take(Application::create({}, throwing.clock()));
  Scene throwScene(*throwApp);
  throwScene.throwPrepare = true;
  const auto thrown = throwApp->run(throwScene, throwing);
  check(!thrown && thrown.error().phase == ApplicationPhase::PREPARE && throwing.closed && throwScene.stopped);

  ApplicationOptions limited;
  limited.writer.max_output_bytes = 1;
  example::ReplayTerminal overLimit;
  auto limitApp = take(Application::create(limited, overLimit.clock()));
  Scene limitScene(*limitApp);

  const auto limit = limitApp->run(limitScene, overLimit);
  check(!limit && std::get<FrameError>(limit.error().cause) == FrameError::OUTPUT_LIMIT && overLimit.closed);
}

class DiagnosticScene final : public Scene {
public:
  using Scene::Scene;

  bool usesPreparedLayout() const noexcept override {
    return preparedLayout;
  }

  std::expected<void, TreeError> prepare(Application &app, CellSize size) override {
    remaining = layoutRequests;

    return Scene::prepare(app, size);
  }

  std::expected<void, TreeError> prepareViews(Application &app, const LayoutSnapshot &) override {
    if (remaining) {
      --remaining;

      return app.tree().invalidate(app.root(), Invalidation::LAYOUT);
    }

    return {};
  }

  bool preparedLayout{};
  std::size_t layoutRequests{}, remaining{};
};

void diagnostics(bool preparedLayout = false, bool resize = false, std::size_t layoutRequests = 0) {
  ApplicationOptions options;
  auto recorder = std::make_shared<Diagnostics>();
  take(recorder->enable({1024}));
  options.scheduler.diagnostics = recorder;
  const auto input =
    resize ? example::ReplayTerminal::Input{100, {}, CellSize{40, 10}} : example::ReplayTerminal::Input{100, "x", {}};
  example::ReplayTerminal terminal({input}, 200);

  auto app = take(Application::create(options, terminal.clock()));
  DiagnosticScene scene(*app);

  scene.preparedLayout = preparedLayout;
  scene.layoutRequests = layoutRequests;
  scene.change = scene.prepareChanges = true;
  take(app->run(scene, terminal));

  const auto trace = recorder->snapshot();
  std::uint64_t correlation{}, frame{};

  for (const auto &record : trace.records) {
    if (record.event == DiagnosticEvent::INPUT) {
      correlation = record.context.correlationId;
    }

    if (record.event == DiagnosticEvent::FRAME_SUBMITTED && record.context.correlationId == correlation &&
        correlation) {
      frame = record.context.frameId;
    }
  }

  check(correlation && frame);

  const auto accepted = std::ranges::any_of(trace.records, [&](const auto &record) {
    return record.event == DiagnosticEvent::OUTPUT && record.reason == DiagnosticReason::OUTPUT_ACCEPTED &&
           record.context.correlationId == correlation && record.context.frameId == frame;
  });
  check(accepted);
  check(std::ranges::any_of(trace.records, [&](const auto &record) {
    return record.event == DiagnosticEvent::FRAME_REQUESTED && record.context.correlationId == correlation &&
           record.context.frameId == frame;
  }));
}

void stopAndObserver() {
  example::ReplayTerminal terminal({{50, "qrest", {}}}, 5000);

  auto app = take(Application::create({}, terminal.clock()));
  Scene scene(*app);
  scene.stopOnInput = true;
  const auto result = take(app->run(scene, terminal));
  check(result.inputs == 1 && scene.events.size() == 1 && terminal.closed);
  ApplicationOptions options;
  options.observer.callback = [](void *, ApplicationPhase phase, std::chrono::nanoseconds) {
    if (phase == ApplicationPhase::RENDER) {
      throw std::runtime_error("observer");
    }
  };
  example::ReplayTerminal observed;
  auto observerApp = take(Application::create(options, observed.clock()));
  Scene observerScene(*observerApp);

  auto failure = observerApp->run(observerScene, observed);
  check(!failure && std::get<ApplicationFailure>(failure.error().cause) == ApplicationFailure::OBSERVER_FAILED);
  check(observed.closed && observerScene.stopped);
  std::stop_source stop;
  stop.request_stop();
  example::ReplayTerminal cancelled;
  auto stopApp = take(Application::create({}, cancelled.clock()));
  Scene stopScene(*stopApp);
  take(stopApp->run(stopScene, cancelled, stop.get_token()));
  check(cancelled.closed && stopScene.stopped && cancelled.output.empty());
}

struct PreparationTrace {
  std::vector<std::string> phases;
  std::vector<CellSize> sizes;
  std::optional<TreeError> measureFailure;
  std::size_t preparations{}, viewPasses{}, paints{};
  bool throwMeasure{};
};

class PreparationProbe : public Widget {
public:
  explicit PreparationProbe(PreparationTrace &trace) : trace_(trace) {
  }

  std::expected<CellSize, TreeError> measure(CellSize available) override {
    trace_.phases.emplace_back("measure");

    if (trace_.throwMeasure) {
      throw std::runtime_error("measure");
    }

    if (trace_.measureFailure) {
      return std::unexpected(*trace_.measureFailure);
    }

    return available;
  }

  std::expected<void, TreeError> arrange(CellRect) override {
    trace_.phases.emplace_back("arrange");

    return {};
  }

  std::expected<void, TreeError> paint(PaintContext &) override {
    trace_.phases.emplace_back("paint");
    ++trace_.paints;

    return {};
  }

private:
  PreparationTrace &trace_;
};

class PreparedScene : public ApplicationScene {
public:
  explicit PreparedScene(Application &app) : probe(take(app.tree().emplace<PreparationProbe>(app.root(), trace))) {
  }

  bool usesPreparedLayout() const noexcept override {
    return optIn;
  }

  std::expected<void, TreeError> prepare(Application &, CellSize) override {
    trace.phases.emplace_back("prepare");
    ++trace.preparations;
    currentPass = 0;

    return {};
  }

  std::expected<void, TreeError> prepareViews(Application &app, const LayoutSnapshot &snapshot) override {
    trace.phases.emplace_back("views");
    ++trace.viewPasses;
    ++currentPass;
    trace.sizes.push_back(take(snapshot.viewport(probe)));
    take(app.tree().edit(probe, [](PreparationProbe &) {
    }));

    if (throwViews) {
      throw std::runtime_error("views");
    }

    if (failure) {
      return std::unexpected(*failure);
    }

    const auto invalidation = currentPass <= layoutRequests ? Invalidation::LAYOUT : Invalidation::PAINT;

    take(app.tree().invalidate(probe, invalidation));
    check(take(snapshot.viewport(probe)) == trace.sizes.back());

    return {};
  }

  void stop(Application &) override {
    stopped = true;
  }

  PreparationTrace trace;
  Node<PreparationProbe> probe;
  std::optional<TreeError> failure;
  std::size_t layoutRequests{}, currentPass{};
  bool optIn{true}, throwViews{}, stopped{};
};

void preparationOrderAndIdle() {
  check(!ApplicationScene{}.usesPreparedLayout());

  example::ReplayTerminal oldTerminal({}, 300);
  auto oldApp = take(Application::create({}, oldTerminal.clock()));
  PreparedScene oldScene(*oldApp);

  oldScene.optIn = false;

  const auto oldResult = take(oldApp->run(oldScene, oldTerminal));

  check(oldResult.frames == 1 && oldScene.trace.preparations == 1 && oldScene.trace.viewPasses == 0);
  check(oldScene.trace.phases == std::vector<std::string>{"prepare", "measure", "arrange", "paint"});

  example::ReplayTerminal terminal({}, 300);
  auto app = take(Application::create({}, terminal.clock()));
  PreparedScene scene(*app);

  scene.layoutRequests = 1;

  const auto result = take(app->run(scene, terminal));

  check(result.frames == 1 && scene.trace.preparations == 1 && scene.trace.viewPasses == 2);
  check(scene.trace.phases == std::vector<std::string>{"prepare", "measure", "arrange", "views", "measure", "arrange",
                                                       "views", "measure", "arrange", "paint"});
  check(scene.trace.sizes == std::vector<CellSize>{{80, 24}, {80, 24}});
  check(take(app->tree().consumeFrameRequest()) == Invalidation::NONE);
}

void preparationResize() {
  example::ReplayTerminal terminal({{100, {}, CellSize{32, 7}}}, 300);
  auto app = take(Application::create({}, terminal.clock()));
  PreparedScene scene(*app);

  const auto result = take(app->run(scene, terminal));

  check(result.frames == 2 && scene.trace.preparations == 2 && scene.trace.viewPasses == 2);
  check(scene.trace.sizes == std::vector<CellSize>{{80, 24}, {32, 7}});
  check(scene.trace.paints == 2);
}

void preparationFailures() {
  ApplicationOptions invalid;

  invalid.maxPreparationPasses = 0;

  const auto rejected = Application::create(invalid);

  check(!rejected && std::get<ApplicationFailure>(rejected.error().cause) == ApplicationFailure::INVALID_OPTIONS);

  for (const auto budget : {std::size_t{1}, std::size_t{3}}) {
    ApplicationOptions options;

    options.maxPreparationPasses = budget;

    example::ReplayTerminal terminal;
    auto app = take(Application::create(options, terminal.clock()));
    PreparedScene scene(*app);

    scene.layoutRequests = budget;

    const auto result = app->run(scene, terminal);

    check(!result && result.error().phase == ApplicationPhase::PREPARE);
    check(std::get<TreeError>(result.error().cause) == TreeError::WOULD_BLOCK);
    check(scene.trace.preparations == 1 && scene.trace.viewPasses == budget && scene.trace.paints == 0);
    check(terminal.output.empty() && terminal.closed && scene.stopped);
  }

  for (const bool throwing : {false, true}) {
    example::ReplayTerminal terminal;
    auto app = take(Application::create({}, terminal.clock()));
    PreparedScene scene(*app);

    scene.throwViews = throwing;
    scene.failure = TreeError::STALE_NODE;

    const auto result = app->run(scene, terminal);

    check(!result && result.error().phase == ApplicationPhase::PREPARE);

    if (throwing) {
      check(std::get<ApplicationFailure>(result.error().cause) == ApplicationFailure::CALLBACK_FAILED);
    } else {
      check(std::get<TreeError>(result.error().cause) == TreeError::STALE_NODE);
    }

    check(scene.trace.preparations == 1 && scene.trace.viewPasses == 1 && scene.trace.paints == 0);
    check(terminal.output.empty() && terminal.closed && scene.stopped);
  }
}

void owningLayoutSnapshot() {
  auto app = take(Application::create());
  auto container = take(app->tree().emplace<Column>(app->root()));
  auto hidden = take(app->tree().emplace<Widget>(container));
  auto descendant = take(app->tree().emplace<Widget>(hidden));
  auto visible = take(app->tree().emplace<Widget>(container));

  take(app->tree().setVisible(hidden, false));
  take(app->tree().setEnabled(visible, false));

  const auto invalid = LayoutSnapshot::capture(app->tree(), app->root());

  check(!invalid && invalid.error() == TreeError::INVALID_GEOMETRY);

  const auto rect = take(CellRect::from({}, {40, 8}));
  std::optional<LayoutSnapshot> snapshot;

  take(app->tree().frame([&] {
    take(layoutTree(app->tree(), app->root(), rect, rect));
    snapshot.emplace(take(LayoutSnapshot::capture(app->tree(), app->root())));
  }));

  const auto visibleSize = take(app->tree().inspect(visible)).geometry.local.size();

  check(take(snapshot->viewport(visible)) == visibleSize && visibleSize.width == 40);
  check(take(snapshot->viewport(hidden)) == CellSize{});
  check(take(snapshot->viewport(descendant)) == CellSize{});

  const auto subtree = take(LayoutSnapshot::capture(app->tree(), descendant));
  const auto outside = subtree.viewport(hidden);

  check(take(subtree.viewport(descendant)) == CellSize{});
  check(!outside && outside.error() == TreeError::STALE_NODE);
  take(app->tree().invalidate(visible, Invalidation::LAYOUT));
  check(take(snapshot->viewport(visible)) == visibleSize);
  take(app->tree().remove(visible));

  auto replacement = take(app->tree().emplace<Widget>(container));
  const auto absent = snapshot->viewport(replacement);
  auto other = take(Application::create());
  const auto wrongSession = snapshot->viewport(other->root());

  check(replacement.id().slot == visible.id().slot && replacement.id().generation != visible.id().generation);
  check(!absent && absent.error() == TreeError::STALE_NODE);
  check(!wrongSession && wrongSession.error() == TreeError::WRONG_SESSION);

  auto copied = *snapshot;
  auto moved = std::move(copied);
  const auto movedFrom = copied.viewport(visible);

  check(!movedFrom && movedFrom.error() == TreeError::STALE_NODE);
  check(take(moved.viewport(visible)) == visibleSize);
  app.reset();
  check(take(snapshot->viewport(visible)) == visibleSize);
}

void preparationLayoutFailures() {
  for (const bool throwing : {false, true}) {
    example::ReplayTerminal terminal;
    auto app = take(Application::create({}, terminal.clock()));
    PreparedScene scene(*app);

    scene.trace.throwMeasure = throwing;
    scene.trace.measureFailure = TreeError::INVALID_ARGUMENT;

    const auto result = app->run(scene, terminal);
    const auto expected = throwing ? TreeError::CALLBACK_FAILED : TreeError::INVALID_ARGUMENT;

    check(!result && result.error().phase == ApplicationPhase::PREPARE);
    check(std::get<TreeError>(result.error().cause) == expected);
    check(scene.trace.preparations == 1 && scene.trace.viewPasses == 0 && scene.trace.paints == 0);
    check(terminal.output.empty() && terminal.closed && scene.stopped);
  }
}
} // namespace

int main() {
  try {
    std::cerr << "idleAndUnchanged\n";
    idleAndUnchanged();
    std::cerr << "queueAndDecoder\n";
    queueAndDecoder();
    decoderDeadline();
    nativeSemanticBatch();
    std::cerr << "outputAndCleanup\n";
    outputAndCleanup();
    eofCutoffRetainsFrameCap();
    std::cerr << "diagnostics\n";
    diagnostics();
    diagnostics(true);
    diagnostics(true, true);
    diagnostics(true, false, 2);
    std::cerr << "stopAndObserver\n";
    stopAndObserver();
    preparationOrderAndIdle();
    preparationResize();
    preparationFailures();
    owningLayoutSnapshot();
    preparationLayoutFailures();
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';

    return 1;
  }
}

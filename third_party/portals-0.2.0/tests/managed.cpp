// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include "../examples/replay/terminal.hpp"
#include "support/vt_oracle.hpp"
#include <portals/managed.hpp>

#include <iostream>
#include <source_location>
#include <stdexcept>
#include <thread>

namespace {
using namespace portals;
constexpr CellSize VIEWPORT{32, 4};
constexpr std::uint64_t IDLE_END_MS = 5000;

void check(bool condition, std::string_view message, std::source_location where = std::source_location::current()) {
  if (!condition) {
    throw std::runtime_error(std::string(message) + " at line " + std::to_string(where.line()));
  }
}

template <class T, class E>
T take(std::expected<T, E> result, std::source_location where = std::source_location::current()) {
  check(result.has_value(), "unexpected API failure", where);

  if constexpr (!std::is_void_v<T>) {
    return std::move(*result);
  }
}

GraphemeText text(std::string_view value) {
  return take(GraphemeText::from(value));
}

bool treeCause(const ManagedError &error, TreeError cause) {
  const auto *value = std::get_if<TreeError>(&error.cause);

  return value && *value == cause;
}

struct Fixture {
  example::ReplayTerminal terminal;
  std::unique_ptr<Application> app;
  std::unique_ptr<ManagedScene> scene;

  explicit Fixture(std::size_t dataCapacity = 8, std::vector<example::ReplayTerminal::Input> input = {})
      : terminal(std::move(input), IDLE_END_MS, VIEWPORT) {
    ApplicationOptions options;

    options.scheduler.data_capacity = dataCapacity;
    app = take(Application::create(options, terminal.clock()));
    scene = take(ManagedScene::create(*app));
  }

  void prepare() {
    take(scene->prepare(*app, VIEWPORT));
  }

  std::string draft(Node<LineEdit> node) {
    std::string result;

    take(app->tree().read(node, [&](const LineEdit &editor) {
      result = editor.text().bytes();
    }));

    return result;
  }

  bool checked(Node<CheckBox> node) {
    bool result{};

    take(app->tree().read(node, [&](const CheckBox &checkbox) {
      result = checkbox.checked();
    }));

    return result;
  }
};

struct Lifetime {
  int &destroyed;

  ~Lifetime() {
    ++destroyed;
  }
};

struct Mounted final : Widget {
  int &alive;
  bool fail;

  Mounted(int &count, bool throwMount) : alive(count), fail(throwMount) {
    ++alive;
  }

  ~Mounted() override {
    --alive;
  }

  void mount() override {
    if (fail) {
      throw std::runtime_error("mount failed");
    }
  }
};

void partialConstructionAndWeakState() {
  Fixture fixture;
  int alive{};
  const auto root = fixture.scene->root();
  const auto successful = take(fixture.scene->emplace<Mounted>(root, alive, false));
  const auto failed = fixture.scene->emplace<Mounted>(root, alive, true);

  check(!failed && failed.error().phase == ManagedPhase::MOUNT && treeCause(failed.error(), TreeError::CALLBACK_FAILED),
        "mount error retains original category");
  check(alive == 1 && take(fixture.app->tree().inspect(root)).children.size() == 1,
        "failed mount retires only attempted child");
  take(fixture.app->tree().inspect(successful));
  fixture.scene.reset();
  check(alive == 0 && !fixture.app->tree().inspect(root), "RAII cleans complete partially constructed form");
  fixture.scene = take(ManagedScene::create(*fixture.app));

  const auto noInvalidation = fixture.scene->state(1, Invalidation::NONE);

  check(!noInvalidation && treeCause(noInvalidation.error(), TreeError::INVALID_ARGUMENT),
        "NONE cannot promise rejected-notification recovery");

  auto model = take(fixture.scene->state(std::string("initial")));
  const auto editor = take(fixture.scene->emplace<LineEdit>(fixture.scene->root()));

  take(fixture.scene->bind(editor, model));

  const auto snapshot = take(model.snapshot());
  bool wrongSet{}, wrongRead{};
  std::thread worker([model, &wrongSet, &wrongRead] {
    const auto set = model.set(std::string("foreign"));
    const auto read = model.snapshot();

    wrongSet = !set && set.error() == TreeError::WRONG_THREAD;
    wrongRead = !read && read.error() == TreeError::WRONG_THREAD;
  });

  worker.join();
  check(wrongSet && wrongRead && take(model.snapshot())->value == "initial", "weak state rejects foreign UI access");
  take(fixture.scene->close());

  const auto stale = model.set(std::string("after close"));

  check(!stale && stale.error() == TreeError::STALE_NODE && take(model.snapshot())->value == "initial",
        "closed subtree rejects set but preserves final snapshot");
  fixture.scene.reset();

  const auto expired = model.snapshot();

  check(alive == 0 && !fixture.app->tree().inspect(root), "scene RAII retires partial form");
  check(!expired && expired.error() == TreeError::CLOSED && snapshot->value == "initial",
        "weak handle expires while independent immutable snapshot survives");
}

void selfRemovalKeepsAdmittedCallback() {
  Fixture fixture;
  int destroyed{}, calls{};
  const auto button = take(fixture.scene->emplace<Button>(fixture.scene->root(), text("Remove")));
  auto lifetime = std::make_shared<Lifetime>(destroyed);
  std::weak_ptr<Lifetime> observer = lifetime;

  take(fixture.scene->onClick(button, [&, lifetime]() -> std::expected<void, TreeError> {
    ++calls;
    take(fixture.app->tree().remove(button));
    check(!observer.expired() && destroyed == 0, "self-removal retains executing capture");

    return {};
  }));
  lifetime.reset();
  take(fixture.app->router().invoke(button, {Action::ACTIVATE}));

  const auto repeated = fixture.app->router().invoke(button, {Action::ACTIVATE});

  check(calls == 1 && !repeated && repeated.error() == TreeError::STALE_NODE,
        "removed identity receives no subsequent callback");
  fixture.prepare();
  take(fixture.scene->close());
  check(observer.expired() && destroyed == 1, "retired callback capture eventually released once");
}

void compositionAndRejectedNotification() {
  Fixture fixture(1, {{100, "x", {}}});
  const auto node = take(fixture.scene->emplace<CheckBox>(fixture.scene->root(), text("Enabled")));
  const auto model = take(fixture.scene->state(false));
  int callbacks{};

  take(fixture.scene->bind(node, model, [&](CheckBoxResult result) -> std::expected<void, TreeError> {
    ++callbacks;
    check(result && result->changed && result->snapshot->value == take(model.snapshot())->value,
          "business callback observes completed standard toggle");
    check(fixture.checked(node) == result->snapshot->value, "standard presentation precedes business callback");

    return {};
  }));

  const auto duplicate = fixture.scene->bind(node, model);
  const auto raw =
    fixture.scene->onEvent<CheckBox>(node, [](CheckBox &, const RoutedEvent &) -> std::expected<EventReply, TreeError> {
      return EventReply::HANDLED;
    });

  check(!duplicate && treeCause(duplicate.error(), TreeError::INVALID_ARGUMENT) && !raw &&
          treeCause(raw.error(), TreeError::INVALID_ARGUMENT),
        "standard binding cannot be replaced or doubled");
  fixture.prepare();
  take(fixture.app->tree().consumeFrameRequest());
  take(model.set(true));
  take(model.set(false));

  const auto latest = take(model.set(true));

  check(latest.rejected == 1 && latest.notification_error.has_value(), "bounded data queue rejects notification");
  check(!fixture.checked(node), "undelivered update has not silently changed presentation");
  check(take(fixture.app->tree().consumeFrameRequest()) != Invalidation::NONE,
        "lost notification still schedules dirty frame");
  fixture.prepare();
  check(fixture.checked(node) && callbacks == 0, "dirty-frame preparation recovers latest without business event");
  take(fixture.app->router().invoke(node, {Action::ACTIVATE}));
  check(!take(model.snapshot())->value && callbacks == 1, "activation toggles once despite full queue");
  take(model.set(true));
  fixture.prepare();
  fixture.prepare();
  take(fixture.app->scheduler().dispatch());
  fixture.prepare();
  check(fixture.checked(node) && callbacks == 1, "old deliveries and repeated preparation cannot roll back or repeat");
  take(fixture.scene->onInput([&](const InputEvent &) -> std::expected<bool, ManagedError> {
    take(model.set(false));
    take(model.set(true));

    const auto update = take(model.set(false));

    check(update.rejected == 1, "runtime input fills data lane before next frame");

    return true;
  }));

  const auto result = take(fixture.scene->run(fixture.terminal));
  test_support::VtOracle oracle(VIEWPORT.width, VIEWPORT.height);

  oracle.feed(fixture.terminal.output);

  const auto pixels = oracle.snapshot();

  check(result.frames == 2 && callbacks == 1 && pixels.cells[1].glyph == " ",
        "real dirty frame presents latest unchecked state without repeated action or idle redraw");
}

void lineEditConflictHideAndDisable() {
  Fixture fixture(1);
  const auto node = take(fixture.scene->emplace<LineEdit>(fixture.scene->root()));
  const auto model = take(fixture.scene->state(std::string("old"), Invalidation::LAYOUT));
  std::vector<LineEditStatus> results;

  take(fixture.scene->bind(node, model, [&](LineEditResult result) -> std::expected<void, TreeError> {
    results.push_back(result.status);

    if (result.status == LineEditStatus::COMMITTED) {
      check(result.commit && take(model.snapshot())->value == result.commit->snapshot->value,
            "editor result follows standard commit");
    }

    return {};
  }));
  take(fixture.app->router().setFocus(node));
  take(fixture.app->router().dispatch(TextInputEvent{U'!'}));
  check(fixture.draft(node) == "old!" && take(model.snapshot())->value == "old", "typing is a local draft");
  take(fixture.app->tree().setVisible(node, false));
  fixture.prepare();
  take(fixture.app->tree().setVisible(node, true));
  take(fixture.app->tree().setEnabled(node, false));
  fixture.prepare();
  take(fixture.app->router().invoke(node, {Action::ACTIVATE}));
  check(fixture.draft(node) == "old!" && results.size() == 1,
        "hidden/disabled draft and binding survive without input");
  take(fixture.app->tree().setEnabled(node, true));
  take(fixture.app->router().setFocus(node));
  take(model.set(std::string("external")));
  take(model.set(std::string("latest")));
  fixture.prepare();
  check(fixture.draft(node) == "old!", "sync does not overwrite dirty draft");
  take(fixture.app->router().invoke(node, {Action::ACTIVATE}));
  check(results.back() == LineEditStatus::CONFLICT && fixture.draft(node) == "old!" &&
          take(model.snapshot())->value == "latest",
        "Enter reports revision conflict without changing either value");
  take(fixture.app->router().invoke(node, {Action::CANCEL}));
  check(results.back() == LineEditStatus::CANCELLED && fixture.draft(node) == "latest",
        "Escape recovers newest external model");
  take(fixture.app->router().dispatch(TextInputEvent{U'?'}));
  take(fixture.app->router().invoke(node, {Action::ACTIVATE}));
  check(results.back() == LineEditStatus::COMMITTED && take(model.snapshot())->value == "latest?",
        "preserved binding still commits after hide/disable and conflict");

  const auto callbackCount = results.size();

  fixture.prepare();
  take(fixture.app->scheduler().dispatch());
  fixture.prepare();
  check(results.size() == callbackCount, "editor synchronization never repeats result callbacks");
  take(fixture.app->tree().remove(node));
  fixture.prepare();
  check(!fixture.app->router().invoke(node, {Action::ACTIVATE}), "removed editor is stale");
}

struct Meter final : Widget {
  GraphemeText display = text("unset");
  CellRect area;

  std::expected<CellSize, TreeError> measure(CellSize) override {
    return CellSize{8, 1};
  }

  std::expected<void, TreeError> arrange(CellRect local) override {
    area = take(CellRect::from({}, local.size()));

    return {};
  }

  std::expected<void, TreeError> paint(PaintContext &context) override {
    const auto style = computedStyle() ? computedStyle()->normal : CellStyle{};
    const RenderCommand command = TextCommand{{}, display, area, style};

    return context.submit(std::span(&command, 1));
  }
};

void customCallbacksRetireTheirNodes() {
  Fixture fixture;
  const auto first = take(fixture.scene->emplace<Meter>(fixture.scene->root()));
  const auto later = take(fixture.scene->emplace<Meter>(fixture.scene->root()));
  int firstCalls{}, laterCalls{}, events{};

  take(fixture.scene->onEvent<Meter>(first, [&](Meter &, const RoutedEvent &) -> std::expected<EventReply, TreeError> {
    ++events;

    return EventReply::HANDLED;
  }));
  take(fixture.app->router().invoke(first, {Action::ACTIVATE}));
  take(fixture.scene->onPrepare<Meter>(first, [&](Meter &widget, Node<Widget>) -> std::expected<void, TreeError> {
    ++firstCalls;
    take(fixture.app->tree().remove(later));
    take(fixture.app->tree().remove(first));
    widget.display = text("retired");

    return {};
  }));
  take(fixture.scene->onPrepare<Meter>(later, [&](Meter &, Node<Widget>) -> std::expected<void, TreeError> {
    ++laterCalls;

    return {};
  }));
  fixture.prepare();
  fixture.prepare();
  check(firstCalls == 1 && laterCalls == 0 && events == 1,
        "custom final handler works and preparation skips nodes removed by earlier callback");
  check(!fixture.app->tree().inspect(first) && !fixture.app->tree().inspect(later),
        "self-removal keeps current borrow usable but retires both identities");
}

void removedPreparationPreservesFailure() {
  for (const auto cause :
       {TreeError::NODE_LIMIT, TreeError::STALE_NODE, TreeError::CLOSED, TreeError::CALLBACK_FAILED}) {
    Fixture fixture;
    const auto root = fixture.scene->root();
    const auto node = take(fixture.scene->emplace<Meter>(root));

    take(fixture.scene->onPrepare(node, [&](Meter &, Node<Widget>) -> std::expected<void, TreeError> {
      take(fixture.app->tree().remove(node));

      if (cause == TreeError::CALLBACK_FAILED) {
        throw std::runtime_error("custom preparation failed after removal");
      }

      return std::unexpected(cause);
    }));

    const auto result = fixture.scene->run(fixture.terminal);

    check(!result && result.error().phase == ManagedPhase::PREPARE && result.error().node == node.id() &&
            treeCause(result.error(), cause),
          "self-removal cannot suppress explicit or translated custom preparation failure");
    check(fixture.terminal.closed && !fixture.app->tree().inspect(root),
          "failed custom preparation still restores transport and cleans the scene");
  }
}

void customPreparationThemeAndIdle() {
  Fixture fixture;
  const auto column = take(fixture.scene->emplace<Column>(fixture.scene->root()));
  const auto label = take(fixture.scene->emplace<Label>(column, text("Label")));
  const auto meter = take(fixture.scene->emplace<Meter>(column));
  const auto model = take(fixture.scene->state(7));
  int prepared{};

  take(fixture.scene->onPrepare<Meter>(meter, [&](Meter &widget, Node<Widget>) -> std::expected<void, TreeError> {
    ++prepared;
    widget.display = text("value " + std::to_string(take(model.snapshot())->value));

    return {};
  }));
  take(fixture.scene->style(meter, StyleTarget{.type = "Meter"}));
  take(fixture.scene->theme(take(parsePss("Control { foreground: #010203; } Meter { foreground: #040506; }"))));
  take(fixture.app->tree().read(label, [](const Label &widget) {
    check(widget.computedStyle() && widget.computedStyle()->normal.foreground == RgbColor{1, 2, 3},
          "standard Control style target registered automatically");
  }));
  take(fixture.scene->theme(take(parsePss("Control { foreground: #070809; } Meter { foreground: #0a0b0c; }"))));
  take(fixture.app->tree().read(meter, [](const Meter &widget) {
    check(widget.computedStyle() && widget.computedStyle()->normal.foreground == RgbColor{10, 11, 12},
          "custom type participates in existing theme replacement");
  }));

  const auto root = fixture.scene->root();
  const auto result = take(fixture.scene->run(fixture.terminal));

  check(result.frames == 1 && prepared == 1 && fixture.terminal.closed,
        "five seconds idle produce one preparation/frame and orderly terminal close");
  check(!fixture.app->tree().inspect(root), "run retires scene-owned subtree");
  test_support::VtOracle oracle(VIEWPORT.width, VIEWPORT.height);

  oracle.feed(fixture.terminal.output);

  const auto pixels = oracle.snapshot();

  check(pixels.cells[VIEWPORT.width].glyph == "v" && pixels.cells[VIEWPORT.width + 6].glyph == "7",
        "final custom widget presents state in actual terminal frame");
}

class RestoreFailureTerminal : public example::ReplayTerminal {
public:
  RestoreFailureTerminal() : ReplayTerminal({{50, "\r", {}}}, 200, VIEWPORT) {
  }

  std::expected<void, TerminalError> close() noexcept override {
    (void)ReplayTerminal::close();

    return std::unexpected(TerminalError{TerminalErrorKind::SYSTEM, RESTORE_ERROR});
  }

  static constexpr int RESTORE_ERROR = 71;
};

void typedCallbackFailureRestorationAndCleanup() {
  RestoreFailureTerminal terminal;
  auto app = take(Application::create({}, terminal.clock()));
  auto scene = take(ManagedScene::create(*app));
  const auto root = scene->root();
  const auto button = take(scene->emplace<Button>(root, text("Fail")));
  int calls{}, destroyed{};
  auto lifetime = std::make_shared<Lifetime>(destroyed);
  std::weak_ptr<Lifetime> observer = lifetime;

  take(scene->onClick(button, [&, lifetime]() -> std::expected<void, TreeError> {
    ++calls;

    return std::unexpected(TreeError::NODE_LIMIT);
  }));
  lifetime.reset();
  take(app->router().setFocus(button));

  const auto result = scene->run(terminal);

  check(!result && result.error().phase == ManagedPhase::INPUT && result.error().node == button.id() &&
          treeCause(result.error(), TreeError::NODE_LIMIT),
        "business error retains typed phase, node and original category");
  check(result.error().application && result.error().application->restoration &&
          result.error().application->restoration->native_code == RestoreFailureTerminal::RESTORE_ERROR,
        "terminal restoration failure remains separate from business error");
  check(!result.error().cleanup && !app->tree().inspect(root) && terminal.closed && calls == 1,
        "callback failure still restores terminal and retires subtree");
  scene.reset();
  take(app->router().focused());
  check(observer.expired() && destroyed == 1, "error shutdown releases callback capture");
}

void sceneInterceptorPreservesTypedError() {
  example::ReplayTerminal terminal({{50, "x", {}}}, 200, VIEWPORT);
  auto app = take(Application::create({}, terminal.clock()));
  auto scene = take(ManagedScene::create(*app));
  const auto root = scene->root();

  take(scene->onInput([root](const InputEvent &) -> std::expected<bool, ManagedError> {
    return std::unexpected(ManagedError{ManagedPhase::STATE, root.id(), TreeError::DEPTH_LIMIT, {}, {}});
  }));

  const auto result = scene->run(terminal);

  check(!result && result.error().phase == ManagedPhase::STATE && result.error().node == root.id() &&
          treeCause(result.error(), TreeError::DEPTH_LIMIT),
        "scene interceptor preserves its original typed failure");
  check(terminal.closed && !app->tree().inspect(root), "interceptor failure still performs cleanup");
}

void sceneInterceptorStateAndReplacement() {
  int destroyed{};
  Fixture fixture;
  const InputEvent event = TextInputEvent{U'x'};

  take(fixture.scene->onInput([count = 0](const InputEvent &) mutable -> std::expected<bool, ManagedError> {
    return ++count == 2;
  }));
  check(!take(fixture.scene->input(*fixture.app, event)) && take(fixture.scene->input(*fixture.app, event)),
        "mutable interceptor captures retain state across input events");

  auto lifetime = std::make_shared<Lifetime>(destroyed);
  std::weak_ptr<Lifetime> observer = lifetime;

  take(fixture.scene->onInput([&, lifetime](const InputEvent &) -> std::expected<bool, ManagedError> {
    take(fixture.scene->onInput([](const InputEvent &) -> std::expected<bool, ManagedError> {
      return true;
    }));
    check(!observer.expired(), "replacement keeps the currently executing interceptor alive");

    return false;
  }));
  lifetime.reset();
  check(!take(fixture.scene->input(*fixture.app, event)) && observer.expired() && destroyed == 1,
        "replaced interceptor is released after its invocation returns");
  check(take(fixture.scene->input(*fixture.app, event)), "replacement handles the next event");

  lifetime = std::make_shared<Lifetime>(destroyed);
  observer = lifetime;

  take(fixture.scene->onInput([&, lifetime](const InputEvent &) -> std::expected<bool, ManagedError> {
    take(fixture.scene->close());
    check(!observer.expired(), "close retains the admitted interceptor until return");

    return true;
  }));
  lifetime.reset();
  check(take(fixture.scene->input(*fixture.app, event)) && observer.expired() && destroyed == 2,
        "close releases the executing interceptor only after it finishes");
}

void sceneInterceptorPreventsActivation() {
  for (const bool consume : {false, true}) {
    int clicks{};
    Fixture fixture(8, {{100, "\r", {}}});
    const auto button = take(fixture.scene->emplace<Button>(fixture.scene->root(), text("Activate")));

    take(fixture.scene->onClick(button, [&]() -> std::expected<void, TreeError> {
      ++clicks;

      return {};
    }));
    take(fixture.app->router().setFocus(button));
    take(fixture.scene->onInput([consume](const InputEvent &) -> std::expected<bool, ManagedError> {
      return consume;
    }));
    take(fixture.scene->run(fixture.terminal));
    check(clicks == (consume ? 0 : 1), "consumed Enter prevents the otherwise observable standard activation");
  }
}

void boundedOptionsAndRejectedForeignCalls() {
  example::ReplayTerminal terminal({}, IDLE_END_MS, VIEWPORT);
  auto app = take(Application::create({}, terminal.clock()));

  for (const auto options : {ManagedOptions{0, 1}, ManagedOptions{1, 0}}) {
    const auto invalid = ManagedScene::create(*app, options);

    check(!invalid && treeCause(invalid.error(), TreeError::INVALID_ARGUMENT), "zero ownership budgets rejected");
    check(take(app->tree().inspect(app->root())).children.empty(), "invalid options do not mount a partial subtree");
  }

  auto scene = take(ManagedScene::create(*app, ManagedOptions{1, 1}));
  const auto first = take(scene->emplace<Label>(scene->root(), text("first")));
  const auto model = take(scene->state(false));
  const auto excessNode = scene->emplace<Widget>(scene->root());
  const auto excessState = scene->state(2);

  check(!excessNode && treeCause(excessNode.error(), TreeError::NODE_LIMIT) && !excessState &&
          treeCause(excessState.error(), TreeError::NODE_LIMIT),
        "separate node/state budgets reject growth without changing existing objects");
  check(take(model.snapshot())->value == false && take(app->tree().inspect(scene->root())).children.size() == 1,
        "ownership admission errors preserve existing model and child");
  take(app->tree().remove(first));

  const auto pending = scene->emplace<Widget>(scene->root());

  check(!pending && treeCause(pending.error(), TreeError::NODE_LIMIT), "retired entries count until preparation");
  take(scene->prepare(*app, VIEWPORT));
  take(scene->emplace<Label>(scene->root(), text("replacement")));

  bool wrongPrepare{}, wrongInput{};
  std::thread worker([&] {
    const auto prepare = scene->prepare(*app, VIEWPORT);
    const auto input = scene->input(*app, TextInputEvent{U'x'});

    wrongPrepare = !prepare && prepare.error() == TreeError::WRONG_THREAD;
    wrongInput = !input && input.error() == TreeError::WRONG_THREAD;
  });

  worker.join();
  check(wrongPrepare && wrongInput, "foreign prepare/input rejected before runtime access");

  const auto result = take(scene->run(terminal));

  check(result.frames == 1 && terminal.closed, "rejected foreign calls do not latch failure or stop valid UI run");
}
} // namespace

int main() {
  try {
    partialConstructionAndWeakState();
    selfRemovalKeepsAdmittedCallback();
    compositionAndRejectedNotification();
    lineEditConflictHideAndDisable();
    customCallbacksRetireTheirNodes();
    removedPreparationPreservesFailure();
    customPreparationThemeAndIdle();
    typedCallbackFailureRestorationAndCleanup();
    sceneInterceptorPreservesTypedError();
    sceneInterceptorStateAndReplacement();
    sceneInterceptorPreventsActivation();
    boundedOptionsAndRejectedForeignCalls();
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';

    return 1;
  }

  std::cout << "managed Gate A checks passed\n";

  return 0;
}

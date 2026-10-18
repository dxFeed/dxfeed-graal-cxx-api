// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include <portals/ui.hpp>

#include <iostream>
#include <source_location>
#include <stdexcept>
#include <thread>
#include <tuple>
#include <utility>

namespace {
using namespace portals;

template <class T>
concept CanNest = requires(T &&value) { ui::column(std::forward<T>(value)); };

static_assert(CanNest<ui::Description<Label>>);
static_assert(!CanNest<ui::Description<Label> &>);
static_assert(!std::copy_constructible<ui::Description<Label>>);

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

bool treeCause(const ManagedError &error, TreeError cause) {
  const auto *value = std::get_if<TreeError>(&error.cause);

  return value && *value == cause;
}

struct Fixture {
  std::unique_ptr<Application> app;
  std::unique_ptr<ManagedScene> scene;

  explicit Fixture(std::size_t capacity = 32, WidthPolicy width = {}) {
    ApplicationOptions options;

    options.widthPolicy = width;
    app = take(Application::create(options));
    scene = take(ManagedScene::create(*app, {.maxNodes = capacity}));
  }

  void prepare() {
    take(scene->prepare(*app, {40, 10}));
  }
};

struct Custom final : Widget {
  std::string caption;
  int events{};
  int prepares{};

  explicit Custom(std::string text) : caption(std::move(text)) {
  }
};

struct MountFailure final : Widget {
  void mount() override {
    throw std::runtime_error("expected mount failure");
  }
};

struct Reentrant final : Widget {
  ManagedScene &scene;
  bool &blocked;

  Reentrant(ManagedScene *owner, bool *result) : scene(*owner), blocked(*result) {
  }

  void unmount() override {
    const auto attempt = scene.emplace<Widget>(scene.root());

    blocked = !attempt && treeCause(attempt.error(), TreeError::WOULD_BLOCK);
  }
};

template <class T> struct ArgumentWidget final : Widget {
  T value;

  explicit ArgumentWidget(T argument) : value(std::move(argument)) {
  }
};

struct OverloadedArgument final : Widget {
  bool tupleConstructor{};

  explicit OverloadedArgument(std::tuple<int, int>) : tupleConstructor(true) {
  }

  OverloadedArgument(int, int) {
  }
};

struct RemovalOutcome {
  bool called{};
  bool removed{};
  bool blocked{};
};

struct RemovesSibling final : Widget {
  ManagedScene &scene;
  Node<Widget> sibling;
  RemovalOutcome &outcome;

  RemovesSibling(ManagedScene *owner, Node<Widget> target, RemovalOutcome *result)
      : scene(*owner), sibling(target), outcome(*result) {
  }

  void unmount() override {
    const auto result = scene.remove(sibling);

    outcome = {true, result.has_value(), !result && treeCause(result.error(), TreeError::WOULD_BLOCK)};
  }
};

struct CaptureLifetime {
  int &destructions;
  int visits{};

  explicit CaptureLifetime(int &counter) : destructions(counter) {
  }

  ~CaptureLifetime() {
    ++destructions;
  }
};

void customArgumentShape() {
  Fixture fixture;
  using Tuple = std::tuple<int, std::string>;
  using Pair = std::pair<int, std::string>;

  const auto tuple = take(ui::mount(*fixture.scene, ui::custom<ArgumentWidget<Tuple>>(Tuple{7, "tuple"})));
  const auto pair = take(ui::mount(*fixture.scene, ui::custom<ArgumentWidget<Pair>>(Pair{9, "pair"})));
  const auto overloaded = take(ui::mount(*fixture.scene, ui::custom<OverloadedArgument>(std::tuple{1, 2})));

  take(fixture.app->tree().read(tuple, [](const ArgumentWidget<Tuple> &widget) {
    check(widget.value == Tuple{7, "tuple"}, "one tuple remains one constructor argument");
  }));
  take(fixture.app->tree().read(pair, [](const ArgumentWidget<Pair> &widget) {
    check(widget.value == Pair{9, "pair"}, "one pair remains one constructor argument");
  }));
  take(fixture.app->tree().read(overloaded, [](const OverloadedArgument &widget) {
    check(widget.tupleConstructor, "tuple capture does not silently select an unpacked overload");
  }));
}

void setupOrderIsPreorder() {
  Fixture fixture;
  std::vector<std::string_view> calls;
  const auto mark = [&](std::string_view name) {
    return [&, name](ManagedScene &, auto) -> std::expected<void, ManagedError> {
      calls.push_back(name);

      return {};
    };
  };

  auto recipe = ui::column(ui::row(ui::label("first").setup(mark("first"))).setup(mark("row")),
                           ui::label("second").setup(mark("second")))
                  .setup(mark("root first"))
                  .setup(mark("root second"));

  take(ui::mount(*fixture.scene, std::move(recipe)));
  check(calls == std::vector<std::string_view>{"root first", "root second", "row", "first", "second"},
        "parent modifiers run in registration order before children in argument order");
}

void textLimitKeepsTypedError() {
  Fixture fixture;
  const auto existing = take(ui::mount(*fixture.scene, ui::label("existing")));
  const std::string oversized(TextLimits{}.max_input_bytes + 1, 'x');
  const auto originalError = GraphemeText::from(oversized).error();
  ui::NodeRef<Label> reference;
  const auto failed = ui::mount(*fixture.scene, ui::column(ui::label(oversized).reference(reference)));

  check(!failed && treeCause(failed.error(), TreeError::NODE_LIMIT) && failed.error().text == originalError,
        "text input limit retains the original category and offset");
  check(reference.node() == Node<Label>{} &&
          take(fixture.app->tree().inspect(fixture.scene->root())).children == std::vector<Node<Widget>>{existing},
        "text limit publishes no reference and preserves the previous subtree");
}

void storedStringsAndSingleUse() {
  Fixture fixture;
  ui::NodeRef<Label> labelRef;
  ui::NodeRef<Custom> customRef;
  ui::Description<Column> stored;

  {
    std::string caption = "owned text";
    std::string custom = "owned custom";

    LinearOptions rowOptions;

    rowOptions.gap = 2;
    stored = ui::column(ui::label(std::string_view(caption)).reference(labelRef),
                        ui::row(rowOptions, ui::custom<Custom>(std::string_view(custom)).reference(customRef)));
    caption.assign(100, 'x');
    custom.assign(100, 'y');
  }

  check(labelRef.node() == Node<Label>{}, "reference is not published by description construction");

  auto moved = std::move(stored);
  const auto empty = ui::mount(*fixture.scene, std::move(stored));

  check(!empty && treeCause(empty.error(), TreeError::INVALID_ARGUMENT), "moved-from description rejected");

  const auto root = take(ui::mount(*fixture.scene, std::move(moved)));
  const auto repeated = ui::mount(*fixture.scene, std::move(moved));

  check(!repeated && treeCause(repeated.error(), TreeError::INVALID_ARGUMENT), "consumed description rejected");
  take(fixture.app->tree().read(labelRef.node(), [](const Label &label) {
    check(label.text().bytes() == "owned text", "source string storage is not borrowed");
  }));
  take(fixture.app->tree().read(customRef.node(), [](const Custom &custom) {
    check(custom.caption == "owned custom", "custom string_view argument is materialized");
  }));

  const auto children = take(fixture.app->tree().inspect(root)).children;

  check(children.size() == 2 && children.front() == labelRef.node(), "preorder parent-child ownership");
  check(take(fixture.app->tree().inspect(children.back())).children.front() == customRef.node(),
        "mixed nested layouts");
  take(fixture.scene->remove(root));
  check(!fixture.app->tree().inspect(labelRef.node()), "ref remains observing stale identity after remove");
}

void referencesAreTransactional() {
  Fixture fixture;
  ui::NodeRef<Label> ref;
  const auto duplicate =
    ui::mount(*fixture.scene, ui::column(ui::label("first").reference(ref), ui::label("second").reference(ref)));

  check(!duplicate && treeCause(duplicate.error(), TreeError::INVALID_ARGUMENT), "duplicate slot rejected");
  check(ref.node() == Node<Label>{}, "failed mount publishes no refs");
  check(take(fixture.app->tree().inspect(fixture.scene->root())).children.empty(), "failed root removed");

  const auto original = take(ui::mount(*fixture.scene, ui::label("live").reference(ref)));
  const auto reused = ui::mount(*fixture.scene, ui::label("replacement").reference(ref));

  check(!reused && treeCause(reused.error(), TreeError::INVALID_ARGUMENT), "published slot cannot be overwritten");
  check(ref.node() == original && take(fixture.app->tree().inspect(fixture.scene->root())).children.size() == 1,
        "failed reuse preserves previous node and reference");

  ui::NodeRef<Label> reserved;
  bool nestedRejected{};
  const auto outer = take(ui::mount(
    *fixture.scene, ui::label("outer").reference(reserved).setup([&](ManagedScene &scene,
                                                                     Node<Label>) -> std::expected<void, ManagedError> {
      const auto nested = ui::mount(scene, ui::label("nested").reference(reserved));

      nestedRejected = !nested && treeCause(nested.error(), TreeError::INVALID_ARGUMENT);
      check(reserved.node() == Node<Label>{}, "reentrant mount cannot publish pending outer reference");

      return {};
    })));

  check(nestedRejected && reserved.node() == outer, "reentrant mount cannot steal another transaction's ref");

  ui::NodeRef<Label> moved;
  auto destination = std::move(moved);
  const auto invalidRef = ui::mount(*fixture.scene, ui::label("moved ref").reference(moved));

  check(!invalidRef && treeCause(invalidRef.error(), TreeError::INVALID_ARGUMENT) &&
          destination.node() == Node<Label>{},
        "moved-from reference is rejected without dereference");
}

void rollbackReclaimsBindingsAndBudget() {
  Fixture fixture(4);
  const auto existing = take(ui::mount(*fixture.scene, ui::label("existing")));
  const auto model = take(fixture.scene->state(std::string("initial")));
  ui::NodeRef<LineEdit> failedRef;
  std::weak_ptr<int> lifetime;
  bool blocked{};

  for (int attempt = 0; attempt < 3; ++attempt) {
    auto owner = std::make_shared<int>(attempt);

    lifetime = owner;

    auto description = ui::column(
      ui::lineEdit(model, {.result = [owner](LineEditResult) -> std::expected<void, TreeError> {
                     return {};
                   }})
        .reference(failedRef),
      ui::custom<Reentrant>(fixture.scene.get(), &blocked)
        .setup([](ManagedScene &, Node<Reentrant> node) -> std::expected<void, ManagedError> {
          return std::unexpected(ManagedError{ManagedPhase::BIND, node.id(), TreeError::INVALID_GEOMETRY, {}, {}});
        }));

    owner.reset();

    const auto failed = ui::mount(*fixture.scene, std::move(description));

    check(!failed && treeCause(failed.error(), TreeError::INVALID_GEOMETRY) &&
            failed.error().phase == ManagedPhase::BIND,
          "original typed setup error survives rollback");
    check(!failed.error().cleanup && blocked, "rollback guards unmount reentry");
    check(failedRef.node() == Node<LineEdit>{}, "partially bound editor ref not published");
    check(take(fixture.app->tree().inspect(fixture.scene->root())).children == std::vector<Node<Widget>>{existing},
          "rollback preserves only earlier subtree");
    take(fixture.app->router().focused());
    check(lifetime.expired(), "disconnected callback eventually reclaimed by existing router");
  }

  auto valid = take(ui::mount(*fixture.scene, ui::column(ui::lineEdit(model), ui::label("new"))));

  check(take(model.snapshot())->value == "initial", "state survives partial mount failures");
  fixture.prepare();
  take(fixture.scene->remove(valid));
  take(ui::mount(*fixture.scene, ui::column(ui::label("a"), ui::label("b"))));
  fixture.prepare();
}

void mountErrorsAndRemovedNodes() {
  Fixture fixture(3);
  ui::NodeRef<Label> ref;
  const auto failed =
    ui::mount(*fixture.scene, ui::column(ui::label("ready").reference(ref), ui::custom<MountFailure>()));

  check(!failed && treeCause(failed.error(), TreeError::CALLBACK_FAILED), "checked mount failure propagated");
  check(ref.node() == Node<Label>{}, "mount failure does not publish preceding ref");
  check(take(fixture.app->tree().inspect(fixture.scene->root())).children.empty(), "partial tree removed");

  const auto retired =
    ui::mount(*fixture.scene,
              ui::label("remove").setup([&](ManagedScene &, Node<Label> node) -> std::expected<void, ManagedError> {
                take(fixture.app->tree().remove(node));

                return {};
              }));

  check(!retired && treeCause(retired.error(), TreeError::STALE_NODE) && !retired.error().cleanup,
        "successful setup cannot return a dead mounted node");

  const std::string invalidBytes = "valid prefix\xFF";
  const auto expectedTextError = GraphemeText::from(invalidBytes).error();
  const auto invalidText = ui::mount(*fixture.scene, ui::label(invalidBytes));

  check(!invalidText && treeCause(invalidText.error(), TreeError::INVALID_ARGUMENT) &&
          invalidText.error().text == expectedTextError,
        "invalid UTF-8 retains exact category and byte offset at mount");
  take(ui::mount(*fixture.scene, ui::column(ui::label("a"), ui::label("b"))));
}

void customAndCallbackLifetime() {
  int destructions{};
  Fixture fixture;
  ui::NodeRef<Custom> custom;
  ui::NodeRef<Button> button;
  int clicks{};
  std::weak_ptr<CaptureLifetime> lifetime;

  {
    auto owner = std::make_shared<CaptureLifetime>(destructions);

    lifetime = owner;

    auto description =
      ui::column(ui::custom<Custom>("extension")
                   .reference(custom)
                   .style({.type = "Control"})
                   .onEvent([](Custom &widget, const RoutedEvent &) -> std::expected<EventReply, TreeError> {
                     ++widget.events;

                     return EventReply::HANDLED;
                   })
                   .onPrepare([](Custom &widget, Node<Widget>) -> std::expected<void, TreeError> {
                     ++widget.prepares;

                     return {};
                   }),
                 ui::button("remove", [&, owner]() -> std::expected<void, TreeError> {
                   ++clicks;
                   ++owner->visits;
                   take(fixture.scene->remove(button.node()));
                   check(owner->visits == 1 && !lifetime.expired() && destructions == 0,
                         "admitted callback retains its owned capture after logical removal");

                   return {};
                 }).reference(button));

    take(ui::mount(*fixture.scene, std::move(description)));
  }

  fixture.prepare();
  take(fixture.scene->theme(turboTheme()));
  take(fixture.app->router().invoke(custom.node(), {Action::ACTIVATE}));
  take(fixture.app->tree().read(custom.node(), [](const Custom &widget) {
    check(widget.events == 1 && widget.prepares == 1 && widget.styleRevision() != 0,
          "custom final retains callbacks and style after description destruction");
  }));
  take(fixture.app->router().invoke(button.node(), {Action::ACTIVATE}));
  check(clicks == 1 && !fixture.app->tree().inspect(button.node()), "managed remove safe inside admitted callback");
  take(fixture.app->router().focused());
  check(lifetime.expired() && destructions == 1, "owned capture is reclaimed exactly once after callback return");

  const auto repeated = fixture.app->router().invoke(button.node(), {Action::ACTIVATE});

  check(!repeated && repeated.error() == TreeError::STALE_NODE && clicks == 1 && destructions == 1,
        "retired callback cannot be admitted again");
}

void deferredUnmountKeepsCoreSemantics() {
  RemovalOutcome immediate;
  RemovalOutcome deferred;
  Fixture fixture;
  const Node<Widget> existing = take(ui::mount(*fixture.scene, ui::label("existing")));
  const auto direct =
    take(ui::mount(*fixture.scene, ui::custom<RemovesSibling>(fixture.scene.get(), existing, &immediate)));

  take(fixture.scene->remove(direct));
  check(immediate.called && immediate.blocked && !immediate.removed,
        "managed mutation reentering an active remove is blocked");

  take(fixture.app->tree().withScope(fixture.scene->root(), [&](const UiScope &) {
    auto recipe =
      ui::custom<RemovesSibling>(fixture.scene.get(), existing, &deferred)
        .setup([](ManagedScene &, Node<RemovesSibling> node) -> std::expected<void, ManagedError> {
          return std::unexpected(ManagedError{ManagedPhase::MOUNT, node.id(), TreeError::INVALID_GEOMETRY, {}, {}});
        });
    const auto failed = ui::mount(*fixture.scene, std::move(recipe));

    check(!failed && treeCause(failed.error(), TreeError::INVALID_GEOMETRY) && !failed.error().cleanup,
          "rollback under an outer borrow preserves its original error");
    check(!deferred.called && fixture.app->tree().inspect(existing).has_value(),
          "outer borrow postpones unmount and its external effects");
  }));

  check(deferred.called && deferred.removed && !deferred.blocked && !fixture.app->tree().inspect(existing),
        "deferred unmount retains the core's sibling-removal semantics");
}

void sharedWidthPolicyAndStandardSemantics() {
  constexpr WidthPolicy WIDTH{AmbiguousWidth::WIDE, EmojiWidth::ONE, 42};
  Fixture fixture(8, WIDTH);
  const auto checked = take(fixture.scene->state(false));
  const auto name = take(fixture.scene->state(std::string("draft")));
  ui::NodeRef<Label> label;
  ui::NodeRef<CheckBox> checkbox;
  ui::NodeRef<LineEdit> editor;
  int changes{};

  take(ui::mount(*fixture.scene, ui::column(ui::label("width").reference(label),
                                            ui::checkbox("enabled", checked,
                                                         [&](CheckBoxResult receipt) -> std::expected<void, TreeError> {
                                                           check(receipt && receipt->snapshot->value,
                                                                 "standard commit precedes callback");
                                                           ++changes;

                                                           return {};
                                                         })
                                              .reference(checkbox),
                                            ui::lineEdit(name).reference(editor))));
  fixture.prepare();
  take(fixture.app->tree().read(label.node(), [&](const Label &widget) {
    check(widget.text().policy() == WIDTH, "label uses scene width policy");
  }));
  take(fixture.app->tree().read(editor.node(), [&](const LineEdit &widget) {
    check(widget.text().policy() == WIDTH && widget.text().bytes() == "draft", "editor shares policy and bound model");
  }));
  take(fixture.app->router().invoke(checkbox.node(), {Action::ACTIVATE}));
  check(take(checked.snapshot())->value && changes == 1, "checkbox keeps standard semantics");

  const auto duplicate =
    ui::mount(*fixture.scene, ui::checkbox("duplicate", checked)
                                .onEvent([](CheckBox &, const RoutedEvent &) -> std::expected<EventReply, TreeError> {
                                  return EventReply::HANDLED;
                                }));

  check(!duplicate && treeCause(duplicate.error(), TreeError::INVALID_ARGUMENT),
        "raw adapter cannot replace standard binding");
}

void exceptionsAndForeignMount() {
  Fixture fixture;
  const auto thrown = ui::mount(
    *fixture.scene, ui::label("failure").setup([](ManagedScene &, Node<Label>) -> std::expected<void, ManagedError> {
      throw std::bad_alloc();
    }));

  check(!thrown && treeCause(thrown.error(), TreeError::NO_MEMORY) && !thrown.error().cleanup,
        "allocation exception within setup typed and subtree reclaimed");

  bool rejected{};
  std::thread worker([&] {
    const auto result = ui::mount(*fixture.scene, ui::label("foreign"));

    rejected = !result && treeCause(result.error(), TreeError::WRONG_THREAD);
  });

  worker.join();
  check(rejected && take(fixture.app->tree().inspect(fixture.scene->root())).children.empty(),
        "foreign mount rejected without mutation");
}
} // namespace

int main() {
  try {
    customArgumentShape();
    setupOrderIsPreorder();
    textLimitKeepsTypedError();
    storedStringsAndSingleUse();
    referencesAreTransactional();
    rollbackReclaimsBindingsAndBudget();
    mountErrorsAndRemovedNodes();
    customAndCallbackLifetime();
    deferredUnmountKeepsCoreSemantics();
    sharedWidthPolicyAndStandardSemantics();
    exceptionsAndForeignMount();
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';

    return 1;
  }

  std::cout << "owning description Gate A checks passed\n";

  return 0;
}

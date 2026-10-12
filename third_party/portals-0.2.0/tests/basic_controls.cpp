// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include "../examples/gallery/gallery.hpp"
#include "support/test_terminal.hpp"

#include <array>
#include <iostream>
#include <optional>
#include <thread>

namespace allocation_test {
thread_local std::ptrdiff_t remaining = -1;
}

void *operator new(std::size_t size) {
  if (allocation_test::remaining == 0) {
    throw std::bad_alloc();
  }

  if (allocation_test::remaining > 0) {
    --allocation_test::remaining;
  }

  if (void *p = std::malloc(size ? size : 1)) {
    return p;
  }

  throw std::bad_alloc();
}

void operator delete(void *p) noexcept {
  std::free(p);
}

void operator delete(void *p, std::size_t) noexcept {
  std::free(p);
}

void *operator new[](std::size_t size) {
  return ::operator new(size);
}

void operator delete[](void *p) noexcept {
  ::operator delete(p);
}

void operator delete[](void *p, std::size_t) noexcept {
  ::operator delete(p);
}

void *operator new(std::size_t size, const std::nothrow_t &) noexcept {
  try {
    return ::operator new(size);
  } catch (...) {
    return nullptr;
  }
}

void *operator new[](std::size_t size, const std::nothrow_t &) noexcept {
  try {
    return ::operator new[](size);
  } catch (...) {
    return nullptr;
  }
}

void operator delete(void *p, const std::nothrow_t &) noexcept {
  ::operator delete(p);
}

void operator delete[](void *p, const std::nothrow_t &) noexcept {
  ::operator delete[](p);
}

namespace {
using namespace portals;
using gallery::take;
int failures{};

void check(bool condition, const char *message) {
  if (!condition) {
    std::cerr << message << '\n';
    ++failures;
  }
}

std::string row(const test_support::VtOracle::Snapshot &snapshot, int y) {
  std::string result;

  for (int x = 0; x < snapshot.size.width; ++x) {
    const auto &cell = snapshot.cells.at(static_cast<std::size_t>(y * snapshot.size.width + x));

    if (!cell.tail) {
      result += cell.glyph;
    }
  }

  while (!result.empty() && result.back() == ' ') {
    result.pop_back();
  }

  return result;
}

void galleryReplay() {
  gallery::Scene scene;
  test_support::FakeClock clock;
  test_support::TestTerminal terminal(clock, {30, 9});
  FrameWriter writer(terminal);
  auto draw = [&] {
    take(writer.submit(scene.render()));

    const auto written = writer.flush(TerminalDeadline::max());

    check(!written.error && !writer.pending(), "gallery frame completes through real renderer/writer");

    return terminal.snapshot();
  };
  auto initial = draw();

  check(row(initial, 0) == "Portals controls", "gallery heading snapshot");
  check(row(initial, 1) == "Unicode: \xe7\x95\x8c e\xcc\x81", "gallery Unicode snapshot uses grapheme width");
  check(row(initial, 2) == "[ Run ]" && row(initial, 3) == "[ ] Alerts", "initial control snapshot");
  check(row(initial, 4) == "Disabled label" && row(initial, 5) == "[ Disabled button ]" &&
          row(initial, 6) == "[ ] Disabled check",
        "disabled group snapshot");
  check(initial.cells.at(60).style.bold && initial.cells.at(60).style.underline, "initial button focus painted");
  check(initial.cells.at(120).style.italic && initial.cells.at(150).style.italic && initial.cells.at(180).style.italic,
        "disabled ancestor affects label, button and checkbox paint");
  check(take(scene.tree->measure(scene.label, {})).width == 13, "label measures Unicode cells rather than bytes");

  scene.input("\r");
  take(scene.router->dispatch(KeyEvent{InputKey::ENTER, 0, {}, KeyAction::RELEASE}));
  scene.input(" ");
  scene.input("\x1b[<0;2;3M\x1b[<0;2;3m");
  check(scene.calls == 3, "Enter, Space and mouse press each activate once; releases do not");
  take(scene.router->invoke(scene.button, {Action::ACTIVATE}));
  check(scene.calls == 4, "semantic activation shares the same handler");
  scene.input("\x1b[200~ \x1b[201~");
  take(scene.router->dispatch(TextInputEvent{U' ', {.alt = true}}));
  take(scene.router->dispatch(MouseInputEvent{1, 2, MouseButton::LEFT, MouseAction::MOVE, {}}));
  check(scene.calls == 4, "paste, modified Space and mouse motion never activate");

  scene.input("\t ");

  auto keyboard = draw();

  check(scene.changes == 1 && take(scene.model->snapshot())->value, "keyboard toggles authoritative property once");
  check(row(keyboard, 3) == "[x] Alerts" && row(keyboard, 6) == "[x] Disabled check", "shared model paints both views");
  check(keyboard.cells.at(90).style.bold && !keyboard.cells.at(60).style.bold, "Tab focus snapshot");
  check(scene.disabledCalls == 0, "model synchronization is not a business action");

  scene.input("\x1b[<0;2;3M\x1b[<0;2;3m");

  auto mouse = draw();

  check(scene.calls == 5 && mouse.cells.at(60).style.bold, "mouse focuses and activates the painted button once");
  check(take(scene.router->hitTest({1, 2})) == scene.button, "paint and hit geometry coincide");
  scene.input("\x1b[<0;2;4M\x1b[<0;2;4m");

  auto mouse_check = draw();

  check(scene.changes == 2 && !take(scene.model->snapshot())->value && row(mouse_check, 3) == "[ ] Alerts",
        "mouse checkbox press toggles once and release does not toggle again");
  scene.input("\x1b[<0;2;6M");
  take(scene.router->invoke(scene.disabledCheckbox, {Action::ACTIVATE}));
  check(scene.disabledCalls == 0, "router excludes disabled group for mouse and semantic actions");
  take(scene.router->setFocus(scene.button));
  take(scene.tree->setEnabled(scene.button, false));

  auto disabled = draw();

  check(disabled.cells.at(60).style.italic && disabled.cells.at(90).style.bold,
        "disabled button and recovered focus snapshot");
  take(scene.router->invoke(scene.button, {Action::ACTIVATE}));
  check(scene.calls == 5, "disabled button rejects explicit activation");
  take(scene.tree->setVisible(scene.checkbox, false));

  auto hidden = draw();

  check(row(hidden, 3) == "Disabled label", "hidden checkbox leaves real layout and paint");
  take(scene.router->invoke(scene.checkbox, {Action::ACTIVATE}));
  check(scene.changes == 2, "hidden checkbox receives no action");
  check(take(scene.router->focused()).id().session == 0, "no eligible focus remains");
}

void widthPolicyChange() {
  auto scheduler = take(Scheduler::create());
  auto tree = take(WidgetTree::create(*scheduler));
  const auto root = take(tree->root());
  const auto node = take(tree->emplace<Button>(root, gallery::text("Narrow")));
  WidthPolicy policy;

  policy.ambiguous = AmbiguousWidth::WIDE;
  policy.revision = 7;
  take(setControlText(*tree, node, take(GraphemeText::from("\xc2\xa1", policy))));

  const auto viewport = take(CellRect::from({}, {6, 1}));
  std::optional<CellBuffer> buffer;

  take(tree->frame([&] {
    take(layoutTree(*tree, root, viewport, viewport));

    std::vector<RenderCommand> commands;
    gallery::SurfaceSink sink(take(tree->inspect(node)).geometry, commands);

    take(tree->paint(node, sink));
    buffer.emplace(take(renderFullFrame({6, 1}, commands, policy)));
  }));

  check(take(tree->measure(node, {})).width == 6, "changed width policy measures text plus decorations in cells");
  check(take(buffer->at({2, 0})).glyph == "\xc2\xa1" && take(buffer->at({3, 0})).cell.kind == CellKind::CONTINUATION &&
          take(buffer->at({5, 0})).glyph == "]",
        "all decorations use changed text policy and wide cluster remains whole");
}

void textAndInvalidation() {
  gallery::Scene scene;
  std::string owned = "owned";

  take(setControlText(*scene.tree, scene.label, gallery::text(owned)));
  owned.assign("changed externally");
  take(scene.tree->read(scene.label, [](const Label &label) {
    check(label.text().bytes() == "owned", "control owns its text");
  }));
  take(scene.tree->consumeFrameRequest());
  take(setControlText(*scene.tree, scene.label, gallery::text("owned")));
  check(take(scene.tree->consumeFrameRequest()) == Invalidation::NONE, "equal text is no-op");
  take(setControlText(*scene.tree, scene.label, gallery::text("new")));
  check(take(scene.tree->consumeFrameRequest()) == Invalidation::LAYOUT, "new text invalidates layout");

  ControlStyle style;

  style.normal.bold = true;
  take(setControlStyle(*scene.tree, scene.label, style));
  check(take(scene.tree->consumeFrameRequest()) == Invalidation::PAINT, "style invalidates only paint");
  take(setControlStyle(*scene.tree, scene.label, style));
  check(take(scene.tree->consumeFrameRequest()) == Invalidation::NONE, "equal style is no-op");
  take(scene.tree->frame([&] {
    auto synced = syncControl(*scene.tree, scene.label, {});
    auto model = scene.checkBinding.sync();
    auto text = setControlText(*scene.tree, scene.label, gallery::text("blocked"));

    check(!synced && synced.error() == TreeError::WOULD_BLOCK && !model && model.error() == TreeError::WOULD_BLOCK &&
            !text && text.error() == TreeError::WOULD_BLOCK,
          "all control mutations reject active frame");
  }));
  take(setControlText(*scene.tree, scene.label, gallery::text("\x1b[31m")));

  auto rendered = scene.render();

  check(take(rendered.at({0, 1})).glyph == "?", "ESC remains rendered placeholder, never raw VT");

  auto narrow = scene.render({1, 9});

  check(take(narrow.at({0, 2})).glyph == "[", "narrow viewport clips control geometry safely");
}

struct Fixture {
  std::unique_ptr<Scheduler> scheduler;
  std::unique_ptr<WidgetTree> tree;
  Node<Widget> root;
  std::unique_ptr<EventRouter> router;
  Node<CheckBox> node;
  std::shared_ptr<Property<bool>> model;

  explicit Fixture(std::size_t capacity = 8) {
    SchedulerOptions options;

    options.data_capacity = capacity;
    scheduler = take(Scheduler::create(options));
    tree = take(WidgetTree::create(*scheduler));
    root = take(tree->root());
    router = take(EventRouter::create(*tree));
    node = take(tree->emplace<CheckBox>(root, gallery::text("Flag")));
    model = take(Property<bool>::create(*tree, root, false, Invalidation::PAINT));
  }

  bool checked() {
    bool value{};

    take(tree->read(node, [&](const CheckBox &control) {
      value = control.checked();
    }));

    return value;
  }
};

void bindingAndOverflow() {
  Fixture f(1);
  int calls{};
  std::optional<CheckBoxResult> receipt;
  auto binding = take(bindCheckbox(*f.tree, *f.router, f.node, f.model, [&](CheckBoxResult result) {
    ++calls;
    receipt = std::move(result);
  }));

  take(f.tree->consumeFrameRequest());
  take(binding.sync());
  check(take(f.tree->consumeFrameRequest()) == Invalidation::NONE, "equal checkbox sync is no-op");
  take(f.model->set(true));

  auto full = take(f.model->set(false));

  check(full.rejected == 1 && !f.checked(), "full queue commits external value despite notification loss");
  take(f.router->invoke(f.node, {Action::ACTIVATE}));
  check(calls == 1 && receipt && *receipt && (*receipt)->rejected == 1 && (*receipt)->snapshot->value && f.checked(),
        "user toggle commits latest value and reports full queue without ambiguous presentation");
  take(f.model->set(false));
  take(binding.sync());
  check(!f.checked(), "explicit sync recovers newest external value under overflow");
  take(f.scheduler->dispatch());
  check(!f.checked() && calls == 1, "old queued notification neither rolls back view nor duplicates business action");
  take(f.model->set(true));
  take(f.scheduler->dispatch());
  check(f.checked() && calls == 1, "external update synchronizes through ordinary subscription");
  check(f.scheduler->metrics().data.high_water == 1, "property queue remains bounded");

  bool wrong_thread{};
  std::thread worker([&] {
    auto result = binding.sync();

    wrong_thread = !result && result.error() == TreeError::WRONG_THREAD;
  });

  worker.join();
  check(wrong_thread, "binding sync respects UI affinity");
}

struct WatchedButton : Button {
  bool &destroyed;

  explicit WatchedButton(bool &flag) : Button(gallery::text("Delete")), destroyed(flag) {
  }

  ~WatchedButton() override {
    destroyed = true;
  }
};

struct OnDestroy {
  std::function<void()> action;

  ~OnDestroy() {
    action();
  }
};

void callbackLifetimes() {
  Fixture f;
  const auto parent = take(f.tree->emplace<Widget>(f.root));
  bool destroyed{};
  const auto button = take(f.tree->emplace<WatchedButton>(parent, destroyed));
  ActionBinding token;
  int released{};
  auto captured = std::make_shared<OnDestroy>();

  captured->action = [&] {
    ++released;
    take(f.tree->invalidate(f.root, Invalidation::PAINT));
  };
  token = take(bindButton(*f.router, button, [&, captured] {
    token.disconnect();
    take(f.tree->remove(parent));
    check(!destroyed && released == 0, "router borrow retains removed widget and disconnected callback captures");
  }));
  captured.reset();
  take(f.router->invoke(button, {Action::ACTIVATE}));
  take(f.router->focused());
  check(destroyed && released == 1, "UI capture destructor reenters after callback and subtree is reclaimed");

  std::optional<CheckBoxBinding> binding;
  int calls{};

  binding.emplace(take(bindCheckbox(*f.tree, *f.router, f.node, f.model, [&](CheckBoxResult result) {
    ++calls;
    check(take(std::move(result)).snapshot->value, "toggle committed before destructive callback");
    binding.reset();
    take(f.tree->remove(f.node));
    check(take(f.model->snapshot())->value, "committed model survives token and checkbox destruction");
  })));
  take(f.router->invoke(f.node, {Action::ACTIVATE}));
  take(f.scheduler->dispatch());
  check(calls == 1, "deleted checkbox binding admits no later observer or action");

  const auto replacement = take(f.tree->emplace<CheckBox>(f.root, gallery::text("Owner")));
  std::weak_ptr<Property<bool>> weak = f.model;
  auto owner = take(bindCheckbox(*f.tree, *f.router, replacement, f.model, [](CheckBoxResult result) {
    take(std::move(result));
  }));

  f.model.reset();
  check(!weak.expired(), "binding explicitly owns shared model");
  take(owner.sync());
}

void allocationFailures() {
  Fixture f;
  constexpr std::ptrdiff_t PROBE_LIMIT = 40;

  for (int kind = 0; kind != 3; ++kind) {
    bool created{};

    for (std::ptrdiff_t budget = 0; budget < PROBE_LIMIT; ++budget) {
      auto text = gallery::text("Allocated text");
      std::expected<Node<Control>, TreeError> result;

      allocation_test::remaining = budget;

      if (kind == 0) {
        auto node = f.tree->emplace<Label>(f.root, std::move(text));

        result = node ? std::expected<Node<Control>, TreeError>(*node) : std::unexpected(node.error());
      } else if (kind == 1) {
        auto node = f.tree->emplace<Button>(f.root, std::move(text));

        result = node ? std::expected<Node<Control>, TreeError>(*node) : std::unexpected(node.error());
      } else {
        auto node = f.tree->emplace<CheckBox>(f.root, std::move(text));

        result = node ? std::expected<Node<Control>, TreeError>(*node) : std::unexpected(node.error());
      }

      allocation_test::remaining = -1;

      if (result) {
        take(f.tree->remove(*result));
        created = true;

        break;
      }

      check(result.error() == TreeError::NO_MEMORY, "control construction allocation failure is explicit");
    }

    check(created, "control construction eventually succeeds under finite allocation budget");
  }

  bool changed{};

  for (std::ptrdiff_t budget = 0; budget < PROBE_LIMIT; ++budget) {
    auto replacement = gallery::text("Replacement");

    take(f.tree->consumeFrameRequest());
    allocation_test::remaining = budget;

    std::expected<void, TreeError> result;
    bool argument_failed{};

    try {
      result = setControlText(*f.tree, f.node, std::move(replacement));
    } catch (const std::bad_alloc &) {
      argument_failed = true;
    }

    allocation_test::remaining = -1;

    if (!argument_failed && result) {
      changed = true;

      break;
    }

    check(argument_failed || result.error() == TreeError::NO_MEMORY,
          "argument allocation throws; internal text replacement allocation returns no_memory");
    check(take(f.tree->consumeFrameRequest()) == Invalidation::NONE,
          "failed text replacement leaves invalidation unchanged");
    take(f.tree->read(f.node, [](const CheckBox &control) {
      check(control.text().bytes() == "Flag", "failed text replacement retains original owning text");
    }));
  }

  check(changed, "text replacement eventually succeeds under finite allocation budget");
}

void bindingReplacementAndCaptureReentry() {
  Fixture f;
  const auto second = take(f.tree->emplace<CheckBox>(f.root, gallery::text("Second")));
  std::optional<CheckBoxBinding> binding;
  int released{}, new_calls{};
  auto release = std::make_shared<OnDestroy>();

  release->action = [&] {
    ++released;
    binding.reset();
    take(f.tree->invalidate(f.root, Invalidation::PAINT));
  };
  binding.emplace(take(bindCheckbox(*f.tree, *f.router, f.node, f.model, [release](CheckBoxResult result) {
    take(std::move(result));
  })));
  release.reset();

  auto replacement = take(bindCheckbox(*f.tree, *f.router, second, f.model, [&](CheckBoxResult result) {
    take(std::move(result));
    ++new_calls;
  }));

  *binding = std::move(replacement);
  take(f.router->focused());
  check(!binding && released == 1, "old callback capture may destroy the replacement binding during UI reclamation");
  take(f.router->invoke(second, {Action::ACTIVATE}));
  take(f.scheduler->dispatch());
  check(new_calls == 0, "reentry disconnected replacement action and subscription");

  std::weak_ptr<Property<bool>> weak = f.model;

  f.model.reset();
  take(f.router->focused());
  check(weak.expired(), "disconnected bindings release their model after router reclamation");
}

void precommitFailure() {
  Fixture f;
  const auto source = take(f.tree->emplace<Widget>(f.root));
  auto model =
    std::shared_ptr<Property<bool>>(take(Property<bool>::create(*f.tree, source, false, Invalidation::PAINT)));
  int failures_seen{};
  auto binding = take(bindCheckbox(*f.tree, *f.router, f.node, model, [&](CheckBoxResult result) {
    check(!result && result.error() == TreeError::STALE_NODE, "precommit error is explicit");
    ++failures_seen;
  }));

  take(f.tree->remove(source));
  take(f.router->invoke(f.node, {Action::ACTIVATE}));
  check(failures_seen == 1 && !f.checked() && !take(model->snapshot())->value,
        "failed set leaves model and checkbox unchanged");
}
} // namespace

int main() {
  galleryReplay();
  textAndInvalidation();
  widthPolicyChange();
  bindingAndOverflow();
  callbackLifetimes();
  precommitFailure();
  allocationFailures();
  bindingReplacementAndCaptureReentry();

  return failures ? 1 : 0;
}

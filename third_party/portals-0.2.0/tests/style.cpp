// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include "support/vt_oracle.hpp"
#include <portals/controls.hpp>
#include <portals/diff.hpp>
#include <portals/line_edit.hpp>
#include <portals/style.hpp>
#include <portals/virtual_table.hpp>

#include <array>
#include <fstream>
#include <iostream>
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
int failures{};

void check(bool condition, const char *message) {
  if (!condition) {
    std::cerr << message << '\n';
    ++failures;
  }
}

template <class T, class E> T take(std::expected<T, E> result) {
  if (!result) {
    std::cerr << "fixture failed\n";
    std::abort();
  }

  if constexpr (!std::is_void_v<T>) {
    return std::move(*result);
  }
}

struct Fixture {
  std::unique_ptr<Scheduler> scheduler = take(Scheduler::create());
  std::unique_ptr<WidgetTree> tree = take(WidgetTree::create(*scheduler));
  Node<Widget> root = take(tree->root());
  StyleEngine styles{*tree};
};

ComputedStyle readStyle(Fixture &fixture, Node<Widget> node) {
  ComputedStyle result;

  take(fixture.tree->read(node, [&](const Widget &widget) {
    result = widget.computedStyle().value();
  }));

  return result;
}

struct Sink : PaintContext {
  std::vector<RenderCommand> commands;

  std::expected<void, TreeError> submit(std::span<const RenderCommand> values) override {
    for (const auto &command : values) {
      commands.push_back(command);
    }

    return {};
  }
};

CellBuffer paint(Fixture &fixture, Node<Widget> node, CellSize size = {16, 3}) {
  std::optional<CellBuffer> result;
  const auto bounds = take(CellRect::from({}, size));

  take(fixture.tree->frame([&] {
    take(fixture.tree->arrange(fixture.root, bounds, bounds));
    take(fixture.tree->arrange(node, bounds, bounds));
    Sink sink;

    take(fixture.tree->paint(node, sink));
    result.emplace(take(renderFullFrame(size, sink.commands)));
  }));

  return std::move(*result);
}

void parserAndCascade(const char *themeDirectory) {
  auto parsed = take(parsePss("Button { foreground: #ff0000; min-width: 12; }"));
  auto style = resolveStyle(parsed, {"Button", "", {}});

  check(style.normal.foreground == RgbColor{255, 0, 0} && style.minWidth == 12,
        "PSS affects computed paint and layout values");

  auto bad = parsePss("Button {\n foreground: banana;\n}");

  check(!bad && bad.error().line == 2 && bad.error().column == 14,
        "malformed value reports exact one-based line and byte column");

  for (std::string_view source :
       {"Button { x: true; }", "Button:selected { min-width: 1; }", "Button { min-width: -1; }",
        "Button { min-width: 2147483648; }", "Button { foreground: #000000 }", "Button > Label {}", "/* unfinished",
        "Button:unknown {}", "Button#a#b {}", "Button:focused:normal {}"}) {
    check(!parsePss(source), "strict v0 rejects unsupported grammar and values");
  }

  auto sheet = take(parsePss("* {foreground: #000001; background: #000002; bold: true;}"
                             ".primary {foreground: #000003;} Button.primary.hot:focused {foreground: #000004;}"
                             "#save {foreground: #000005;} Button#save.primary.hot:focused {foreground: #000006;}"
                             "#save {background: #000007; bold: false;}"));
  auto matching = resolveStyle(sheet, {"Button", "save", {"hot", "primary"}});

  check(matching.normal.foreground == RgbColor{0, 0, 5} && matching.focused.foreground == RgbColor{0, 0, 6} &&
          matching.focused.background == RgbColor{0, 0, 7} && !matching.normal.bold,
        "compound selector specificity cascades each declaration independently");
  check(resolveStyle(sheet, {"Label", "", {}}).normal.foreground == RgbColor{0, 0, 1},
        "unmatched compound rules do not leak to another type");

  for (const auto name : {"modern", "turbo"}) {
    std::ifstream input(std::string(themeDirectory) + "/" + name + ".pss");
    std::string source{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};

    check(input.good() || input.eof(), "shipped theme is readable");
    check(!source.empty(), "shipped theme contains rules");

    auto fromPss = take(parsePss(source));
    auto fromCpp = std::string_view(name) == "modern" ? modernTheme() : turboTheme();

    for (const auto type : {"Button", "CheckBox", "LineEdit", "Table", "Custom"}) {
      check(resolveStyle(fromPss, {type, "", {}}) == resolveStyle(fromCpp, {type, "", {}}),
            "typed factory and shipped PSS give identical complete styles");
    }
  }
}

void transactionsAndControls() {
  Fixture fixture;
  ControlStyle manual;

  manual.normal.underline = true;

  auto first = take(fixture.tree->emplace<Button>(fixture.root, take(GraphemeText::from("Save")), manual));
  auto second = take(fixture.tree->emplace<CheckBox>(fixture.root, take(GraphemeText::from("Flag"))));
  auto router = take(EventRouter::create(*fixture.tree));
  int activations{};
  auto action = take(bindButton(*router, first, [&] {
    ++activations;
  }));

  take(fixture.styles.bind(first, {"Button", "save", {"primary"}}));
  take(fixture.styles.bind(second, {"CheckBox", "flag", {}}));
  take(fixture.tree->consumeFrameRequest());
  check(take(fixture.styles.replacePss("* {foreground: #123456;}")) == Invalidation::PAINT,
        "color-only replacement requests paint");
  check(take(fixture.tree->consumeFrameRequest()) == Invalidation::PAINT, "paint does not request layout");
  check(readStyle(fixture, first).normal.underline, "unspecified declarations preserve manual options");

  const auto before = readStyle(fixture, first);
  const auto revision = fixture.styles.revision();
  auto invalid = fixture.styles.replacePss("* {foreground: #ffffff;}\nButton { min-width: wrong; }");

  check(!invalid && invalid.error().line == 2 && fixture.styles.revision() == revision &&
          readStyle(fixture, first) == before && take(fixture.tree->consumeFrameRequest()) == Invalidation::NONE,
        "parse failure retains whole sheet, revision, widgets and pending work");
  take(fixture.tree->frame([&] {
    auto rejected = fixture.styles.replace(turboTheme());

    check(!rejected && rejected.error().cause == TreeError::WOULD_BLOCK, "theme switch during a frame is rejected");
    take(fixture.tree->read(first, [&](const Widget &widget) {
      check(widget.styleRevision() == revision, "first node retains frame style revision");
    }));
    take(fixture.tree->read(second, [&](const Widget &widget) {
      check(widget.styleRevision() == revision, "second node retains same frame style revision");
    }));
  }));

  check(take(fixture.styles.replacePss("* {foreground: #123456;} Button {min-width: 20;}")) == Invalidation::LAYOUT,
        "metric replacement requests layout");
  check(take(fixture.tree->measure(first, {100, 3})).width == 20, "style metric affects actual widget measurement");

  auto buffer = paint(fixture, first, {24, 1});

  check(take(buffer.at({0, 0})).glyph == "[" && take(buffer.at({2, 0})).glyph == "S" &&
          take(buffer.at({2, 0})).style.foreground == RgbColor{0x12, 0x34, 0x56},
        "actual ASCII button and caption paint resolved style");
  take(router->invoke(first, {Action::ACTIVATE}));
  take(fixture.styles.replace(turboTheme()));
  take(router->invoke(first, {Action::ACTIVATE}));
  check(activations == 2, "theme switches do not replace or invoke actions");
  take(syncControl(*fixture.tree, first, first));
  buffer = paint(fixture, first);
  check(take(buffer.at({2, 0})).style == readStyle(fixture, first).focused,
        "focused state drives actual control paint");
  take(fixture.tree->setEnabled(first, false));
  take(syncControl(*fixture.tree, first, first));
  buffer = paint(fixture, first);
  check(take(buffer.at({2, 0})).style == readStyle(fixture, first).disabled,
        "disabled state drives actual control paint");

  std::thread foreign([&] {
    auto result = fixture.styles.refresh();

    check(!result && result.error().cause == TreeError::WRONG_THREAD, "style mutation checks UI affinity");
  });

  foreign.join();
  take(fixture.tree->remove(second));
  take(fixture.styles.refresh());
  check(!fixture.styles.bind(second, {"CheckBox", "", {}}), "stale target cannot be rebound by slot reuse");
}

class CustomDefaults : public Widget {
public:
  WidgetTree *tree{};
  Node<Widget> target;
  StyleEngine *engine{};
  bool fail{};
  bool allocationFailure{};
  mutable bool mutationRejected{};
  mutable bool reentryRejected{};

  ComputedStyle styleDefaults() const override {
    auto mutation = tree->remove(target);
    auto reentry = engine->refresh();

    mutationRejected = !mutation && mutation.error() == TreeError::WOULD_BLOCK;
    reentryRejected = !reentry && reentry.error().cause == TreeError::WOULD_BLOCK;

    if (allocationFailure) {
      throw std::bad_alloc();
    }

    if (fail) {
      throw std::runtime_error("custom defaults failed");
    }

    return {};
  }
};

class CountedButton : public Button {
public:
  CountedButton() : Button(take(GraphemeText::from("Counted"))) {
  }

  ComputedStyle styleDefaults() const override {
    ++calls;

    return Control::styleDefaults();
  }

  mutable int calls{};
};

void paintUsesSnapshot() {
  Fixture fixture;
  auto button = take(fixture.tree->emplace<CountedButton>(fixture.root));

  take(fixture.styles.bind(button, {"Button", "", {}}));

  int calls{};

  take(fixture.tree->read(button, [&](const CountedButton &widget) {
    calls = widget.calls;
  }));
  auto frame = paint(fixture, button);

  take(fixture.tree->read(button, [&](const CountedButton &widget) {
    check(widget.calls == calls, "paint consumes staged style without calling custom defaults again");
  }));
}

void customAtomicity() {
  Fixture fixture;
  auto first = take(fixture.tree->emplace<Label>(fixture.root, take(GraphemeText::from("A"))));
  auto custom = take(fixture.tree->emplace<CustomDefaults>(fixture.root));

  take(fixture.tree->edit(custom, [&](CustomDefaults &widget) {
    widget.tree = fixture.tree.get();
    widget.target = first;
    widget.engine = &fixture.styles;
  }));
  take(fixture.styles.bind(first, {"Label", "", {}}));
  take(fixture.styles.bind(custom, {"Custom", "", {}}));
  take(fixture.styles.replace(modernTheme()));
  take(fixture.tree->consumeFrameRequest());

  const auto old = readStyle(fixture, first);
  const auto revision = fixture.styles.revision();

  take(fixture.tree->read(custom, [&](const CustomDefaults &widget) {
    check(widget.mutationRejected && widget.reentryRejected, "custom defaults execute under mutation/reentry guard");
  }));
  take(fixture.tree->edit(custom, [](CustomDefaults &widget) {
    widget.fail = true;
  }));

  auto rejected = fixture.styles.replace(turboTheme());

  check(!rejected && rejected.error().cause == TreeError::CALLBACK_FAILED && fixture.styles.revision() == revision &&
          readStyle(fixture, first) == old && take(fixture.tree->consumeFrameRequest()) == Invalidation::NONE,
        "throwing later custom defaults leaves earlier staged widget and invalidation untouched");
  take(fixture.tree->edit(custom, [](CustomDefaults &widget) {
    widget.allocationFailure = true;
  }));
  rejected = fixture.styles.refresh();
  check(!rejected && rejected.error().cause == TreeError::NO_MEMORY && readStyle(fixture, first) == old,
        "allocation failure in staging leaves prior style intact");
}

void batchTransactions() {
  Fixture fixture;
  auto first = take(fixture.tree->emplace<CountedButton>(fixture.root));
  auto second = take(fixture.tree->emplace<CountedButton>(fixture.root));
  const std::array<StyleRegistration, 2> registrations{{{first, {.type = "Button"}}, {second, {.type = "Button"}}}};
  const auto revision = fixture.styles.revision();

  take(fixture.styles.bindBatch(registrations));

  check(fixture.styles.revision() == revision + 1, "batch commits a single revision");
  take(fixture.tree->read(first, [](const CountedButton &widget) {
    check(widget.calls == 1, "batch recomputes each widget exactly once");
  }));

  take(fixture.styles.replace(modernTheme()));
  check(readStyle(fixture, first) == readStyle(fixture, second), "batch registrations participate in theme updates");
  take(fixture.styles.replacePss("#primary { min-width: 17; }"));

  const std::array<StyleRegistration, 1> update{{{first, {.type = "Button", .id = "primary"}}}};

  take(fixture.styles.bindBatch(update));
  check(readStyle(fixture, first).minWidth == 17 && readStyle(fixture, second).minWidth != 17,
        "batch upserts existing metadata without changing other registrations");

  const auto committedRevision = fixture.styles.revision();
  const std::array<StyleRegistration, 2> duplicate{{{first, {.id = "new"}}, {first, {.id = "other"}}}};
  auto rejected = fixture.styles.bindBatch(duplicate);

  check(!rejected && rejected.error().cause == TreeError::INVALID_ARGUMENT &&
          fixture.styles.revision() == committedRevision && readStyle(fixture, first).minWidth == 17,
        "duplicate batch targets leave registry and values unchanged");

  take(fixture.tree->remove(second));
  rejected = fixture.styles.bindBatch(registrations);
  check(!rejected && rejected.error().cause == TreeError::STALE_NODE &&
          fixture.styles.revision() == committedRevision && readStyle(fixture, first).minWidth == 17,
        "late stale target does not publish earlier metadata");

  auto custom = take(fixture.tree->emplace<CustomDefaults>(fixture.root));

  take(fixture.tree->edit(custom, [&](CustomDefaults &widget) {
    widget.tree = fixture.tree.get();
    widget.target = first;
    widget.engine = &fixture.styles;
    widget.fail = true;
  }));

  const std::array<StyleRegistration, 2> throwing{{{first, {.type = "Button"}}, {custom, {.type = "Custom"}}}};

  take(fixture.tree->consumeFrameRequest());
  rejected = fixture.styles.bindBatch(throwing);
  check(!rejected && rejected.error().cause == TreeError::CALLBACK_FAILED &&
          fixture.styles.revision() == committedRevision && readStyle(fixture, first).minWidth == 17 &&
          take(fixture.tree->consumeFrameRequest()) == Invalidation::NONE,
        "late defaults failure rolls back whole batch including invalidation");
  take(fixture.tree->read(custom, [](const CustomDefaults &widget) {
    check(!widget.computedStyle(), "failed batch does not publish a new widget style");
  }));
  take(fixture.styles.refresh());
  check(readStyle(fixture, first).minWidth == 17, "failed metadata is absent from subsequent refresh");
}

class OrderedDefaults final : public Widget {
public:
  OrderedDefaults(int tag, std::vector<int> &order) : tag_(tag), order_(order) {
  }

  ComputedStyle styleDefaults() const override {
    order_.push_back(tag_);

    return {};
  }

private:
  int tag_;
  std::vector<int> &order_;
};

void batchIdentityAndOrder() {
  std::vector<int> order;
  Fixture fixture;
  auto first = take(fixture.tree->emplace<OrderedDefaults>(fixture.root, 1, order));
  auto second = take(fixture.tree->emplace<OrderedDefaults>(fixture.root, 2, order));
  auto third = take(fixture.tree->emplace<OrderedDefaults>(fixture.root, 3, order));
  const std::array initial{StyleRegistration{second, {.id = "second"}}, StyleRegistration{first, {.id = "first"}}};

  take(fixture.styles.bindBatch(initial));
  check(order == std::vector{2, 1}, "initial defaults follow registration order, not slot order");
  order.clear();
  take(fixture.styles.replacePss("#changed {min-width: 13;} #new {min-width: 19;}"));
  order.clear();

  const std::array mixed{StyleRegistration{third, {.id = "new"}}, StyleRegistration{first, {.id = "changed"}},
                         StyleRegistration{second, {.id = "second"}}};

  take(fixture.styles.bindBatch(mixed));
  check(order == std::vector{2, 1, 3}, "updates keep prior order and insertions append in input order");
  check(readStyle(fixture, first).minWidth == 13 && readStyle(fixture, third).minWidth == 19 &&
          readStyle(fixture, second).minWidth == 0,
        "mixed batch maps each metadata update to its own identity");
  take(fixture.tree->remove(second));

  auto reused = take(fixture.tree->emplace<OrderedDefaults>(fixture.root, 4, order));

  check(reused.id().slot == second.id().slot && reused.id().generation != second.id().generation,
        "fixture reuses the removed slot with a different generation");
  order.clear();
  take(fixture.styles.bindBatch(std::array{StyleRegistration{reused, {.id = "new"}}}));
  check(order == std::vector{1, 3, 4}, "stale binding is pruned and replacement appends as a new identity");
  take(fixture.tree->consumeFrameRequest());
  order.clear();

  const auto revision = fixture.styles.revision();
  const std::array duplicateBeforeStale{StyleRegistration{first, {}}, StyleRegistration{first, {}},
                                        StyleRegistration{second, {}}};
  auto rejected = fixture.styles.bindBatch(duplicateBeforeStale);

  check(!rejected && rejected.error().cause == TreeError::INVALID_ARGUMENT,
        "earlier duplicate wins over later stale target");

  const std::array staleBeforeDuplicate{StyleRegistration{second, {}}, StyleRegistration{first, {}},
                                        StyleRegistration{first, {}}};

  rejected = fixture.styles.bindBatch(staleBeforeDuplicate);
  check(!rejected && rejected.error().cause == TreeError::STALE_NODE, "earlier stale target wins over later duplicate");
  check(order.empty() && fixture.styles.revision() == revision &&
          take(fixture.tree->consumeFrameRequest()) == Invalidation::NONE && readStyle(fixture, first).minWidth == 13 &&
          readStyle(fixture, reused).minWidth == 19,
        "validation failures call no defaults and preserve styles, revision and invalidation");

  allocation_test::remaining = 0;
  rejected = fixture.styles.bindBatch(duplicateBeforeStale);
  allocation_test::remaining = -1;

  check(!rejected && rejected.error().cause == TreeError::INVALID_ARGUMENT,
        "allocation pressure does not mask an earlier duplicate argument");

  const std::array liveBeforeStale{StyleRegistration{first, {}}, StyleRegistration{second, {}}};

  allocation_test::remaining = 0;
  rejected = fixture.styles.bindBatch(liveBeforeStale);
  allocation_test::remaining = -1;

  check(!rejected && rejected.error().cause == TreeError::STALE_NODE,
        "allocation pressure does not mask a stale argument after a live target");
}

void batchBudgetAndAllocation() {
  Fixture fixture;
  StyleEngine styles(*fixture.tree, 2);
  auto first = take(fixture.tree->emplace<Widget>(fixture.root));
  auto second = take(fixture.tree->emplace<Widget>(fixture.root));
  auto third = take(fixture.tree->emplace<Widget>(fixture.root));
  const std::array<StyleRegistration, 2> initial{{{first, {.type = "Widget"}}, {second, {.type = "Widget"}}}};

  take(styles.bindBatch(initial));

  const std::array<StyleRegistration, 2> replacement{{{first, {.id = "updated"}}, {third, {.type = "Widget"}}}};
  auto rejected = styles.bindBatch(replacement);

  check(!rejected && rejected.error().cause == TreeError::NODE_LIMIT && styles.revision() == 1,
        "over-budget batch retains all previous bindings");
  take(fixture.tree->remove(second));
  take(fixture.tree->consumeFrameRequest());

  const auto revision = styles.revision();
  const auto previous = readStyle(fixture, first);
  int failuresSeen{};
  bool committed{};

  for (std::ptrdiff_t budget = 0; budget < 160 && !committed; ++budget) {
    allocation_test::remaining = budget;

    auto result = styles.bindBatch(replacement);

    allocation_test::remaining = -1;

    if (result) {
      committed = true;
    } else {
      ++failuresSeen;
      check(result.error().cause == TreeError::NO_MEMORY && styles.revision() == revision &&
              readStyle(fixture, first) == previous && take(fixture.tree->consumeFrameRequest()) == Invalidation::NONE,
            "mixed batch allocation failures preserve revision, styles and invalidation");
      take(fixture.tree->read(third, [](const Widget &widget) {
        check(!widget.computedStyle(), "failed insertion publishes no style to the new target");
      }));
    }
  }

  check(committed && failuresSeen >= 2 && styles.revision() == revision + 1,
        "batch allocation sweep reaches one complete commit");
  check(styles.revision() == 2, "batch prunes stale bindings before enforcing capacity");

  const auto committedRevision = styles.revision();
  bool wrongThread{};
  std::thread worker([&] {
    auto result = styles.bindBatch(replacement);

    wrongThread = !result && result.error().cause == TreeError::WRONG_THREAD;
  });

  worker.join();
  check(wrongThread && styles.revision() == committedRevision, "batch rejects wrong-thread calls before staging");
  take(fixture.tree->frame([&] {
    auto result = styles.bindBatch(replacement);

    check(!result && result.error().cause == TreeError::WOULD_BLOCK && styles.revision() == committedRevision,
          "batch rejects frame-time calls before staging");
  }));
  take(styles.bindBatch({}));
  check(styles.revision() == committedRevision + 1, "empty batch performs one ordinary refresh");
}

struct Page : ModelSnapshot<int> {
  std::vector<ModelRow<int>> rows{{{1, 1}, 7}, {{2, 1}, 8}};

  Page() : ModelSnapshot(1, 2) {
  }

  ModelRows<int> read(RowRange range) const override {
    return {RangeState::READY, std::span(rows).subspan(static_cast<std::size_t>(range.first), range.count), {}};
  }
};

void adaptedPaletteVariants() {
  struct PaletteCase {
    const char *name;
    StyleSheet (*create)();
    ThemeTokens colors;
  };

  // Pinned Textual RGB values are independent expectations for the public factories.
  const std::array palettes{PaletteCase{"Nord",
                                        nordTheme,
                                        {{0xD8, 0xDE, 0xE9},
                                         {0x2E, 0x34, 0x40},
                                         {0x88, 0xC0, 0xD0},
                                         {0x81, 0xA1, 0xC1},
                                         {0x43, 0x4C, 0x5E},
                                         {0xB4, 0x8E, 0xAD}}},
                            PaletteCase{"Gruvbox",
                                        gruvboxTheme,
                                        {{0xFB, 0xF1, 0xC7},
                                         {0x28, 0x28, 0x28},
                                         {0x85, 0xA5, 0x98},
                                         {0xA8, 0x9A, 0x85},
                                         {0x50, 0x49, 0x45},
                                         {0xFA, 0xBD, 0x2F}}},
                            PaletteCase{"Tokyo Night",
                                        tokyoNightTheme,
                                        {{0xA9, 0xB1, 0xD6},
                                         {0x1A, 0x1B, 0x26},
                                         {0xBB, 0x9A, 0xF7},
                                         {0x7A, 0xA2, 0xF7},
                                         {0x41, 0x48, 0x68},
                                         {0xFF, 0x9E, 0x64}}},
                            PaletteCase{"Catppuccin Latte",
                                        catppuccinLatteTheme,
                                        {{0x4C, 0x4F, 0x69},
                                         {0xEF, 0xF1, 0xF5},
                                         {0x88, 0x39, 0xEF},
                                         {0xDC, 0x8A, 0x78},
                                         {0xCC, 0xD0, 0xDA},
                                         {0xFE, 0x64, 0x0B}}}};

  for (const auto &palette : palettes) {
    const auto failuresBefore = failures;
    const auto &colors = palette.colors;
    const auto style = resolveStyle(palette.create(), {.type = "Table"});

    check(style.normal == CellStyle{colors.foreground, colors.background}, "palette normal colors match source");
    check(style.focused == CellStyle{colors.background, colors.accent, true}, "palette focus reverses primary colors");
    check(style.disabled == CellStyle{colors.muted, colors.background}, "palette disabled uses secondary color");
    check(style.selected == CellStyle{colors.foreground, colors.selection, true}, "palette selection uses panel color");
    check(style.header == CellStyle{colors.heading, colors.background, true}, "palette header uses source accent");
    check(style.caret == CellStyle{colors.accent, colors.background}, "palette caret reverses focused field");
    check(style.minWidth == 0, "palette replacement does not introduce a layout metric");

    if (failures != failuresBefore) {
      std::cerr << "Palette: " << palette.name << '\n';
    }
  }
}

void focusedEditorCaretIsVisible() {
  for (const auto &theme :
       {modernTheme(), turboTheme(), nordTheme(), gruvboxTheme(), tokyoNightTheme(), catppuccinLatteTheme()}) {
    Fixture fixture;
    auto editor = take(fixture.tree->emplace<LineEdit>(fixture.root));
    auto router = take(EventRouter::create(*fixture.tree));
    std::shared_ptr<Property<std::string>> model =
      take(Property<std::string>::create(*fixture.tree, fixture.root, "Portals", Invalidation::PAINT));
    auto binding = take(bindLineEdit(*fixture.tree, *router, editor, model, [](LineEditResult) {
    }));

    take(fixture.styles.bind(editor, {.type = "Control"}));
    take(fixture.styles.replace(theme));
    take(router->setFocus(editor));
    take(binding.sync(take(router->focused())));
    take(router->dispatch(KeyEvent{InputKey::LEFT, 0, {}, KeyAction::PRESS}));
    take(router->dispatch(KeyEvent{InputKey::LEFT, 0, {}, KeyAction::PRESS}));

    // Exercise the reported 'l' position and the blank insertion cell after the draft.
    for (const auto position : {5, 7}) {
      if (position == 7) {
        take(router->dispatch(KeyEvent{InputKey::END, 0, {}, KeyAction::PRESS}));
      }

      auto buffer = paint(fixture, editor, {16, 1});

      for (const auto mode : {ColorMode::TRUE_COLOR, ColorMode::ANSI16}) {
        test_support::VtOracle terminal(16, 1);

        terminal.feed(take(encodeDiff(nullptr, buffer, 4096, mode)));

        const auto snapshot = terminal.snapshot();
        const auto &caret = snapshot.cells[position].style;
        const auto &field = snapshot.cells[4].style;

        check(caret.background != field.background && caret.foreground != caret.background,
              "focused editor caret contrasts with field and text in every built-in theme and color mode");
      }
    }
  }
}

void viewsAndEditor() {
  Fixture fixture;
  auto model = take(Model<int>::create(*fixture.tree, fixture.root, std::make_shared<Page>()));
  std::vector<TableColumn<int>> columns{{{1, 1}, take(GraphemeText::from("Value")), 8, [](const int &value) {
                                           return take(GraphemeText::from(std::to_string(value)));
                                         }}};
  auto table =
    take(fixture.tree->emplace<VirtualTable<int>>(fixture.root, take(model->openRange()), std::move(columns)));
  auto list =
    take(fixture.tree->emplace<VirtualList<int>>(fixture.root, take(model->openRange()), [](const ModelRow<int> &row) {
      return take(GraphemeText::from(std::to_string(row.value)));
    }));
  auto editor = take(fixture.tree->emplace<LineEdit>(fixture.root));
  auto router = take(EventRouter::create(*fixture.tree));
  std::shared_ptr<Property<std::string>> textModel =
    take(Property<std::string>::create(*fixture.tree, fixture.root, std::string("Edit"), Invalidation::PAINT));
  auto editorBinding = take(bindLineEdit(*fixture.tree, *router, editor, textModel, [](LineEditResult) {
  }));

  take(editorBinding.sync({}));

  take(fixture.styles.bind(table, {"Table", "", {}}));
  take(fixture.styles.bind(list, {"List", "", {}}));
  take(fixture.styles.bind(editor, {"LineEdit", "", {}}));
  take(fixture.styles.replace(turboTheme()));
  take(setVirtualTableSelection(*fixture.tree, table, RowId{1, 1}));
  take(setVirtualListSelection(*fixture.tree, list, RowId{1, 1}));
  take(prepareVirtualTable(*fixture.tree, table, take(model->pin()), {16, 3}));
  take(prepareVirtualList(*fixture.tree, list, take(model->pin()), {16, 3}));

  auto tableBuffer = paint(fixture, table);
  auto listBuffer = paint(fixture, list);
  auto editorBuffer = paint(fixture, editor);

  check(take(tableBuffer.at({0, 0})).style == readStyle(fixture, table).header &&
          take(tableBuffer.at({0, 1})).style == readStyle(fixture, table).selected &&
          take(tableBuffer.at({0, 2})).style == readStyle(fixture, table).normal,
        "real table header/selected/body paints use resolved states");
  check(take(listBuffer.at({0, 0})).style == readStyle(fixture, list).selected,
        "real virtual list paints selected state without model reformatting");
  check(take(editorBuffer.at({0, 0})).style == readStyle(fixture, editor).normal,
        "real editor paints theme without changing draft");
  take(fixture.styles.replace(modernTheme()));
  tableBuffer = paint(fixture, table);
  check(take(tableBuffer.at({0, 0})).style == readStyle(fixture, table).header,
        "paint-only theme update needs no new table model preparation");
}

class PartialOutput : public FrameOutput {
public:
  std::string pending;
  std::string delivered;
  test_support::VtOracle terminal{1, 1};

  std::expected<void, TerminalError> submit(std::string bytes) override {
    pending = std::move(bytes);

    return {};
  }

  TerminalWrite flush(TerminalDeadline, std::stop_token) override {
    const auto count = std::min<std::size_t>(3, pending.size());

    terminal.feed(std::string_view(pending).substr(0, count));
    delivered.append(pending, 0, count);
    pending.erase(0, count);

    return {pending.empty() ? TerminalStatus::PROGRESS : TerminalStatus::DEADLINE, count, pending.size(), {}};
  }
};

void allocationRollback() {
  Fixture fixture;
  auto button = take(fixture.tree->emplace<Button>(fixture.root, take(GraphemeText::from("A"))));

  take(fixture.styles.bind(button, {"Button", "save", {"primary"}}));
  take(fixture.styles.replace(modernTheme()));
  take(fixture.tree->consumeFrameRequest());

  const auto old = readStyle(fixture, button);
  const auto revision = fixture.styles.revision();
  bool completed{};
  int failuresObserved{};

  for (int budget = 0; budget < 256; ++budget) {
    allocation_test::remaining = budget;

    auto result = fixture.styles.replacePss("* {foreground: #ffffff;} Button.primary#save {min-width: 12;}");

    allocation_test::remaining = -1;

    if (result) {
      completed = true;

      break;
    }

    ++failuresObserved;
    check(result.error().cause == TreeError::NO_MEMORY, "injected allocation failure is an observable style error");
    check(fixture.styles.revision() == revision && readStyle(fixture, button) == old &&
            take(fixture.tree->consumeFrameRequest()) == Invalidation::NONE,
          "every failed allocation preserves revision, style and invalidation");
  }

  check(completed && failuresObserved != 0, "allocation sweep reaches a complete atomic commit");
  std::cout << "Allocation sweep: budgets 0.." << failuresObserved - 1
            << " returned NO_MEMORY without mutation; commit at budget " << failuresObserved << '\n';
}

void ansiOutput() {
  auto makeFrame = [](RgbColor foreground, RgbColor background) {
    auto buffer = take(CellBuffer::from({1, 1}));
    auto glyph = take(GraphemeText::from("x"));

    take(buffer.put({}, glyph, 0, {foreground, background}));

    return buffer;
  };
  auto old = makeFrame({255, 255, 85}, {0, 0, 170});
  auto next = makeFrame({85, 255, 255}, {255, 85, 255});
  auto full = take(encodeDiff(nullptr, old, 4096, ColorMode::ANSI16));
  auto delta = take(encodeDiff(&old, next, 4096, ColorMode::ANSI16));

  check(full.find(";93;44m") != std::string::npos && delta.find(";96;105m") != std::string::npos,
        "ANSI16 uses actual ordinary and bright SGR codes in full and incremental output");
  check(full.find("38;2") == std::string::npos && delta.find("48;2") == std::string::npos,
        "ANSI16 never emits true-color SGR");
  check(take(encodeDiff(nullptr, old)).find("38;2;255;255;85") != std::string::npos,
        "default encoder remains true color");
  check(take(encodeDiff(&old, old, 4096, ColorMode::ANSI16)).empty(), "unchanged ANSI16 frame has no output");

  test_support::VtOracle fullOracle(1, 1);
  test_support::VtOracle incrementalOracle(1, 1);

  fullOracle.feed(take(encodeDiff(nullptr, next, 4096, ColorMode::ANSI16)));
  incrementalOracle.feed(full);
  incrementalOracle.feed(delta);
  check(fullOracle.snapshot() == incrementalOracle.snapshot(),
        "independent oracle agrees for full and incremental ANSI16");
  check(fullOracle.snapshot().cells[0].style.foreground == RgbColor{85, 255, 255} &&
          fullOracle.snapshot().cells[0].style.background == RgbColor{255, 85, 255},
        "oracle observes intended palette colors, not just syntactically plausible SGR");

  PartialOutput output;
  FrameWriterLimits limits;

  limits.colorMode = ColorMode::ANSI16;
  FrameWriter writer(output, limits);

  take(writer.submit(std::move(old)));

  while (writer.pending()) {
    auto result = writer.flush(TerminalDeadline::max());

    check(!result.error, "partial ANSI16 output remains retryable");
  }

  check(output.delivered == full && writer.committedFrame(), "partial output commits only complete ANSI16 transaction");
  output.delivered.clear();
  take(writer.submit(std::move(next)));

  while (writer.pending()) {
    (void)writer.flush(TerminalDeadline::max());
  }

  check(output.delivered == delta, "writer retains ANSI16 mode for subsequent incremental frames");
  check(output.terminal.snapshot() == fullOracle.snapshot(),
        "partial output reaches the same independently observed screen");
}
} // namespace

int main(int argc, char **argv) {
  if (argc != 2) {
    return 2;
  }

  parserAndCascade(argv[1]);
  transactionsAndControls();
  customAtomicity();
  batchTransactions();
  batchIdentityAndOrder();
  batchBudgetAndAllocation();
  paintUsesSnapshot();
  viewsAndEditor();
  adaptedPaletteVariants();
  focusedEditorCaretIsVisible();
  ansiOutput();
  allocationRollback();

  return failures ? 1 : 0;
}

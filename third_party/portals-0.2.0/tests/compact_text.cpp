// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

#include "../examples/replay/terminal.hpp"
#include <portals/input_match.hpp>
#include <portals/table_column.hpp>
#include <portals/text_format.hpp>
#include <portals/ui.hpp>

#include <memory>

namespace {
struct Record {
  double price;
  std::string name;
  std::unique_ptr<int> noncopyable;
};

struct TextRow {
  std::string value;
};

class TextPage final : public portals::ModelSnapshot<TextRow> {
public:
  explicit TextPage(std::string value) : ModelSnapshot(1, 1), row_{{1, 1}, {std::move(value)}} {
  }

  portals::ModelRows<TextRow> read(portals::RowRange range) const override {
    return {
      portals::RangeState::READY, std::span(&row_, 1).subspan(static_cast<std::size_t>(range.first), range.count), {}};
  }

private:
  portals::ModelRow<TextRow> row_;
};
} // namespace

TEST_CASE("key matching preserves press repeat release and modifier semantics") {
  using namespace portals;
  constexpr InputEvent PRESS = KeyEvent{.key = InputKey::F2};
  constexpr InputEvent REPEAT = KeyEvent{.key = InputKey::F2, .action = KeyAction::REPEAT};
  constexpr InputEvent RELEASE = KeyEvent{.key = InputKey::F2, .action = KeyAction::RELEASE};

  static_assert(keyPressed(PRESS, InputKey::F2));
  static_assert(keyPressed(REPEAT, InputKey::F2));
  static_assert(!keyPressed(RELEASE, InputKey::F2));
  static_assert(!keyPressed(PRESS, InputKey::LEFT));
  static_assert(keyPressed(PRESS, {InputKey::F1, InputKey::F2}));
  static_assert(!keyPressed(RELEASE, {InputKey::F1, InputKey::F2}));
  static_assert(!keyPressed(PRESS, std::initializer_list<InputKey>{}));
  CHECK_FALSE(keyPressed(InputEvent{TextInputEvent{U'x'}}, InputKey::CHARACTER));
  CHECK_FALSE(keyPressed(InputEvent{MouseInputEvent{}}, InputKey::F2));

  const InputEvent modified = KeyEvent{.key = InputKey::LEFT, .modifiers = {.shift = true}};

  CHECK(keyPressed(modified, InputKey::LEFT));
  CHECK_FALSE(keyPressed(modified, InputKey::LEFT, {}));
  CHECK(keyPressed(modified, InputKey::LEFT, {.shift = true}));
}

TEST_CASE("checked formatting preserves text errors and display policy") {
  using namespace portals;
  const auto valid = formatText("{} {:.2f}", "Price", 12.5);

  REQUIRE(valid);
  CHECK(valid->bytes() == "Price 12.50");

  WidthPolicy policy;

  policy.ambiguous = AmbiguousWidth::WIDE;

  const auto measured = formatText(policy, "{}", "\xc2\xa1");

  REQUIRE(measured);
  CHECK(measured->policy() == policy);
  CHECK(measured->measure() == 2);

  const auto formatter = cellFormat<std::string>("{}", policy);

  CHECK(formatter(std::string("\xc2\xa1")).policy() == policy);

  const std::string invalid(1, static_cast<char>(0xff));
  const auto rejected = formatText("{}", invalid);
  const auto original = GraphemeText::from(invalid);

  REQUIRE_FALSE(rejected);
  REQUIRE_FALSE(original);
  CHECK(rejected.error().kind == original.error().kind);
  CHECK(rejected.error().byte_offset == original.error().byte_offset);
}

TEST_CASE("member deduction owns metadata and checked cell format") {
  using namespace portals;
  const Record row{12.5, "Ada", std::make_unique<int>(42)};
  auto column = tableColumn({8, 3}, "Price", &Record::price, cellFormat<double>("{:.2f}"), 9);

  static_assert(std::same_as<decltype(column), std::expected<TableColumn<Record>, TextError>>);
  REQUIRE(column);
  CHECK(column->id == ColumnId{8, 3});
  CHECK(column->width == 9);
  CHECK(column->header.bytes() == "Price");
  CHECK(column->format(row).bytes() == "12.50");
  CHECK(*row.noncopyable == 42);

  auto deduced =
    tableColumn(std::move(*columnOptions({2, 1}, "Name", 8)), &Record::name, cellFormat<std::string>("{}"));

  CHECK(deduced.format(row).bytes() == "Ada");

  const auto invalidHeader =
    tableColumn({9, 1}, std::string(1, static_cast<char>(0xff)), &Record::price, cellFormat<double>("{}"));

  CHECK_FALSE(invalidHeader);
  CHECK_THROWS_AS(cellFormat<std::string>("{}")(std::string(1, static_cast<char>(0xff))),
                  std::bad_expected_access<TextError>);
}

TEST_CASE("fixed schemas assign local identities without invoking formatters") {
  using namespace portals;
  int calls = 0;
  const auto formatter = [&](double value) {
    ++calls;

    return formatText("{}", value).value();
  };
  auto columns = tableColumns(tableColumn("Same caption", &Record::price, formatter, 9),
                              tableColumn("Same caption", &Record::price, formatter, 12));

  REQUIRE(columns);
  CHECK(calls == 0);
  CHECK((*columns)[0].id == ColumnId{1, 1});
  CHECK((*columns)[1].id == ColumnId{2, 1});
  CHECK((*columns)[1].width == 12);

  std::swap((*columns)[0], (*columns)[1]);

  CHECK((*columns)[0].id == ColumnId{2, 1});

  const auto other = tableColumns(tableColumn("Different locale", &Record::price, formatter));

  REQUIRE(other);
  CHECK((*other)[0].id == ColumnId{1, 1});

  WidthPolicy policy;

  policy.ambiguous = AmbiguousWidth::WIDE;

  const auto wide = tableColumns(policy, tableColumn("\xc2\xa1", &Record::price, formatter));

  REQUIRE(wide);
  CHECK((*wide)[0].header.measure() == 2);

  const std::string invalid(1, static_cast<char>(0xff));
  const auto rejected = tableColumns(tableColumn(invalid, &Record::price, formatter));
  const auto original = GraphemeText::from(invalid);

  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().kind == original.error().kind);
  CHECK(rejected.error().byte_offset == original.error().byte_offset);
  CHECK(calls == 0);
}

TEST_CASE("grid recipes publish observers atomically and preserve placement customization") {
  using namespace portals;
  auto app = Application::create();

  REQUIRE(app);

  auto scene = ManagedScene::create(**app);

  REQUIRE(scene);

  ui::NodeRef<Label> child;
  const auto mounted = ui::mount(**scene, ui::grid(ui::label("one").reference(child), ui::label("two")));

  REQUIRE(mounted);
  CHECK(child.node() != Node<Label>{});

  const auto inspected = (*app)->tree().inspect(*mounted);

  REQUIRE(inspected);
  CHECK(inspected->children.size() == 2);

  GridOptions options;

  options.columns.resize(2);
  options.cells = {{inspected->children[0].id(), 0, 0}, {inspected->children[1].id(), 0, 1}};
  CHECK(setGridOptions((*app)->tree(), *mounted, options));

  ui::NodeRef<Label> rollback;
  const auto failed = ui::mount(**scene, ui::grid(ui::label("not published").reference(rollback),
                                                  ui::label(std::string(1, static_cast<char>(0xff)))));

  CHECK_FALSE(failed);
  CHECK(rollback.node() == Node<Label>{});
  CHECK((*app)->tree().inspect(*mounted));
}

TEST_CASE("cell validation failure follows the existing typed preparation boundary") {
  using namespace portals;
  example::ReplayTerminal terminal({}, 1, {20, 5});
  auto app = Application::create({}, terminal.clock());

  REQUIRE(app);

  auto scene = ManagedScene::create(**app);

  REQUIRE(scene);

  auto model = (*scene)->model<TextRow>(std::make_shared<const TextPage>(std::string(1, static_cast<char>(0xff))));
  auto column = tableColumn({1, 1}, "Value", &TextRow::value, cellFormat<std::string>("{}"), 10);

  REQUIRE(model);
  REQUIRE(column);

  const std::array columns{*column};
  const auto mounted = ui::mount(**scene, ui::virtualTable(*model, columns));

  REQUIRE(mounted);

  const auto result = (*scene)->run(terminal);

  REQUIRE_FALSE(result);
  REQUIRE(result.error().application);
  const auto *cause = std::get_if<TreeError>(&result.error().application->cause);

  REQUIRE(cause);
  CHECK(*cause == TreeError::CALLBACK_FAILED);
  CHECK(terminal.closed);
}

// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include "package_consumer/virtual_support.hpp"
#include <portals/ui.hpp>

using namespace portals;

// Braced legacy options must remain unambiguous after named overloads are added.
static_assert(requires(ManagedModel<int> model, std::span<const TableColumn<int>> columns,
                       ManagedListFormatter<int> format, ManagedChartAccessor<int> accessor) {
  ui::virtualTable(model, columns, {});
  ui::virtualList(model, format, {});
  ui::chart(model, accessor, {});
});

template <class Config>
concept TableConfigAccepted =
  requires(ManagedModel<int> model, std::span<const TableColumn<int>> columns, Config &&config) {
    ui::virtualTable(model, columns, std::forward<Config>(config));
  };

static_assert(TableConfigAccepted<ui::VirtualTableConfig<int>>);
static_assert(TableConfigAccepted<ui::VirtualTableConfig<int> &>);
static_assert(TableConfigAccepted<const ui::VirtualTableConfig<int> &>);
static_assert(!TableConfigAccepted<ui::VirtualTableConfig<double>>);
static_assert(!TableConfigAccepted<ui::ChartConfig<int>>);

namespace {
using namespace virtualConsumer;

template <class Factory> void invalidSettings(Factory factory) {
  ReplayTerminal terminal;
  auto app = take(Application::create({}, terminal.clock()));
  auto scene = take(ManagedScene::create(*app));
  const auto model = take(scene->model<Ohlc>(std::make_shared<const Prices>()));
  const auto mounted = take(ui::mount(*scene, factory(model)));
  const auto result = scene->run(terminal);

  check(!result && result.error().phase == ManagedPhase::PREPARE && result.error().node == mounted.id(),
        "Named settings lost preparation error identity");
  check(std::get<TreeError>(result.error().cause) == TreeError::INVALID_ARGUMENT,
        "Invalid named settings were ignored");
}

void originLifetimeAndError() {
  ReplayTerminal terminal;
  auto app = take(Application::create({}, terminal.clock()));
  auto scene = take(ManagedScene::create(*app));
  const auto model = take(scene->model<Ohlc>(std::make_shared<const Prices>()));
  auto owned = std::make_shared<int>(0);
  const std::weak_ptr<int> observed = owned;
  auto recipe = ui::chart(
    model,
    [](const Ohlc &value) {
      return std::optional{value};
    },
    ui::ChartConfig<Ohlc>{.first = [owned]() -> std::expected<std::uint64_t, TreeError> {
      ++*owned;

      return std::unexpected(TreeError::INVALID_GEOMETRY);
    }});

  owned.reset();
  check(!observed.expired(), "Named chart lost origin capture before mount");

  const auto node = take(ui::mount(*scene, std::move(recipe)));
  const auto result = scene->run(terminal);

  check(!result && result.error().phase == ManagedPhase::PREPARE && result.error().node == node.id(),
        "Named chart origin lost typed error identity");
  check(std::get<TreeError>(result.error().cause) == TreeError::INVALID_GEOMETRY,
        "Named chart origin callback was not forwarded");
  check(observed.expired(), "Named chart retained captures after run cleanup");
}
} // namespace

int main() {
  // Construction must stay valid without a live scene; model validation belongs to mount.
  const ui::VirtualListConfig<int> listConfig{};
  const ui::ChartConfig<int> chartConfig{};
  auto list = ui::virtualList(ManagedModel<int>{}, {}, listConfig);
  auto chart = ui::chart(ManagedModel<int>{}, {}, chartConfig);
  auto table = ui::virtualTable(ManagedModel<int>{}, {}, ui::VirtualTableConfig<int>{});

  (void)list;
  (void)chart;
  (void)table;

  invalidSettings([](ManagedModel<Ohlc> model) {
    ui::VirtualTableConfig<Ohlc> config{};

    config.options.max_columns = 0;

    return ui::virtualTable(model, virtualConsumer::priceColumns(), config);
  });
  invalidSettings([](ManagedModel<Ohlc> model) {
    ui::VirtualListConfig<Ohlc> config{};

    config.options.row_height = 0;

    return ui::virtualList(
      model,
      [](const ModelRow<Ohlc> &) {
        return virtualConsumer::text("row");
      },
      config);
  });
  invalidSettings([](ManagedModel<Ohlc> model) {
    ui::ChartConfig<Ohlc> config{};

    config.options.candle_gap = -1;

    return ui::chart(
      model,
      [](const Ohlc &value) {
        return std::optional{value};
      },
      config);
  });
  originLifetimeAndError();
}

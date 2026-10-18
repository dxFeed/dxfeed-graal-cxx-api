// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

#include <portals/aggregate_columns.hpp>

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace {
using namespace portals;

GraphemeText text(std::string_view value) {
  auto result = GraphemeText::from(value);
  REQUIRE(result.has_value());

  return std::move(*result);
}

struct Row {
  std::string label;
  std::unique_ptr<int> amount;
};

struct GenericFormatter {
  GraphemeText operator()(const std::string &value) const {
    return text(value);
  }

  GraphemeText operator()(const std::unique_ptr<int> &value) const {
    return text(std::to_string(*value));
  }
};

struct WrongResult {
  std::string operator()(const std::string &) const;
  std::string operator()(const std::unique_ptr<int> &) const;
};

struct PartialFormatter {
  GraphemeText operator()(const std::string &) const;
};

struct MoveOnlyFormatter {
  std::unique_ptr<int> state;
  GraphemeText operator()(const std::string &) const;
  GraphemeText operator()(const std::unique_ptr<int> &) const;
};

struct NonAggregate {
  virtual ~NonAggregate() = default;
};

template <class Formatter>
concept CanMakeAll = requires(std::array<TableColumnOptions, 2> options, Formatter formatter) {
  aggregateColumns<Row>(std::move(options), std::move(formatter));
};

template <std::size_t N>
concept CanUseMetadataCount = requires(std::array<TableColumnOptions, N> options) {
  aggregateColumns<Row>(std::move(options), GenericFormatter{});
};

template <std::size_t I, class T, class Formatter>
concept CanMakeSelected = requires(TableColumnOptions options, Formatter formatter) {
  aggregateColumn<I, T>(std::move(options), std::move(formatter));
};

static_assert(!std::is_copy_constructible_v<Row>);
static_assert(boost::pfr::tuple_size_v<Row> == 2);
static_assert(CanMakeAll<GenericFormatter>);
static_assert(!CanMakeAll<WrongResult>);
static_assert(!CanMakeAll<PartialFormatter>);
static_assert(!CanMakeAll<MoveOnlyFormatter>);
static_assert(CanUseMetadataCount<2>);
static_assert(!CanUseMetadataCount<1>);
static_assert(CanMakeSelected<1, Row, GenericFormatter>);
static_assert(!CanMakeSelected<2, Row, GenericFormatter>);
static_assert(!CanMakeSelected<0, Row, WrongResult>);
static_assert(!CanMakeSelected<0, NonAggregate, GenericFormatter>);

TEST_CASE("aggregate columns preserve field order, types, explicit metadata and row borrows") {
  const Row row{"Ada", std::make_unique<int>(42)};
  const auto *label = &row.label;
  const auto *amount = &row.amount;
  auto formatter = [label, amount](const auto &value) -> GraphemeText {
    using Field = std::remove_cvref_t<decltype(value)>;

    if constexpr (std::is_same_v<Field, std::string>) {
      CHECK(&value == label);

      return text(value);
    } else {
      static_assert(std::is_same_v<Field, std::unique_ptr<int>>);
      CHECK(&value == amount);

      return text(std::to_string(*value));
    }
  };
  std::array<TableColumnOptions, 2> options{{{{11, 2}, text("Translated name"), 9}, {{12, 3}, text("Total"), 4}}};

  auto columns = aggregateColumns<Row>(std::move(options), formatter);

  CHECK(columns[0].id == ColumnId{11, 2});
  CHECK(columns[0].header.bytes() == "Translated name");
  CHECK(columns[0].width == 9);
  CHECK(columns[1].id == ColumnId{12, 3});
  CHECK(columns[1].header.bytes() == "Total");
  CHECK(columns[1].width == 4);
  CHECK(columns[0].format(row).bytes() == "Ada");
  CHECK(columns[1].format(row).bytes() == "42");
}

TEST_CASE("fixed aggregate captions use automatic identities and preserve validation") {
  const Row row{"Ada", std::make_unique<int>(42)};
  const auto columns =
    aggregateColumns<Row>(std::array{ColumnCaption{"Name", 9}, ColumnCaption{"Total", 4}}, GenericFormatter{});

  REQUIRE(columns);
  CHECK((*columns)[0].id == ColumnId{1, 1});
  CHECK((*columns)[1].id == ColumnId{2, 1});
  CHECK((*columns)[0].format(row).bytes() == "Ada");
  CHECK((*columns)[1].format(row).bytes() == "42");

  const std::string invalid(1, static_cast<char>(0xff));
  const auto rejected =
    aggregateColumns<Row>(std::array{ColumnCaption{invalid, 9}, ColumnCaption{"Total", 4}}, GenericFormatter{});

  REQUIRE_FALSE(rejected);
  CHECK(rejected.error().kind == GraphemeText::from(invalid).error().kind);
}

TEST_CASE("each column owns an independent copy of the generic formatter") {
  const Row row{"A", std::make_unique<int>(7)};
  auto formatter = [calls = 0](const auto &value) mutable -> GraphemeText {
    ++calls;
    using Field = std::remove_cvref_t<decltype(value)>;

    if constexpr (std::is_same_v<Field, std::string>) {
      return text(value + std::to_string(calls));
    } else {
      return text(std::to_string(*value) + std::to_string(calls));
    }
  };
  std::array<TableColumnOptions, 2> options{{{{1, 1}, text("A")}, {{2, 1}, text("B")}}};

  auto columns = aggregateColumns<Row>(std::move(options), formatter);

  CHECK(columns[0].format(row).bytes() == "A1");
  CHECK(columns[1].format(row).bytes() == "71");
  CHECK(columns[0].format(row).bytes() == "A2");
}

TEST_CASE("selected column preserves callback errors and captured lifetime") {
  const Row row{"Ada", std::make_unique<int>(7)};
  std::weak_ptr<int> weak;
  auto column = [&] {
    auto owner = std::make_shared<int>(3);
    weak = owner;

    return aggregateColumn<1, Row>({{9, 1}, text("Value")}, [owner](const std::unique_ptr<int> &value) {
      if (*value == 7) {
        throw std::runtime_error("formatter failed");
      }

      return text(std::to_string(*value + *owner));
    });
  }();

  CHECK_FALSE(weak.expired());
  CHECK_THROWS_AS(column.format(row), std::runtime_error);
  column.format = {};
  CHECK(weak.expired());
}
} // namespace

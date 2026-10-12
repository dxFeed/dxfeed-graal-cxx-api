// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0

#include <portals/table_column.hpp>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace {
using namespace portals;

int failures{};

void check(bool condition, const char *message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    ++failures;
  }
}

GraphemeText text(std::string_view value) {
  auto result = GraphemeText::from(value);

  if (!result) {
    std::abort();
  }

  return std::move(*result);
}

struct Record {
  std::string name;
  std::unique_ptr<int> amount;

  const std::string &label() const {
    return name;
  }
};

struct TextFormatter {
  GraphemeText operator()(const std::string &value) const {
    return text(value);
  }
};

struct WrongProjection {
  int operator()(int) const;
};

struct MutableRowProjection {
  int operator()(Record &) const;
};

struct VoidProjection {
  void operator()(const Record &) const;
};

struct WrongFormatter {
  GraphemeText operator()(int) const;
};

struct StringFormatter {
  std::string operator()(const std::string &) const;
};

struct ReferenceFormatter {
  const GraphemeText &operator()(const std::string &) const;
};

struct ConstValueFormatter {
  const GraphemeText operator()(const std::string &) const;
};

struct ConvertibleText {
  operator GraphemeText() const;
};

struct ConvertibleFormatter {
  ConvertibleText operator()(const std::string &) const;
};

struct MoveOnlyProjection {
  std::unique_ptr<int> state;
  const std::string &operator()(const Record &) const;
};

struct MoveOnlyFormatter {
  std::unique_ptr<int> state;
  GraphemeText operator()(const std::string &) const;
};

struct ReferenceProjection {
  std::string &operator()(const Record &) const;
};

struct RvalueFormatter {
  GraphemeText operator()(std::string &&) const;
};

struct LvalueFormatter {
  GraphemeText operator()(std::string &) const;
};

template <class Projection, class Formatter>
concept CanMakeColumn = requires(TableColumnOptions options, Projection projection, Formatter formatter) {
  {
    tableColumn<Record>(std::move(options), std::move(projection), std::move(formatter))
  } -> std::same_as<TableColumn<Record>>;
};

using MemberProjection = decltype(&Record::name);

static_assert(std::is_aggregate_v<TableColumnOptions>);
static_assert(!std::is_copy_constructible_v<Record>);
static_assert(ColumnProjection<Record, MemberProjection>);
static_assert(ColumnProjection<Record, decltype(&Record::label)>);
static_assert(ColumnFormatter<Record, MemberProjection, TextFormatter>);
static_assert(ColumnFormatter<Record, ReferenceProjection, LvalueFormatter>);
static_assert(CanMakeColumn<MemberProjection, TextFormatter>);
static_assert(!ColumnProjection<Record, WrongProjection>);
static_assert(!ColumnProjection<Record, MutableRowProjection>);
static_assert(!ColumnProjection<Record, MoveOnlyProjection>);
static_assert(!ColumnProjection<Record, MemberProjection &>);
static_assert(!ColumnFormatter<Record, WrongProjection, TextFormatter>);
static_assert(!ColumnFormatter<Record, VoidProjection, TextFormatter>);
static_assert(!ColumnFormatter<Record, MemberProjection, WrongFormatter>);
static_assert(!ColumnFormatter<Record, MemberProjection, StringFormatter>);
static_assert(!ColumnFormatter<Record, MemberProjection, ReferenceFormatter>);
static_assert(!ColumnFormatter<Record, MemberProjection, ConstValueFormatter>);
static_assert(!ColumnFormatter<Record, MemberProjection, ConvertibleFormatter>);
static_assert(!ColumnFormatter<Record, MemberProjection, MoveOnlyFormatter>);
static_assert(!ColumnFormatter<Record, MemberProjection, TextFormatter &>);
static_assert(!ColumnFormatter<Record, MemberProjection, LvalueFormatter>);
static_assert(!ColumnFormatter<Record, MemberProjection, RvalueFormatter>);
static_assert(!ColumnFormatter<Record, ReferenceProjection, RvalueFormatter>);
static_assert(!CanMakeColumn<WrongProjection, TextFormatter>);
static_assert(!CanMakeColumn<VoidProjection, TextFormatter>);
static_assert(!CanMakeColumn<MemberProjection, StringFormatter>);
static_assert(!CanMakeColumn<MoveOnlyProjection, TextFormatter>);
static_assert(!CanMakeColumn<MemberProjection, MoveOnlyFormatter>);

void membersAndMetadata() {
  const Record row{"Ada", std::make_unique<int>(7)};
  auto column = tableColumn<Record>({{12, 3}, text("Name"), 8}, &Record::name, TextFormatter{});
  auto getter = tableColumn<Record>({{13, 2}, text("Label")}, &Record::label, TextFormatter{});

  check(column.id == ColumnId{12, 3} && column.width == 8 && column.header.bytes() == "Name",
        "factory preserves identity, owning header and cell width");
  check(getter.width == 1, "options retain the existing default cell width");
  check(column.format(row).bytes() == "Ada" && getter.format(row).bytes() == "Ada",
        "data and const function member pointers read a move-only row");
}

void exactResultCategories() {
  const Record row{"borrowed", std::make_unique<int>(42)};
  auto borrowed = tableColumn<Record>({{1, 1}, text("Name")}, &Record::name, [&row](const std::string &value) {
    check(&value == &row.name, "member projection keeps the original borrow");

    return text(value);
  });
  auto moved = tableColumn<Record>(
    {{2, 1}, text("Amount")},
    [](const Record &value) {
      return std::make_unique<int>(*value.amount);
    },
    [](std::unique_ptr<int> &&value) {
      return text(std::to_string(*value));
    });
  auto consumed = tableColumn<Record>(
    {{3, 1}, text("Amount")},
    [](const Record &value) {
      return std::make_unique<int>(*value.amount);
    },
    [](std::unique_ptr<int> value) {
      return text(std::to_string(*value));
    });
  auto owned_reference = tableColumn<Record>(
    {{4, 1}, text("Owned")},
    [value = std::string("owned")](const Record &) mutable -> std::string & {
      return value;
    },
    [](std::string &value) {
      return text(value);
    });

  check(borrowed.format(row).bytes() == "borrowed", "formatter receives const lvalue projection result");
  check(moved.format(row).bytes() == "42", "formatter receives move-only temporary as rvalue reference");
  check(consumed.format(row).bytes() == "42", "formatter can consume move-only temporary by value");
  check(owned_reference.format(row).bytes() == "owned", "formatter receives mutable lvalue projection result");
}

void capturesAreOwned() {
  const Record row{"Ada", std::make_unique<int>(7)};
  std::weak_ptr<std::string> projection_lifetime;
  std::weak_ptr<std::string> formatter_lifetime;

  {
    auto column = [&] {
      auto prefix = std::make_shared<std::string>("Hello ");
      auto suffix = std::make_shared<std::string>("!");

      projection_lifetime = prefix;
      formatter_lifetime = suffix;

      auto projection = [prefix](const Record &value) mutable {
        return *prefix + value.name;
      };
      auto formatter = [suffix](std::string value) mutable {
        return text(value + *suffix);
      };

      return tableColumn<Record>({{1, 1}, text("Greeting")}, projection, formatter);
    }();

    check(!projection_lifetime.expired() && !formatter_lifetime.expired(),
          "column owns both captures after original callbacks and owners die");
    check(column.format(row).bytes() == "Hello Ada!", "owned mutable callbacks remain callable");

    auto copy = column;

    column.format = {};
    check(copy.format(row).bytes() == "Hello Ada!", "copied column owns independently callable callback storage");
  }

  check(projection_lifetime.expired() && formatter_lifetime.expired(), "destroyed columns release both capture owners");
}
} // namespace

int main() {
  membersAndMetadata();
  exactResultCategories();
  capturesAreOwned();

  return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}

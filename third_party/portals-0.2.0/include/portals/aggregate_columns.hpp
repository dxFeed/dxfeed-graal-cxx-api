// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once

#include <portals/table_column.hpp>

#include <boost/pfr/core.hpp>

#include <array>
#include <concepts>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace portals {
/** @brief Project one field from an immutable, borrowed aggregate row.
 * @tparam I Zero-based index in PFR's aggregate field order.
 * @tparam T SimpleAggregate record supported by Boost.PFR.
 * The result borrows the row and must be consumed during the formatter call.
 */
template <std::size_t I, class T> struct AggregateFieldProjection {
  /** @brief Borrow the selected field without copying the row.
   * @param row Immutable record that must outlive the formatter invocation.
   * @return Reference to field I in row.
   */
  decltype(auto) operator()(const T &row) const {
    return boost::pfr::get<I>(row);
  }
};

namespace detail {
template <class T, class Formatter, std::size_t... I>
consteval bool aggregateFieldsFormattable(std::index_sequence<I...>) {
  return (ColumnFormatter<T, AggregateFieldProjection<I, T>, Formatter> && ...);
}

template <class T, class Formatter, std::size_t N, std::size_t... I>
std::array<TableColumn<T>, N> makeAggregateColumns(std::array<TableColumnOptions, N> options, Formatter formatter,
                                                   std::index_sequence<I...>) {
  return {tableColumn<T>(std::move(options[I]), AggregateFieldProjection<I, T>{}, formatter)...};
}
} // namespace detail

/** @brief Create a typed column for one selected field of a PFR SimpleAggregate.
 * @tparam I Zero-based field index, less than the aggregate field count.
 * @tparam T SimpleAggregate row type supported by Boost.PFR, without base classes,
 * reference or const members, or C array members.
 * @tparam Formatter Copyable callable accepting the borrowed const field and returning exactly GraphemeText.
 * @param options Explicit owning ID, display header, and width. PFR member names are not captions or i18n keys.
 * @param formatter Callable copied into the returned column; any captured borrows remain the caller's responsibility.
 * @return Existing typed column, with the same validation and exception behavior as tableColumn.
 * The row and its field are borrowed for each callback invocation. Direct callback exceptions propagate.
 */
template <std::size_t I, class T, class Formatter>
  requires(std::is_aggregate_v<T> && I < boost::pfr::tuple_size_v<T> &&
           ColumnFormatter<T, AggregateFieldProjection<I, T>, Formatter>)
TableColumn<T> aggregateColumn(TableColumnOptions options, Formatter formatter) {
  return tableColumn<T>(std::move(options), AggregateFieldProjection<I, T>{}, std::move(formatter));
}

/** @brief Create columns for every field in PFR's aggregate declaration order.
 * @tparam T SimpleAggregate row type supported by Boost.PFR, without base classes,
 * reference or const members, or C array members.
 * @tparam N Number of explicit metadata entries; must equal the PFR field count.
 * @tparam Formatter Copyable callable accepting every borrowed const field type and returning exactly GraphemeText.
 * @param options Explicit owning IDs, display headers, and widths in field order. Names are never inferred.
 * @param formatter Generic callable copied independently into every column; captured references remain borrowed.
 * @return An array of existing typed columns, preserving metadata and field order.
 * Construction can throw while copying callbacks or allocating storage. The formatter is never invoked during
 * construction. Row fields are borrowed only for the formatter call; direct callback exceptions propagate.
 */
template <class T, std::size_t N, class Formatter>
  requires(std::is_aggregate_v<T> && N == boost::pfr::tuple_size_v<T> &&
           detail::aggregateFieldsFormattable<T, Formatter>(std::make_index_sequence<N>{}))
std::array<TableColumn<T>, N> aggregateColumns(std::array<TableColumnOptions, N> options, Formatter formatter) {
  return detail::makeAggregateColumns<T>(std::move(options), std::move(formatter), std::make_index_sequence<N>{});
}

namespace detail {
template <class T, class Formatter, std::size_t N, std::size_t... I>
std::expected<std::array<TableColumn<T>, N>, TextError> makeAggregateSchema(std::array<ColumnCaption, N> captions,
                                                                            Formatter formatter, WidthPolicy policy,
                                                                            std::index_sequence<I...>) {
  if constexpr (N == 0) {
    return std::array<TableColumn<T>, 0>{};
  } else {
    return tableColumns(
      policy, tableColumn<T>(captions[I].header, AggregateFieldProjection<I, T>{}, formatter, captions[I].width)...);
  }
}
} // namespace detail

/** @brief Assemble a fixed aggregate schema without manual column IDs or generations.
 * @tparam T PFR SimpleAggregate record; borrowed const fields in declaration order.
 * @tparam N Caption count, equal to the aggregate field count.
 * @tparam Formatter Copyable exact GraphemeText formatter accepting every field type.
 * @param captions Owning display captions and widths, in declaration order.
 * @param formatter Independent owning copy per column, using the table's width policy.
 * @param policy Policy used to validate every header; must match the formatter and table.
 * @return Owning fixed columns with deterministic IDs 1..N, or the first header TextError.
 * Assemble once and retain the returned columns for subsequent reordering. Dynamic
 * schema replacement uses explicit TableColumnOptions instead. No identity is derived
 * from display text, and construction does not invoke the formatter or copy any row.
 */
template <class T, std::size_t N, class Formatter>
  requires(std::is_aggregate_v<T> && N == boost::pfr::tuple_size_v<T> &&
           detail::aggregateFieldsFormattable<T, Formatter>(std::make_index_sequence<N>{}))
std::expected<std::array<TableColumn<T>, N>, TextError> aggregateColumns(std::array<ColumnCaption, N> captions,
                                                                         Formatter formatter, WidthPolicy policy = {}) {
  return detail::makeAggregateSchema<T>(std::move(captions), std::move(formatter), policy,
                                        std::make_index_sequence<N>{});
}
} // namespace portals

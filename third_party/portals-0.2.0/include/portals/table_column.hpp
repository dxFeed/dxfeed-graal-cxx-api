// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once

#include <portals/virtual_table.hpp>

#include <array>
#include <concepts>
#include <functional>
#include <type_traits>
#include <utility>

namespace portals {
/** @brief Owning metadata for a typed column; validation remains part of table preparation. */
struct TableColumnOptions {
  ColumnId id;           ///< Nonzero unique value/generation pair within the table configuration.
  GraphemeText header;   ///< Owning header text using the table's width policy.
  std::int32_t width{1}; ///< Positive width in terminal cells; columns remain in configured order.
};

/** @brief Owning column recipe whose identity is assigned by tableColumns.
 * @tparam T Immutable record borrowed by the formatter; never copied by the recipe.
 * Use for a fixed schema. Assemble once, then retain/reorder the returned columns;
 * rebuilding recipes in a different order creates a different positional schema.
 */
template <class T> struct TableColumnSpec {
  std::string header;                            ///< Owning UTF-8 caption, validated when the schema is assembled.
  std::int32_t width{1};                         ///< Width in terminal cells, validated by table preparation.
  std::function<GraphemeText(const T &)> format; ///< Owning callback; reference captures remain borrowed.
};

/** @brief Owning fixed-schema caption and sizing, without identity bookkeeping. */
struct ColumnCaption {
  std::string header;    ///< UTF-8 copied into the schema; captions may change with locale.
  std::int32_t width{1}; ///< Positive width in terminal cells, validated by table preparation.
};

/** @brief An owning copyable callable that projects a borrowed immutable row.
 * @tparam T Domain record type; projection receives const T & without copying the row.
 * @tparam Projection Copy-constructible object invocable as an lvalue with const T &.
 * Reference results borrow their referent; callbacks must not retain row borrows.
 */
template <class T, class Projection>
concept ColumnProjection =
  std::is_object_v<Projection> && std::copy_constructible<Projection> && std::invocable<Projection &, const T &>;

/** @brief A copyable formatter returning exactly owning GraphemeText for a projection result.
 * @tparam T Immutable domain record type accepted by the projection.
 * @tparam Projection Object satisfying ColumnProjection; its result keeps its value category.
 * @tparam Formatter Copy-constructible object invocable as an lvalue with that result.
 * The result must be GraphemeText by value, not a reference or a convertible text type.
 */
template <class T, class Projection, class Formatter>
concept ColumnFormatter =
  ColumnProjection<T, Projection> && std::is_object_v<Formatter> && std::copy_constructible<Formatter> &&
  requires(Projection &projection, Formatter &formatter, const T &row) {
    { std::invoke(formatter, std::invoke(projection, row)) } -> std::same_as<GraphemeText>;
  };

/** @brief Compose an owning typed column from explicit projection and text formatting.
 * @tparam T Domain record type; rows are borrowed as const T & and never copied here.
 * @tparam Projection Copy-constructible projection satisfying ColumnProjection.
 * @tparam Formatter Copy-constructible formatter satisfying ColumnFormatter.
 * @param options Owning identity, header and cell width; moved into the returned column.
 * @param projection Callable owned by value; std::invoke supports member pointers and lambdas.
 * @param formatter Callable owned by value; consumes the projection's exact result category.
 * @return Existing TableColumn<T> with an owning composed callback and unchanged metadata.
 *
 * Construction may throw while copying/moving callbacks or allocating their std::function storage.
 * Neither callback is invoked during construction. References held by callbacks or returned by a
 * projection remain borrowed; callers must keep their referents alive for each invocation. A temporary
 * projection result lives through the formatter call. Callbacks must not retain row/result borrows,
 * perform I/O, mutate the tree/model, or create unbounded temporary storage. Text must use the table's
 * width policy. Table preparation retains its existing metadata/policy validation and callback exception
 * handling; direct invocation of the returned format callback propagates exceptions. Invocation follows
 * the table's UI-thread rules; this factory adds no synchronization or runtime state.
 */
template <class T, class Projection, class Formatter>
  requires ColumnFormatter<T, Projection, Formatter>
TableColumn<T> tableColumn(TableColumnOptions options, Projection projection, Formatter formatter) {
  return {options.id, std::move(options.header), options.width,
          [projection = std::move(projection), formatter = std::move(formatter)](const T &row) mutable -> GraphemeText {
            return std::invoke(formatter, std::invoke(projection, row));
          }};
}

/** @brief Deduce the domain record from a data or member-function pointer.
 * @tparam T Record owning the member; rows are borrowed as const T &.
 * @tparam Member Member type, invocable with the immutable record through std::invoke.
 * @tparam Formatter Copyable formatter returning exactly owning GraphemeText.
 * @param options Explicit stable identity, owning header and width in cells.
 * @param member Member pointer owned by value; no record is copied.
 * @param formatter Owning callback following the original ColumnFormatter contract.
 * @return Existing typed column with unchanged validation and callback behavior.
 */
template <class T, class Member, class Formatter>
  requires ColumnFormatter<T, Member T::*, Formatter>
TableColumn<T> tableColumn(TableColumnOptions options, Member T::*member, Formatter formatter) {
  return tableColumn<T, Member T::*, Formatter>(std::move(options), member, std::move(formatter));
}

/** @brief Construct typed metadata from UTF-8 without unchecked header extraction.
 * @param id Explicit nonzero stable column identity; validation remains with the table.
 * @param header UTF-8 bytes borrowed only for this call, copied on success.
 * @param width Column width in cells; validated by the existing table preparation.
 * @param policy Display policy matching the receiving table.
 * @return Owning column options or the unchanged header TextError.
 * @throws std::bad_alloc If independent text storage cannot be allocated.
 */
inline std::expected<TableColumnOptions, TextError> columnOptions(ColumnId id, std::string_view header,
                                                                  std::int32_t width = 1, WidthPolicy policy = {}) {
  auto text = GraphemeText::from(header, policy);

  if (!text) {
    return std::unexpected(text.error());
  }

  return TableColumnOptions{id, std::move(*text), width};
}

/** @brief Deduce a field's record and validate a plain UTF-8 header.
 * @tparam T Record type inferred from the member pointer; borrowed immutable rows.
 * @tparam Member Projected member type, satisfying ColumnProjection.
 * @tparam Formatter Copyable exact GraphemeText formatter for that projection.
 * @param id Explicit stable identity, retained unchanged.
 * @param header Header bytes borrowed during construction and owned on success.
 * @param member Owning member pointer; no row copies or reads during construction.
 * @param formatter Callback owned by value; borrowed captures require external lifetime.
 * @param width Width in terminal cells, validated during table preparation.
 * @param policy Width policy used for the owning header; formatter text must match it.
 * @return Existing TableColumn<T> or the original header TextError.
 * Callback construction/allocation exceptions propagate, as in the original factory.
 */
template <class T, class Member, class Formatter>
  requires ColumnFormatter<T, Member T::*, Formatter>
std::expected<TableColumn<T>, TextError> tableColumn(ColumnId id, std::string_view header, Member T::*member,
                                                     Formatter formatter, std::int32_t width = 1,
                                                     WidthPolicy policy = {}) {
  auto options = columnOptions(id, header, width, policy);

  if (!options) {
    return std::unexpected(options.error());
  }

  return tableColumn<T, Member T::*, Formatter>(std::move(*options), member, std::move(formatter));
}

/** @brief Describe a fixed-schema column without exposing identity bookkeeping.
 * @tparam T Borrowed immutable row type, explicit for lambda projections.
 * @tparam Projection Copyable projection satisfying ColumnProjection.
 * @tparam Formatter Copyable exact GraphemeText formatter for the projected value.
 * @param header UTF-8 caption copied now and validated by tableColumns.
 * @param projection Owned callable; projection results borrow only for invocation.
 * @param formatter Owned formatter using the eventual table's width policy.
 * @param width Width in terminal cells; validated by the retained table.
 * @return Owning recipe, with no ID until assembled; construction may allocate.
 */
template <class T, class Projection, class Formatter>
  requires ColumnFormatter<T, Projection, Formatter>
TableColumnSpec<T> tableColumn(std::string_view header, Projection projection, Formatter formatter,
                               std::int32_t width = 1) {
  return {std::string(header), width,
          [projection = std::move(projection), formatter = std::move(formatter)](const T &row) mutable -> GraphemeText {
            return std::invoke(formatter, std::invoke(projection, row));
          }};
}

/** @brief Infer a fixed-schema record from a member pointer, without explicit IDs.
 * @tparam T Immutable row type inferred from its member pointer.
 * @tparam Member Data or const member-function type accepted through std::invoke.
 * @tparam Formatter Copyable exact GraphemeText formatter.
 * @param header Caption copied now, validated during tableColumns assembly.
 * @param member Owned member pointer; no domain row is copied.
 * @param formatter Owned callback; borrowed captures need external lifetime.
 * @param width Width in cells; validated by the retained table.
 * @return Owning identity-free recipe; construction may allocate.
 */
template <class T, class Member, class Formatter>
  requires ColumnFormatter<T, Member T::*, Formatter>
TableColumnSpec<T> tableColumn(std::string_view header, Member T::*member, Formatter formatter,
                               std::int32_t width = 1) {
  return tableColumn<T, Member T::*, Formatter>(header, member, std::move(formatter), width);
}

namespace detail {
template <class T, std::size_t N, std::size_t... I>
std::expected<std::array<TableColumn<T>, N>, TextError>
assembleColumns(WidthPolicy policy, std::array<TableColumnSpec<T>, N> specs, std::index_sequence<I...>) {
  constexpr std::uint64_t INITIAL_COLUMN_GENERATION = 1;

  auto built = std::array{[&](std::size_t index) -> std::expected<TableColumn<T>, TextError> {
    auto header = GraphemeText::from(specs[index].header, policy);

    if (!header) {
      return std::unexpected(header.error());
    }

    return TableColumn<T>{
      {index + 1, INITIAL_COLUMN_GENERATION}, std::move(*header), specs[index].width, std::move(specs[index].format)};
  }(I)...};

  for (const auto &column : built) {
    if (!column) {
      return std::unexpected(column.error());
    }
  }

  return std::array<TableColumn<T>, N>{std::move(*built[I])...};
}
} // namespace detail

/** @brief Assemble a fixed schema and allocate deterministic table-local identities.
 * @tparam T Immutable record type inferred from the first recipe.
 * @tparam Rest Additional TableColumnSpec<T> arguments, in display order.
 * @param policy Width policy used for every validated owning header.
 * @param first First owning recipe, copied or moved into assembly.
 * @param rest Remaining recipes, copied from lvalues or consumed from rvalues.
 * @return Owning columns with IDs 1..N and initial generation, or the first header TextError.
 * IDs are assigned once in argument order, independently per table; no global counter.
 * Reordering the resulting array preserves identities. For dynamic schema replacement,
 * use explicit TableColumnOptions/ColumnId instead of rebuilding a positional schema.
 * Caption/locale changes never determine identity. Existing validation and callback
 * exception behavior are unchanged; assembly itself may throw during allocation/copies.
 */
template <class T, class... Rest>
  requires(std::same_as<std::remove_cvref_t<Rest>, TableColumnSpec<T>> && ...)
std::expected<std::array<TableColumn<T>, 1 + sizeof...(Rest)>, TextError>
tableColumns(WidthPolicy policy, TableColumnSpec<T> first, Rest &&...rest) {
  constexpr std::size_t COLUMN_COUNT = 1 + sizeof...(Rest);

  return detail::assembleColumns(
    policy, std::array<TableColumnSpec<T>, COLUMN_COUNT>{std::move(first), std::forward<Rest>(rest)...},
    std::make_index_sequence<COLUMN_COUNT>{});
}

/** @brief Assemble fixed-schema columns with the default display policy.
 * @tparam T Immutable record type inferred from the first recipe.
 * @tparam Rest Additional exact TableColumnSpec<T> arguments, in display order.
 * @param first First recipe, copied or moved into the owning schema.
 * @param rest Remaining recipes, retaining their formatter ownership rules.
 * @return Checked columns with deterministic table-local IDs or the first header TextError.
 * Use the policy overload for nondefault terminal widths; dynamic schemas use explicit IDs.
 */
template <class T, class... Rest>
  requires(std::same_as<std::remove_cvref_t<Rest>, TableColumnSpec<T>> && ...)
std::expected<std::array<TableColumn<T>, 1 + sizeof...(Rest)>, TextError> tableColumns(TableColumnSpec<T> first,
                                                                                       Rest &&...rest) {
  return tableColumns(WidthPolicy{}, std::move(first), std::forward<Rest>(rest)...);
}
} // namespace portals

// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once

#include <portals/virtual_list.hpp>

namespace portals {
/** @brief Owning table settings; preparation rejects invalid dimensions and exceeded budgets.
 * Inherited VirtualListOptions define fixed body-row height, overscan, width policy and row styles.
 */
struct VirtualTableOptions : VirtualListOptions {
  CellStyle header;                      ///< One fixed cell row of column headings, unaffected by selection.
  std::size_t max_columns{64};           ///< Positive bound on configured columns, including offscreen columns.
  std::size_t max_prepared_cells{16384}; ///< Positive bound on visible body cells; no silent truncation.
  std::size_t max_prepared_text_bytes{1024 * 1024}; ///< Positive total UTF-8 byte budget for headers and visible cells.
};

/** @brief A typed owning column definition; identity is independent of visual order.
 * @tparam T Immutable domain record type; accessed by const reference, never copied by the table.
 * Formatters own their captures, return owning text with the table width policy, and must not
 * retain row borrows, perform I/O, mutate the tree/model, or produce unbounded temporary storage.
 */
template <class T> struct TableColumn {
  ColumnId id;           ///< Nonzero unique value/generation pair within this table configuration.
  GraphemeText header;   ///< Owning header text, clipped without splitting graphemes.
  std::int32_t width{1}; ///< Positive width in cells; columns are contiguous, in vector order.
  std::function<GraphemeText(const T &)>
    format; ///< Owning typed accessor/formatter, only called for visible ready cells.
};

/// @cond INTERNAL
namespace detail {
inline constexpr std::int32_t TABLE_HEADER_HEIGHT = 1;

struct TableHeading {
  std::int32_t x{};
  std::int32_t width{};
  GraphemeText text;
};

struct TableRow {
  std::optional<RowId> id;
  std::vector<GraphemeText> cells = std::vector<GraphemeText>(std::initializer_list<GraphemeText>{});
};

std::expected<void, TreeError> paintTable(PaintContext &context, CellSize viewport, VirtualTableOptions options,
                                          std::span<const TableHeading> headings, std::span<const TableRow> rows,
                                          std::optional<RowId> selected);
} // namespace detail

/// @endcond

template <class T> class VirtualTable;
/** @brief Prepare a local final viewport between frames, using the same pin as every sibling view.
 * Returns a new request exactly once for caller dispatch. No worker is started. A rejected completion
 * can be retried on that token; cancel it if dispatch fails, then prepare again for a new receipt.
 * Hidden ancestors cancel work and release prepared rows. Failure leaves the previous presentation intact.
 * The viewport includes the fixed one-cell header. Exceeded row/cell/text budgets return INVALID_ARGUMENT.
 * Tree affinity/frame errors, invalid dimensions/ranges, NO_MEMORY and CALLBACK_FAILED are explicit.
 * @tparam T Immutable domain row type satisfying VirtualTable's provider/delegate contract.
 * @param tree UI owner; borrowed only for this call, no calls during its frame.
 * @param node Checked observer of the table to prepare.
 * @param frame One shared application pin; the view retains an owning copy on success.
 * @param viewport Final local size from the application's layout constraints, in nonnegative cells.
 * @return Empty optional when no new dispatch is needed, an owning request receipt, or TreeError.
 * @note A changed presentation invalidates paint; an unchanged preparation is a no-op. Exact viewport mismatch at paint
 * returns INVALID_GEOMETRY. Snapshot/delegate failures leave old frame, rows and IDs together; retry may cancel prior
 * work.
 */
template <class T>
std::expected<std::optional<ModelRequest<T>>, TreeError>
prepareVirtualTable(WidgetTree &tree, Node<VirtualTable<T>> node, const ModelFrame<T> &frame, CellSize viewport);
/** @brief Change the integral first row on UI and invalidate paint; prepare before painting again.
 * @tparam T Immutable domain row type, as defined by VirtualTable.
 * @param tree Borrowed UI owner; mutation inside a frame is rejected.
 * @param node Checked table observer; does not convey ownership.
 * @param first Requested position; preparation clamps it to the last row, or zero for an empty model.
 * @return Success or the tree's affinity, lifetime, frame or invalidation error.
 * @note Cancels old work and navigation intent, but preserves the selected RowId.
 */
template <class T>
std::expected<void, TreeError> setVirtualTableFirst(WidgetTree &tree, Node<VirtualTable<T>> node, std::uint64_t first);
/** @brief Set stable selection on UI, clear pending navigation, and invalidate paint.
 * @tparam T Immutable domain row type, as defined by VirtualTable.
 * @param tree Borrowed UI owner; mutation inside a frame is rejected.
 * @param node Checked table observer.
 * @param selected Owning stable ID, or nullopt to clear; an ID need not be currently visible.
 * @return Success, INVALID_ARGUMENT for a zero ID component, or a tree error.
 * @note No model lookup or loading occurs; removed IDs remain selected until explicitly cleared.
 */
template <class T>
std::expected<void, TreeError> setVirtualTableSelection(WidgetTree &tree, Node<VirtualTable<T>> node,
                                                        std::optional<RowId> selected);
/** @brief Validate settings on UI, invalidate layout and require preparation before the next paint.
 * @tparam T Immutable domain row type, as defined by VirtualTable.
 * @param tree Borrowed UI owner; mutation inside a frame is rejected.
 * @param node Checked table observer.
 * @param options Owning replacement settings; height and budgets must be positive and existing headers must match the
 * policy.
 * @return Success, INVALID_ARGUMENT for invalid settings or incompatible existing columns, or a tree error.
 * @note The model's range budget is checked later by preparation; overscan is never silently capped.
 */
template <class T>
std::expected<void, TreeError> setVirtualTableOptions(WidgetTree &tree, Node<VirtualTable<T>> node,
                                                      VirtualTableOptions options);
/** @brief Replace ordered column definitions transactionally and invalidate layout.
 * @tparam T Immutable domain row type borrowed by TableColumn formatters.
 * @param tree Borrowed UI owner; mutation inside a frame is rejected.
 * @param node Checked table observer.
 * @param columns Borrowed definitions copied on success; unique nonzero IDs, positive widths and matching policies
 * required.
 * @return Success; INVALID_ARGUMENT for invalid columns/budgets, NO_MEMORY, CALLBACK_FAILED on capture copy failure,
 * or a tree affinity/lifetime/frame error.
 * @note The span is not retained. Validation/copy failure preserves the previous configuration and presentation.
 */
template <class T>
std::expected<void, TreeError> setVirtualTableColumns(WidgetTree &tree, Node<VirtualTable<T>> node,
                                                      std::span<const TableColumn<std::type_identity_t<T>>> columns);

/** @brief Explicitly retry a provider error/pending range at the next preparation; no worker is started.
 * @tparam T Immutable domain row type, as defined by VirtualTable.
 * @param tree Borrowed UI owner; mutation inside a frame is rejected.
 * @param node Checked table observer.
 * @return Success or a tree affinity, lifetime, frame or invalidation error.
 * @note Cancels the previous request and invalidates paint; preparation returns a fresh receipt.
 */
template <class T> std::expected<void, TreeError> retryVirtualTable(WidgetTree &tree, Node<VirtualTable<T>> node);
/** @brief Bind arrows/Home/End, left press and integral wheel scrolling to the existing router.
 * Caller retains the scoped binding. Tree/router outlive it; no widget pointer escapes dispatch.
 * @tparam T Immutable domain row type, as defined by VirtualTable.
 * @param router Existing UI router; no additional queue or focus system is created.
 * @param tree The same borrowed tree used by router; it must outlive the registration.
 * @param node Checked table observer to register as focusable.
 * @return Owning disconnect token, or the router's registration/affinity/lifetime error.
 * @note Handled actions invalidate paint. Pending selection is resolved only by preparation.
 */
template <class T>
std::expected<ActionBinding, TreeError> bindVirtualTable(EventRouter &router, WidgetTree &tree,
                                                         Node<VirtualTable<T>> node);

/** @brief One UI-owned table with bounded owning cell text and one independent model consumer.
 * Construct with a consumer of the frame's model. Each column owns captures and returns owning text;
 * each formatter runs only during preparation and must not perform I/O, mutate models/tree, or retain row borrows.
 * Preparation is transactional on allocation/delegate failure. The retained frame and text survive
 * through render inside WidgetTree::frame. No row widgets, worker, hidden pin or terminal I/O exist.
 * Use tree-checked free mutations; read getters through tree.read. Destroy on the UI owner.
 * @tparam T Domain value exposed by ModelRow<T>. It need not be copyable/default-constructible;
 * this view only borrows const rows during its column formatters. The provider must publish immutable
 * transitively owned values and stable nonzero RowId pairs. The copyable formatters own captures,
 * return GraphemeText with options.width_policy, and may throw; preparation reports those failures.
 */
template <class T> class VirtualTable : public SizedWidget {
public:
  /** @brief Own consumer, columns and options; validation happens on preparation.
   * @param consumer Independent move-only range slot, normally obtained from Model<T>::openRange().
   * @param columns Borrowed definitions copied into owned storage during this call; IDs must be unique and widths
   * positive.
   * @param options Owning height, overscan, width-policy and styles.
   * @param policy Owning layout constraints inherited by SizedWidget.
   * @note Construct through WidgetTree::emplace on UI; allocation/capture failures follow its errors.
   */
  VirtualTable(ModelConsumer<T> consumer, std::span<const TableColumn<std::type_identity_t<T>>> columns,
               VirtualTableOptions options = {}, SizePolicy policy = {})
      : SizedWidget(policy), consumer_(std::move(consumer)), columns_(columns.begin(), columns.end()),
        options_(options) {
  }

  ~VirtualTable() override {
    request_.cancel();
  } ///< Cancel work; worker ownership may outlive this view.

  /** @return Integral intended scroll position, clamped at the last successful preparation. */
  std::uint64_t firstRow() const noexcept {
    return scroll_.rasterOffset();
  } ///< Integral scroll position, clamped at preparation.

  /** @return Owning logical/raster scroll position; reads require the UI owner's tree.read. */
  ScrollState scrollState() const noexcept {
    return scroll_;
  }

  /** @return First ready visible row identity retained across hidden periods, or no established anchor. */
  std::optional<RowId> anchorRow() const noexcept {
    return anchor_;
  }

  /** @return Owning stable ID, including an offscreen/removed ID, or nullopt when cleared. */
  std::optional<RowId> selection() const noexcept {
    return selected_;
  } ///< Stable identity, retained across model revisions and hidden views.

  /** @return Owning interval of the last prepared visible rows, or an empty interval before preparation. */
  RowRange visibleRange() const noexcept {
    return prepared_ ? prepared_->window.visible : RowRange{};
  } ///< Last successfully prepared visible rows.

  /** @return Owning viewport-plus-overscan interval; hidden/unprepared tables return an empty interval. */
  RowRange preparedRange() const noexcept {
    return prepared_ ? prepared_->window.requested : RowRange{};
  } ///< Last successfully prepared viewport plus overscan.

  /** @return Last prepared ready/pending/error state; an unprepared/hidden table reports pending. */
  RangeState rangeState() const noexcept {
    return prepared_ ? prepared_->range.state() : RangeState::PENDING;
  } ///< Last prepared model readiness; error text is owned by its retained model range.

  /** @param available Borrowed-by-value cell bounds; negative extents normalize to zero.
   * @return Normalized available extent without model access, formatting, allocation or invalidation.
   */
  std::expected<CellSize, TreeError> measure(CellSize available) override {
    return CellSize{std::max(0, available.width), std::max(0, available.height)};
  } ///< Viewport-sized intrinsic extent, independent of logical row count.

  /** @param local Owning parent-local rectangle supplied by layoutTree inside the frame.
   * @return Success after storing the local size; does not prepare rows or invalidate the tree.
   */
  std::expected<void, TreeError> arrange(CellRect local) override {
    arranged_ = local.size();

    return {};
  } ///< Remember local bounds without reading/loading/formatting rows.

  /** @param context Synchronous borrowed sink; rendered text must not outlive the enclosing tree.frame.
   * @return Success, WOULD_BLOCK for dirty/unprepared content, INVALID_GEOMETRY after resize,
   * or the sink error. Caller must discard partial commands on error.
   * @note UI frame only; no I/O, model reads, delegate calls or invalidation.
   */
  std::expected<void, TreeError> paint(PaintContext &context) override {
    if (!prepared_ || dirty_) {
      return std::unexpected(TreeError::WOULD_BLOCK);
    }

    if (arranged_ != prepared_->viewport) {
      return std::unexpected(TreeError::INVALID_GEOMETRY);
    }

    auto options = options_;

    if (const auto &style = computedStyle()) {
      options.normal = style->normal;
      options.selected = style->selected;
      options.header = style->header;
    }

    return detail::paintTable(context, arranged_, options, prepared_->headings, prepared_->rows, selected_);
  } ///< Emit widget-local commands; caller applies surface translation and ancestor clip.

  /** @return Owning manual row and header appearances beneath theme declarations. */
  ComputedStyle styleDefaults() const override {
    ComputedStyle result;

    result.normal = options_.normal;
    result.selected = options_.selected;
    result.header = options_.header;

    return result;
  }

private:
  /// @cond INTERNAL
  template <class U>
  friend std::expected<std::optional<ModelRequest<U>>, TreeError>
  prepareVirtualTable(WidgetTree &, Node<VirtualTable<U>>, const ModelFrame<U> &, CellSize);
  template <class U>
  friend std::expected<void, TreeError> setVirtualTableFirst(WidgetTree &, Node<VirtualTable<U>>, std::uint64_t);
  template <class U>
  friend std::expected<void, TreeError> setVirtualTableSelection(WidgetTree &, Node<VirtualTable<U>>,
                                                                 std::optional<RowId>);
  template <class U>
  friend std::expected<void, TreeError> setVirtualTableOptions(WidgetTree &, Node<VirtualTable<U>>,
                                                               VirtualTableOptions);
  template <class U> friend std::expected<void, TreeError> retryVirtualTable(WidgetTree &, Node<VirtualTable<U>>);
  template <class U>
  friend std::expected<ActionBinding, TreeError> bindVirtualTable(EventRouter &, WidgetTree &, Node<VirtualTable<U>>);

  template <class U>
  friend std::expected<void, TreeError> setVirtualTableColumns(WidgetTree &, Node<VirtualTable<U>>,
                                                               std::span<const TableColumn<std::type_identity_t<U>>>);

  template <class U>
  friend std::expected<void, TreeError> scrollVirtualTable(WidgetTree &, Node<VirtualTable<U>>, double);
  template <class U>
  friend std::expected<RowLookup, TreeError> ensureVirtualTableVisible(WidgetTree &, Node<VirtualTable<U>>,
                                                                       const ModelFrame<U> &, RowId);

  /// @endcond

  static std::expected<void, TreeError> validateColumns(std::span<const TableColumn<std::type_identity_t<T>>> columns,
                                                        VirtualTableOptions options) {
    if (options.row_height <= 0 || options.max_columns == 0 || options.max_prepared_cells == 0 ||
        options.max_prepared_text_bytes == 0 || columns.size() > options.max_columns ||
        (options.width_policy.ambiguous != AmbiguousWidth::NARROW &&
         options.width_policy.ambiguous != AmbiguousWidth::WIDE) ||
        (options.width_policy.emoji != EmojiWidth::ONE && options.width_policy.emoji != EmojiWidth::TWO)) {
      return std::unexpected(TreeError::INVALID_ARGUMENT);
    }

    std::size_t bytes{};

    for (std::size_t i = 0; i < columns.size(); ++i) {
      const auto &column = columns[i];

      if (column.id.value == 0 || column.id.generation == 0 || column.width <= 0 || !column.format ||
          column.header.policy() != options.width_policy ||
          column.header.bytes().size() > options.max_prepared_text_bytes - bytes ||
          std::ranges::any_of(columns.first(i), [&](const auto &prior) {
            return prior.id == column.id;
          })) {
        return std::unexpected(TreeError::INVALID_ARGUMENT);
      }

      bytes += column.header.bytes().size();
    }

    return {};
  }

  struct Prepared {
    Prepared(const ModelFrame<T> &owner, const ModelRange<T> &data, CellSize size, detail::ListWindow interval)
        : frame(owner), range(data), viewport(size), window(interval) {
    }

    ModelFrame<T> frame;
    ModelRange<T> range;
    CellSize viewport;
    detail::ListWindow window;
    std::vector<detail::TableRow> rows = std::vector<detail::TableRow>(0);
    std::vector<detail::TableHeading> headings =
      std::vector<detail::TableHeading>(std::initializer_list<detail::TableHeading>{});
  };

  std::expected<std::optional<ModelRequest<T>>, TreeError> prepare(const ModelFrame<T> &frame, CellSize viewport,
                                                                   bool &changed) {
    auto consumer_range = frame.range(consumer_.id());

    if (!consumer_range) {
      return std::unexpected(consumer_range.error());
    }

    auto nextScroll = scroll_;

    if (anchor_ && anchorRevision_ != frame.dataRevision() && !intent_) {
      auto location = frame.locateRow(*anchor_);

      if (!location) {
        return std::unexpected(location.error());
      }

      if (location->state == RowLookupState::FOUND) {
        (void)nextScroll.setOffset(location->index, nextScroll.fraction());
      }
      // Removed anchors and legacy providers retain the old ordinal, clamped below.
    }

    nextScroll.clamp(frame.rowCount());

    auto valid = validateColumns(columns_, options_);

    if (!valid || viewport.width < 0 || viewport.height < 0) {
      return std::unexpected(TreeError::INVALID_ARGUMENT);
    }

    const CellSize body{columns_.empty() ? 0 : viewport.width,
                        std::max(0, viewport.height - detail::TABLE_HEADER_HEIGHT)};
    auto window = detail::listWindow(frame.rowCount(), nextScroll.rasterOffset(), body, options_);

    if (!window) {
      return std::unexpected(window.error());
    }

    if (prepared_ && !dirty_ && !retry_ && prepared_->frame.revision() == frame.revision() &&
        prepared_->viewport == viewport && prepared_->window.visible == window->visible &&
        (window->requested.count == 0 || prepared_->range.state() != RangeState::PENDING ||
         !request_.stopRequested())) {
      return std::optional<ModelRequest<T>>{};
    }

    try {
      auto range = consumer_range;

      if (window->requested.count == 0) {
        range = ModelRange<T>{};
      } else if (retry_ || range->range() != window->requested || range->state() == RangeState::PENDING) {
        range = frame.read(window->requested);
      }

      if (!range) {
        return std::unexpected(range.error());
      }

      if (!retry_ && range->state() == RangeState::PENDING && window->requested.count != 0) {
        auto cached = frame.readCached(*consumer_range, prepared_ ? &prepared_->range : nullptr, window->requested);

        if (!cached) {
          return std::unexpected(cached.error());
        }

        if (cached->dataRevision() != 0) {
          range = std::move(cached);
        }
      }

      // Stage all owning data before touching the visible state or issuing provider work.
      // A delegate may fail halfway through: frame, row identities and text still commit together.
      // Construct in place: MSVC Debug vector move is noexcept but may allocate an iterator proxy.
      auto next = std::make_unique<Prepared>(frame, *range, viewport, *window);
      auto selection = selected_;
      const bool apply_intent = intent_ && intent_revision_ == frame.dataRevision();

      std::size_t text_bytes{};
      std::int32_t x{};

      // Column validation bounds this scan independently of the logical row count.
      // Keep geometry clipped before arithmetic; even INT32_MAX widths cannot overflow x.
      for (const auto &column : columns_) {
        text_bytes += column.header.bytes().size();

        if (x >= viewport.width || viewport.height == 0) {
          continue;
        }

        const auto width = std::min(column.width, viewport.width - x);

        next->headings.push_back({x, width, column.header});
        x += width;
      }

      const auto visible_columns = next->headings.size();

      if (visible_columns != 0 && window->visible.count > options_.max_prepared_cells / visible_columns) {
        return std::unexpected(TreeError::INVALID_ARGUMENT);
      }

      next->rows.reserve(window->visible.count);

      for (std::size_t i = 0; i < window->visible.count; ++i) {
        const auto index = window->visible.first + i;
        const ModelRow<T> *row = range->state() == RangeState::READY
                                   ? &range->rows()[static_cast<std::size_t>(index - window->requested.first)]
                                   : nullptr;

        if (row && (row->id.value == 0 || row->id.generation == 0)) {
          return std::unexpected(TreeError::INVALID_ARGUMENT);
        }

        // Construct row vectors in place: debug vector moves can allocate bookkeeping.
        auto &output = next->rows.emplace_back();

        output.id = row ? std::optional{row->id} : std::nullopt;
        output.cells.reserve(visible_columns);

        for (std::size_t column = 0; column < visible_columns; ++column) {
          auto text =
            row ? columns_[column].format(row->value)
                : *GraphemeText::from(range->state() == RangeState::FAILED ? "!" : "...", options_.width_policy);

          if (text.policy() != options_.width_policy ||
              text.bytes().size() > options_.max_prepared_text_bytes - text_bytes) {
            return std::unexpected(TreeError::INVALID_ARGUMENT);
          }

          text_bytes += text.bytes().size();
          output.cells.push_back(std::move(text));
        }

        if (row && apply_intent && index == *intent_) {
          selection = row->id;
        }
      }

      std::optional<ModelRequest<T>> dispatch;
      const bool needs_request = window->requested.count != 0 && (retry_ || range->state() == RangeState::PENDING) &&
                                 (retry_ || request_.stopRequested() || request_.range() != window->requested ||
                                  request_.dataRevision() != frame.dataRevision());

      // Work is requested only after every fallible formatting/allocation step succeeded.
      // Drop our cancelled lease before requesting, so a one-slot budget can recover after caller release.
      if (needs_request) {
        request_.cancel();
        request_ = {};

        auto request = consumer_.request(window->requested);

        if (!request) {
          return std::unexpected(request.error());
        }

        dispatch.emplace(*request);
        request_ = std::move(*request);
      } else if (range->state() != RangeState::PENDING || window->requested.count == 0) {
        request_.cancel();
        request_ = {};
      }

      // From this point the presentation commit is nonallocating. Old owners survive until all fields agree.
      prepared_.swap(next);
      changed = true;
      selected_ = selection;
      scroll_ = nextScroll;

      if (!prepared_->rows.empty() && prepared_->rows.front().id) {
        anchor_ = prepared_->rows.front().id;
        anchorRevision_ = frame.dataRevision();
      } else if (frame.rowCount() == 0) {
        anchor_.reset();
      }

      dirty_ = false;
      retry_ = false;

      if (intent_ && (!apply_intent || range->state() == RangeState::READY)) {
        intent_.reset();
      }

      return dispatch;
    } catch (const std::bad_alloc &) {
      return std::unexpected(TreeError::NO_MEMORY);
    } catch (const std::length_error &) {
      return std::unexpected(TreeError::INVALID_ARGUMENT);
    } catch (...) {
      return std::unexpected(TreeError::CALLBACK_FAILED);
    }
  }

  void choose(std::uint64_t position) {
    // Intent is positional only within one data revision. Publication/reorder never rewrites a stable ID.
    const auto count = prepared_->window.visible.count;

    intent_ = position;
    intent_revision_ = prepared_->frame.dataRevision();

    (void)scroll_.ensureVisible(position, count, prepared_->frame.rowCount());
    anchor_.reset();
    dirty_ = true;
  }

  std::expected<bool, TreeError> scrollBy(double rows) {
    if (!prepared_) {
      return std::unexpected(TreeError::WOULD_BLOCK);
    }

    auto changed = scroll_.moveBy(rows, prepared_->frame.rowCount());

    if (changed && *changed) {
      anchor_.reset();
      dirty_ = true;
      intent_.reset();
      request_.cancel();
    }

    return changed;
  }

  std::expected<RowLookup, TreeError> ensureVisible(const ModelFrame<T> &frame, RowId id) {
    auto range = frame.range(consumer_.id());

    if (!range) {
      return std::unexpected(range.error());
    }

    if (!prepared_) {
      return std::unexpected(TreeError::WOULD_BLOCK);
    }

    auto location = frame.locateRow(id);

    if (!location) {
      return std::unexpected(location.error());
    }

    if (location->state != RowLookupState::FOUND) {
      return *location;
    }

    const auto height = std::max(0, prepared_->viewport.height - detail::TABLE_HEADER_HEIGHT);
    const auto capacity = static_cast<std::uint64_t>(height == 0 ? 0 : std::max(1, height / options_.row_height));

    (void)scroll_.ensureVisible(location->index, capacity, frame.rowCount());
    anchor_.reset();
    dirty_ = true;
    intent_.reset();
    request_.cancel();

    return *location;
  }

  EventReply action(Action action) {
    if (!prepared_ || prepared_->frame.rowCount() == 0) {
      return EventReply::IGNORED;
    }

    auto position = intent_.value_or(scroll_.rasterOffset());
    bool located = intent_.has_value();

    // Locate identities only in bounded visible delegates; global index lookup belongs to the provider.
    if (!located && selected_) {
      for (std::size_t i = 0; i < prepared_->rows.size(); ++i) {
        if (prepared_->rows[i].id == selected_) {
          position = prepared_->window.visible.first + i;
          located = true;

          break;
        }
      }
    }

    const auto last = prepared_->frame.rowCount() - 1;

    position = std::min(position, last);

    switch (action) {
    case Action::MOVE_UP:
      position -= located && position > 0 ? 1 : 0;

      break;
    case Action::MOVE_DOWN:
      position += located && position < last ? 1 : 0;

      break;
    case Action::MOVE_HOME:
      position = 0;

      break;
    case Action::MOVE_END:
      position = last;

      break;
    default:
      return EventReply::IGNORED;
    }

    choose(position);

    return EventReply::HANDLED;
  }

  ModelConsumer<T> consumer_;
  std::vector<TableColumn<T>> columns_;
  VirtualTableOptions options_;
  std::unique_ptr<Prepared> prepared_;
  ModelRequest<T> request_;
  std::optional<RowId> selected_;
  std::optional<std::uint64_t> intent_;
  std::uint64_t intent_revision_{};
  ScrollState scroll_;
  std::optional<RowId> anchor_;
  std::uint64_t anchorRevision_{};
  CellSize arranged_;
  bool dirty_{true};
  bool retry_{};
};

template <class T>
std::expected<std::optional<ModelRequest<T>>, TreeError>
prepareVirtualTable(WidgetTree &tree, Node<VirtualTable<T>> node, const ModelFrame<T> &frame, CellSize viewport) {
  std::expected<std::optional<ModelRequest<T>>, TreeError> result;
  bool changed{};
  auto shown = detail::listShown(tree, node);

  if (!shown) {
    return std::unexpected(shown.error());
  }

  auto edited = tree.edit(node, [&](VirtualTable<T> &view) {
    if (!*shown) {
      view.request_.cancel();
      view.request_ = {};
      view.prepared_.reset();
      view.dirty_ = true;
      view.intent_.reset();

      return;
    }

    result = view.prepare(frame, viewport, changed);
  });

  if (!edited) {
    return std::unexpected(edited.error());
  }

  if (result && changed) {
    auto invalidated = tree.invalidate(node, Invalidation::PAINT);

    if (!invalidated) {
      return std::unexpected(invalidated.error());
    }
  }

  return result;
}

template <class T>
std::expected<void, TreeError> setVirtualTableFirst(WidgetTree &tree, Node<VirtualTable<T>> node, std::uint64_t first) {
  auto edited = tree.edit(node, [&](VirtualTable<T> &view) {
    if (view.scroll_.rasterOffset() != first || view.scroll_.fraction() != 0 || view.anchor_) {
      (void)view.scroll_.setOffset(first);
      view.anchor_.reset();
      view.dirty_ = true;
      view.intent_.reset();
      view.request_.cancel();
    }
  });

  return edited ? tree.invalidate(node, Invalidation::PAINT) : edited;
}

template <class T>
std::expected<void, TreeError> setVirtualTableSelection(WidgetTree &tree, Node<VirtualTable<T>> node,
                                                        std::optional<RowId> selected) {
  if (selected && (selected->value == 0 || selected->generation == 0)) {
    return std::unexpected(TreeError::INVALID_ARGUMENT);
  }

  auto edited = tree.edit(node, [&](VirtualTable<T> &view) {
    view.selected_ = selected;
    view.intent_.reset();
  });

  return edited ? tree.invalidate(node, Invalidation::PAINT) : edited;
}

template <class T>
std::expected<void, TreeError> setVirtualTableOptions(WidgetTree &tree, Node<VirtualTable<T>> node,
                                                      VirtualTableOptions options) {
  std::expected<void, TreeError> result;
  auto edited = tree.edit(node, [&](VirtualTable<T> &view) {
    result = view.validateColumns(view.columns_, options);

    if (!result) {
      return;
    }

    view.options_ = options;
    view.dirty_ = true;
    view.request_.cancel();
  });

  if (!edited || !result) {
    return edited ? result : edited;
  }

  return tree.invalidate(node, Invalidation::LAYOUT);
}

template <class T>
std::expected<void, TreeError> setVirtualTableColumns(WidgetTree &tree, Node<VirtualTable<T>> node,
                                                      std::span<const TableColumn<std::type_identity_t<T>>> columns) {
  std::expected<void, TreeError> result;
  auto edited = tree.edit(node, [&](VirtualTable<T> &view) {
    result = view.validateColumns(columns, view.options_);

    if (!result) {
      return;
    }

    try {
      std::vector<TableColumn<T>> next(columns.begin(), columns.end());

      view.columns_.swap(next);
    } catch (const std::bad_alloc &) {
      result = std::unexpected(TreeError::NO_MEMORY);

      return;
    } catch (...) {
      result = std::unexpected(TreeError::CALLBACK_FAILED);

      return;
    }
    view.dirty_ = true;
    view.request_.cancel();
  });

  if (!edited || !result) {
    return edited ? result : edited;
  }

  return tree.invalidate(node, Invalidation::LAYOUT);
}

template <class T> std::expected<void, TreeError> retryVirtualTable(WidgetTree &tree, Node<VirtualTable<T>> node) {
  auto edited = tree.edit(node, [&](VirtualTable<T> &view) {
    view.retry_ = true;
    view.request_.cancel();
  });

  return edited ? tree.invalidate(node, Invalidation::PAINT) : edited;
}

template <class T>
std::expected<ActionBinding, TreeError> bindVirtualTable(EventRouter &router, WidgetTree &tree,
                                                         Node<VirtualTable<T>> node) {
  return router.bind(node, [&tree, node](VirtualTable<T> &view, const RoutedEvent &event) {
    EventReply reply{EventReply::IGNORED};

    if (const auto *action = std::get_if<ActionEvent>(&event)) {
      reply = view.action(action->action);
    } else if (const auto *mouse = std::get_if<MouseInputEvent>(&event); mouse && view.prepared_) {
      auto snapshot = tree.inspect(node);

      if (!snapshot || !snapshot->geometry.valid || !snapshot->geometry.clip.contains({mouse->column, mouse->row})) {
        return EventReply::IGNORED;
      }

      if (mouse->action == MouseAction::PRESS && mouse->button == MouseButton::LEFT && !view.dirty_) {
        // Router coordinates are surface cells; translate using the current arranged and clipped geometry.
        const auto y =
          static_cast<std::int64_t>(mouse->row) - snapshot->geometry.surface.origin().y - detail::TABLE_HEADER_HEIGHT;

        if (y < 0) {
          return EventReply::IGNORED;
        }

        const auto offset = static_cast<std::uint64_t>(y / view.options_.row_height);

        if (offset < view.prepared_->window.visible.count) {
          view.choose(view.prepared_->window.visible.first + offset);
          reply = EventReply::HANDLED;
        }
      } else if (mouse->action == MouseAction::SCROLL &&
                 (mouse->button == MouseButton::WHEEL_UP || mouse->button == MouseButton::WHEEL_DOWN)) {
        const auto count = view.prepared_->frame.rowCount();

        const double movement = mouse->button == MouseButton::WHEEL_UP ? -1.0 : 1.0;

        (void)view.scroll_.moveBy(movement, count);
        view.anchor_.reset();
        view.dirty_ = true;
        view.intent_.reset();
        view.request_.cancel();
        reply = EventReply::HANDLED;
      }
    }

    if (reply == EventReply::HANDLED) {
      (void)tree.invalidate(node, Invalidation::PAINT);
    }

    return reply;
  });
}

/** @brief Accumulate fractional wheel input on UI using the last prepared model count.
 * @tparam T Immutable row type satisfying VirtualTable's provider contract.
 * @param tree Borrowed UI owner, outside its frame.
 * @param node Checked non-owning view observer.
 * @param rows Finite signed movement in logical rows, positive toward the end.
 * @return Success, WOULD_BLOCK before preparation, INVALID_ARGUMENT for nonfinite movement, or tree error.
 * @note Only raster changes invalidate paint/cancel range work; fractional changes require no preparation.
 */
template <class T>
std::expected<void, TreeError> scrollVirtualTable(WidgetTree &tree, Node<VirtualTable<T>> node, double rows) {
  std::expected<bool, TreeError> result;
  auto edited = tree.edit(node, [&](VirtualTable<T> &view) {
    result = view.scrollBy(rows);
  });

  if (!edited) {
    return edited;
  }

  if (!result) {
    return std::unexpected(result.error());
  }

  return *result ? tree.invalidate(node, Invalidation::PAINT) : std::expected<void, TreeError>{};
}

/** @brief Reveal a stable row using a caller-supplied shared pin and the last prepared viewport.
 * @tparam T Immutable row type satisfying VirtualTable's provider contract.
 * @param tree Borrowed UI owner, outside its frame.
 * @param node Checked non-owning view observer.
 * @param frame Pin of the same model, used for identity and bounds; prepare this pin before painting.
 * @param id Full stable row identity; selection is unchanged.
 * @return Explicit FOUND/NOT_FOUND/UNSUPPORTED, or frame lookup/tree error; WOULD_BLOCK before preparation.
 * @note FOUND invalidates paint and cancels prior work. Zero body height cannot reveal a row;
 * a row taller than a nonempty viewport is revealed at its leading cell.
 * The caller retains dispatch responsibility for the next prepareVirtualTable result.
 */
template <class T>
std::expected<RowLookup, TreeError> ensureVirtualTableVisible(WidgetTree &tree, Node<VirtualTable<T>> node,
                                                              const ModelFrame<T> &frame, RowId id) {
  std::expected<RowLookup, TreeError> result;
  auto edited = tree.edit(node, [&](VirtualTable<T> &view) {
    result = view.ensureVisible(frame, id);
  });

  if (!edited) {
    return std::unexpected(edited.error());
  }

  if (result && result->state == RowLookupState::FOUND) {
    auto invalidated = tree.invalidate(node, Invalidation::PAINT);

    if (!invalidated) {
      return std::unexpected(invalidated.error());
    }
  }

  return result;
}

} // namespace portals

// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#pragma once
#include <array>
#include <cmath>
#include <functional>
#include <portals/canvas.hpp>
#include <portals/model.hpp>
#include <vector>

namespace portals {
/** @brief One domain bucket; finite values satisfy low <= open/close <= high. */
struct Ohlc {
  double open{};  ///< First accepted tick in the domain bucket.
  double high{};  ///< Maximum accepted tick.
  double low{};   ///< Minimum accepted tick.
  double close{}; ///< Last accepted tick.
};

/** @brief Constant-storage domain aggregation, performed before display snapshot coalescing.
 * One producer owns mutation; copy value() into immutable provider storage before publishing.
 * No allocation, I/O or UI affinity. Caller chooses bucket boundaries and tick ordering.
 */
class OhlcBucket {
public:
  /** @brief Add an ordered price tick without dropping its effect.
   * @param price Finite domain price; NaN/infinity are explicitly rejected.
   * @return True when included; false leaves the aggregate unchanged.
   */
  bool add(double price) noexcept;
  /** @brief Copy the current aggregate.
   * @return Empty before the first finite tick, otherwise an independent OHLC value.
   */
  std::optional<Ohlc> value() const noexcept;
  /** @brief Begin the next caller-defined bucket; no queued or published state is affected. */
  void reset() noexcept;

private:
  std::optional<Ohlc> value_;
};

/** @brief Inclusive logical Y range, increasing upward. Equal endpoints center finite samples. */
struct ChartRange {
  double minimum{}; ///< Finite lower bound.
  double maximum{}; ///< Finite upper bound, not less than minimum.
};
/** @brief Retained chart rendering mode. */
enum class ChartMode {
  LINE,   ///< Join consecutive finite closes, leaving gaps disconnected.
  CANDLES ///< Draw high-low wick and open-close body in each occupied column.
};

/** @brief Candle brushes; Unicode falls back to ASCII when the width policy requires wide symbols. */
enum class ChartGlyphSet {
  ASCII,      ///< Portable vertical bar and hash brushes.
  BOX_DRAWING ///< Thin wick, filled block body and horizontal doji, each exactly one cell.
};

/** @brief Owning chart settings, validated during preparation. */
struct ChartOptions {
  ChartMode mode{ChartMode::LINE}; ///< Render closes or OHLC candles.

  std::optional<ChartRange> y_range; ///< Explicit finite scale, or automatic visible-only scale.
  CellStyle background;              ///< Clear the complete local viewport with this style.
  CellStyle line{RgbColor{80, 180, 240}, {}, false, false, false};   ///< Line and isolated point style.
  CellStyle rising{RgbColor{60, 200, 120}, {}, false, false, false}; ///< Close >= open candle style.
  CellStyle falling{RgbColor{230, 90, 90}, {}, false, false, false}; ///< Close < open candle style.
  WidthPolicy width_policy;                      ///< Captured text policy; frame renderer must use the same policy.
  ChartGlyphSet glyph_set{ChartGlyphSet::ASCII}; ///< Candle appearance; ASCII preserves the portable default.
  std::int32_t candle_gap{};                     ///< Nonnegative empty columns between candles; ignored in LINE mode.
};

/// @cond INTERNAL
namespace detail {
struct ChartGlyphs {
  GraphemeText line;
  GraphemeText wick;
  GraphemeText body;
  GraphemeText doji;
};

std::expected<ChartGlyphs, TreeError> chartGlyphs(WidthPolicy policy, ChartGlyphSet symbols = ChartGlyphSet::ASCII);
bool validOhlc(const Ohlc &value) noexcept;
bool validChartOptions(ChartOptions options) noexcept;
std::expected<void, TreeError> paintChart(PaintContext &context, CellSize viewport, ChartOptions options,
                                          std::span<const std::optional<Ohlc>> samples, const ChartGlyphs &glyphs);
} // namespace detail

/// @endcond

template <class T> class Chart;
/** @brief Prepare at most one domain bucket per viewport column between frames.
 * Shares the caller's ModelFrame with sibling tables. Returns new pending-range work once;
 * the caller dispatches that token, then pins again after scheduler delivery. No workers start here.
 * Invalid OHLC ordering/nonfinite fields become gaps; provider pending/error states produce a blank plot.
 * @tparam T Immutable provider row type, borrowed by the chart's accessor.
 * @param tree UI tree owning the node; must not be inside a frame.
 * @param node Checked observing chart handle.
 * @param frame Same model's immutable pin; a foreign consumer fails before provider/accessor calls.
 * @param viewport Final local nonnegative cell extent; resize requires preparation again.
 * @param first First domain row, clamped to rowCount; no full model scan or implicit downsampling.
 * @return New request or empty optional, or tree/model/validation/callback/allocation error.
 * Failed staging preserves the old presentation. Reentrant preparation is WOULD_BLOCK.
 */
template <class T>
std::expected<std::optional<ModelRequest<T>>, TreeError> prepareChart(WidgetTree &tree, Node<Chart<T>> node,
                                                                      const ModelFrame<T> &frame, CellSize viewport,
                                                                      std::uint64_t first = 0);

/** @brief UI-owned retained plot of a bounded immutable model range.
 * Accessors run only in prepareChart, never paint/measure. They must not retain row borrows,
 * mutate the model/tree, block or perform I/O. Projected OHLC values are presentation data;
 * provider ownership is retained and the business model is never copied by the view.
 * @tparam T Immutable domain record type; the same provider may serve VirtualTable<T>.
 */
template <class T> class Chart : public Widget {
public:
  /** @brief Own one range consumer and the typed projection.
   * @param consumer Same-model UI-owned consumer transferred into the chart.
   * @param accessor Owning callable returning an OHLC value, or nullopt to break the line.
   * @param options Owning mode, scale and paint settings; invalid values fail preparation.
   */
  Chart(ModelConsumer<T> consumer, std::function<std::optional<Ohlc>(const T &)> accessor, ChartOptions options = {})
      : consumer_(std::move(consumer)), accessor_(std::move(accessor)), options_(options) {
  }

  /** @brief Cancel outstanding work on UI without waiting. */
  ~Chart() override {
    request_.cancel();
  }

  /** @return Last prepared provider readiness, or PENDING before preparation. */
  RangeState rangeState() const noexcept {
    return prepared_ ? prepared_->range.state() : RangeState::PENDING;
  }

  /** @return Retained provider revision, or zero before preparation. */
  std::uint64_t dataRevision() const noexcept {
    return prepared_ ? prepared_->frame.dataRevision() : 0;
  }

  /** @return Last prepared bounded domain interval, or empty before preparation. */

  RowRange visibleRange() const noexcept {
    return prepared_ ? prepared_->interval : RowRange{};
  }

  /** @param available Available cell extent; negatives normalize to zero.
   * @return Normalized extent without model access or allocation.
   */
  std::expected<CellSize, TreeError> measure(CellSize available) override {
    return available.normalized();
  }

  /** @param local Parent-relative bounds, borrowed by value.
   * @return Success after recording size; does not read the provider.
   */
  std::expected<void, TreeError> arrange(CellRect local) override {
    arranged_ = local.size();

    return {};
  }

  /** @param context Synchronous sink; command text borrows survive through the tree frame.
   * @return WOULD_BLOCK before preparation, INVALID_GEOMETRY after resize, or renderer/sink result.
   * No model access, delegate calls, allocation, I/O or invalidation; discard commands on error.
   */
  std::expected<void, TreeError> paint(PaintContext &context) override {
    if (!prepared_) {
      return std::unexpected(TreeError::WOULD_BLOCK);
    }

    if (arranged_ != prepared_->viewport) {
      return std::unexpected(TreeError::INVALID_GEOMETRY);
    }

    auto options = options_;

    if (const auto &style = computedStyle()) {
      options.background = style->normal;
    }

    // Every text cell replaces its complete style. Inherit the plot background
    // for brushes without an explicit fill, otherwise terminal defaults leak through.
    for (auto *brush : {&options.line, &options.rising, &options.falling}) {
      if (!brush->background) {
        brush->background = options.background.background;
      }
    }

    return detail::paintChart(context, arranged_, options, prepared_->samples, prepared_->glyphs);
  }

  /** @return Manual plot appearance beneath stylesheet declarations; series colors remain independent. */
  ComputedStyle styleDefaults() const override {
    ComputedStyle result;

    result.normal = options_.background;

    return result;
  }

private:
  template <class U>
  friend std::expected<std::optional<ModelRequest<U>>, TreeError>
  prepareChart(WidgetTree &, Node<Chart<U>>, const ModelFrame<U> &, CellSize, std::uint64_t);

  struct Prepared {
    Prepared(const ModelFrame<T> &pin, ModelRange<T> rows, CellSize size, RowRange visible, detail::ChartGlyphs symbols)
        : frame(pin), range(std::move(rows)), viewport(size), interval(visible), glyphs(std::move(symbols)) {
    }

    ModelFrame<T> frame;
    ModelRange<T> range;
    CellSize viewport;

    RowRange interval;
    detail::ChartGlyphs glyphs;
    // The count constructor may throw; MSVC Debug's noexcept default constructor can allocate a proxy.
    std::vector<std::optional<Ohlc>> samples = std::vector<std::optional<Ohlc>>(0);
  };

  std::expected<std::optional<ModelRequest<T>>, TreeError> prepare(const ModelFrame<T> &frame, CellSize viewport,
                                                                   std::uint64_t first) {
    if (preparing_) {
      return std::unexpected(TreeError::WOULD_BLOCK);
    }

    struct Guard {
      bool &value;

      explicit Guard(bool &v) : value(v) {
        value = true;
      }

      ~Guard() {
        value = false;
      }
    } guard(preparing_);

    auto consumer_range = frame.range(consumer_.id());
    auto range = consumer_range;

    if (!range) {
      return std::unexpected(range.error());
    }

    if (!accessor_ || !detail::validChartOptions(options_) || viewport.width < 0 || viewport.height < 0) {
      return std::unexpected(TreeError::INVALID_ARGUMENT);
    }

    first = std::min(first, frame.rowCount());

    const auto stride = options_.mode == ChartMode::CANDLES ? std::int64_t{options_.candle_gap} + 1 : 1;
    const auto columns = (std::int64_t{viewport.width} + stride - 1) / stride;
    const RowRange interval{
      first,
      viewport.height == 0 ? 0 : static_cast<std::size_t>(std::min<std::uint64_t>(columns, frame.rowCount() - first))};

    try {
      if (interval.count == 0) {
        range = ModelRange<T>{};
      } else if (range->range() != interval || range->state() == RangeState::PENDING) {
        range = frame.read(interval);
      }

      if (!range) {
        return std::unexpected(range.error());
      }

      if (range->state() == RangeState::PENDING && interval.count != 0) {
        auto cached = frame.readCached(*consumer_range, prepared_ ? &prepared_->range : nullptr, interval);

        if (!cached) {
          return std::unexpected(cached.error());
        }

        if (cached->dataRevision() != 0) {
          range = std::move(cached);
        }
      }

      auto glyphs = detail::chartGlyphs(options_.width_policy, options_.glyph_set);

      if (!glyphs) {
        return std::unexpected(glyphs.error());
      }

      auto next = std::make_unique<Prepared>(frame, *range, viewport, interval, std::move(*glyphs));

      next->samples.reserve(range->rows().size());

      for (const auto &row : range->rows()) {
        auto sample = accessor_(row.value);

        if (sample && !detail::validOhlc(*sample)) {
          sample.reset();
        }

        next->samples.push_back(sample);
      }

      std::optional<ModelRequest<T>> dispatch;

      if (interval.count && range->state() == RangeState::PENDING &&
          (request_.stopRequested() || request_.range() != interval ||
           request_.dataRevision() != frame.dataRevision())) {
        request_.cancel();
        request_ = {};

        auto work = consumer_.request(interval);

        if (!work) {
          return std::unexpected(work.error());
        }

        dispatch = *work;
        request_ = std::move(*work);
      } else if (!interval.count || range->state() != RangeState::PENDING) {
        request_.cancel();
        request_ = {};
      }

      prepared_.swap(next);

      return dispatch;
    } catch (const std::bad_alloc &) {
      return std::unexpected(TreeError::NO_MEMORY);
    } catch (const std::length_error &) {
      return std::unexpected(TreeError::INVALID_ARGUMENT);
    } catch (...) {
      return std::unexpected(TreeError::CALLBACK_FAILED);
    }
  }

  ModelConsumer<T> consumer_;
  std::function<std::optional<Ohlc>(const T &)> accessor_;
  ChartOptions options_;
  ModelRequest<T> request_;
  std::unique_ptr<Prepared> prepared_;
  CellSize arranged_;
  bool preparing_{};
};

template <class T>
std::expected<std::optional<ModelRequest<T>>, TreeError> prepareChart(WidgetTree &tree, Node<Chart<T>> node,
                                                                      const ModelFrame<T> &frame, CellSize viewport,
                                                                      std::uint64_t first) {
  std::expected<std::optional<ModelRequest<T>>, TreeError> result;
  auto edited = tree.edit(node, [&](Chart<T> &chart) {
    result = chart.prepare(frame, viewport, first);
  });

  if (!edited) {
    return std::unexpected(edited.error());
  }

  if (result) {
    auto invalidated = tree.invalidate(node, Invalidation::PAINT);

    if (!invalidated) {
      return std::unexpected(invalidated.error());
    }
  }

  return result;
}
} // namespace portals

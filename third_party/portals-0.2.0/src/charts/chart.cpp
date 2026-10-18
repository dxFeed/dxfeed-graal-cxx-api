// Copyright (c) 2026 ttldtor.
// SPDX-License-Identifier: BSL-1.0
#include <portals/chart.hpp>

namespace portals {
bool OhlcBucket::add(double price) noexcept {
  if (!std::isfinite(price)) {
    return false;
  }

  if (!value_) {
    value_ = Ohlc{price, price, price, price};
  } else {
    value_->high = std::max(value_->high, price);
    value_->low = std::min(value_->low, price);
    value_->close = price;
  }

  return true;
}

std::optional<Ohlc> OhlcBucket::value() const noexcept {
  return value_;
}

void OhlcBucket::reset() noexcept {
  value_.reset();
}

namespace detail {
std::expected<ChartGlyphs, TreeError> chartGlyphs(WidthPolicy policy, ChartGlyphSet symbols) {
  // Box-drawing symbols are ambiguous-width characters. Keep the entire set
  // one cell wide instead of letting individual brush writes fail during paint.
  const bool unicode = symbols == ChartGlyphSet::BOX_DRAWING && policy.ambiguous == AmbiguousWidth::NARROW;
  auto line = GraphemeText::from("*", policy);
  auto wick = GraphemeText::from(unicode ? "\xe2\x94\x82" : "|", policy);
  auto body = GraphemeText::from(unicode ? "\xe2\x96\x88" : "#", policy);
  auto doji = GraphemeText::from(unicode ? "\xe2\x94\x81" : "#", policy);

  if (!line || !wick || !body || !doji) {
    return std::unexpected(TreeError::INVALID_ARGUMENT);
  }

  return ChartGlyphs{std::move(*line), std::move(*wick), std::move(*body), std::move(*doji)};
}

bool validOhlc(const Ohlc &v) noexcept {
  return std::isfinite(v.open) && std::isfinite(v.high) && std::isfinite(v.low) && std::isfinite(v.close) &&
         v.low <= v.open && v.low <= v.close && v.high >= v.open && v.high >= v.close;
}

bool validChartOptions(ChartOptions options) noexcept {
  return (options.mode == ChartMode::LINE || options.mode == ChartMode::CANDLES) &&
         (options.glyph_set == ChartGlyphSet::ASCII || options.glyph_set == ChartGlyphSet::BOX_DRAWING) &&
         options.candle_gap >= 0 &&
         (!options.y_range || (std::isfinite(options.y_range->minimum) && std::isfinite(options.y_range->maximum) &&
                               options.y_range->minimum <= options.y_range->maximum));
}

namespace {
ChartRange visibleScale(std::span<const std::optional<Ohlc>> samples, ChartOptions options) {
  if (options.y_range) {
    return *options.y_range;
  }

  std::optional<ChartRange> result;

  for (const auto &sample : samples) {
    if (!sample) {
      continue;
    }

    const auto low = options.mode == ChartMode::LINE ? sample->close : sample->low;

    const auto high = options.mode == ChartMode::LINE ? sample->close : sample->high;

    if (!result) {
      result = ChartRange{low, high};
    } else {
      result->minimum = std::min(result->minimum, low);
      result->maximum = std::max(result->maximum, high);
    }
  }

  return result.value_or(ChartRange{});
}

// In-range inputs make subtraction safe except when the endpoints have opposing
// extreme signs. Halving only that case avoids overflow without erasing subnormals.
double fraction(double value, ChartRange range) {
  const auto extent = range.maximum - range.minimum;

  if (std::isfinite(extent)) {
    return (value - range.minimum) / extent;
  }

  return (value / 2 - range.minimum / 2) / (range.maximum / 2 - range.minimum / 2);
}

std::int32_t row(double value, ChartRange range, std::int32_t height) {
  if (range.minimum == range.maximum) {
    return (height - 1) / 2;
  }

  const auto position = fraction(std::clamp(value, range.minimum, range.maximum), range);

  return static_cast<std::int32_t>(std::llround((1 - position) * (height - 1)));
}

std::expected<void, TreeError> paintSegment(Canvas &canvas, double first, double last, std::int32_t left,
                                            ChartRange scale, std::int32_t height, CellStyle style) {
  if (std::max(first, last) < scale.minimum || std::min(first, last) > scale.maximum) {
    return {};
  }

  const auto first_y = std::clamp(first, scale.minimum, scale.maximum);

  const auto last_y = std::clamp(last, scale.minimum, scale.maximum);
  const ChartRange segment{std::min(first, last), std::max(first, last)};

  auto x = [&](double value, double original, std::int32_t unchanged) {
    if (value == original) {
      return unchanged;
    }

    const auto t = first < last ? fraction(value, segment) : 1 - fraction(value, segment);

    return static_cast<std::int32_t>(std::llround(left + t));
  };
  // Clip in logical space before cell conversion. Both endpoints can be far outside;
  // retaining their crossing avoids inventing gaps or overflowing integer coordinates.

  return canvas.line({x(first_y, first, left), row(first_y, scale, height)},
                     {x(last_y, last, left + 1), row(last_y, scale, height)}, style);
}

std::expected<void, TreeError> paintLine(Canvas &canvas, std::span<const std::optional<Ohlc>> samples, ChartRange scale,
                                         std::int32_t height, CellStyle style) {
  std::optional<double> previous;

  for (std::size_t index = 0; index < samples.size(); ++index) {
    if (!samples[index]) {
      previous.reset();

      continue;
    }

    const auto value = samples[index]->close;

    std::expected<void, TreeError> result;

    if (previous) {
      result = paintSegment(canvas, *previous, value, static_cast<std::int32_t>(index) - 1, scale, height, style);
    } else if (value >= scale.minimum && value <= scale.maximum) {
      result = canvas.point({static_cast<std::int32_t>(index), row(value, scale, height)}, style);
    }

    if (!result) {
      return result;
    }

    previous = value;
  }

  return {};
}
} // namespace

std::expected<void, TreeError> paintChart(PaintContext &context, CellSize viewport, ChartOptions options,
                                          std::span<const std::optional<Ohlc>> samples, const ChartGlyphs &glyphs) {
  const auto bounds = CellRect::from({}, viewport);

  if (!bounds) {
    return std::unexpected(TreeError::INVALID_GEOMETRY);
  }

  if (viewport.width == 0 || viewport.height == 0) {
    return {};
  }

  const std::array<RenderCommand, 1> background{FillCommand{*bounds, *bounds, options.background}};

  auto result = context.submit(background);

  if (!result) {
    return result;
  }

  const auto scale = visibleScale(samples, options);

  if (options.mode == ChartMode::LINE) {
    Canvas line(context, *bounds, &glyphs.line);

    return paintLine(line, samples, scale, viewport.height, options.line);
  }

  Canvas wick(context, *bounds, &glyphs.wick);

  Canvas body(context, *bounds, &glyphs.body);
  Canvas doji(context, *bounds, &glyphs.doji);
  const auto stride = std::int64_t{options.candle_gap} + 1;

  for (std::size_t index = 0; index < samples.size(); ++index) {
    const auto column = static_cast<std::int64_t>(index) * stride;

    if (column >= viewport.width) {
      break;
    }

    if (!samples[index]) {
      continue;
    }

    const auto &sample = *samples[index];

    if (sample.high < scale.minimum || sample.low > scale.maximum) {
      continue;
    }

    const auto x = static_cast<std::int32_t>(column);

    const auto style = sample.close >= sample.open ? options.rising : options.falling;
    result =
      wick.line({x, row(sample.high, scale, viewport.height)}, {x, row(sample.low, scale, viewport.height)}, style);

    if (!result) {
      return result;
    }

    if (std::max(sample.open, sample.close) < scale.minimum || std::min(sample.open, sample.close) > scale.maximum) {
      continue;
    }

    const auto open = row(sample.open, scale, viewport.height);
    const auto close = row(sample.close, scale, viewport.height);

    result = open == close ? doji.point({x, open}, style) : body.line({x, open}, {x, close}, style);

    if (!result) {
      return result;
    }
  }

  return {};
}
} // namespace detail
} // namespace portals

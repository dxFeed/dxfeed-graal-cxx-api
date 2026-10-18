# Canvas and model-backed charts

`Canvas` emits clipped widget-local cells during a retained widget's `paint`.
`Chart<T>` is a retained widget: prepare it between frames, then use the normal
`WidgetTree` layout/paint and renderer path. Neither component owns terminal I/O
or a frame loop. See [model](model.md), [widget tree](widget-tree.md),
[renderer](render.md), and [scheduler](scheduler.md).

## One provider, two views

A table and chart use independent range consumers of the **same** `Model<T>`.
Pin once after scheduler dispatch and pass that pin to both preparations:

```cpp
// tree, parent and model are existing UI-owned objects; unwrap handles expected errors.
auto chart = unwrap(tree.emplace<portals::Chart<portals::Ohlc>>(
  parent, unwrap(model.openRange()),
  [](const portals::Ohlc &bar) { return std::optional{bar}; },
  portals::ChartOptions{.mode = portals::ChartMode::CANDLES}));

auto frame = unwrap(model.pin());
auto chartWork = unwrap(portals::prepareChart(tree, chart, frame, {80, 20}));
auto tableWork = unwrap(portals::prepareVirtualTable(tree, table, frame, {40, 20}));
// Dispatch returned work receipts through the application's existing provider.
// Then arrange and paint both widgets inside tree.frame, rendering before it returns.
```

The accessor borrows a typed immutable row, returning an optional `Ohlc`
presentation value. A custom record can expose its OHLC member without copying
its business model. The retained range and frame keep provider storage alive.
Accessors run only during preparation and must not retain row borrows, mutate the
tree/model, block, or perform I/O. Getter reads use `tree.read`; all view mutation,
preparation and destruction belong to the UI owner.

`prepareChart(..., viewport, first)` shows consecutive domain buckets beginning
at `first`, one bucket per column by default. In candle mode, `candle_gap` adds
nonnegative empty columns between buckets. The visible count is rounded up from
`viewport.width / (candle_gap + 1)`; consecutive domain records are retained,
never skipped. Line mode ignores candle spacing. It reads at most `viewport.width` rows and
never scans the full model. Empty/zero-height windows read no rows. A provider
that has finer-grained history must aggregate to the desired domain resolution
before publication; the chart does not silently skip domain records. The Model's
range budget remains authoritative: an oversized requested viewport is an error.

`ChartOptions::glyph_set` selects portable `ASCII` candles (`|` / `#`) or
`BOX_DRAWING` candles with thin wicks, filled block bodies and horizontal doji. Unicode
brushes fall back to ASCII under an ambiguous-wide width policy so every brush
still occupies exactly one cell. The dashboard selects Unicode by default and
ASCII with `--ascii`, with one empty column between candles in both modes;
the library preserves its portable, densely packed ASCII default.

A chart bound to `StyleEngine` uses the resolved normal style for its plot
background. Line and candle brushes inherit that background unless their own
`CellStyle::background` is explicit. Their foreground colors remain independent,
preserving rising/falling meaning across theme changes. Without a theme, the
same inheritance uses `ChartOptions::background`. Theme replacement repaints
the prepared chart without rereading the model.

Pending reads return one `ModelRequest<T>` for caller dispatch. Complete it with
an immutable same-revision provider result, dispatch the scheduler, pin again,
and prepare. Pending/failed data paint a blank background and are distinguishable
through `rangeState()`. New publication cancels stale work through the existing
Model contract. Removing a chart cancels its outstanding request; no worker is
joined. Failed preparation preserves the prior presentation. A foreign pin is
rejected before provider reads or accessor calls.

## Domain aggregation precedes display coalescing

Feed **every ordered tick** into the producer-owned `OhlcBucket`. Its constant-size
state preserves first open, maximum high, minimum low and last close. `add`
returns false for NaN/infinity without changing the aggregate. The application
owns tick ordering, bucket boundaries, history retention and rejected-tick policy.
Call `reset()` only when starting the next domain bucket.

Publish immutable snapshots of that aggregate; do not put raw ticks in a
replace-latest display queue. `Model::publisher()` currently uses rejecting
publication. Keep one application-owned pending snapshot between the existing
Scheduler display ingress and the Model queue. Both the ingress callback and
retry step below execute on UI; producer threads only use `ingress.publisher`.

```cpp
using Snapshot = portals::ModelSnapshot<portals::Ohlc>;
std::shared_ptr<const Snapshot> pendingLatest;

auto tryForward = [publisher = model.publisher(), &pendingLatest]()
  -> std::expected<void, portals::SchedulerError> {
  if (!pendingLatest) {
    return {};
  }

  auto sent = publisher.publish(pendingLatest); // Copy ownership; rejection must retain it.
  if (!sent) {
    return std::unexpected(sent.error());
  }

  pendingLatest.reset();
  return {};
};
auto ingress = unwrap(scheduler.connect<Snapshot>(
  scope, [&pendingLatest](auto snapshot) { pendingLatest = std::move(snapshot); },
  portals::SchedulerLane::DATA, portals::SchedulerOverflow::REPLACE_LATEST));

// Run this step after each bounded dispatch in the application's existing loop.
// A later snapshot may replace pendingLatest: it already includes every domain tick.
unwrap(scheduler.dispatch());
if (auto sent = tryForward(); !sent && sent.error() != portals::SchedulerError::WOULD_BLOCK) {
  reportError(sent.error()); // Application handles closure/failure; the owner is still retained.
}
// WOULD_BLOCK leaves pendingLatest for the next cycle, even if the producer has stopped.
```

Disconnect the ingress before destroying its borrowed pending slot. The scope
outlives the connection. Snapshots must arrive in increasing revision order and
include prior domain contributions. This wiring uses one extra owning pointer,
not a second unbounded queue or a new model. A producer may fill a queue slot
between dispatch and forwarding; retaining the pointer makes that race retryable.
Only successful queue admission clears the slot. The application must handle
ingress publication errors and non-backpressure forwarding errors explicitly.

The caller gates painting with its existing monotonic-clock frame deadline,
independently of tick rate, and services control/data between frames. Forwarded
Model publications apply during subsequent scheduler dispatch, not at admission.
Queue acceptance alone does not mean that the model applied a revision; inspect
the pin or `Model::lastError()`. Do not busy-loop retry inside a callback.

## Coordinates, gaps and paint

Logical Y grows upward; cell Y grows downward. Automatic Y scale reads only
prepared valid samples (close for line, high/low for candles). An explicit finite
`ChartRange` overrides it. The maximum maps to row zero and minimum to the last
row, with nearest-cell rounding. Equal bounds center samples vertically. Single
points remain visible. Scaling handles subnormal values and opposite finite
double extremes without converting a nonfinite coordinate to an integer.

An absent accessor result, any nonfinite OHLC field, or invalid OHLC ordering
(`low <= open/close <= high`) is a gap. Consecutive valid closes connect; gaps
break the line. Finite points outside an explicit Y window remain part of their
segment: logical clipping retains crossings before cell conversion. Candles
clip their high-low wick and open-close body separately. An entirely offscreen
segment/candle emits no data cells. Empty data only clears the background.

The line brush is `*`; the default ASCII candle wick is `|` and its body is `#`.
The `BOX_DRAWING` set uses a horizontal doji whenever open and close map to the
same screen row, including values made indistinguishable by rounding or clipping.
`ChartOptions` selects mode, explicit/automatic scale, background, line, rising
and falling styles, width policy, and candle glyph set. Both candle directions work without color
through body/wick geometry; provide styles suited to the target terminal. A
resized chart must be prepared with its final local viewport before painting;
a mismatch returns `INVALID_GEOMETRY`.

For custom widget paint, `Canvas(context, clip)` draws styled spaces (usually a
background color). Pass an optional one-cell `GraphemeText` brush to draw glyphs.
The brush must remain unchanged through rendering of the enclosing tree frame;
Canvas does not own it. Points are clipped, and lines use inclusive dominant-axis
DDA with ties rounded away from zero. The line iterates only clipped dominant-axis
coordinates even for INT32_MIN/MAX endpoints. The frame sink still applies
surface translation and ancestor clipping. Errors propagate, and the caller must
discard partial commands when painting fails.

## Verification and limits

`portals.chart` checks exact cells and wick/body glyphs, an independent full-line
raster oracle followed by clipping, huge offscreen endpoints, OHLC aggregation,
empty/single/gap/degenerate/extreme data, shared row identity with a table,
preparation failure/reentry, foreign pins and pending completion. Allocation fault-injection
checks every preparation allocation boundary and preserves old cells, revision and
invalidation after failure. A one-slot saturation test retains the last display
snapshot through WOULD_BLOCK and delivers it without producer republication; a
new aggregate can replace the retained snapshot before retry. A real Scheduler
with injected time demonstrates 1,000 and 7,000 updates per simulated second at
the same ten-frame cap, while checking every published bucket against a separate
OHLC oracle. These are deterministic correctness tests, not CPU benchmarks.

No indicators, exchange connection, automatic time-bucketing, resampling,
interactive chart navigation, axis labels or standalone application runner are
provided by this component.

## Reusing completed pages during navigation

Chart preparation can reuse its retained immutable completion snapshot for a new
consecutive interval under the current model pin's revision, row count and read
budget. Providers may prefetch a bounded page around an exact request. Only the
current interval is sampled; paint performs no cache reads. A miss remains pending,
and failed provider reads cannot fall back to older ready data. This does not
merge consumer results or change `ModelFrame::read` base-snapshot semantics.

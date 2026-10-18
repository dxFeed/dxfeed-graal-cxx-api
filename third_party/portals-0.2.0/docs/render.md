# Full-frame reference renderer

`<portals/render.hpp>` provides `renderFullFrame`: a synchronous scene-to-cells
oracle. Every call starts with a fresh blank `CellBuffer`. It has no dependency on
diff, damage tracking, a terminal writer, widgets or a style engine, so later
optimized output can be checked against its semantic result.

## Commands and ordering

A `RenderCommand` is a `TextCommand` or `FillCommand`. All geometry uses the
[checked half-open cell geometry](geometry.md). Smaller signed `z` values paint
first; equal values preserve the supplied sequence order. Input order is never
mutated. There are no implicit layers or retained scene state.

`TextCommand` contains an origin, a reference to validated `GraphemeText`, an
explicit clip, a `CellStyle` and a z value. It paints one horizontal run. Each
cluster advances by its measured width even when clipped. Controls and unsupported
clusters display the text component's single-cell `?` placeholder; newline and
tab do not move the cursor. Width policy, including its revision, must match the
frame policy for every run, even an empty or offscreen run.

`FillCommand` paints styled ASCII spaces in `area ∩ clip ∩ surface`. These are
written lead cells, including with default style; untouched cells remain the
buffer's default blank cells. Empty fills do no cell work.

```cpp
const auto text = portals::GraphemeText::from("hello");
const auto clip = portals::CellRect::from({0, 0}, {20, 2});
if (!text || !clip) {
  return;
}

const std::array<portals::RenderCommand, 1> scene{
    portals::TextCommand{{1, 0}, *text, *clip}};
auto frame = portals::renderFullFrame({20, 2}, scene);
if (!frame) {
  // Handle the owning RenderError before presenting output.
  return;
}
```

The example requires `<array>` and `<portals/render.hpp>`. Construct text once
under the chosen frame policy; parsing Unicode during widget paint is unnecessary.

## Whole clusters and clipping

New graphemes must fit completely in both clip and surface. Neither original
UTF-8 nor wide-cell pairs are split at either boundary. Negative origins are valid;
text advancement uses `int64_t`, so adding a run's measured width to an extreme
`int32_t` origin cannot wrap. A coordinate is narrowed only after its complete
footprint fits the nonnegative surface.

Overlays follow [CellBuffer's atomic old-cluster rule](cell-buffer.md): touching
an old continuation removes its lead too. For example, a wide glyph at columns
0–1 followed by a clipped narrow write at column 1 yields blank at 0 and the new
glyph at 1. Cleanup can cross the overlay clip. This deliberate consequence
preserves complete old clusters instead of leaving orphan halves. A fill has the
same rule, including when its cells intersect two different old wide clusters.

## Ownership, errors and thread use

The scene span and text references are borrowed only until the call returns.
Keep them alive and unchanged for that duration. The renderer retains no pointer,
mutates no input and triggers no callback. The returned buffer owns every display
byte and style; destroying the scene or text does not affect it. Subsequent views
follow `CellBuffer`'s documented mutation/move invalidation rules.

There is no terminal or UI thread affinity. Independent calls and concurrent
reads of unchanged inputs are safe; synchronizing input mutation is the caller's
responsibility. UI scheduling and coherent model/style revisions remain outside
this component. Width-policy changes require the caller to rebuild text and
invalidate relevant layout/paint.

`RenderLimits` bounds command count, aggregate original text bytes and work units.
A repeated run consumes its bytes again. Work means all input text graphemes,
including clipped runs, plus visible fill cells; it is not a CPU-time measurement.
Sorting is bounded by command count, and intern lookup costs remain bounded by
the explicit buffer caps. Limits reject a whole scene without truncation, retry
or a partial result. Zero scene caps accept only inputs with zero counted cost.

Scene preflight checks sums/products before allocation or overflowing arithmetic.
The buffer's extent/pool/width-policy failures are returned as `buffer_error` with
the underlying `CellBufferError`. Renderer temporary storage failure becomes
`ALLOCATION_FAILURE`. No failure changes a previous frame, because no previous
frame is supplied. There is no output encoding or terminal I/O here.

## Verification

`portals.render` checks semantic snapshots of empty scenes, stable overlap order,
styles, CJK/combining clipping, complete wide pairs, old-wide cleanup across clip,
fill intersections, unsupported controls, input destruction, matching/mismatched
width policies, negative/extreme origins and explicit scene/buffer caps. A separate
ASCII cell oracle checks 60 deterministic generated scenes without using renderer
ordering or numeric intern IDs. Existing geometry/text/buffer tests remain active.

Compare frames by resolved glyph bytes, style, cell role and width. Glyph/style
IDs are buffer-local and are not a semantic equality contract across frames.

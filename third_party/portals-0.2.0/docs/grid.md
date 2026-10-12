# Grid and Overlay

`<portals/grid.hpp>` adds sparse `Grid` and `Overlay` containers to the existing
`Layout` / `SizedWidget` contracts. Run `layoutTree` inside the same tree frame
that paints and renders. These classes perform no tree traversal, I/O, model
mutation, or hit testing. Input spans are borrowed only during each method call.

## Tracks and placement

`GridOptions` owns explicit, nonempty `columns` and `rows`, each using `AxisSize`.
Every visible child needs one `GridCell` keyed by its complete `NodeId`.
Placement order does not reorder children. Hidden or removed node entries remain
stored, never rebind to another identity, and contribute no intrinsic demand.
Remove stale entries when changing the configuration to reclaim placement budget.
A replacement node needs its own placement, even when it reuses a registry slot.

All configured spans are validated, including hidden/stale entries: spans must be
positive and wholly inside the explicit axes. Duplicate identities are invalid.
Overlap is allowed; later siblings paint above earlier siblings when the frame
owner paints in tree order, matching EventRouter's reverse-sibling hit testing.
There is no dense rows-times-columns allocation or automatic track creation.
`max_tracks_per_axis` and `max_placements` are positive configurable work budgets
(default 4096 each); exceeding them returns `NODE_LIMIT`. Inactive placements
count toward the budget. Track counts must fit the cell representation.

Each track starts at its preferred size (or zero for intrinsic), clamped to
min/max. Intrinsic nonfixed tracks absorb child demand: single-track children
first, then spanning children in sibling order. A span includes its internal
gaps. Unmet demand is divided equally among eligible tracks, with remainder
cells assigned in track order; capped tracks yield remaining demand to others.
Explicit preferred and fixed tracks do not absorb intrinsic demand. This is a
deterministic bounded policy, not CSS Grid or a global constraint solver.

After measurement, each axis uses the existing Row allocator: flex weights
share spare space with stable remainders and caps, later automatic/flex tracks
shrink first under shortage, and minimum/fixed sizes survive. Spanning children
use the rectangle from their first track to the end of their last track.

## Alignment, clipping and stacking

Each GridCell has horizontal and vertical `LayoutAlign`. Overlay supplies the
same two alignments for every child in one shared padded content rectangle.
Overlay measures the maximum requested child width and height plus padding.
Centering assigns `floor(free / 2)` before the child and the remainder after it.
Stretch fills the slot subject to child min/max; fixed child extents remain fixed.
A flex child fills available space even with nonstretch alignment, consistently
with Row/Column cross-axis policies. Negative free space gives zero offset.

Minimum/fixed children can exceed their cell. Cells are allocation regions, not
extra clipping ancestors: overflow may overlap adjacent cells and is clipped by
the actual widget ancestors and viewport. The arranged NodeGeometry is the one
source for paint clipping and EventRouter. A frame owner must translate paint
commands into surface coordinates, intersect their clips with NodeGeometry.clip,
paint siblings in tree order, and render borrowed text before leaving the frame.
Explicit command z values remain the frame owner's responsibility.

## Mutation and errors

Constructors own their configuration. Invalid constructor settings are reported
by layout methods. Use `setGridOptions` and `setOverlayOptions` on the UI
owner between frames; successful changes invalidate layout and paint. The grid
setter prepares a full copy before replacement, so failed validation/allocation
preserves the old settings. A visible child without a placement is an error at
layout time; future/hidden identities may already have stored placements.
`options()` returns an owning copy on UI and can throw `bad_alloc` for Grid.

Malformed policies, alignments, identities and spans return `INVALID_ARGUMENT`.
Coordinate/extent overflow returns `INVALID_GEOMETRY`, work budget overflow
returns `NODE_LIMIT`, and layout allocation failure returns `NO_MEMORY`.
Tree operations additionally preserve their owner-thread and lifetime errors.
No callback retains the child spans or attempts tree/model mutation.

`portals.layout-grid` exercises nested placement, spans, identity, rounding,
shortage and actual layout/paint/render/FrameWriter/TestTerminal/EventRouter
agreement. See also [linear layouts](layout.md), [geometry](geometry.md), and
[the frame ownership contract](widget-tree.md).

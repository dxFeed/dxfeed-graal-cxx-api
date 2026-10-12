# Row, Column and size policies

All sizes are integer terminal columns/rows. Rectangles are parent-local,
half-open values; `WidgetTree::arrange` translates and intersects clips through
the checked geometry API. Row orders visible children left to right; Column
orders them top to bottom. Disabled children still occupy space. Hidden children
and their descendants are never measured or arranged and contribute no gap.

Each child may implement the const `LayoutItem` interface. Ordinary widgets use
automatic policies and their guarded `Widget::measure` result. `SizedWidget`
owns a convenient policy pair. Each axis has minimum, preferred, maximum,
mode and a positive flex weight. `INTRINSIC_EXTENT` selects the measured size;
other preferred values are nonnegative. Preferred is clamped to minimum/maximum.
Invalid bounds, modes, weights and options return `TreeError::INVALID_ARGUMENT`.

Fixed children preserve their clamped preferred extent. Automatic children use
their preferred extent. Flex children also start at their clamped preferred
extent, then share spare space in proportion to weights. Each round assigns
integer quotients and gives remaining cells in stable child order, skipping
children whose maximum has been reached. Capped shares are redistributed; when
all flex maxima are reached, unused space remains. Equal weights in 11 cells
therefore produce widths 6 and 5, in insertion order.

When space is short, later automatic/flex children yield preferred cells first,
down to their minima. Fixed extents and all minima survive. `LayoutOverflow::CLIP`
preserves these extents even when children/gaps/padding exceed the available
space; ancestor clips remove the overflow from painting and hit testing.
There is no negative size or implicit compression below minimum. Representational
overflow is an explicit `INVALID_GEOMETRY`, rather than wrapping coordinates.

Padding is applied on each side; gaps occur only between participating children.
Main alignment applies to the remaining group space after allocation. Start and
stretch start at the leading inset, center gives floor(free/2) before the group,
and end consumes all spare space before it. Cross stretch (or cross flex) fills
the available extent subject to bounds; fixed remains fixed. Cross start/center/end
aligns the resulting child extent using the same floor rule. Underflow aligns
at the leading inset because there is no spare space.

## Frame ownership and errors

`layoutTree` is an external two-phase owner-thread traversal. It obtains owning
structure/policy/measurement snapshots through `inspect` and `read`, measures
children before parents, then calls guarded `arrange` on parents before children.
Its temporary traversal table dies when the pass returns; it is not another
retained tree. A plain parent supports a single-child viewport passthrough;
multiple children require a `Layout` container.

The caller must invoke `layoutTree` **inside an already active `tree.frame`**.
The pass does not create a frame. This precondition is contractual: the current
tree API has no active-frame query. One frame can encompass layout, paint and
rendering of borrowed commands:

A preliminary pass may instead capture an owning `LayoutSnapshot`, as the
[layout-driven view preparation](virtual-preparation.md) runner does. It does
not retain widget borrows or render commands; final layout, paint and rendering
still share one frame.

```cpp
// Between frames, after a viewport change:
auto invalidation = tree.invalidate(root, portals::Invalidation::LAYOUT);
if (!invalidation) {
  return invalidation;
}

std::expected<void, portals::TreeError> layout;
auto frame = tree.frame([&] {
  layout = portals::layoutTree(tree, root, viewport, viewport);

  if (!layout) {
    return;
  }

  // Paint the arranged tree and render borrowed commands before returning.
});
```

`setSizePolicy` and `setLinearOptions` validate owning values, edit on the
UI owner between frames, and request layout invalidation. Construction and other
custom policy sources must follow the same invalidation discipline. Viewport
changes likewise require `invalidate(layout)` before entering the next frame.
After a failed pass the coordinator must abort use of that pass, including paint
and hit testing; the solver does not promise transactional rollback or automatic
invalidation from inside a frame. Invalidated geometry is never a valid old layout.

All tree errors propagate; arithmetic failures return `INVALID_GEOMETRY`, library
allocation failures return `NO_MEMORY`, and thrown custom callbacks follow
`tree.read`/measure/arrange's `CALLBACK_FAILED` boundary. No callback or span is
retained. Pure `Layout` and `LayoutItem` callbacks run inside a checked read borrow
in the enclosing frame and must not call tree operations, recursively dispatch
layout, mutate models or perform I/O. A scoped thread-local guard technically
rejects recursive `layoutTree` calls with `WOULD_BLOCK`. The tree technically
protects its ordinary Widget hooks and rejects editing inside frames. Arbitrary
model mutation/I/O and direct tree calls from a custom pure callback remain
contract violations; the library does not claim to sandbox user C++ code.

## Extending layouts and controls

Controls can derive from `Widget` and provide intrinsic measure only, or derive
from `SizedWidget` for configurable policies. Grid/Overlay can implement `Layout`:
`childAvailable` supplies measurement bounds, `measureChildren` aggregates owning
`MeasuredChild` values (including stable `NodeId` for matching placements after hiding siblings), and `arrangeChildren` returns exactly one owning local
rectangle per visible child. The interface pointer remains inside `tree.read`.
A custom solver must follow the frame/purity/error contract above. No Node
downcast or core Widget header change is required.

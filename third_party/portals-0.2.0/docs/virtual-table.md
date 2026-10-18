# VirtualTable

`VirtualTable<T>` presents immutable typed records through contiguous fixed-width
columns and fixed-height rows. The first cell row is always the header. A model
with no rows still displays its headers; an empty column set displays a blank
viewport. There are no row or cell widgets, editing controls, or implicit workers.

## Ownership and preparation

Create the table on the UI owner with an independent `ModelConsumer<T>` from
`Model<T>::openRange()`, a span of `TableColumn<T>`, and `VirtualTableOptions`.
The constructor synchronously copies the definitions, headers, and formatter
captures. The span and its elements need live only until that call returns.
`T` need not be copyable or default-constructible: a formatter borrows `const T&`
from the immutable snapshot and returns an owning `GraphemeText`.

Each column has a unique nonzero `ColumnId` pair, positive width in cells,
header text, and a copyable formatter. Identity remains independent of visual
order; providers must increment the generation when reusing an identity.
Headers and formatter results use the options' width policy. Formatters must
not perform I/O, mutate the tree/model, keep row borrows, or allocate unbounded
temporary storage. Exceptions are translated at preparation, except that
termination inside a user callback cannot be recovered by the table.

Between frames, apply queued model publications, obtain one `ModelFrame<T>`,
and pass that same pin to `prepareVirtualTable` and sibling views. The local
viewport includes the header and must match the final arranged size exactly.
The application supplies this final size from its layout constraints. Preparation
must not run during `WidgetTree::frame`.

Preparation reads one viewport-plus-overscan range for all columns. Only ready
rows intersecting the body viewport and columns intersecting its width are
formatted. Overscan rows are requested but never formatted. The table stages
owning cell text, header text, stable IDs, and a copy of the pin together. A
formatter or allocation failure preserves the previous successful presentation.
A successful unchanged preparation does not invalidate or schedule another frame.
Updates outside the viewport may reprepare the bounded window; they never scan
the logical row count or locate IDs globally.

The table reuses VirtualList's checked window arithmetic and ancestor visibility
inspection. Its column clipping and header input geometry remain local to the
table; the range-request and navigation protocol follows VirtualList without a
new shared widget framework. See [the model contract](model.md) and
[VirtualList preparation](virtual-list.md).

A provider completion may contain a bounded prefetched page around the requested
interval. During later preparation the view can read the new exact window from
its retained completion snapshot when both data revision and row count match the
current pin. It retains one such owner, formats only current visible rows, and
respects the model's read budget. A cache miss remains pending and schedules the
new exact request; provider failures are not replaced by older ready data. A
revision change or hidden view releases the old prepared owner. Public base reads
and independent consumer results retain their existing exact semantics.

## Rendering, requests, and selection

`measure`, `arrange`, and `paint` do no provider reads, formatting, mutation,
loading, or terminal I/O. Paint emits widget-local commands. The application
translates them through current node geometry and ancestor clipping, then renders
borrowed command text before the enclosing `tree.frame` returns. Each header and
cell has its own half-open clip; a wide grapheme never spills into the next column.
The last fixed-height row may be partially clipped. The body and header backgrounds
cover unused cells deterministically.

Pending cells show `...`; failed cells show `!`. Preparation returns each new
`ModelRequest<T>` once for caller dispatch and never starts a worker. Keep the
receipt until completion, or cancel it if dispatch fails. `retryVirtualTable`
requests a fresh receipt on the next preparation. Cancelled receipts still occupy
the model's in-flight budget until their owners release them. Hidden ancestors and
destruction cancel requests without joining workers; hidden preparation releases
the prepared range and text while retaining the selected stable ID.

`bindVirtualTable` uses the existing router for arrows, Home/End, left press, and
integral wheel movement. Keep the returned scoped `ActionBinding`; the tree/router
outlive it. A header click does not select a row. Body clicks account for surface
origin, clipping, header height, and fixed row height. Selection uses `RowId`, spans
the whole body row, and survives offscreen movement or reordering. Removed IDs
remain selected until explicitly cleared. Navigation searches only the prepared
visible IDs; pending positional intent is scoped to its data revision.

## Mutation, budgets, and errors

Use the tree-checked free functions on the owner thread:

- `setVirtualTableFirst` sets an integral first row, cancels old work/intent, and
  invalidates paint; preparation clamps it to the model bounds.
- `setVirtualTableSelection` changes a stable ID and invalidates paint only.
- `setVirtualTableColumns` synchronously validates and copies a borrowed span.
  On validation, allocation, or capture-copy failure, the old configuration and
  presentation remain unchanged. On success, replacement invalidates layout and
  requires preparation before paint. The span is never retained.
- `setVirtualTableOptions` validates settings and existing header compatibility,
  invalidates layout, and requires preparation. To change the width policy,
  clear columns, set the new policy, then install rebuilt headers between frames.

Defaults bound configured columns to 64, visible body cells to 16384, and the
sum of all configured header UTF-8 bytes plus visible cell UTF-8 bytes to 1 MiB.
These are configurable positive limits, checked without silent truncation.
The existing model range limit independently bounds requested rows, including
overscan. The cell product is checked by division before multiplication could
overflow. Header validation is bounded by configured columns; no work scales with
logical row count. These bounds cover retained table data, not arbitrary temporary
allocations inside an application formatter.

Invalid dimensions, IDs, widths, policies, and exceeded budgets report
`TreeError::INVALID_ARGUMENT`; allocation failure reports `NO_MEMORY`, and other
formatter/capture-copy exceptions report `CALLBACK_FAILED`. Tree affinity,
lifetime, frame, and model-session errors stay explicit. Paint reports
`WOULD_BLOCK` for unprepared/dirty data and `INVALID_GEOMETRY` for a viewport
mismatch. Discard partial render commands on any paint/sink error. A setter can
make the old presentation dirty; a later failed preparation does not clear that
requirement.

## Verification scope

`tests/virtual_table.cpp` uses real model publications, tree frames, layout,
router dispatch, rendering, FrameWriter, and the independent TestTerminal oracle.
It covers million-row bounded work, invisible columns, unrelated range updates,
publication during a real frame, typed string/integer columns, clipping and wide
graphemes, selection, empty dimensions, explicit budgets, pending/error/retry,
source destruction, wrong affinity/session, and real allocation failpoints.
It is a headless semantic check, not an interactive terminal or performance claim.

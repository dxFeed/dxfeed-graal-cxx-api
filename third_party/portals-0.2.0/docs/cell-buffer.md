# Cell buffer

`<portals/cell_buffer.hpp>` provides an owning framebuffer in terminal columns
and rows. Construct it with `CellBuffer::from(size, policy, limits)`, then write
one validated `GraphemeText` element with `put(position, text, grapheme_index,
style)`. The index selects a complete extended grapheme, not a UTF-8 byte.
`CellStyle` is a small value containing optional RGB foreground/background and
bold/italic/underline flags; themes and terminal encoding are later components.

```cpp
#include <portals/cell_buffer.hpp>

auto text = portals::GraphemeText::from("hello");
auto frame = portals::CellBuffer::from({80, 24});
if (text && frame) {
  auto written = frame->put({2, 3}, *text, 0);
  if (written) {
    auto cell = frame->at({2, 3}); // cell->glyph is "h"
  }
}
```

## Whole-glyph writes

Every cell is a blank, lead or continuation. Blanks resolve ASCII space and the
default style. Narrow leads occupy one column; wide leads occupy two and have
an immediately adjacent continuation in the same row. The two cells share
local glyph/style IDs. A continuation's width is zero, so traversal must emit
the lead once and skip the continuation. `at()` resolves the same glyph and
style for either half; `cells()` borrows row-major metadata.

Writing onto either half removes the entire older glyph. Writing a wide glyph
can remove two intersected old glyphs, including their halves outside the new
footprint. This matters to overlay clipping: callers must place new glyphs
wholly inside their clip, but cleanup can remove an older lead outside it when
its continuation was touched. Wide glyphs at the last column are rejected
atomically. Nothing is clipped, truncated or repaired by the buffer.

Only the text adapter decides segmentation and width. The buffer requires
complete equality of the text's captured width policy, including revision, even
when a particular ASCII glyph would have the same width. Rebuild text, geometry
and the buffer together when policy changes. Unsupported logical clusters copy
the adapter's ASCII `?` display placeholder, never raw control bytes.
See [text policy](text.md) and [cell geometry](geometry.md).

## Ownership and invalidation

The buffer owns its cell array, display UTF-8 bytes and styles. It retains no
reference to the source text. `CellView::glyph` and `cells()` are borrowed until
a successful mutation, move, assignment or destruction. Failed operations
preserve existing cells, views, table counts and accounting. Copied cell/style
metadata survives independently, but glyph/style IDs only make sense for their
source buffer between clear/resize/move. Equal numeric IDs in different buffers
have no semantic meaning; no public write or lookup API accepts IDs.

There is one buffer owner and copying is disabled. Moves transfer coherent
storage and leave a distinct source empty with no allocated arrays. The source
can still be queried, cleared, resized or assigned; self-move is a no-op.
An unchanged buffer supports concurrent reads. Mutations and moves require
caller synchronization; UI usage belongs to its UI owner. The component starts
no threads and performs no I/O, callbacks or global-state mutation.

## Bounds, failures and memory

Limits cap normalized rows/columns, their checked product, interned glyph count,
interned UTF-8 bytes and interned style count. Negative extents normalize to zero.
The blank glyph, its one byte and the default style count toward their caps, so
intern pool caps must be positive even for a zero-cell extent. Rows/columns can
have zero caps to allow only an empty extent. Limits themselves are checked
against coordinate/ID representations, `PTRDIFF_MAX` allocation products and
`size_t` accounting overflow before allocation. A malformed configuration gives
`INVALID_LIMITS`; invalid policy, extent, index, coordinate, right-edge and pool
failures each have an explicit error category.

Creation allocates fixed-capacity glyph records, UTF-8 byte and style arrays;
the cell array holds exactly the current extent. `put()` scans the bounded pools
for equal glyph/style entries in O(existing glyphs + existing styles) comparisons and performs **no allocation**. It validates every
failure condition before changing cells or inserting entries. It does not evict
entries: overwritten glyphs/styles consume slots until clear/resize. An intern
hit still works when a pool is full; a new entry returns a resource error.

`clear()` resets all cells and intern counts while retaining allocated capacity.
`resize()` creates an independent blank replacement with the same policy/caps;
it preserves the whole previous buffer on a limit or allocation failure. A
successful resize discards previous contents and IDs, including when the size
stays the same. Allocation failures return `ALLOCATION_FAILURE`. During resize,
both old and replacement arrays can coexist; this transient peak is bounded by
the two extents and pool caps, but is larger than current `memory()`.

`memory()` reports `sizeof(CellBuffer)`, exact allocated cell/glyph-record/style
array element bytes and UTF-8 byte capacity, plus current used pool counts. The
sum includes unused intern capacity, excludes allocator headers/alignment
outside the C++ arrays, and does **not** measure process RSS or throughput. Pool
allocation cost is paid up front even for an empty extent. Callers can lower the
caps explicitly; an unbounded growth strategy is not hidden in frame rendering.

The headless tests cover ownership, combining/CJK/emoji/unsupported clusters,
whole-glyph overwrites, every semantic/resource failure, repeated intern hits,
clear/resize/moves, and injected failure at every resize allocation. An
independent semantic footprint oracle checks 12,000 bounded write/clear/resize
transitions against every cell, without relying on numeric ID equality.

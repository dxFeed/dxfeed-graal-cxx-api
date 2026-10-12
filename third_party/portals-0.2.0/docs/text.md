# UTF-8 text and terminal width

`<portals/text.hpp>` exposes `GraphemeText`, an owning UTF-8 value with immutable
extended grapheme clusters and cell measurements. Bytes, graphemes and cells are
separate units. The private dependency is [utf8proc v2.12.0](../vendor/utf8proc/README.md),
Unicode 18.0.0. Segmentation follows [UAX #29](https://www.unicode.org/reports/tr29/)
with stateful processing of every adjacent scalar pair. No normalization or
input repair occurs. Invalid UTF-8 returns the byte offset of the offending
sequence, including truncated tails, overlong forms and encoded surrogates.
Embedded NUL remains part of the input.

```cpp
auto text = portals::GraphemeText::from("e\xcc\x81");
if (text) {
  auto columns = text->measure(); // 1
  auto cursor = text->nextCursor(0); // byte offset 3
  auto prefix = text->clipPrefix(1); // complete cluster
}
```

Construction copies its borrowed input. `bytes()` and `graphemes()` borrow from
the resulting object until its destruction, assignment or move; copied values
own independent storage. A distinct moved-from value is empty; self-move assignment leaves the value unchanged. Concurrent reads are
safe; mutations/destruction require caller synchronization. Construction and
copy may throw `std::bad_alloc` or `std::length_error`; semantic/resource errors
use `std::expected` and never return partial text. Move construction may allocate
debug iterator bookkeeping before transferring storage; failure leaves the source
unchanged. Move assignment allocates nothing and remains `noexcept`.
Cursor and clipping operations
allocate nothing, perform no I/O and throw no exceptions. Cursor movement uses
binary search; clipping scans the fitting prefix. Cursor offsets must be whole
grapheme boundaries, including zero/end; invalid offsets return `NOT_BOUNDARY`.
Movement at the corresponding endpoint is idempotent. Negative clip budgets
normalize to zero; a two-cell cluster that cannot fit ends the prefix. These
operations do not write terminal cells; renderer continuation/overwrite rules
belong to the cell-buffer component.

Width is a deterministic supported terminal profile, **not** a font-shaping or
terminal compatibility promise. [UAX #11](https://www.unicode.org/reports/tr11/)
does not itself define terminal glyph widths. `WidthPolicy` defaults to narrow
Ambiguous characters and two-cell emoji. Wide Ambiguous mode promotes printable
one-cell scalars marked Ambiguous by utf8proc. Ordinary cluster widths sum the
positive scalar widths; combining/zero-width scalars add nothing.

Recognized emoji are keycaps (`[0-9#*]`, optional VS16, keycap mark), regional
indicator clusters (including an unmatched indicator), and clusters with an
Extended_Pictographic base and either native scalar width two, VS16, ZWJ, modifier
or tag sequence. VS15 suppresses that pictographic promotion. This is a small
profile, not a complete RGI emoji validator. The emoji one/two-cell choice applies
to single recognized emoji as well as compound sequences. Additional combining
marks stay with their logical cluster. The implementation uses upstream
properties, not a parallel Unicode table.

A zero-cell or ordinary greater-than-two-cell cluster is `unsupported`, measured
as one cell. Controls (including CRLF, tab, ESC, NUL and C1), unassigned scalars,
line/paragraph separators and unsupported formatting clusters are also
`unsupported`. A renderer **must display ASCII `?` and must never emit their
original bytes**. Logical text and boundaries are preserved, allowing the full
official grapheme corpus to test segmentation independently of printability.
There is no multiline/tab expansion in this component.

Default caps are 1 MiB input, 64 KiB per cluster, 1,048,576 clusters and 2,097,152
cells. Callers can change the caps; input length is also bounded by `PTRDIFF_MAX`
and accumulated cells by `INT32_MAX`, matching cell geometry. Empty text is valid
with zero caps. No grapheme is silently truncated when a cap is exceeded. Cell
addition is checked before arithmetic. Resource errors identify the rejected
cluster's start; input-length and invalid-policy errors use offset zero.

The full captured `WidthPolicy`, including caller capability `revision`, belongs
in measurement/layout/cell-buffer cache keys. Changing it requires constructing
new measurements and invalidating affected geometry and paint. The object offers
no mutable policy setter and triggers no UI invalidation itself.

Headless tests cover semantic widths, UTF-8 failures, controls, ownership, limits,
cursor/clip boundaries and the complete official Unicode 18 GraphemeBreakTest
corpus, with no filtering. Vendored file and fixture hashes are checked without
network access. Width fixtures test this policy rather than any physical terminal.

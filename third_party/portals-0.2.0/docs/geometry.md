# Cell geometry

`<portals/geometry.hpp>` supplies `CellPoint`, `CellSize` and `CellRect` in the
`portals` namespace. Coordinates and extents use `std::int32_t`: columns increase
rightward and rows downward from the upper-left origin. Negative origins are
valid for local/offscreen geometry. Canvas logical coordinates and future GUI
units require an explicit transform; these types define no such conversions.

`CellSize` is an available extent and may store negative components. Its
`normalized()` result clamps each negative component to zero. `CellRect::from`
performs that normalization and checks both exclusive ends before creating a
rectangle. Rectangles have private storage, nonnegative extents and representable
origins/ends. A default rectangle is empty at `(0, 0)`.

The cell set is `[x, x + width) × [y, y + height)`. Either zero extent makes it
empty. Right/bottom edges are excluded, so touching edges have no shared interior.
Clipping is `rectangle.intersection(clip)`. Intersection cannot fail for valid
rectangles: its origin is the per-axis maximum of the input origins and each
extent is the overlap on that axis, clamped to zero. An empty result retains this
origin and any overlap on the other axis; it need not be located inside either
input. Equality compares origin and extents, even for empty rectangles.
`contains(point)` always returns false on empties. `contains(rectangle)` uses
cell-set inclusion, so every empty rectangle is a subset of every rectangle,
regardless of its position. Intersection is commutative and idempotent, including
degenerate rectangles.

Creation and translation return `std::expected<T, GeometryError>` with
`COORDINATE_OVERFLOW` if an origin or exclusive end would leave the `int32_t`
range. This local error type handles the one recoverable geometry failure; it
does not commit the rest of the library to an error policy. Expected fits the
nonthrowing, allocation-free layout boundary and keeps failure observable.
Inputs already have `int32_t` components; callers converting wider or fractional
values must validate them before passing them here. No saturating coordinates,
wrapping or unchecked implicit unit conversions are provided.

All addition/subtraction is performed in `int64_t` before checked narrowing or
under a valid-rectangle invariant. An intersection's positive extent is at most
either input extent, which proves that narrowing it is safe. The maximum positive
extent is `INT32_MAX`; the full signed coordinate span is intentionally not one
representable rectangle. A nonempty rectangle beginning at `INT32_MAX` is rejected
because its exclusive end would not fit. Translating even an empty rectangle
checks its origin; translating a nonempty rectangle checks its ends as well.

```cpp
#include <portals/geometry.hpp>

const auto local = portals::CellRect::from({-2, 1}, {8, 3});
const auto viewport = portals::CellRect::from({0, 0}, {80, 24});
if (local && viewport) {
  const auto visible = local->intersection(*viewport); // (0, 1), size (6, 3)
  const auto surface = visible.translated({4, 2});
  // Check surface before using its value; translation can overflow.
}
```

Every parameter/result is an owning value with independent lifetime and no
borrowed storage. These operations are thread-agnostic, have no callbacks, I/O,
allocation or exceptions, and do not mutate inputs or invalidate other values.
Concurrent reads are safe; callers synchronize shared assignment. Geometry
changes do not dispatch layout/paint invalidation: the future UI owner will do so
when installing new geometry. No runtime tree or GUI functionality is implied.

The headless `portals.geometry` test checks integer limits, normalization,
translation failures and half-open edges, then exhausts a bounded deterministic
rectangle/cell domain against an independent interval/cell-set oracle. Checks
remain active in Release with `NDEBUG`; no random seed or external test library
is required.

Reference: [C++23 expected specification](https://eel.is/c++draft/expected),
[fixed-width integer types](https://eel.is/c++draft/cstdint.syn).

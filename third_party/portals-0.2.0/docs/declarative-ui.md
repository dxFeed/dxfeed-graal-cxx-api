# Declarative composition {#declarative-ui}

`<portals/ui.hpp>` supplies owning, single-use descriptions in `portals::ui`.
They compose ordinary controls and custom widgets on the existing
[managed lifecycle](managed-controls.md). A description is a construction value;
it is not another widget tree or a reactive runtime. Mounted objects remain
owned by `WidgetTree`, while `ManagedScene` retains bindings and presentation work.

## Store descriptions, then mount

A description owns its child descriptions and callback objects. Standard text
factories copy their input immediately, so a description can outlive the source
`std::string` or `std::string_view`. Creating a description does not mount widgets
or subscribe to events. Descriptions are movable and noncopyable; mounting
consumes them. Reusing a consumed or moved-from description returns
`TreeError::INVALID_ARGUMENT` through `ManagedError`.

Use `ui::column(...)` and `ui::row(...)` for nested composition. Their overloads
accept `LinearOptions` before the children when gaps, padding or alignment need
customization. Standard control factories are `label`, `button`, `checkbox` and
`lineEdit`; `virtualList`, `virtualTable` and `chart` compose managed virtual views.
Binding state and models belong to the same `ManagedScene` as before.
`custom<T>(args...)` constructs the concrete public `Widget` subtype, including
`final` classes. It does not introduce another base class.

Factories and fluent modifiers construct ordinary C++ values: allocation or
user-defined argument-copy exceptions can escape while assembling a description.
The `ui::mount(scene, std::move(description))` boundary returns typed managed
errors for mount, binding and initialization failures. A failed attempt removes
its new subtree and releases its managed registration budget. Existing siblings
and states remain intact. External side effects of user callbacks are not rolled
back. The original cause and an independent cleanup error remain distinguishable.

`ManagedScene::remove` retires identities immediately. Managed mutations reentering
that call return `WOULD_BLOCK`. If the caller holds an outer `tree.edit` or
`withScope` borrow, physical destruction and `unmount` wait for that borrow to
finish. Those deferred hooks follow the existing tree contract and can remove
other nodes; their external effects are not undone by description rollback.

## Observe without owning the widget

`ui::NodeRef<T>` is a copyable observing slot. Attach it with `.reference(ref)`
and obtain the existing checked `Node<T>` through `ref.node()`. References are
published after the entire description mounts successfully; before that they
are empty. Reusing a populated slot or binding it twice in one description is
rejected. Removing the mounted widget leaves a stale checked identity, not a
live owning pointer. `Application`, `ManagedScene` and mutable widget operations
keep their UI-thread contracts.

## Explicit customization

Standard controls inherit their normal managed bindings, styles and LineEdit
semantics. A business result callback runs after standard behavior; it cannot
undo a committed edit or checkbox toggle. The existing LineEdit validator remains
the commit veto. No manual token storage or standard presentation sync is needed.

`ui::LineEditConfig` uses named fields for the editor result callback, editor
options, sizing, clipboard and validation. For example, `ui::lineEdit(name,
{.validate = nonempty})` does not require positional placeholder arguments.
Invalid factory text retains its original `TextError` in `ManagedError::text`;
construction conveniences do not discard diagnostic kind or byte offset.

Fluent `.style(target)`, `.onEvent(handler)` and `.onPrepare(handler)` attach
custom identity and behavior through the same managed APIs. `.setup(callback)`
is the explicit escape hatch for low-level initialization. Its callback receives
the scene and typed node and returns `expected<void, ManagedError>`. Existing
router, layout and tree operations remain available through the caller's
`Application`; the composition layer does not hide them behind a new runtime.

Custom constructor arguments and lambda captures still obey C++ ownership rules.
Owning a callback does not make objects captured by reference live longer. Explicit
borrowed pointers or reference wrappers must remain valid for the uses made by
the custom widget. Consult the `custom` factory reference for argument storage
and forwarding requirements; arbitrary user types are not deep-copied magically.

## Virtual views in the same composition

Create a model once with `scene.model<T>(snapshot)` and pass its observing handle
to any number of descriptions. The row type is deduced from that handle; arrays
and vectors of existing `TableColumn<T>` convert to the table's span parameter.
Each recipe opens its range only when mounted. There are no application-owned
binding tokens, per-view style registrations or `prepareViews` hooks to retain.

```cpp
auto model = take(scene.model<Ohlc>(snapshot));
auto columns = priceColumns();
ui::NodeRef<VirtualTable<Ohlc>> table;

auto description = ui::column(
  ui::label("Prices"),
  ui::row(
    ui::virtualTable(model, columns).reference(table),
    ui::chart(model, [](const Ohlc &value) { return std::optional{value}; })));

take(ui::mount(scene, std::move(description)));
```

Here `take`, `snapshot` and `priceColumns` are application setup helpers/data;
the complete programs below provide executable setup and error checks.

`virtualTable` copies its column descriptions immediately, including headers and
formatter objects. The source span may expire before mount. List formatters,
chart accessors, chart-origin callbacks and provider callbacks are owned values.
As elsewhere in C++, a reference inside a callback is still a borrow. The model
handle remains weak and UI-affine: the receiving scene must own that model.

Virtual factories retain their existing managed parameters. List/table settings
and `SizePolicy` remain available, and chart settings include its optional origin
callback. The final optional `prepared` callback receives the coherent borrowed
model frame and an optional owning provider request. It runs in composition
registration order, including layout retries. Resident snapshots need no provider
callback; pending work without one fails with `INVALID_ARGUMENT` during preparation.
Provider dispatch, request retention and cache policy remain application decisions.

All children share the existing description transaction. A failed factory or
later setup removes the newly mounted subtree, bindings, ranges and captures;
no `NodeRef` is published. The pre-existing scene model survives rollback.
An origin callback may observe published refs when preparation begins. Standard
list/table input bindings cannot be replaced with `.onEvent`; that conflicting
setup returns a typed error and rolls back, as for ordinary bound controls.
Hidden views cancel their pending range work; disabling affects input, not visibility.
Idle frames, theme changes, rejected notifications and removal use the same
[managed virtual-view lifecycle](virtual-preparation.md).

### Complete before/after form with virtual views

These two installed-package programs implement the same form: a committing name
editor, a self-removing button, a custom `final Badge`, a list, a table and an OHLC
chart. Both receive identical scripted input, resize and theme changes. The tests
check semantic outcomes and compare every emitted terminal byte. Their shared
fixture contains domain rendering, input and assertions, not hidden mounting code.

- [Before: explicit managed construction](../tests/package_consumer/composed_before.cpp)
- [After: one owning composition](../tests/package_consumer/composed_after.cpp)
- [Shared domain and replay assertions](../tests/package_consumer/composed_support.hpp)
- [Finite terminal and immutable price snapshot](../tests/package_consumer/virtual_support.hpp)

\include{lineno} composed_before.cpp

\include{lineno} composed_after.cpp

\include{lineno} composed_support.hpp

Run `portals_composed_before` or `portals_composed_after` from the installed-consumer
build directory (`.exe` on Windows). They accept no options, finish without a native
window, and print a semantic summary followed by hex-encoded terminal output.
Failures return a nonzero process status. Run `ctest -R consumer.composed` there
to execute both programs and the equivalence comparison.

The benefit is structural: parent handles and explicit binding calls disappear
from construction, and the whole form gains transactional mount and owning inputs.
Callbacks, error handling and test replay remain explicit. This is not a claim that
the full program is shorter: observing slots and equivalence assertions add code.
Typed columns and named view settings are available as additive conveniences below.
The description layer does not change the current delayed-range cache policy or
provide compile-time scene construction.

## Typed columns and named view settings

Include `<portals/table_column.hpp>` to compose a projection and a formatter into
the existing `TableColumn<T>`. The projection reads `const T&`; the formatter
receives its exact result category and returns owning `GraphemeText` by value.
Data members, const member functions and lambdas work through `std::invoke`.
`ColumnProjection` and `ColumnFormatter` constraints reject incompatible calls
at the factory, including formatters that return borrowed text or `std::string`.

```cpp
const auto price = [](double value) {
  return GraphemeText::from(std::format("{:.0f}", value)).value();
};
const std::array columns{
  tableColumn<Ohlc>({.id = {1, 1},
                     .header = GraphemeText::from("Close").value(),
                     .width = 9},
                    &Ohlc::close, price)};

auto table = ui::virtualTable(model, columns,
  ui::VirtualTableConfig<Ohlc>{.policy = tablePolicy, .prepared = provider});
```

Here `model` is the scene's `ManagedModel<Ohlc>`, `tablePolicy` is a `SizePolicy`
and `provider` is a `ManagedViewHandler<Ohlc>`. The complete program below includes
their definitions. `std::format` requires `<format>`; it is ordinary C++23 formatting,
not a second text or reactive system. Select the same `WidthPolicy` for the header,
formatted text and table options when changing the default policy.

`TableColumnOptions` names the stable column ID, owning header and positive width
in terminal cells. IDs are not derived from field names or display order. Existing
column arrays remain valid, and the returned column can also be used by low-level
`VirtualTable` or `setVirtualTableColumns` callers. No sorting or model reads happen
at column construction. During preparation only visible ready cells are formatted.
The adapter adds one projection call per formatted cell, no row copies or scans;
callback cost remains application-defined.

Both callables must be copy constructible because `TableColumn` still uses
`std::function`. Their captures are owned, but captured references and projection
references remain borrowed. A temporary result lasts through the formatter call.
Do not retain row/result borrows, perform I/O or mutate the model/tree in either
callback. Construction can throw on allocation or capture copy/move; preparation
uses the existing typed callback/allocation errors. Direct `column.format(row)`
invocation propagates exceptions to its caller.

Named settings avoid positional placeholders:

| Recipe | Configuration | Fields |
| --- | --- | --- |
| `ui::virtualTable` | `ui::VirtualTableConfig<T>` | `options`, `policy`, `prepared` |
| `ui::virtualList` | `ui::VirtualListConfig<T>` | `options`, `policy`, `prepared` |
| `ui::chart` | `ui::ChartConfig<T>` | `options`, `first`, `prepared` |

All fields default to the corresponding positional factory defaults. For example,
`ui::chart(model, accessor, ui::ChartConfig<Ohlc>{.prepared = provider})` starts at
row zero without an explicit origin callback. Configurations own callbacks, copy
from lvalues and consume rvalues; the model handle remains weak and UI-affine.
Use the explicit configuration type with designated initializers. This keeps
existing calls such as `ui::chart(model, accessor, {})` unambiguous: bare `{}` still
means the old `ChartOptions`. Named overloads preserve mount rollback, input,
preparation order, pending-work requirements and typed errors.

[Complete named-settings form](../tests/package_consumer/composed_named.cpp)
runs exactly the same replay as the [positional form](../tests/package_consumer/composed_after.cpp)
and [managed form](../tests/package_consumer/composed_before.cpp). It accepts no
arguments, opens no window and returns nonzero on failure. Run
`portals_composed_named` (`.exe` on Windows) in the installed-consumer build directory,
or `ctest -R consumer.composed` to compare the full captured output. Explicit names
improve readability; this is not a promise of fewer total lines. The
[compact examples](compact-examples.md) show the migrated dashboard and additional
checked text and fixed-schema column conveniences.

\include{lineno} composed_named.cpp

## Complete installed consumer

The program below uses only the installed `portals::core` package. It retains the
same custom final `Meter`, domain behavior, palette and deterministic input as
the complete before/managed examples. Its full captured terminal output is
compared with the existing managed form. The shared fixture supplies domain
rendering and assertions; it contains no description mounting or binding setup.

[Open the complete consumer source](../tests/package_consumer/form_declarative.cpp).
The generated HTML reference embeds the same file below with line numbers.

\include{lineno} form_declarative.cpp

The executable is a finite, headless replay consumer, with no interactive window.
From its build output directory, run `./portals_form_declarative` to execute the
normal scenario and write its outcome summary and captured terminal bytes to
standard output. Run `./portals_form_declarative --failure` to exercise the
intentional `INVALID_GEOMETRY` callback error and verify its phase, node and
terminal cleanup. That verification returns success when the expected error is
preserved. These are the only supported invocation forms; Windows builds use
the same executable name with the `.exe` suffix.

The shared `form_support.hpp` and `form_replay.hpp` are included in full on the
[managed controls page](managed-controls.md).

### What became simpler

All three complete programs run the same replay, including validation, conflicting
edits, rejected notifications, hide/disable/remove, theme changes and cleanup.
The installed-package test compares both their outcome summaries and terminal
byte streams. The declarative program expresses the hierarchy in one `column`
and mounts it once; standard binding setup stays inside the control factories.

| Complete source | Lines | Nonempty lines |
| --- | ---: | ---: |
| Low-level `form_before.cpp` | 256 | 193 |
| Managed `form_after.cpp` | 218 | 160 |
| Declarative `form_declarative.cpp` | 236 | 165 |

These counts include the failure mode, business callbacks and replay assertions.
The declarative form needs six observing slots so the unchanged test can inspect
every control. It makes the hierarchy explicit and mount failure cleanup local,
but does not reduce the complete program's line count relative to the managed
version. Custom event and presentation logic remain explicit C++.

This layer does not implement reconciliation, dynamic tree replacement,
a new CSS cascade or SQLite integration. Descriptions
assemble a tree once. The [performance experiment](managed-performance.md)
compares retained lifecycle costs at several sizes; it does not measure
compilation performance. Shorter construction code is not a speed claim.

The interactive [gallery and custom-widget examples](examples.md) now use these
descriptions. The gallery preserves its former terminal output under the original
themes; additional palettes use the same style engine.

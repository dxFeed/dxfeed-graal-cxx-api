# Prepare virtual views from layout {#virtual-preparation}

Virtual tables, lists and charts can use their arranged sizes without duplicating
layout arithmetic in application code. Choose either the managed factories in
`portals/managed.hpp` or the explicit hooks in `portals/virtual_preparation.hpp`.
The existing explicit `CellSize` preparation overloads remain available.

This is an experimental extension of the [application runner](application.md).
Both paths use the same tree, scheduler, model and renderer. Managed factories
own registration and model pins; provider scheduling and retries remain application
policy. The dashboard uses the managed path while retaining its bounded provider.

## Managed models and views

Create one model with `scene.model<T>(initialSnapshot, modelOptions)`, then pass
its `ManagedModel<T>` handle to `scene.virtualList<T>`, `scene.virtualTable<T>`
or `scene.chart<T>`. Each factory opens a separate range consumer, mounts the
widget, and registers layout-driven preparation and its standard `List`, `Table`
or `Chart` style identity. Lists and tables also receive their existing input
bindings; charts add no input binding. The returned node, such as
`Node<VirtualTable<T>>`, is an observer, just as with `emplace`. No binding token,
model pin or per-view size is retained
by ordinary consumer code. Explicit initial focus and theme selection remain
application choices.

`ManagedModel<T>` is a weak, UI-affine handle. Its `publisher()` operation must
run on the creating UI thread and returns the existing worker-safe
`Publisher<ModelSnapshot<T>>`. Copy that endpoint on UI before using it from a
worker; do not call the weak handle from the worker. The scene owns the model
until `close()`, and a view factory rejects a model from another scene with
`INVALID_ARGUMENT`. `ManagedOptions::maxModels` is a positive model-owner budget,
default 64; exceeding it reports `NODE_LIMIT` in the `STATE` phase.

For each dirty, submittable frame, `ManagedScene::prepare` synchronizes ordinary
controls, pins each owned model once, and flushes pending styles. All registered
views and all layout retries share those pins. `prepareViews` visits views in
registration order. For a chart it first evaluates the optional
`ManagedChartOrigin` callback; an empty origin callback means row zero. After
successful view preparation, it invokes that view's optional
`ManagedViewHandler<T>`, including on layout retries and when there is no new
request. A handler therefore must tolerate repeated invocation and should
invalidate only when its retained presentation changes. There is no new polling
loop or scheduler.

The handler receives a borrowed `const ModelFrame<T>&` and a borrowed
`const std::optional<ModelRequest<T>>&`. They are valid for that invocation.
Copy a present request into provider-owned storage before returning if work
continues later; keep its existing typed completion and cancellation protocol.
Do not retain references to either argument, dispatch the scheduler, repin
models, or mutate the managed registry in this callback. Checked low-level
presentation updates are allowed. Provider publication and completion can still
return `WOULD_BLOCK`: retain the unaccepted value or completion and retry through
the provider's existing service path. The managed layer does not retry it for you.

A handler may be omitted when the snapshot always supplies the requested data,
as in the complete example below. If preparation instead yields a pending request
without a handler, the scene cancels that request and reports
`PREPARE / INVALID_ARGUMENT`; it cannot silently leave provider work unowned.
Callback errors preserve their original `TreeError` and view identity through
`ManagedScene::run`. Allocation exceptions become `NO_MEMORY`, and other thrown
exceptions become `CALLBACK_FAILED`.

Hiding a view or an ancestor still prepares zero geometry to cancel range work
and release prepared rows. Disabling suppresses input but keeps preparation and
rendering. Removing a view retires its range consumer; late completions cannot
revive it. Use `scene.remove` outside preparation to retire managed registrations
as well. An admitted input callback may remove its own node and finishes with
its captures alive. If low-level code removes a node during preparation, retired
entries are pruned at the next ordinary preparation or at close; a later sibling
whose identity is already stale is skipped. An admitted handler's returned error
is still reported even if it removed its node.

Destroy the scene before its `Application`, on UI and outside callbacks or tree
frames. `close()` removes its subtree, retires callbacks and destroys owned
models; weak model handles then return `CLOSED` on UI, and copied publishers
cannot publish to the closed endpoint. A failed factory removes only its new
child and closes its range consumer; earlier successful children and the model
remain scene-owned. Constructor/provider side effects are not rolled back.
The existing [managed control rules](managed-controls.md) also apply.

## Explicit hooks: one frame, two preparation phases

For a custom `ApplicationScene`, opt into `usesPreparedLayout()` and implement
the existing two hooks. `prepare(application, terminalSize)` runs once for a
dirty, submittable frame.
Synchronize ordinary controls here and retain **one pin per model** in the scene.
Then `prepareViews(application, layout)` prepares every sibling from that pin
and the supplied owning `LayoutSnapshot`:

```mermaid
flowchart TB
  accTitle: Layout-driven virtual-view preparation
  accDescr: Prepare pins each model once. Preliminary layout and view preparation repeat only for new layout invalidation while budget remains. The final frame paints and renders before submission.
  dispatch[One scheduler dispatch] --> ready{"Dirty frame<br/>ready to submit?"}
  ready -- No --> service[Continue bounded service loop]
  ready -- Yes --> prepare["prepare: synchronize controls<br/>Pin each model once"]
  prepare --> layout["Preliminary tree.frame<br/>Layout and capture geometry"]
  layout --> views["prepareViews: list, table, chart<br/>Reuse the same model pins"]
  views --> dirty{New LAYOUT?}
  dirty -- Yes --> budget{"Pass budget<br/>remains?"}
  budget -- Yes --> layout
  budget -- No --> fail["PREPARE / WOULD_BLOCK<br/>No partial frame submitted"]
  dirty -- No --> frame["Final tree.frame<br/>Layout, paint, render into CellBuffer"]
  frame --> submit[Submit and flush]
```

The first pending range request can invalidate layout while the second view
still needs its dimensions. `LayoutSnapshot` owns identities and cell sizes, so
both views see the same geometry even after that invalidation. It owns no
widgets, model rows or borrowed render commands. `viewport(node)` checks the
complete captured identity, including session and generation. It returns zero
size for a node beneath a hidden ancestor. Disabled nodes remain visible.

Capture is a record of one layout, not a subscription to later tree changes.
Use the supplied snapshot during that preparation pass. A subsequent mutation
can make its geometry obsolete; it does not silently update. Removal still
fails the view's existing checked tree access. Never reuse a snapshot across
frames as if it described the current tree.

`ApplicationOptions::maxPreparationPasses` is a positive pass budget, default 3.
Exhaustion ends preparation with `ApplicationPhase::PREPARE` and
`TreeError::WOULD_BLOCK`; no partial frame is submitted. Hooks must converge:
do not unconditionally invalidate layout on each call. Pure measurement and
arrangement can run more than once; painting only happens in the final frame.
An idle scene does not call either hook or redraw periodically.

For a full tree with N nodes, snapshot capture takes O(N log N) time and O(N)
storage; it sorts by slot once and then looks up each full identity in O(log N).
Copies share immutable storage. This cost is additional to the existing layout
algorithm: the opt-in path performs at least one preliminary layout and one
final layout. The pass budget bounds repetition, not the cost of an arbitrary
user widget's measurement callback.

## Providers and lifetime with explicit hooks

Each preparation overload returns the existing optional `ModelRequest<T>`.
Send new receipts to the application's provider and retain/cancel them under
the existing [model contract](model.md). Do not dispatch the scheduler, repin
the model or replace the scene's pin inside `prepareViews`, including retries.
Enqueueing a completion is allowed; delivery belongs to a later scheduler
dispatch. Same-revision range completions still need a fresh pin next frame.

Hidden list/table preparation cancels range work and releases prepared rows.
The snapshot chart overload passes zero size for hidden nodes, cancelling its
request through the existing chart path. A completion from an obsolete request
does not revive the hidden view. Keep preparing registered hidden views if
their outstanding work needs cancellation. A removed view is no longer a
valid preparation target.

Views retain owning pin copies through rendering. The application must outlive
its scene; model, provider and callback cleanup remain scene responsibilities.
The overloads preserve `TreeError` rather than converting failures to a generic
exception. A thrown hook is still reported by the runner's existing exception
boundary. The default scene path has no preliminary layout pass.

## Complete installed-package examples

The three programs use identical nested layout, immutable OHLC data and
finite replay input. They compare explicit extents, explicit snapshot hooks and
managed registration. All link to the relocated installed package through
`find_package(Portals CONFIG REQUIRED)`.

The source listings and exact replay equivalence check are maintained with the
package consumer, so the examples are compiled rather than illustrative API
pseudocode. See [installed package testing](package.md) for the fixture workflow.

| Concern | Explicit extents | Explicit layout snapshot |
| --- | --- | --- |
| Nested sizes | Repeat padding, gaps, split rounding and title height in `prepare` | Pass the runner's snapshot to each view |
| Model pin | Local pin in `prepare` | One retained pin shared by `prepareViews` retries |
| Provider | Explicit request receipts | The same explicit request receipts |
| Geometry cost | One final layout | Preliminary capture plus final layout; bounded retries |
| Customization | Explicit `CellSize` | Captured local sizes, with explicit overloads still available |

The first step removes duplicated geometry rules without claiming that the
entire example has fewer lines. The explicit snapshot scene deliberately keeps
pin ownership and provider requests explicit. Counters and captured sizes in
this fixture are test observations; applications do not need them. The managed
example below then removes those routine ownership and preparation obligations.

### Original baseline: explicit extents

[Complete before program](../tests/package_consumer/virtual_before.cpp).

\include{lineno} virtual_before.cpp

### Before managed ownership: explicit layout snapshot

[Complete scene](../tests/package_consumer/virtual_scene.hpp) and
[complete after program](../tests/package_consumer/virtual_after.cpp).

\include{lineno} virtual_scene.hpp

\include{lineno} virtual_after.cpp

### After: managed model and view lifecycle

[Complete managed program](../tests/package_consumer/virtual_managed.cpp).

\include{lineno} virtual_managed.cpp

| Consumer responsibility | Explicit snapshot scene | Managed factories |
| --- | --- | --- |
| Model owner and range creation | Retained model; open a range per view | Scene-owned model; factory-owned ranges |
| Frame pin | Retain and refresh it in `prepare` | One scene-owned pin per model and frame |
| View preparation | Call each helper from `prepareViews` | Registered by the factory |
| Layout opt-in hooks | Implement in the custom scene | Supplied by `ManagedScene` |
| Standard view input and styles | Explicit when needed | Registered by the factory |
| Provider work | Own request and retry policy | The same policy in a preparation handler |

`consumer.virtual-managed-equivalence` compares the complete terminal capture
of `virtual_after.cpp` and `virtual_managed.cpp`, including resize. The managed
program also asserts two frames for its initial/resize replay, with no idle
redraw, and checks that the model handle is closed after the run. This fixture
uses immediately available immutable rows; pending-provider lifecycle coverage
belongs to the separate lifecycle tests and the dashboard.

### Shared provider and finite terminal

[Shared support](../tests/package_consumer/virtual_support.hpp) supplies the same
immutable rows and finite terminal to all three programs. It opens no native windows.

\include{lineno} virtual_support.hpp

The separate [lifecycle test](../tests/package_consumer/virtual_lifecycle.cpp)
exercises real pending requests, an odd-width resize, ancestor hiding, cancelled
late completions and independent same-data-revision sibling completions.

## Limits of this step

Managed factories remove routine model/range ownership and per-view preparation.
The row type is deduced from the managed model, so column arrays and formatter
lambdas can be passed directly without repeating template arguments or wrapping
them in `std::span` or `std::function`.
Custom layout, formatters, chart accessors, provider dispatch and retry policy
remain explicit. The old hooks and explicit-size APIs remain available for
applications that need full control. This step adds no virtual-view DSL,
reconciliation, typed column builder, query facade, network page-cache policy or
SQLite provider. It does not solve continuous scrolling across uncached network
ranges. Future provider policies must preserve the current bounded requests and
immutable snapshot contracts.

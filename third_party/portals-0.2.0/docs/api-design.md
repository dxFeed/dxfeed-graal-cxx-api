# API design scenarios

Status: design-only, 2026-10-07. The five sketches below select a C++23 API
direction for review. These names and signatures are **not implemented**,
installed headers, executable examples, or a compatibility promise. The implemented API is documented in the public headers. The separate
[module spike](../experiments/module-spike/README.md) tests language/linkage
mechanics with a toy library; its tests cannot prove these UI contracts.

## Common contracts

The tree owns nodes exclusively. `Node<T>` is a copyable observing handle with
session identity, slot and generation; it owns no node and exposes no raw
pointer. A slot cannot wrap its generation into a live old identity: retire it
on exhaustion. `app.ui().edit(handle, operation)` checks session, generation and
UI affinity before borrowing `T&` for that call. A wrong session, stale node or
wrong thread returns a typed error without calling the operation. References
must not escape; `remove(handle)` invalidates subtree handles immediately and
reclaims storage after any active dispatch ends. Reentrant tree changes are
queued until that dispatch ends. Layout/paint cannot mutate the tree.
Fluent `handle.emplace` delegates to the checked UI operation. Event selectors
such as `clicked()`/`actions()` only form observing source descriptors;
`connect` validates the descriptor's node/session/generation before registering.

`emplace<T>(owned configuration)` returns `Result<Node<T>>`; copied/moved
configuration is owned by the node. `Result<T>` denotes a proposed
`std::expected<T, Error>` policy for fallible operations, with errors such as
`WRONG_THREAD`, `STALE_NODE`, `INVALID_ARGUMENT`, `NO_MEMORY`, `WOULD_BLOCK`,
`CLOSED`, `provider_failed` and `capability_unavailable`. Exact payloads remain
open. Successful mutation commits wholly; an error leaves state unchanged.
Domain validation is a separate typed result, suitable for inline presentation.
Allocating factories/mutations translate allocation failure into `NO_MEMORY`.
Unexpected exceptions escaping an application callback are caught at dispatch,
reported through the application error sink, and stop that callback/subscription;
the runtime does not promise rollback of arbitrary application side effects.

All node access/mutation, scope creation/destruction, callbacks, model revision
application and invalidation run on one UI executor. The error sink also runs
there; workers publish owned errors through a bounded mailbox. Session/terminal
failures reach the application run result; widgets never write VT bytes or
stdout themselves. Shutdown releases UI resources on the UI owner before the
executor stops. Thread-affine owning tokens must not be destroyed on workers.

`UiScope` is move-only and owns scoped subscriptions/timers. `connect(scope,
source, callback)` copies/moves the callback and returns a move-only
`Subscription`; either token destruction or scope close disconnects it.
Disconnect prevents any new invocation, including already queued events;
current invocation finishes before captured storage or scope context is freed.
Self-close logically disconnects immediately and defers reclamation to the
dispatch boundary. It does not synchronously join a worker. Capturing an
observing node is safe only through checked `edit`; arbitrary reference captures
still require their own owner to outlive the scope. All owning tokens in these
sketches are local to a UI-owned controller and released before `app`.

Invalidation accumulates flags, at most one pending frame request per session.
Paint changes schedule repaint; intrinsic metrics/layout changes schedule
measure/arrange **and** paint; hit-test/accessibility semantics update alongside
geometry. No-op setters do nothing. Text changes affecting intrinsic size use
layout+paint; fixed-size text editing still updates paint and caret semantics.
Theme colors invalidate paint, metrics/width policy invalidate layout+paint;
one style revision is pinned for the whole frame. No measure/arrange/paint
callback performs I/O, waits on provider locks, starts a domain mutation, or
retains a borrowed context. Terminal geometry is integer columns/rows with
half-open clips; Canvas logical coordinates require an explicit transform.

The sketches use `TRY(expression)` as documentation shorthand for checking and
propagating `Result` errors, not a provided macro. Application-specific helpers
(`validate_name`, `save`, `draw_game`) are also illustrative. Callback registration
and node construction can fail and therefore use `TRY`. Controller state and
scopes below must be declared after `app`, or otherwise explicitly destroyed
before it. No snippet is submitted to a compiler as a fake API implementation.

## 1. Form with validation

```cpp
auto form = TRY(app.root().emplace<Column>(ColumnOptions{}));
auto name = TRY(form.emplace<LineEdit>(LineEditOptions{.text = ""}));
auto hint = TRY(form.emplace<Label>(LabelOptions{.text = "Enter your name"}));
auto submit = TRY(form.emplace<Button>(ButtonOptions{.text = "Save"}));
auto scope = TRY(app.ui().makeScope());

auto clicked = TRY(app.ui().connect(scope, submit.clicked(), [&, name, hint] {
  auto text = TRY(app.ui().read_text_copy(name));
  auto validation = validate_name(text);
  if (!validation) {
    return app.ui().set_text(hint, validation.message());
  }
  return save(text); // Application-owned operation, not paint or layout.
}));
```

The root owns `form` and all children; handles are observing. `read_text_copy`
returns an owning UTF-8 string, so validation never holds editor storage across
mutation. A setter copies input during the call. Editing/caret movement use
grapheme boundaries rather than bytes. Validation failure changes only the hint;
`save` errors propagate to the error sink, and asynchronous save must use the
worker boundary below. Deleting the form invalidates the three handles and
disconnects node-owned event sources; the controller scope additionally governs
its captures. Every operation runs on the UI owner. Text invalidates layout+paint
when intrinsic metrics change; focus/caret changes invalidate presentation paint.
Required future evidence: invalid input, stale handles after subtree removal,
callback self-removal, grapheme edits and failure with unchanged widget state.

## 2. Two views of one model

```cpp
auto model = TRY(QuoteModel::create(initial_snapshot));
auto table = TRY(app.root().emplace<TableView>(TableOptions{.model = model}));
auto candles = TRY(app.root().emplace<CandleChart>(ChartOptions{.model = model}));

// Between frames, on the UI owner. Both views read the same pinned revision.
TRY(app.ui().apply(model, next_snapshot));
```

`model` is a strong shared owner of a UI-applied immutable revision source.
Each view holds a strong model owner and its own scoped observation; views do
not own each other. Destroying the last application reference leaves the model
alive while either view needs it; destroying a view disconnects only its own
observation. There is no model-to-view strong reference cycle.

The frame coordinator pins one `shared_ptr<const QuoteSnapshot>` per model
identity, before invoking any view, and releases it after the frame. A snapshot
owns its rows and stable row IDs; borrowed spans/cells live only while their
snapshot owner lives. Publications arriving mid-frame become eligible next
frame. Removal/reordering affects positions, not surviving row identities;
selection resolves missing IDs explicitly. Multiple models need an application
transaction policy; there is no implicit global atomic snapshot.

Snapshot state is `READY`, `PENDING` or `error`. Pending/error retains the last
ready revision (if any) with a visible status; otherwise a placeholder appears.
Error payloads are owned; painting status performs no fetch. Apply rejects a
foreign model/session or obsolete revision without mutation. Model data changes
invalidate each view independently: paint for values, layout+paint when column
metrics/row geometry changes. Per-view selection/scroll invalidates that view
only. Required future evidence: held snapshot after model teardown, synchronized
revision in both views, out-of-order publication, and independent view removal.

## 3. Quote stream with bounded publication

```cpp
auto channel = TRY(SnapshotMailbox<QuoteSnapshot>::create(
  MailboxOptions{.capacity = config.snapshot_capacity,
                 .overflow = Overflow::REPLACE_LATEST}));
auto scope = TRY(app.ui().makeScope());
auto applied = TRY(app.ui().connect(scope, channel.receiver(),
  [&, model](OwnedPublication<QuoteSnapshot> publication) {
    return app.ui().apply_if_current(model, publication);
  }));

// Worker owns aggregator, sender, request identity and a stop token.
auto worker = start_feed(channel.sender(), request, stop_token);
// Feed aggregates every accepted tick before publishing an immutable snapshot.
```

Each producer owns its aggregator/storage and builds immutable snapshots off the
UI thread. Sender/receiver are owning channel endpoints; publication transfers
a strong snapshot owner, request generation and monotonic revision, with no
borrowed worker memory. The mailbox has a configured finite capacity; replacing
a queued display snapshot is allowed **after** lossless OHLC/volume aggregation.
The feed's raw tick queue has a separate finite capacity and `WOULD_BLOCK`
policy: pause input if supported, otherwise stop with a visible gap/provider
error. It must not silently discard raw ticks. `CLOSED`/feed failure are explicit
owned status publications; status is part of the latest snapshot envelope, so a
later display replacement cannot erase an unresolved feed failure.

The UI drains a configured bounded number of publications between frames.
It validates receiver lifetime, request generation and revision before apply;
obsolete results are discarded without callbacks into destroyed contexts.
Snapshot application is UI-affine; sender publication is worker-safe. Disconnect
stops new UI callbacks. Requesting stop does not prove worker completion:
worker-owned resources survive until exit, and application shutdown joins workers
outside measure/layout/paint before releasing shared provider resources.
Data/status invalidation follows scenario 2; queueing alone does not mutate views.
Required future evidence: overflow, disconnect with queued delivery, late results
after cancellation, feed gaps and skipped display revisions preserving volume.

## 4. External custom widget

```cpp
class Meter final : public Widget {
public:
  Result<DesiredSize> measure(const MeasureContext&) const override;
  Result<void> arrange(const ArrangeContext&) override;
  Result<void> paint(PaintContext&) const override;
  Result<EventReply> handle_event(const EventContext&, const Event&) override;
};

auto meter = TRY(app.root().emplace<Meter>(MeterOptions{.range = range}));
TRY(app.ui().edit(meter, [](Meter& value) { return value.set_level(0.6); }));
```

The external application supplies this class without editing core. The tree
owns its concrete object and calls its virtual destructor on the UI owner after
dispatch completes. All hook contexts/events are borrowed for one call;
`DesiredSize`/`EventReply` are owning values. The widget stores domain state
itself or holds an explicit model owner; it cannot retain paint surfaces or
dispatch references. `set_level` rejects non-finite/out-of-range values without
mutation and invalidates paint for a successful changed value. A range/font
change affecting desired size requests layout+paint.

Hooks execute on the UI owner. Context provides typed draw/clip/service access;
no raw terminal output, hidden provider fetch or blocking I/O is permitted.
Geometry overflow/invalid transforms return typed errors. A hook error reports
through the sink: failed measure uses a documented empty/error placeholder,
failed arrange disables that node's hit-testing for that frame, failed paint
discards that widget's recorded draw commands and emits a clipped error
placeholder. A failing event callback stops handling that event and reports its
error; application side effects already performed are not rolled back. Hooks
do not recursively dispatch or mutate the tree during layout/paint.
Required future evidence: external consumer, clipped drawing, failing hooks,
stale handle access and destructor after active callback teardown.

## 5. Canvas game

```cpp
auto game = std::make_shared<GameState>();
auto canvas = TRY(app.root().emplace<Canvas>(CanvasOptions{
  .paint = [game](CanvasContext& draw) { return draw_game(draw, *game); }}));
auto scope = TRY(app.ui().make_node_scope(canvas));
auto input = TRY(app.ui().connect(scope, canvas.actions(),
  [&, game, canvas](const Action& action) {
    TRY(game->apply(action));
    return app.ui().invalidate(canvas, Invalidation::PAINT);
  }));
auto timer = TRY(app.clock().every(scope, config.step_period, [&, game, canvas] {
  TRY(game->advance_fixed_step());
  return app.ui().invalidate(canvas, Invalidation::PAINT);
}));
```

The tree owns Canvas; callback captures own game state. The controller owns the
node-bound scope and tokens; removing Canvas closes its scope and timers
immediately, with active dispatch reclamation deferred. `Action` is borrowed for
one callback and must be copied if retained. Input/timer state changes and paint
run on the same UI owner, so paint sees a stable state without a provider lock.
Clock/scheduler are explicit services, independent of terminal output.

Simulation uses fixed steps; frame requests coalesce and may skip display, not
accepted state updates. The scheduler bounds catch-up by
`config.max_steps_per_dispatch`; if debt exceeds `config.max_step_debt`, it pauses
the simulation and reports an observable overload result rather than silently
dropping updates. Input requires negotiated capability: use semantic discrete
actions by default; held-key/release behavior is enabled only if available.
Timer registration can fail (`INVALID_ARGUMENT`, `NO_MEMORY`, unavailable clock);
draw/transform/capability failure uses the same clipped error path as widgets.

Canvas borrows its draw context only during paint, uses logical game coordinates
and an explicit viewport transform into terminal cells, and intersects every
draw with the half-open clip. Invalid/non-finite coordinates return an error.
Movement invalidates paint; a changed intrinsic Canvas extent invalidates
layout+paint. Closing the app cancels timers before executor shutdown and returns
session errors through `run`. Required future evidence: deterministic fixed-step
replay under skipped paint, overload pause, capability fallback, clipping and
no timer callback after Canvas destruction.

## Boundaries and references

These are proposed API contracts scoped to these scenarios. Runtime teardown,
snapshot/model interfaces, geometry types and packaging must receive their own
implementation tests before being called supported. No stable C ABI, wire
protocol, remote callbacks, installed modules, solver or complete widget catalogue
is implemented here. The module spike does not change the core's CMake minimum.

Primary references: [CMake C++ modules](https://cmake.org/cmake/help/v4.4/manual/cmake-cxxmodules.7.html),
[Clang ABI-preserving module styles](https://clang.llvm.org/docs/StandardCPlusPlusModules.html#abi-preserving-styles),
[MSVC DLL attributes](https://learn.microsoft.com/en-us/cpp/cpp/dllexport-dllimport?view=msvc-170).
Those describe compilation/linkage mechanisms, not validation of this UI design.

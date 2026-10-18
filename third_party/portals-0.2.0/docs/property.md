# Typed properties

`Property<T>` is a UI-owned value with immutable owning snapshots and ordinary
[Scheduler subscriptions](scheduler.md). Include `<portals/property.hpp>`.
It is a small value/notification component: there is no binding language, event
bus, transaction across properties, or lossless multicast.

## Ownership and access

Create a property with `Property<T>::create(tree, target, initial, invalidation,
options)`. The tree and its scheduler are borrowed and must outlive the property.
The target is a checked `Node<Widget>` in that tree. It identifies where a change
requests paint or layout; `Invalidation::LAYOUT` also includes paint.
`PropertyOptions::max_observers` bounds the endpoint list (64 by default).

All property operations and destruction belong to the tree's UI thread. A foreign
thread receives `TreeError::WRONG_THREAD`; wrong-thread destruction terminates,
matching the tree and subscription ownership rules. Use a separate Scheduler
publisher to transfer worker values to UI code before calling `set`.

`snapshot()` returns `shared_ptr<const PropertyChange<T>>`, with `value` and a
monotonic `revision` starting at zero. The owner survives replacement, source
removal, or destruction of the property and tree. It may be transferred to another
thread if `T` supports destruction on the thread releasing the last owner.
Any UI reentry from that destructor requires the UI owner thread and a still-live
UI context. `T` must transitively own its data; mutable aliases must not mutate a
published value. Revisions are local to the source, not globally comparable IDs.
The bound node retains the tree's session/slot/generation diagnostic identity.

`observe(receiver, callback)` returns the existing move-only `Subscription`.
The callback receives `(W&, shared_ptr<const PropertyChange<T>>)` for a `Node<W>`.
The receiver must belong to the same tree. Registration does not deliver the
initial value. Read `snapshot()` explicitly when initial state is needed.
The source target is invalidated by `set`; callbacks that change other nodes
request the corresponding invalidation themselves.

## Commit and backpressure

`set(value)` first validates the target and frame boundary, compares the value,
and prepares an immutable next snapshot. Equal values neither advance revision,
request a frame, nor queue notifications. A changed value requests the configured
invalidation and commits its snapshot before attempting notifications.

An `unexpected(TreeError)` means the operation made no library-controlled change
to the value, revision, queue or invalidation. Allocation failure is `NO_MEMORY`;
exceptions from value comparison/copy/move are `CALLBACK_FAILED`; exhausted revision
space is `IDENTITY_EXHAUSTED`. Construction of caller arguments is outside this
boundary, as are application side effects in user code. Removed targets reject
`set` with `STALE_NODE`; retained snapshots still work. Mutations inside a tree
frame are rejected with `WOULD_BLOCK`.

A successful `PropertyUpdate<T>` has an owning `snapshot`, `changed`, and delivery
counts `accepted`, `rejected`, `cancelled`, plus the first `notification_error`.
Publication uses the data lane and `SchedulerOverflow::REJECT`, in registration
order. The scheduler accepts one receiver at a time: a full queue can accept the
first observer and reject the next. The update remains committed and its frame
request remains pending, including when every notification is rejected. The
receipt makes that outcome explicit; it is not an `unexpected` failure.

The counts describe queue acceptance, not eventual callback execution. Cancellation
can still suppress accepted notifications. Already expired endpoints are removed
before publication; `cancelled` counts endpoints discovered closed while publishing,
such as an observer that disconnects itself and then reenters `set`.

There is no hidden retry. Repeating the same value is still an equal no-op, even
when an earlier receipt reported rejection. A caller requiring resynchronization
must use the current snapshot through its own application policy. Do not use this
API as a lossless domain-event stream. No notification is promised to all observers
when the bounded scheduler queue is full.

## Construction and allocation boundaries

Both `create` and `set` provide ordinary `const T&` and `T&&` overloads. The
const-reference overload copies directly into the immutable snapshot inside the
exception boundary. The rvalue overload forwards directly into that snapshot,
with no intermediate owning `T`. Copying is required only when the caller selects
the const-reference overload; genuinely move-only values and containers such as
`vector<unique_ptr<int>>` remain supported by the rvalue path. Copy traits are not
used to guess which operation is valid.

For recoverable allocation errors with `std::string`, pass an existing lvalue.
MSVC's Debug STL may allocate iterator bookkeeping even inside a `noexcept` move
constructor. Such an allocation failure terminates inside the type and cannot
be caught by Property; the const-reference path uses the throwing copy constructor
and reports `NO_MEMORY`. The same limitation applies to any user type that
terminates internally. Construction of an argument before API entry is also
outside the error boundary. Debug iterators and assertions remain enabled.

An rvalue is compared before being moved. If snapshot construction succeeds but
later tree invalidation fails, the property is unchanged while the caller's
candidate can already be moved-from. Publication order, immutable revisions,
old-value retention and notification receipts are identical for both overloads.
Code taking the address of `create` or `set` must now select its desired overload
explicitly; ordinary call syntax stays unchanged.
## Reentry and cancellation

Callbacks run through the real Scheduler between frames. If observer A handles
revision 1 and calls `set` for revision 2, already accepted revision-1 events stay
ahead of the new events. With observers A and B and sufficient queue capacity,
the sequence is A1, B1, A2, B2. Delivery never recursively calls an observer from
`set`. A callback that changes the property indefinitely consumes the scheduler's
finite data budget and continues on a later dispatch; it does not grow a stack.

Disconnecting a token, closing the receiver's node scope, or removing its subtree
prevents another callback from starting. An executing callback keeps its captures
and its borrowed widget alive until it returns, using `WidgetTree::connect`.
Property destruction independently suppresses queued callbacks through a weak
source state. A currently executing callback may finish with its owning event.
Destroy the property before the borrowed tree. Destroying the tree itself from an
active callback remains prohibited by the tree contract; use `close()` instead.

The property stores bounded publisher endpoints and weak liveness markers, not a
second callback registry. Expired records are pruned on changed `set` and `observe`.
An executing observer still occupies its slot until its callback captures are
released, including after a cancelled publication. Registration rechecks the limit
after temporary callback destructors; source destruction during this step reports
`CLOSED`. The endpoint limit reports `NODE_LIMIT`; the scheduler's own registration
or queue capacity is reported through its existing errors.

Value construction/copy/move/comparison and callback construction/move must not reenter
UI operations. Destructors must not throw or call members of an object currently
being destroyed. An old value is retained until all publication attempts finish;
its destructor may then reenter a live source. The returned receipt still identifies
the original operation's revision, even if such reentry commits a newer value
before the caller receives that receipt. Callback-capture destruction may also
reenter a live property; Scheduler remains responsible for callback reclamation.

## Verification

`portals.property` drives real Scheduler and WidgetTree instances. It checks
paint/layout counters, no-op equality, owning revisions, deterministic reentry,
full and partially full queues, finite observer storage, source and node lifetime,
scope closure, self-disconnect, destructor reentry, allocation failures, and
wrong-thread/session rejection. The test runs without a terminal or interactive
windows. See [the tree contract](widget-tree.md) for frame traversal and rendering.

# Immutable model snapshots and scoped ranges

`Model<T>` separates a typed domain provider from its list, table or chart views.
For a list, `T` can be a scalar; for a table/chart it is an application record.
`ModelRow<T>` carries a stable `RowId` independently of row position. Applications
associate typed record accessors with `ColumnId`; there is no stringly typed cell
conversion or implicit formatting in this layer. Reordering/inserting rows must
preserve their IDs. Reusing a numeric row/column identity requires a new generation;
providers must never wrap or reuse an exhausted pair.

A provider derives from `ModelSnapshot<T>` and supplies immutable data revision,
logical row count and bounded `read(RowRange)`. Reading only accesses already
available memory. It must not perform I/O, wait for a provider mutex, start work,
mutate domain state or format unseen rows. Ready results contain exactly the
requested rows; pending/error results contain no rows. Error text belongs to the
snapshot. The library checks interval arithmetic and result shape, translates
provider exceptions and retains the snapshot behind every returned `ModelRange`.
Providers are responsible for unique stable IDs and agreement between every
ready range of the same data revision, including overlapping ranges returned in
separate snapshot objects. Provider metadata is immutable after construction.

## Ownership and a coherent frame

Create the model on the UI owner with a real `WidgetTree` and a context node.
The tree must outlive the source. A root or long-lived application context lets
the model outlive disposable views. Removing the context closes both Scheduler
endpoints; destroying the source also closes them. A copied publisher can outlive
source, tree and Scheduler, and then reports `CLOSED`.

Before each `WidgetTree::frame`, call `model.pin()` once and pass that same
`ModelFrame<T>` to every view of this model. It retains the base provider snapshot
and one immutable table of consumer results. Its `revision()` is the UI frame
revision, including range requests/readiness; `dataRevision()` identifies domain
data. Different models do not acquire an implicit global transaction. Dispatch
Scheduler only between frames. Source mutations/request creation during a tree
frame are rejected; a worker can enqueue concurrently without changing the pin.

`frame.read(rows)` reads the base provider snapshot. `frame.range(consumer.id())`
reads exactly that consumer's result and does not merge overlapping intervals.
Both return independent `ModelRange<T>` owners. Their row spans and error text
remain valid while a range or its underlying snapshot owner is retained, even
when the view and provider source have been destroyed. A `shared_ptr<const T>`
is not deep immutability: providers must not modify reachable storage through
other aliases. Snapshot/row/work destructors can run on whichever thread drops
the final owner. Any destructor that reenters UI requires a live UI context and
must itself run on the UI owner.

## Asynchronous ranges

`model.openRange()` returns a move-only UI-owned `ModelConsumer<T>`. Each consumer
has a never-reused `ModelRangeId`, even when two consumers request identical or
overlapping rows. `consumer.request(rows, work_owner)` returns a copyable worker
`ModelRequest<T>` and commits pending readiness for that consumer. The library
does not create threads or launch I/O; the caller schedules its provider work
with the returned token. The optional owner keeps provider work/storage alive;
when omitted, the base snapshot is retained.

The worker builds an immutable snapshot containing its requested ready range or
an error and calls `request.complete(snapshot)`. It must use the original data
revision and logical row count. The result is published through the existing
bounded Scheduler. UI delivery checks consumer identity, request generation,
data revision, cancellation and source/context lifetime before updating that
single slot. A replacement request cancels only its own previous generation.
Independent completions can arrive in either order without losing each other.
One consumer's close/destruction never cancels another consumer with equal rows.

`request.cancel()` is cooperative cancellation of that generation. It does not
stop a thread, join it or release its reservation. Closing a consumer cancels its
current generation without allocation and retires its slot; retained immutable
tables may keep the old result until slot reuse or source destruction. Their
revision and data remain unchanged. Applications no longer use a retired handle
for new rendering. An older pin may still read it. Source destruction makes all
live tokens report `stopRequested`; context destruction instead closes delivery,
so a completion may first discover it through `CLOSED`.

`model.publisher().publish(snapshot)` enqueues a full provider replacement.
Only strictly newer data revisions apply. A new revision clears consumer results
to pending and cancels work from the old data revision; each consumer requests
its new interval after reading the next pin's row count. Old queued completions
cannot overwrite it. Domain aggregation belongs to the provider before publishing:
a display snapshot must already preserve every accepted tick's OHLC/volume effect.
No domain events are coalesced by the model.

Assignments replace all fields before releasing old storage. A last-owner destructor may reenter the live UI context, including releasing the assigned owner; code must not use the returned reference after such destruction. No concurrent mutation of the same owner object is supported.

## Bounds and failures

`ModelOptions` separately bounds rows per read/request, live consumer slots and
in-flight request owners. All limits are positive. Logical row count does not
allocate rows or widgets. Capturing a pin and reading a ready range only copy
small owners, not the domain rows. Publication copies at most the bounded consumer
table. The synthetic regression compares allocation counts/bytes for 100 and
1,000,000 logical rows and verifies zero row copies over ten publications.

- Out-of-bounds/oversized ranges and malformed provider results return
  `TreeError::INVALID_ARGUMENT` (completion uses `SchedulerError::INVALID_ARGUMENT`).
- Consumer capacity returns `NODE_LIMIT`; work/mailbox capacity returns
  `WOULD_BLOCK`. Failed requests leave prior state unchanged.
- A work reservation survives cancellation, queued completion and destruction of
  the UI consumer until the last worker/queued token releases it. Provider storage
  is destroyed before freeing the reservation, including destructor reentry.
- Completion accepts at most one successful submission per generation. A full
  mailbox permits retry; a duplicate or cancelled token returns `CLOSED`.
- Queue acceptance is not an application receipt. An allocation failure while
  applying on UI leaves the old table intact and is recorded by `lastError()`.
  It is a sticky diagnostic. Start a new request to retry the intended range.
- UI operations check thread affinity. Consumer/source destruction on a foreign
  thread terminates. Provider reads and transferred immutable owners may be used
  on threads on which the provider's contract permits their destruction.

Successful range/full-snapshot application invalidates layout (and therefore
paint) on the context node. No observer framework or per-row UI objects are
introduced. Source/pins retain storage explicitly; no hidden caching, eviction,
spatial merge, coroutine runtime or automatic worker cancellation is implemented.

See [Scheduler ownership and bounded queues](scheduler.md),
[tree scopes and frame boundaries](widget-tree.md) and
[typed property lifetimes](property.md) for the adjacent contracts.

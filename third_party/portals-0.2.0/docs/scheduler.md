# UI scheduler and bounded publication

`Scheduler` binds to the thread that calls `create`. The application calls
`dispatch()` between frames. It does not run a background executor, write to the
terminal, mutate widgets, or request frames. Callback code applies changes and
chooses the resulting invalidation. Layout and paint must not call `dispatch` or
`waitFor`. Reentrant dispatch and waiting inside a callback return
`REENTRANT_DISPATCH`.

## Queues, budgets and backpressure

`SchedulerOptions` has independent positive capacities for control (32 by
default) and data (256), separate callback budgets (32 each), a timer budget
(32), and a combined live subscription/timer capacity (256). Storage for the
queues and registration index is reserved at creation. Publication allocates
no scheduler storage. Snapshot sizes and application-retained owners are the
application's responsibility; the queue limits count owners, not payload bytes.

Every dispatch services control, then due timers, then data. Each has its own
budget, so continuous data or timer traffic cannot consume control's share.
Control FIFO entries already accepted are serviced in at most
`ceil(control_capacity / control_budget)` dispatch calls, unless explicitly
cancelled or stopped. The default services the entire control queue in one call.
A complete fair cycle is that many dispatch calls. Callbacks must finish
promptly: these are bounds on work counts, not a wall-clock deadline for
arbitrary application code. Late arrivals are eligible in the current dispatch
if its budget remains, otherwise in the next one.

Keyboard, resize and ordinary quit commands use the control lane. Both lanes
return `WOULD_BLOCK` at capacity; the caller must retry after draining or report
backpressure rather than silently discarding an event. `requestStop()` is an
independent flag and wakeup, consumes no queue entry, and works even when both
queues are full. It prevents new publications immediately; the UI finishes any
invocation already selected for dispatch, then stops starting queued callbacks.

The default overflow policy is `REJECT`. Data connections can explicitly select
`REPLACE_LATEST`: at most one pending snapshot for that sender is retained,
replacement keeps its FIFO position, and other senders are unaffected. This
policy is not allowed for control. Full queues can still replace an existing
value for the same sender. A sender without a pending value receives
`WOULD_BLOCK`. Replacement is suitable for a complete display snapshot **after**
lossless domain aggregation. Raw ticks, incremental changes and unresolved
provider errors must not be silently removed; include status in the complete
snapshot or use rejection/backpressure.

## Ownership and cancellation

`connect<T>` creates a `SchedulerConnection<T>`. The controller retains its
move-only `Subscription`; a worker receives only a copy of `Publisher<T>`.
Publication transfers a `shared_ptr<const T>`. The producer must supply an
owning, transitively immutable snapshot and must not mutate aliases. The
callback receives another strong owner, which it may retain. Snapshot deleters
may execute on a producer or the UI, so snapshots must not own thread-affine
UI resources. Callback captures, by contrast, are released on the UI.

```cpp
auto runtime = Scheduler::create();
if (!runtime) { return; } // Report the creation error in the application.
auto scope = (*runtime)->makeScope();
if (!scope) { return; } // Report the scope error in the application.
auto connection = (*runtime)->connect<QuoteSnapshot>(
  *scope, [model](std::shared_ptr<const QuoteSnapshot> snapshot) {
    model->apply(std::move(snapshot));
  }, SchedulerLane::DATA, SchedulerOverflow::REPLACE_LATEST);
// Check connection before giving connection->publisher to a worker.
```

`QuoteSnapshot` and `model` illustrate application types; a model implementation
is not part of this scheduler. Check every result before dereferencing it.

Destroying/disconnecting a subscription or closing its `UiScope` prevents new
invocations, including queued values. Scope close logically disconnects all its
registrations before reclaiming the first callback, including when a capture
destructor reenters the scheduler. The currently executing callback retains its
captures until it returns. Self-close never joins a worker. Arbitrary reference
captures still require an external owner: cancellation cannot prolong a raw
reference's lifetime. Snapshot and callback deleters never run under the queue
mutex.

UI-owned scopes and subscriptions must be released before the scheduler. Their
destruction/move assignment on another thread violates affinity and terminates;
explicit `close`/`disconnect` instead return `WRONG_THREAD` without mutation.
Disconnect/close are idempotent, and scopes and their subscription tokens can be
released in either order. Identities are never reused and exhaustion is reported.

Publishers can outlive the scheduler: they retain closed channel state, not
callback captures, and late publications return `CLOSED`. `requestStop` is a
request, not proof of worker completion. The application owns worker stop/join
and provider storage lifetime; it joins workers outside layout/paint. UI
`stop`, the next `dispatch` after a worker stop request, or scheduler destruction
reclaims callbacks. `waitFor` only observes stop. Destroy the scheduler on its
owner, outside callbacks. It never synchronously joins foreign work.

## Timers, clocks and waiting

`at(scope, absolute_milliseconds, callback)` registers a one-shot timer using
the same scoped cancellation contract. Due timers run in deadline order, then
registration order for ties. Excess due timers remain pending; the scheduler
does not silently discard elapsed updates. A recurring simulation can register
its next absolute deadline explicitly and define its own overload policy.
Periodic/fixed-step simulation and model revision validation are later layers.

`SchedulerClock` is an optional borrowed monotonic millisecond service. Its
reader must be thread-safe, nonblocking, non-reentrant and `noexcept`, and its
context must outlive the scheduler. The default uses `steady_clock`. The clock
is sampled under the queue mutex for timestamps and timer decisions; closed
publishers never access it. Deterministic tests advance fake time without
sleeping and call `dispatch`. Advancing a fake clock in another thread requires
synchronization and `Publisher::wake()` to notify an existing wait.

`waitFor` is a UI-only finite condition-variable wait, ending on queued work,
a due timer, an explicit wake, stop, or the real timeout. The maximum real wait
is independent of injected clock progress. Wake is a coalesced one-shot signal,
so a notification immediately before waiting is not lost. An application that
also owns terminal input must coordinate that input through its adapter; this
API does not add an OS-wide terminal wait multiplexer.

## Errors and measurements

Creation rejects zero/impossible capacities and translates allocation failure
to `NO_MEMORY`. Registration translates allocation failure while taking
callback ownership; allocation in caller argument expressions is outside the
factory boundary. Other exceptions from user-defined callback copy/move
constructors propagate without installing a registration. Registration capacity
returns `WOULD_BLOCK`; foreign scope and thread errors are explicit.

An exception thrown by a dispatched callback disables that subscription and
discards its remaining queue entries. Dispatch catches the exception, increments
failure metrics and invokes the optional borrowed `noexcept` error sink on the
UI. The dispatch result also reports the failure count when no sink was set.
Application side effects already performed are not rolled back.

`metrics()` returns a thread-safe copy: each queue exposes occupancy/high-water,
accepted/rejected/replaced/delivered/cancelled counts, last/max/total queue delay,
plus scheduler cycle, timer and callback failure counts. Latency starts at the
most recent replacement, in injected-clock milliseconds. Counters saturate at
`UINT64_MAX`; no sample history grows inside the scheduler. A future benchmark
can sample these values or record callback timing externally for percentiles.
No native latency or throughput claim is made by the deterministic tests.

`tests/scheduler.cpp` covers sustained flood fairness, overflow/coalescing,
clock-derived metrics, timer order/debt, cancellation and self-close, exceptions,
reentrant deleters, UI affinity, late publishers, stop under saturation and one
bounded native worker wakeup. Timer/latency tests do not sleep. The native wakeup
uses a clock handshake while the wait holds the mutex, so the producer publishes
only after the wait releases it.

The synchronization and owner primitives follow the C++ working draft's
[condition-variable rules](https://eel.is/c++draft/thread.condition.condvar)
and [shared ownership rules](https://eel.is/c++draft/util.smartptr.shared).
Platform failures of standard synchronization primitives follow their standard
exception/termination contracts; Portals does not emulate a failed OS mutex.

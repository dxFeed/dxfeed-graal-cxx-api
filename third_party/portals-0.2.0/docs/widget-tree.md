# Widget tree and lifetime

`WidgetTree` is a C++23 UI-owned retained tree. It borrows an existing `Scheduler`,
which must outlive it. Parents exclusively own their children; models remain
separate explicit application owners. `Node<T>` is a copyable observer with no
pointer operators. Its owning `NodeId` contains a process-unique session, registry
slot and 64-bit generation. Slots retire instead of wrapping their generation.

## Checked access and structure

Create the tree with `WidgetTree::create(scheduler)`, obtain its checked `root()`,
and construct children with `emplace<CustomWidget>(parent, arguments...)`.
`edit(node, callback)` lends a mutable reference for one void callback; `read`
lends a const reference, including during frame traversal. References must not
escape. Const access does not promise deep immutability of arbitrary application
objects reachable through a custom widget.

All operations check the UI thread before accessing mutable state. Foreign
sessions and stale generations return distinct errors before starting user code.
`inspect` returns an owning snapshot of parent, ordered live child handles, flags
and geometry. A snapshot is not a promise that those handles remain live.

Insertion creates only new nodes; attachment, detachment and reparenting are not
provided, so parent cycles cannot be constructed. Construction and library
allocation failures do not partially attach a child. Constructors can have their
own application side effects, which the tree cannot roll back. Construction is
followed by another parent/capacity check before commit.

Default limits are 4096 stored nodes and depth 128, including the root. The depth
limit may be configured up to 256 to bound recursive teardown stack usage. Live
and logically removed but borrowed nodes share the storage budget. There is no
unbounded deferred command queue. Ordinary edits may insert a child immediately;
lifecycle hooks, reclamation and frame callbacks reject insertion with
`WOULD_BLOCK`. This implementation does not provide the deferred insertion queue
illustrated by the earlier design-only sketches.

## Removal and callbacks

`remove` first marks the complete subtree dead without allocation, then closes
its node-owned `UiScope`s. All subsequent checked access fails immediately.
Already queued publications and timers cannot start another widget callback.
Physical storage stays alive until the outermost tree borrow returns. Nested
edits, callback self-removal and ancestor removal therefore preserve active
references. Slot reuse starts only after actual destruction.

`mount` runs once after attachment. A mount exception is reported through the
nonthrowing UI error sink; the committed insertion still returns its handle.
Mount may remove itself: the returned handle is then already stale, and unmount
and destruction still happen exactly once. `unmount` runs children before parent;
its exceptions are reported and cannot prevent cleanup. Reentrant removal during
unmount or destruction is supported. Reclamation rejects new insertions and
makes bounded repeated passes if a hook removes an already visited sibling.
No traversal is needed on ordinary borrow exit when nothing was removed.

`connect<Value>(node, callback)` and `at(node, deadline, callback)` wrap the existing
Scheduler registrations. Their callbacks receive a checked concrete widget
reference for the entire invocation. The returned subscription has the existing
Scheduler ownership rules; the node scope provides an additional cancellation
boundary. Scheduler retains an executing callback's captures, while the tree
retains widget storage. A throwing callback reaches both the tree error sink and
Scheduler's disconnect boundary. `withScope` supports other Scheduler
registrations; any callback borrowing a widget must wrap its complete invocation
in checked `edit`. Arbitrary raw reference captures are the application's burden.

`close` supports shutdown from an edit callback: access closes immediately and
physical cleanup waits for that callback. Destruction must run on UI, outside any
tree callback; destroying the tree from its own callback or another thread
terminates. Close/reclaim the tree before stopping/destroying its Scheduler.
Error sinks are borrowed, nonthrowing callbacks; nested reports are suppressed.

## Geometry, input and frames

Hidden subtrees participate in neither layout, painting nor input. Disabled
subtrees keep layout and paint but do not receive input. Equal visible/enabled
setters do nothing. `inputEligible` checks live/visible/enabled ancestors;
keyboard eligibility does not depend on pending layout. Hit-testing must also
require `geometry.valid` and use the half-open surface clip.

Geometry snapshots own local, translated surface and effective clip rectangles.
Arrange the parent first. Root arrangement intersects with the supplied viewport;
a child translates relative to its parent and inherits its effective clip.
Changing parent placement invalidates descendant placement. Failed arrangement
or translation leaves geometry explicitly invalid, never an old valid hit target.
Layout invalidation conservatively invalidates placement throughout this bounded
tree. Paint invalidation preserves it. Layout includes painting; requests coalesce
into one `Invalidation` value consumed by `consumeFrameRequest` before work.

The layout owner uses an external traversal: inspect ordered children and `read`
the custom const layout interface, then call guarded `measure`/`arrange` hooks.
Recursive calls from one extension hook into another are rejected. No size-policy
solver is included here. Default Widget measure is empty; default arrange and
paint succeed without additional work. Hook failures are explicit return values
and also reach the error sink. The caller supplies any placeholder policy.

Wrap the entire traversal **and render consumption** in `tree.frame(callback)`.
`paint` requires this outer boundary. Mutation and close are rejected inside the
frame. `PaintContext::submit` borrows render commands; a sink may consume them
immediately or record them only until that frame callback ends. In particular,
`TextCommand` borrows `GraphemeText`: call `renderFullFrame` before leaving the
frame and clear recorded commands there. Never render retained borrowed commands
after the frame. The tree does not introduce a second renderer, focus system,
property bus, or paint command owner. Hooks perform no blocking I/O or terminal
writes, and application model mutation is prohibited during frame work.

Paint error handling belongs to the frame owner: it must record each widget's
commands in a temporary transaction and include them only when `paint` succeeds.
On error, it must discard that transaction and emit a clipped error placeholder,
following the API design contract. `WidgetTree::paint` does not roll back earlier
`submit` calls or constrain arbitrary commands supplied to a caller-owned sink.
The application frame coordinator planned for PORTALS-028 supplies this policy
using `NodeGeometry::clip`; it is not implemented by the tree.

## Verification and references

`portals.widget-tree` uses the real Scheduler for self/ancestor removal, queued
publication and timer cancellation, exceptions, lifecycle reentry and shutdown.
It checks session/generation/thread rejection, allocation failure before commit,
allocation-free removal, shared-model ownership, generated mutation sequences,
external custom-widget hooks and borrowed text rendered within the frame.
Generation retirement arithmetic is checked at compile time at the 64-bit limit.

The standard ownership and invocation building blocks are
[unique_ptr](https://eel.is/c++draft/unique.ptr),
[expected](https://eel.is/c++draft/expected), and
[function invocation](https://eel.is/c++draft/func.invoke).
See also [Scheduler cancellation](scheduler.md), [cell geometry](geometry.md), and
[renderer borrowing](render.md). Those primitives alone do not establish the tree
lifetime guarantees; the explicit boundaries above and regression tests do.

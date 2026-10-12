# Application runner {#application}

`portals::Application` owns one `Scheduler`, `WidgetTree`, and `EventRouter` on
its creating UI thread. Construct it before a scene and destroy the scene first.
The scene owns typed node observers, models, bindings and its own data source.
`Application::run` borrows that scene for one run; callbacks must never destroy
or reenter either owner.

```cpp
#include <portals/application.hpp>
#include <portals/controls.hpp>

int main() {
  auto created = portals::Application::create();
  if (!created) return 1;
  auto app = std::move(*created);
  auto text = portals::GraphemeText::from("Quit");
  if (!text) return 1;
  auto button = app->tree().emplace<portals::Button>(app->root(), std::move(*text));
  if (!button) return 1;
  auto quit = portals::bindButton(app->router(), *button, [&] { app->requestStop(); });
  if (!quit) return 1;
  portals::ApplicationScene scene;
  auto result = app->run(scene);
  // The terminal has been restored; report result.error() here in a full program.
  return result ? 0 : 1;
}
```

The runnable [gallery](examples.md) demonstrates input bindings, explicit quit,
control synchronization, two themes, and error reporting. There is no DSL or
second dispatcher. Extend `ApplicationScene::input` for global commands; it runs
inside the existing bounded CONTROL channel before normal `EventRouter` routing.
For ordinary control actions retain the existing `ActionBinding`/control bindings.

## Frame preparation

`prepare(application, size)` runs only when a dirty frame can be submitted. It
runs after the cycle's single scheduler dispatch, outside `WidgetTree::frame`.
Synchronize bindings and focus, take **one pin per model**, and prepare all sibling
views from that pin using their final layout extents. `VirtualTable` and `Chart`
retain their own copies of the pin and immutable prepared data. Dispatch returned
`ModelRequest` receipts outside rendering; no provider work starts implicitly.

For nested virtual views, opt into [layout-driven preparation](virtual-preparation.md).
The runner then calls `prepareViews(application, snapshot)` after a preliminary
layout, outside the tree frame. All siblings use the same owning geometry and
the pins retained by `prepare`; bounded layout retries never repin models.
The original explicit-size path remains the default.

The runner performs layout, traverses visible nodes, translates local paint
commands through the arranged surface geometry and clips, and consumes every
borrowed text command into an owning `CellBuffer` before leaving the frame.
Custom widgets therefore implement the same `measure/arrange/paint` contract as
built-in widgets. Painting must not do I/O, model mutation, scheduler dispatch,
or asynchronous work. Geometry-driven paint invalidation is consumed by the frame
which already painted it; idle does not schedule periodic repaint.

## Service bounds and errors

One owning terminal batch/raw suffix and one rejected CONTROL payload survive
between cycles. Native Windows events are not decoded a second time. POSIX bytes
retain arrival timestamps across suffix retries; decoder deadlines shorten input
waits. Input capacity pressure pauses admission without dropping keys. Every cycle
has one bounded dispatch and at most one frame submission/flush. Dirty state stays
in the tree while output is pending. The next frame deadline is measured from
actual submission/output completion; late work never causes frame catch-up bursts.

EOF is a transport cutoff: the decoder finishes, admitted CONTROL is delivered,
and already submitted output is flushed before cleanup. It does not wait for a
future frame deadline, timer or DATA publication. An accepted edit may therefore
remain dirty and absent from the final output if EOF arrives before its next
frame; there is no shutdown exception to the frame cap. Finite replay scenarios
that assert visible changes must leave sufficient time before EOF. Cancellation
can stop earlier and does not promise to deliver remaining CONTROL.

`inputSlice` is a positive read deadline; `read(now)` would return an immediate
deadline without polling. A configured cadence still causes input/resize sampling
in idle, but no frame is rendered or emitted without invalidation. The loop uses
terminal reads as its wait; the cadence bounds how long worker publications can
wait before dispatch. It does not claim an OS-integrated scheduler wait primitive.

`outputSlice` defaults to two seconds. An unfinished retryable transaction stays
owned by `FrameWriter`; any transport error, including Windows
`OUTPUT_UNCERTAIN`, ends the run. It never reopens or replays uncertain bytes.
Native close still requires the console host/driver to make progress; the runner
does not promise a hard shutdown deadline for a stalled host.

`ApplicationError` preserves the failing phase and original component error.
A separate `restoration` field preserves close errors alongside an earlier
failure. Hook/allocation/observer failures are explicit. Stop first closes the
scheduler, then invokes `scene.stop` to cancel/join scene-owned producers, then
closes the terminal. This also happens after errors and pre-requested cancellation.
No diagnostics, exceptions or metrics are printed while the terminal is active.

## Clocks, replay and measurements

The native overload requires the default steady clock. The custom
`ApplicationTerminal` overload borrows an exclusive terminal through `close` and
supports an injected monotonic clock. Its millisecond epoch must map directly to
the terminal's `TerminalDeadline`; mixing a fake epoch with native I/O is rejected.
Custom transports must honor the configured batch bounds and own their results.
Both overloads are single-use. See `examples/replay/terminal.hpp` for an in-memory
transport; it is example code, not a second production terminal backend.

An optional borrowed `ApplicationObserver` receives preparation/render, encode,
flush, stop and close durations. With no callback, these measurements perform no
clock reads or sample allocation. The observer must not reenter or perform I/O;
its exception fails the run. `CONTROL_LATENCY` samples only the runner's own
input/resize channel, read at callback entry, in logical clock time. It is not a
distribution for every scheduler channel. Returned `ApplicationStats` owns totals;
benchmarks own and bound their sample storage.

Diagnostics remain disabled unless explicitly configured on the scheduler.
Input correlation enters at admission, survives CONTROL delivery and frame
preparation, and reaches the actual writer submission/output. Export snapshots
only after `run` restores the terminal, as the dashboard's `--trace` demonstrates.
EOF, cancellation, and output failure are observable outcomes, not evidence that
a real terminal visually displayed a frame.

## Managed native transport

The native `Application::run(scene)` overload uses `openNativeApplicationTerminal`.
By default, `ApplicationOptions::nativeScreen` is `NativeScreenMode::ALTERNATE_SCREEN`:
the adapter enters a separate screen with VT `1049` before rendering and returns
to the caller's main screen on close. The main screen's contents, cursor and
rendition belong to the terminal host and are preserved through this lifecycle.
The adapter also preserves cursor visibility: Windows saves/restores console
cursor information; POSIX uses xterm saved private mode `25`.

Set `nativeScreen = NativeScreenMode::MAIN_SCREEN` to paint the caller's screen
directly. This explicit opt-out leaves the final frame and cursor in that screen;
it does not erase the frame at exit. The factory accepts the same screen policy
as its final argument. Custom/replay transports own their screen policy and are
not wrapped automatically. `TerminalSession::open/close` retain their low-level
mode-only contract and add no screen switch.

Changing the terminal window size during the run keeps the **new** size after
exit. The terminal host may reflow or crop the main screen and clamp its saved
cursor to the new geometry. Portals does not restore an old window size, an old
pixel layout, or absolute cursor coordinates. This follows the host's
[Windows alternate-screen behavior](https://learn.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences#alternate-screen-buffer)
and [xterm VT1049 protocol](https://invisible-island.net/xterm/ctlseqs/ctlseqs.html).

`ApplicationOptions::nativeMouseReporting` defaults to true. On POSIX, setup
queries coordinate encoding modes 1005, 1006, 1015, and 1016 with DECRQM, saves
tracking modes 9, 1000, 1001, 1002, and 1003 with XTSAVE, then enables 1000 button
tracking and 1006 SGR cell coordinates. Unsupported or permanently incompatible
encodings fail before their mutation. No initial mode is assumed disabled.
The reader preserves non-query startup bytes, including input between fragmented
replies, for its first read. Retained input is capped by `max_input_bytes`; one
constant-sized incomplete query suffix can coexist with it. Missing replies,
excess input, or transport errors fail setup explicitly. Failed setup consumes
already read input and returns no replay transport. Matching query syntax cannot
authenticate terminal replies against byte-identical injected input.

Cleanup restores encodings from their queried values and tracking with XTRESTORE.
This split avoids a verified xterm 390 defect: its encoding restoration casts the
saved numeric value to an 8-bit Boolean, while tracking restoration retains the
integer value. See the versioned primary sources
[ptyx.h macros](https://github.com/ThomasDickey/xterm-snapshots/blob/xterm-390/ptyx.h)
and [charproc.c saved modes](https://github.com/ThomasDickey/xterm-snapshots/blob/xterm-390/charproc.c).
Windows uses native console mouse events and sends no mouse setup bytes.
Mouse opt-out skips only mouse protocol I/O; it still manages the selected screen.

Close uses a fresh `modeTimeout` budget independent of application cancellation.
It first drains retryable output. A failed or uncertain write remains an error;
cleanup nevertheless attempts to return to the main screen. On Windows, it first
cancels and joins the old writer, then synchronizes with the console host after an
uncertain write, before starting the final restoration budget. Cancellation can
finish the client thread while accepted bytes still occupy the host. The
distinct restoration batch begins with CAN to cancel an incomplete VT sequence
and never replays uncertain frame bytes. A separate confirmed
DECSC capture protects the caller's current cursor/rendition even when a partial
entry never switches buffers: xterm's `1049` reset can restore its saved cursor
while already on the main screen. Unconfirmed capture uses CAN only; failed
startup suffixes are discarded rather than completed during rollback. Screen
state is armed for rollback before entry is submitted, so partial entry still
attempts an exit. Ordinary exit uses `1049` restoration without an extra DECRC. Cleanup capacity is
checked before changing the managed modes. Explicit close reports output and OS
restoration failures; repeated close adds no output. Destruction is best effort.

OS-mode restoration and owned-handle cleanup are always attempted, even when VT
restoration fails.
An expired output budget, broken transport, paused host, or abrupt process death
can leave the alternate screen active: no exact restoration is promised then.
Windows cancellation/join and native mode restoration still require host/driver
progress and have no hard shutdown deadline. Check the result of `run` and
`ApplicationError::restoration`; restored OS modes alone do not prove screen restoration.

This policy requires compatible VT1049 support and starts on the caller's main
screen. Nested alternate-screen owners, concurrent terminal output, and external
mode mutations are unsupported. POSIX XTSAVE overwrites one-level tracking and
cursor-visibility caches: current modes are restored, but previous saved caches
are not preserved. Confirmed DECSC capture also overwrites the saved cursor
cache; its previous contents cannot be preserved. Unsupported emulators do not acquire an exact VT restoration
guarantee; use the explicit opt-outs for an external terminal owner.

Setup and restoration bytes bypass FrameWriter and frame byte counters. A custom
transport decorator sees frame submissions and flushes, while factory setup and
close perform separate protocol I/O. Include them in lifecycle timing, not
rendered-frame throughput.

# Terminal session

`<portals/terminal.hpp>` provides a move-only Windows/POSIX terminal session.
Native Windows ConPTY and Linux PTY/pipe fixtures exercise the actual backends.
The separate diagnostic probe does not implement this API.
Other POSIX targets have not been qualified.

## POSIX ownership and modes

Call `TerminalSession::open({input_fd, output_fd}, options)` from the terminal
owner. Both descriptors must refer to TTYs. The session duplicates them with
close-on-exec and closes only its own duplicates. The caller retains its original
descriptors, but must grant exclusive access to **all aliases and the terminals**
until close: duplicate descriptors share file status flags and termios belongs to
the terminal. No logger, widget, other session or external thread may change modes
or consume/write the same streams during this interval.

Both original termios states and both file status flags are saved before the first
mutation, including when input/output are the same descriptor. Input disables
canonical mode, echo, signal-generating control keys, input translations and
XON/XOFF output flow control (`IXON`), uses eight-bit characters, and sets `VMIN=1`, `VTIME=0`.
Output disables `OPOST`; both streams become nonblocking. Ctrl+C is consequently
an input byte, not an application stop signal. No alternate screen, mouse-reporting
or bracketed-paste VT modes are enabled by this layer. The higher-level
[native Application transport](application.md) owns its
alternate-screen lifecycle; the ordinary session `close()` contract below stays
unchanged.

All calls, moves and destruction belong to one executor. A synchronized ownership
transfer between executors is allowed; concurrent calls are not. The session
installs no process-wide signal handler and never runs UI code from a signal.
Allocation failures propagate as exceptions; native and contract failures use
owning `TerminalError` values. The native error code is `errno` on POSIX.

`close()` discards pending output, independently restores every changed mode and
flag in reverse order, and closes the owned duplicates. It reports the first
restoration/close error and is idempotent. Destruction and move assignment perform
the same best-effort cleanup without throwing; use explicit close to observe an
error. A failed `open()` also attempts every acquired restoration; its error keeps
the original native code plus the first rollback error in `restoration_code`.
Disconnected terminals may no longer permit restoration. Abrupt process death,
external descriptor mutation and signal termination do not run a promised cleanup.

## POSIX input, deadlines and resize

`read(deadline, stop)` returns one owning `TerminalRead`. On POSIX, `bytes` contains
at most `max_input_bytes` raw bytes and `events` is empty. Feed these bytes to the
separate `InputDecoder`; the decoder does not own terminal I/O. `events` provides
the bounded semantic-event result on Windows (see below).
Returned bytes, events and cell sizes survive subsequent calls, moves and close.

The deadline is an absolute `steady_clock` time point. Read/write retries after
`EINTR` recompute remaining time. Each poll lasts at most `wait_cadence` (10 ms by
default); stop is checked between native operations. This is a polling latency
bound, not a real-time scheduling guarantee. A pending stop takes precedence over
an expired deadline. EOF follows the last readable bytes, including simultaneous
`POLLIN | POLLHUP`. A PTY hangup is still read as EOF when querying its size fails.

`size()` returns columns/rows as `CellSize`. While reading, the owner samples
`TIOCGWINSZ` on each loop and reports a changed size through `resized`. Rapid changes
may coalesce; a change during an otherwise idle read is observed within the polling
cadence plus OS scheduling. Resize is not an error and does not alter earlier data.
The session neither installs nor replaces `SIGWINCH` handling. The kernel's
[window-size interface](https://man7.org/linux/man-pages/man2/TIOCGWINSZ.2const.html)
is the source of geometry.

## POSIX output and renderer state

`submit(std::string)` accepts one batch of at most `max_output_bytes` (1 MiB by
default); another submission is rejected while bytes remain pending. The session
copies the accepted bytes into compact owned storage so a caller's oversized
reserved capacity is not retained. No borrowed span or callback survives the call.
This costs a bounded copy; it avoids an unbounded internal output queue.

`flush(deadline, stop)` returns the **successful native byte count for that call**,
the remaining count, and a status. It advances by the exact positive result of each
nonblocking write. Deadline, stop and errors preserve the unwritten suffix, allowing
an explicit later flush. A zero-progress native write is an `EIO` error. An empty
batch is already complete and flush returns progress even with an expired deadline.
The caller must handle errors rather than retry indefinitely.

Completed bytes are not proof that a complete VT transaction was interpreted or
displayed. The renderer owns its belief about the visible terminal; interrupted or
failed output requires its explicit recovery/resynchronization policy. No terminal
I/O, flush, mode change or waiting belongs inside paint/measure/arrange.

The private POSIX writer also handles a closed pipe without killing its caller.
It temporarily blocks `SIGPIPE` in the calling thread, preserves a previously
pending signal, consumes a newly pending signal after `EPIPE`, then restores the
previous mask. The process-wide disposition is unchanged. Public session creation
still rejects redirected pipes; the lower-level writer is tested independently.
The implementation follows the native [write](https://man7.org/linux/man-pages/man2/write.2.html),
[poll](https://man7.org/linux/man-pages/man2/poll.2.html),
[thread signal mask](https://man7.org/linux/man-pages/man3/pthread_sigmask.3.html) and
[signal wait](https://man7.org/linux/man-pages/man2/sigtimedwait.2.html) contracts.

## Verification

`portals.terminal.posix` uses its own child process, PTYs and nonblocking pipes.
It checks exact mode/flag restoration, caught exceptions, move ownership, bounded
input/output, resize, deadline/stop, partial writes, input EOF and closed-pipe
survival with default SIGPIPE disposition. A narrow private syscall seam supplies
deterministic short writes, EINTR and elapsed time. Linux link wrappers inject
termios acquisition/restoration failures without adding public injection APIs.
The child has a ten-second watchdog and CTest adds a fifteen-second outer timeout;
no product UI or user terminal is opened or modified.

## Windows native console contract

Pass borrowed input/output `HANDLE` values converted to `std::intptr_t`. Both
must pass `GetConsoleMode`; redirected input or output pipes are rejected before
mode mutation or waits. The session duplicates the handles without inheritance.
The originals remain open. Exclusive access includes all aliases, console modes,
and the console-wide output code page. No console handler is installed.

The backend saves both modes and the output code page before mutation. Input
sets `ENABLE_EXTENDED_FLAGS`, `ENABLE_WINDOW_INPUT`, `ENABLE_MOUSE_INPUT`, and
`ENABLE_VIRTUAL_TERMINAL_INPUT`; it clears processed, line, echo, and Quick Edit
modes. Ctrl+C is input, not a process signal. Output enables processed VT output
and uses CP_UTF8; the input code page is unchanged because input uses Unicode
records. Rollback, close, and exception unwinding attempt all changed state
independently. Explicit close reports the first native error (`GetLastError`).

`ReadConsoleInputW` consumes only an observed available record under exclusive
ownership. `WaitForSingleObject` waits at most `wait_cadence`; ignored records and
repeat expansion also have a work budget. A progress result may therefore contain
no events: the caller should yield to other work before reading again. Size uses
`srWindow` (viewport), sampled while reading, including after non-text records;
resize does not require a text key and may coalesce. Mouse positions subtract the
viewport origin. Console queries themselves depend on a responsive console host.

Windows returns **events only**, never a second byte channel. Unicode key-down
records and nonzero Unicode characters carried by `VK_MENU` (Alt) release
records are converted from UTF-16 to UTF-8 and passed through the existing
`InputDecoder`. This preserves VT arrows, Alt sequences, and bracketed paste
controls as paste payload. Configure decoder bounds/deadlines through
`TerminalOptions::windows_input`; no second escape parser exists. This layer does
not enable bracketed-paste terminal reporting. Invalid surrogate units become
U+FFFD; a high surrogate is retained until its following character. Repeat counts
are expanded incrementally within the event budget; paired surrogate records use
the low-surrogate record's repeat count. Other KEYUP records are ignored because
VT synthetic records do not reliably preserve physical key transitions. The
Alt-release exception preserves text entered through Alt+numpad and ConPTY,
including combining marks and surrogate pairs; it does not expose an Alt action.
Zero-character native
navigation/F1–F12 records report press/repeat; modifier-only/unknown keys are
ignored. Ctrl+Space and Ctrl+2 NUL records map through the decoder's Ctrl+@
representation. Native modifiers augment decoded modifiers; printable Right Alt
with Left Ctrl is treated as AltGr text rather than a command chord.

Native mouse records support left, middle, right, move, and wheel directions.
Multiple button changes emit one event each, in left/middle/right order, using
fixed storage for at most three transitions. Other buttons are ignored; a
Windows double-click flag adds no separate click-count event. Wheel reports emit
one direction event, as the public event type has no magnitude. Completed
semantic events retain source order; an atomic paste appears only at its end
delimiter, so native mouse events occurring during that paste can appear before
the completed paste. Resize is a separate coalesced state observation.

Windows may normalize records before the adapter sees them. In the tested ConPTY,
`WriteConsoleInputW` under VT mode translated VK_UP to ESC-[A, collapsed a supplied
repeat count, and represented Ctrl+Space as Ctrl+Shift+2/NUL. The adapter cannot
recover metadata the OS has removed. Native record tests temporarily disable VT
only during injection, restoring it before the session reads; separate host-pipe
UTF-8/Ctrl+C/resize tests keep the actual VT path throughout and verify combining
marks and ZWJ emoji without a duplicate text event from ordinary key releases.

## Windows output cancellation and shutdown

A session owns one bounded byte batch and at most one writer thread. `submit`
performs no I/O. `flush` starts a synchronous `WriteFile` on that worker and waits
on its thread handle in bounded intervals. Only successful positive completed
writes advance the confirmed offset. On deadline/stop after issuing a write,
`CancelSynchronousIo` is requested and flush returns immediately with its status
plus `error.kind == OUTPUT_UNCERTAIN`. The buffer and worker remain owned even
when cancellation has not completed. The renderer must invalidate its belief
immediately; zero confirmed bytes does not mean zero terminal side effects.

An uncertain session rejects every later submit/flush until close; it never
replays a possibly partially interpreted VT suffix. A deadline/stop checked
before any write was issued does not poison the session. Native write failures
also make the session uncertain; the first failure retains its native error.
`pendingBytes()` then counts unconfirmed owned bytes, not a retryable suffix.

`close` requests cancellation repeatedly to cover the race before entry into
`WriteFile`, waits for actual thread completion, closes the thread handle, and
then restores all saved state. No worker is detached and no caller storage is
borrowed. **Close and destruction require eventual console-host progress.** In
the finite paused-host fixture, flush returns around its 500 ms deadline, while
mode restoration waits until the host resumes output drain around 1500 ms.
These observations qualify that scenario; they are not hard real-time deadlines.
An indefinitely stalled external host/driver cannot be given a finite teardown
guarantee by these Windows APIs. Keep the ConPTY host's output drain alive during
shutdown; a library cannot repair a host that never services its pipes.

See Microsoft's [console modes](https://learn.microsoft.com/en-us/windows/console/setconsolemode),
[low-level input](https://learn.microsoft.com/en-us/windows/console/low-level-console-input-functions),
[WriteFile](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-writefile),
[CancelSynchronousIo](https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-cancelsynchronousio),
and [ConPTY shutdown guidance](https://learn.microsoft.com/en-us/windows/console/creating-a-pseudoconsole-session).

`portals.terminal.windows` creates only its own headless ConPTY children in a
kill-on-close job. A separate pipe reports assertions, while a bounded polling
drain consumes output. It checks actual public-session input, output, stop,
deadline, finite pause/resume, restoration, and child completion. Its watchdog
terminates only its own child; CTest provides an outer timeout. These fixtures do
not qualify an interactive Windows Terminal keyboard/mouse session by themselves.

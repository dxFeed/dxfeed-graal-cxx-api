# Diff encoding and frame output

`<portals/diff.hpp>` separates pure framebuffer comparison from terminal I/O.
`encodeDiff(previous, next, max_bytes)` returns one owning bounded VT byte string.
`FrameWriter::submit` takes a move-only desired frame only after successful
encoding. It makes no transport calls, so it can run after paint; `flush` performs
I/O separately on the terminal owner. The full-frame renderer remains independent.

The first frame, a changed size, or a changed complete width policy causes a clear
and complete repaint. Otherwise only changed rows are sent. A row is rewritten in
full, including spaces, so replacement of wide glyphs and their old continuations
cannot leave stale tails. Equality compares display bytes, styles, widths and
continuation roles, not buffer-local IDs. Explicit default spaces equal blank
cells. An unchanged frame produces an empty string and no terminal calls.

This implementation optimizes rows, not individual runs or minimum byte count.
It performs two bounded scans: the first counts the exact complete output and
checks the byte limit before allocating; the second fills that allocation.
Resource use is bounded by the supplied buffers and byte cap. No performance
claim or hardware rendering acknowledgement follows from successful transmission.

## Transaction ownership

The application owns a stable `TerminalSession`, then a `TerminalFrameOutput`
adapter borrowing it, then a `FrameWriter` borrowing the adapter. Destroy them in
reverse order and close the session explicitly when errors need reporting. One
executor owns all operations. There are no callbacks, worker threads or global
output sinks in the writer. Native transport workers, if any, remain owned by the
session. No direct stdout/stderr output occurs in this component.

`FrameWriter` holds at most one committed and one pending `CellBuffer`. Each is
checked against `max_frame_storage_bytes` using its allocated storage accounting;
the encoded transaction is capped by `max_output_bytes`. The native session has
its own output cap, which must accommodate the writer's transactions. Buffers and
transport bytes are owning values; caller views expire after an accepted move.
The encoder borrows only for the duration of the call. No view survives mutation.

A second submission while pending is rejected without consuming its argument.
There is no queued backlog or silent frame coalescing. `committedFrame()` borrows
the last fully transmitted frame. During partial output it is historical, **not**
a description of the physical screen. `pending()` distinguishes this state.
After complete, consistent transport progress, the pending frame becomes the
committed one. Deadlines and stops preserve the exact suffix; retries continue
that suffix without restarting a UTF-8 scalar or escape sequence. Deadline/stop
must come from transports that preserve known progress; uncertain cancellation
must attach an error. The frame writer normalizes such results to error status
while preserving the native error payload.

Any transport error, rejected transport submission, inconsistent progress, or
exception poisons the writer, discards belief and prohibits further transport
calls. Exceptions propagate after poisoning. Unknown side effects invalidate
belief even when the transport reports zero completed bytes. The native error
remains in `TerminalWrite`; accounting violations use a system error with native
code zero. A call on an already poisoned writer returns a closed error.

## Terminal modes and recovery

The application owns the entire terminal viewport; framebuffer dimensions must
match it, and no other writer may interleave bytes or mutate modes. Required
capabilities are UTF-8, matching grapheme widths, CUP, SGR including RGB, DEC origin
and autowrap modes, scrolling-region reset and replacement mode. These are a
capability contract, not features authenticated by the diagnostic probe.

Every nonempty transaction establishes SGR defaults, absolute origin, the full
scrolling region, replacement mode and disabled autowrap. It positions the cursor
at the start of each changed row and sends whole display graphemes. Autowrap stays
off through the lower-right cell. The suffix restores default SGR, moves home and
enables autowrap. The application must restore its chosen pre-UI modes on exit;
`TerminalSession` restores native console/termios modes, not these VT modes.
Resize must be serialized with frame scheduling: finish the pending transaction
or close the session before a resized full frame can replace it. External terminal
resize during a write can invalidate physical interpretation and requires a full
repaint after the owner observes the new size.

A full transaction begins with CAN (`0x18`), followed by the controlled mode
baseline and clear/full repaint. Recovery after an unknown error is:

1. Stop using the poisoned writer and close the old terminal session, joining any
   owned native writer until it is quiescent. Cancellation requests alone do not
   establish this boundary; closing can wait for a blocked host to resume.
2. Open a new session and construct a new adapter and writer. Never reuse the old
   unfinished byte batch.
3. Submit the desired full frame and flush to completion. CAN aborts a pending
   prefix of this encoder's CSI/UTF-8 grammar on supported terminals; clear/full
   repaint replaces any already interpreted prefix.

This recovery supports only output produced by this encoder, not arbitrary OSC,
DCS, downloaded character sets, or external control streams. If that grammar or
CAN behavior is not guaranteed, the owner must establish a fresh/reset terminal
parser before constructing the replacement writer. Reopening handles by itself
does not reset a parser. Native cancellation and recovery qualification is distinct
from headless proof, and a successful flush means accepted bytes, not display ACK.

## Verification

Tests use a small independently written VT interpreter. It decodes CSI, UTF-8,
styles, combining accents, wide glyphs, delayed wrap and scrolling without encoder
or full-renderer helpers. Its Unicode fixture alphabet is intentionally limited;
it is not a general terminal emulator. Generated frame transitions and a target
produced by the independent full renderer must reproduce cells/styles/widths.
Every byte split of a styled Unicode delta tests exact suffix continuation and
historical belief; every error split tests poison and a new full CAN transaction
on the same interpreter state, without resetting the simulated parser.

The POSIX test uses a real PTY with pressure, stop, drain/resume, exact byte replay,
mode restoration and hangup errors. The native Windows ConPTY fixture also drives
the public `FrameWriter` / `TerminalFrameOutput` / `TerminalSession` chain. Its
240-by-160-cell frame cases cap each transaction at 2 MiB and each framebuffer at
1 MiB, and allow at most sixteen alternating frames to establish pressure. A large
first frame alone is insufficient evidence: ConPTY can accept the viewport before
its renderer blocks on the paused host pipe.

The fixture checks stop/expiry before native I/O, successful commit after drain
resumes, unchanged-frame silence, and uncertain deadline/stop after native I/O.
Uncertainty must discard an already committed belief and reject retries. Only
after close/join and exact native mode/code-page restoration does a new session
and writer perform the full CAN repaint. Native console cell samples check ASCII,
BMP text, CJK lead/tail at the lower-right, cleared stale cells and the final cursor.
The existing host watchdog, hidden console and owned drain/process cleanup cover
these cases too. Samples are complementary to the full headless semantic oracle;
they do not establish exact RGB or supplementary-glyph identity via `CHAR_INFO`.

A PTY plus an interpreter and an OS ConPTY console are distinct from actual
frontend emulators. Real-emulator qualification must be recorded separately; the
abstract transport tests do not establish it.

Primary references: [VT control functions (XTerm)](https://invisible-island.net/xterm/ctlseqs/ctlseqs.html),
[Windows virtual terminal sequences](https://learn.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences),
and [Windows Terminal parser](https://github.com/microsoft/terminal/blob/main/src/terminal/parser/stateMachine.cpp).

# Streaming VT input

`<portals/input.hpp>` supplies a synchronous, bounded byte decoder. It has no
terminal handles, background thread, timer, event loop, environment lookup or
dependency on the diagnostic probe. The caller supplies arrival timestamps and
an explicit input profile. Enabling a decoder option does not enable a terminal
mode; a future terminal session owner must do that separately.

## Pulling events

`InputDecoder::create(options)` validates settings and returns an owning decoder
or `InputError::INVALID_OPTIONS`. One executor owns its mutable state; different
instances can run concurrently. Do not call one instance concurrently. A moved
source must be reset before reuse. Returned events own their values and survive
decoder reset, move and destruction.

`next(bytes, now)` borrows bytes for that call and returns at most one event plus
an exact consumed prefix. The caller advances by `consumed`, handles the event,
and resubmits the suffix with its original arrival timestamp. A result can have
an event and consume zero bytes: timeout, pending issue, double ESC and malformed
UTF-8 recovery need this. `NEED_INPUT` means the supplied view is exhausted;
`WORK_LIMIT` means another pull is required even if no event was produced.
There is no growing output queue. The caller must bound any queue it creates.

Empty `next` calls advance time. Drain events before waiting; after `NEED_INPUT`,
wait for bytes or `deadline()`. `finish(now)` starts EOF and drains at most one
event per call. Repeat until `finished` or `failed`; a `finished` result may still
carry the last event. Further `finish` calls are empty and idempotent, while
`next` returns `CLOSED`. No input is accepted after EOF. `reset()` abandons the
old session, frees retained paste and clears the clock, byte offsets and probe
window, retaining options. It is a caller-declared boundary, not a safe way to
continue an unclosed paste's untrusted suffix.

Call errors reject the entire supplied view. Backward timestamps are rejected
before state changes. Offset overflow is checked for the maximum consumable
prefix before a call. Allocation failures propagate and leave the decoder failed;
after an exception the caller must abandon that session, because a consumed
count cannot be returned. No partially collected paste event escapes.

## Events and the supported profile

Text events carry a single validated Unicode scalar, not a grapheme cluster.
Combining marks, ZWJ and regional indicators can arrive as separate events.
The [text layer](text.md) groups them after insertion. Alt is represented as an
observed modifier on text, not an invented physical scan code. No normalization,
replacement character insertion, display-width calculation or text filtering
occurs in this decoder. UTF-8-encoded control scalars remain data.
ESC followed by `[`, `O`, `]`, `P`, `X`, `^` or `_` is reserved for the
corresponding sequence/string introducer, so these prefixes do not produce
Alt text. This is another unavoidable convention of the selected legacy profile.

Key values include modifiers and `press/repeat/release` for future native
backends, but this VT profile emits only `press`. It cannot establish physical
key state. Mouse events preserve button, motion/release/wheel and modifiers;
their coordinates are checked zero-based cells without viewport clipping.

The exact supported keyboard table is:

| Encoding | Event |
|---|---|
| ASCII 0x20–0x7e, valid UTF-8 | Text scalar |
| ESC followed by one scalar before deadline | Text scalar with Alt |
| Tab (0x09), CR (0x0d), BS/DEL (0x08/0x7f) | Tab, Enter, Backspace |
| Remaining C0 except ESC | Ctrl plus scalar byte+0x40 (NUL is Ctrl+@; LF is Ctrl+J) |
| ESC followed by C0/DEL | Corresponding key plus Alt |
| Lone ESC; consecutive ESC | Escape after deadline/EOF; first Escape immediately |
| CSI/SS3 A B C D H F | Up, Down, Right, Left, Home, End |
| CSI/SS3 P Q R S | F1–F4 |
| CSI n~ with n=1/7, 2, 3, 4/8, 5, 6 | Home, Insert, Delete, End, PageUp, PageDown |
| CSI n~ with n=11,12,13,14,15,17,18,19,20,21,23,24 | F1–F12 |
| CSI Z | Shift+Tab |

Letter-final CSI/SS3 keys accept no parameters, `1`, or `1;m`. Tilde keys
accept `n` or `n;m`. Modifier `m` is 1–8: subtract one, then bits 1/2/4 mean
Shift/Alt/Control. Missing or empty numeric fields are rejected. This is a small
chosen profile, not the entire xterm keyboard grammar.

With `sgr_mouse=true`, `CSI <b;x;y M/m` decodes SGR cell mouse reports. Coordinates
must be 1..2147483648, then subtract one into signed 32-bit fields. Button bits
0..1 identify left/middle/right/no-button; bits 4/8/16 are modifiers, 32 is
motion, 64 is wheel. Wheel directions 64..67 are up/down/left/right. The decoder
rejects unsupported bits, wheel+motion, wheel release, motion release and
no-button transitions other than motion. Extended buttons and pixel coordinates
are outside this profile. Raw 8-bit CSI, X10/1005/1015, kitty keyboard and OSC/DCS
replies are not supported.

## Time and ambiguity

All times are caller-supplied unsigned 64-bit monotonic milliseconds. Defaults
are 30 ms for ESC/Alt completion, 1000 ms for an entire CSI/SS3/control string or
ordinary UTF-8 scalar, and 30000 ms for paste from its complete begin delimiter.
These are configurable policy choices, not measured terminal guarantees. CSI,
SS3 and strings use the initial ESC timestamp; incoming bytes never extend a
deadline. Addition saturates at `UINT64_MAX`, and `now >= deadline` expires
before processing further bytes, including a newly formed saturated deadline.

An ESC followed by an incomplete Alt scalar emits Escape then a truncated issue
at timeout or EOF. The scalar's subsequent bytes are independently decoded.
Ordinary truncated UTF-8 emits an issue and returns to ground. Unclosed control
sequences, discarded sequences, strings and paste fail on timeout: automatic
recovery could expose a data suffix as keyboard commands. Only explicit reset
recovers a failed decoder.

The invariant is event equality for identical byte/timestamp schedules, EOF and
probe-window changes. Subdividing an arrival batch retains its timestamp.
Introducing new time advances or delaying bytes can legitimately change ESC
interpretation. Legacy Ctrl-I/Tab, Ctrl-M/Enter, Ctrl-H/Backspace, Ctrl-[/Escape
and ESC+character/Alt+character cannot be distinguished from their encodings.

## Probe candidates

`expectReplies` installs an explicit inclusive-start/exclusive-deadline window,
generation and one allowed response of each selected kind. It is valid only at
ground with no pending event. The parser sends no queries. Primary DA is
`CSI ?p[;p...]c`; CPR is `CSI row;column R` with positive fields. Both use checked
32-bit parameters. The initial ESC and sequence completion must lie in the
window. A matching candidate consumes that kind's expectation once. Generation
only correlates with the caller's request; it cannot authenticate the sender.

CPR overlaps modified F3. Within an expected window CPR wins; otherwise the
documented key table wins (for example `CSI 1;2R` is Shift+F3), and remaining
unsupported sequences emit issues. Typed, duplicate, stale or forged bytes are
not distinguishable from genuine terminal responses by this syntax. The
diagnostic tool's [probe limitations](terminal-probe.md) apply to interpreting
observations, but its collector is not used by this library decoder.

## Bounds and recovery

Defaults cap CSI/SS3 at 64 bytes including ESC/introducer/final, at 16 numeric
parameters and 10 decimal digits per parameter. These settings can be lowered;
fixed scratch sizes set their maximums. Arithmetic is checked independently of
digit count. A paste payload defaults to 1 MiB; zero permits only empty paste.
Each pull accepts at most 4096 bytes by default. Work is bounded by that budget
plus fixed grammar scratch; no callback can add unbounded work inside a pull.

Retained logical data is bounded by the paste cap plus at most five tentative
end-marker bytes and fixed 64-byte sequence scratch. `std::string` allocation
capacity/allocator overhead and a returned owning event are additional; space
complexity is O(paste cap), with no full-payload copy on delivery. Output events
are the caller's storage obligation. String and discarded-paste bodies are
scanned without retaining them. One pending issue is used only when an Alt tail
must produce Escape and an issue separately. A byte budget is not a total-stream
time bound; the caller must schedule time advances.

Malformed UTF-8 rejects the partial scalar; a non-continuation byte is left
unconsumed and reprocessed once at ground. Invalid leading bytes are consumed.
Malformed control grammar consumes its offending byte, emits an issue and
returns to ground; a parameter after an intermediate enters bounded discard
through the final byte. Sequence overflow emits one issue at the first excess
byte and discards through the final, or through a consumed invalid grammar byte.
Unsupported complete sequences emit an issue and are never replayed as keys.

Unknown OSC/DCS/SOS/PM/APC strings emit an issue at the introducer, then discard
through ST (`ESC \\`); OSC also accepts BEL. Their contents never enter key,
mouse or probe decoding. Timeout fails the stream. EOF emits a truncated issue
for incomplete state and closes it. Issues contain kind and a half-open session
byte range for the consumed rejected prefix, with no raw ANSI excerpt. Subsequent
discard bytes are represented by consumed counts, not an ever-growing diagnostic.

## Paste boundary

With `bracketed_paste=true`, `CSI 200~` begins paste and exact `CSI 201~` ends it.
Only the end delimiter has meaning inside paste. Nested begin delimiters, key,
mouse, DA/CPR, OSC and arbitrary invalid UTF-8/NUL remain raw bytes. Partial
end-marker prefixes are matched across every read boundary.

The first confirmed payload byte over the cap drops the whole payload and emits
one limit issue. The decoder then discards through the exact end delimiter;
there is no partial paste event and no reinterpretation of its suffix as keys.
The original paste deadline remains active while discarding. A prefix that might
still be the end marker does not count as confirmed payload until mismatched.

Literal `ESC[201~` in untrusted data is indistinguishable from a genuine end:
the protocol has no length or authentication. The regression corpus explicitly
demonstrates that a forged delimiter ends paste. This parser therefore does not
promise protection from a hostile terminal stream. Downstream insertion policy
must validate/repair text and treat paste controls as data; never forward raw
paste to a terminal writer, shell or command dispatcher.

## Verification and sources

`portals.input` contains semantic golden fixtures, fake-time tests, all nonempty
partitions of short fixtures (up to 12 bytes), every single split boundary of
long fixtures, bytewise/fixed chunks, small work budgets and deterministic seeded
malformed-stream fuzz. Long all-partition coverage is not claimed. Megabyte
discard fixtures check continued progress and recovery without large retained
input. No test requires a terminal, sleeps or interactive window. Unit tests do
not certify real emulator behavior or negotiated terminal modes.

Primary references: [xterm control sequences](https://invisible-island.net/xterm/ctlseqs/ctlseqs.html)
(Control Bytes, SGR mouse, Bracketed Paste, Device Attributes and DSR),
[Microsoft input sequences](https://learn.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences#input-sequences),
and [Unicode UTF-8 definition, section 3.9](https://www.unicode.org/versions/latest/core-spec/chapter-3/#G7404).

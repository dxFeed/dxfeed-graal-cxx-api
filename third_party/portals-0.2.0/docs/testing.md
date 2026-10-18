# Headless terminal testing

`tests/support/test_terminal.hpp` is private test support, not an installed Portals
API. `TestTerminal` implements `FrameOutput`, owns an independent VT oracle and
input decoder, and borrows a `FakeClock`. One executor owns all calls. The clock
must outlive the terminal. There are no callbacks, background workers, implicit
flushes, real terminal handles or wall-clock sleeps. Input events and snapshots
own their data and remain valid after subsequent calls or terminal destruction.
Invalid test operations throw `std::runtime_error`; allocation errors propagate.
Discard the terminal/writer pair after an injected output failure or oracle error.

`FakeClock::advanceTo()` changes monotonic milliseconds without dispatching
anything. Call `TestTerminal::tick()` to deliver input timeouts explicitly, or
`input()` to deliver bytes at the current time. The next scheduler can reuse this
clock without acquiring a scheduler from this helper. `finish()` explicitly ends
input; ending a trace does not silently manufacture EOF. The clock rejects
backward time and values that cannot be represented by `TerminalDeadline`.

`submit()` takes ownership without changing the screen. `flush()` honors a
pre-requested stop or expired fake deadline without consuming output. An injected
`failAfter(n)` accepts at most `n` pending bytes and then reports an error, even
when that prefix ends inside CSI or UTF-8. This models a failed transport, not a
safe retry. The remaining suffix is retained and further output is rejected.
`FrameWriter` tests verify that this failure poisons the writer and invalidates its
committed frame. Successful flush means accepted bytes, not a hardware guarantee.

## Version 1 trace format

A trace is ASCII text with the exact header `PORTALS-TRACE 1`. Each following
nonempty line has an absolute unsigned millisecond timestamp and one operation:

| Record | Payload and meaning |
| --- | --- |
| `time INPUT hex` | Input byte fragment at this time; lowercase hex, `-` for empty. |
| `time RESIZE width height` | Resize the independent viewport and deliver a resize event. |
| `time OUTPUT hex` | Submit and flush one owning output transaction. |
| `time FAIL_AFTER count` | Inject failure into the next flush after this accepted prefix. |
| `time TICK` | Explicitly drain input timers at this fake time. |
| `time FINISH` | Explicit input EOF. |
| `time SNAPSHOT` | Capture the complete current output state. |

Equal timestamps preserve record order. LF and CRLF input are accepted; recording
emits LF. Blank records, unknown operations, extra fields, malformed hex, numeric
overflow and unsupported/missing versions are errors. Parse diagnostics identify
the offending line; aggregate resource failures identify the limit.
`parseTrace()` owns the decoded records; `recordTrace()` creates canonical text.
The caller owns file I/O and must bound reads before passing a string to the
parser (the fixture reader demonstrates this). This is a local testing format,
not a network protocol or a general serializer.

`replay(trace, input_chunk, output_chunk)` splits input and interpreter feeds at
positive byte sizes while preserving timestamps and transaction boundaries.
It returns owning input/resize events with delivery times, explicit snapshots
plus one final snapshot, and output results. An output failure is an observable
result; submitting further output to that failed transport is a test error.
An empty `INPUT` drains input just like `TICK`. Resize may occur between input
fragments without discarding the decoder's partial sequence.

## Independent snapshots and bounds

The oracle does not call the diff encoder, full renderer, grapheme decoder or
Unicode width helpers. Its deliberately small alphabet covers printable ASCII,
combining acute U+0301, CJK U+4E00–U+9FFF and fixture emoji U+1F300–U+1FAFF.
The CJK/emoji ranges have fixed width two; other scalars are rejected. This does
not model general Unicode grapheme composition, ZWJ, variation selectors or a
complete terminal. The supported CSI subset is cursor positioning, erase-display,
encoder SGR (default, bold, italic, underline and RGB), autowrap and the encoder's
fixed origin/insert/margin baseline. CAN cancels partial CSI/UTF-8.

Snapshots retain viewport size, every cell's glyph/style/width/continuation role,
half-open spans for each lead, cursor coordinates, current style, autowrap and
pending wrap. They are semantic screen checkpoints, not parser save states:
partial CSI/UTF-8 cannot be restored from them. Resize preserves the intersection,
clears a wide glyph cut by the new right edge and homes the cursor. Native emulator
resize policies remain separate platform tests.

The private format has named fixed bounds in `TraceLimits` and `VtOracle`:

- Encoded file, aggregate decoded payload and pending output: 256 KiB each.
- Operations: 4,096; retained input/resize events: 65,536.
- Snapshots: 64 including the implicit final checkpoint.
- Viewport: 4,096 cells total and at most 4,096 per dimension; product checked before allocation.
- Glyph bytes: 256 per cell; CSI parameter bytes: 64; numeric accumulation is checked.

The reusable oracle permits up to 16,384 cells for existing native writer
fixtures; `TestTerminal` and trace validation apply the tighter replay bound.

These caps bound retained snapshot glyph data to 64 MiB plus cell/span metadata,
input events and trace storage. Input pulls also have a progress budget; each
fragment performs at most twice its byte count plus four pulls. Oracle work is
linear in fed bytes plus bounded screen operations; each checkpoint copies at
most the bounded viewport. The input decoder's own sequence/paste limits still
apply. This is intentionally a bounded fixture harness, not an unbounded session
recorder. Changing these format/resource guarantees requires updating the tests
and this document.

## Regressions

`tests/replay.cpp` runs `resize-input.trace`, `wide-edge.trace` and
`writer-failure.trace`, checks hand-authored semantic expectations, and compares
512 input/output chunk combinations per fixture. Recording roundtrips preserve
the operations and replay evidence. Additional checks cover fake ESC deadlines,
stop/deadline output retention, snapshot ownership, real `FrameWriter` poisoning,
malformed records, unsupported versions and resource/arithmetic boundaries.
Existing input/diff/writer tests remain separate characterization coverage.

```powershell
cmake --preset windows-msvc-debug
cmake --build --preset windows-msvc-debug
ctest --preset windows-msvc-debug -R 'portals.(replay|input|diff|writer)$' --output-on-failure
```

Use the corresponding Linux preset on Linux. Windows test targets include
`tests/support/no_dialogs.cpp`; configure and rebuild before running changed test
binaries. Tests terminate themselves without terminal windows or dialog dismissal.
Doxygen includes this page when `PORTALS_BUILD_DOCS=ON`.

## C++ naming check

Project functions and methods use `lowCamelCase`; enumerators and named
`constexpr`, class, global and static constants use `BIG_SNAKE_CASE`.
Ordinary local `const` values and parameters retain their existing variable style.
Standard-library operations, external overrides and externally required linker
entry points retain their contract names. Any necessary declaration suppression
must explain that contract locally.

The repository `.clang-tidy` checks these rules. Configure a compilation database
with tests, tools and examples enabled, then run the Python 3.9+ checker:

```powershell
cmake --preset windows-msvc-debug
python tools/check_naming.py build/windows-msvc-debug --clang-tidy clang-tidy
```

On Linux, use `linux-gcc-debug` and `build/linux-gcc-debug`. Use a clang-tidy version
that can parse the selected C++23 standard library; Clang 20 is checked with the
Linux configuration. Run each platform's database to cover conditional native
code. The script checks project translation units and their project headers,
excludes vendored sources, returns a nonzero status on errors, and never applies
fixes. It can also check a separately configured module experiment database.
The Linux CI naming job uses Clang/clang-tidy 20 and GCC 14's standard library;
it installs the toolchain from Ubuntu 24.04's updated package repositories.
Dependent template references and inactive preprocessor branches still require
the normal build/test matrix; a naming pass alone is not a compilation guarantee.

See the primary [clang-tidy naming documentation](https://clang.llvm.org/extra/clang-tidy/checks/readability/identifier-naming.html).

References: [CTest](https://cmake.org/cmake/help/latest/manual/ctest.1.html),
[C++ steady clock](https://eel.is/c++draft/time.clock.steady),
[C++ integer parsing](https://eel.is/c++draft/charconv.from.chars).

## Optional coverage-guided fuzzing

The opt-in Clang targets instrument the input decoder and Unicode implementation
with libFuzzer, ASan and UBSan. See [bounded fuzzing](../fuzz/README.md) for
build commands, seed corpora, checks, reproduction and limits. Ordinary builds
leave PORTALS_BUILD_FUZZERS disabled.

## Optional native xterm fixture

`tools/native_xterm.sh` runs `portals_terminal_cleanup` in an actual xterm and
records child exit, exact termios/file-flag restoration, viewport changes, driver
stages, and owned-process cleanup. Native launchers are opt-in; ordinary CTest
never opens a terminal emulator.

Both drivers use a private Xvfb display by default, so automated runs cannot
open windows over the caller's desktop. `-iconic` alone does not guarantee this
on WSLg. A `none` driver runs finite native programs and handled-error probes
without synthesizing input. Both drivers need xterm, Xvfb, xauth and xkbcomp
(used by Xvfb). A `dashboard` driver additionally needs the X11/XTest development
libraries at configure time. CMake enables this
fixture's input driver only when both `X11::X11` and `X11::Xtst` are available;
these are test dependencies, not dependencies of the Portals core. Without them,
the supervisor still builds and `none` remains usable; `dashboard` fails before
starting the application or sending input.

```sh
sh tools/native_xterm.sh /usr/bin/xterm build/linux-gcc-debug/portals_terminal_cleanup \
  /tmp/portals-native-dashboard 12000 0 dashboard 5000 \
  build/linux-gcc-debug/examples/dashboard/portals_dashboard --rate 0 --duration-ms 10000 \
  --trace /tmp/portals-native-dashboard.trace
```

The log directory must not exist. By default, the launcher creates its own free
nonzero Xvfb display with a private MIT-MAGIC-COOKIE-1 authority file, disables TCP
listeners, and waits for that server's readiness. It ignores inherited isolation
markers and replaces them with the created display and window title. The driver
requires exact matching markers, a nonzero local display, private authority, the
owned xterm window title and the XTest extension before mapping, focusing or
sending input. It never drives display `:0` or the caller's desktop. Unicode F8
translation belongs only to this xterm instance; no desktop keymap or clipboard
is modified. `PORTALS_XVFB` may name an alternative Xvfb executable, and
`PORTALS_XVFB_WORKDIR` its required working directory for a local test installation;
both default to normal system behavior.

For deliberate interactive inspection only, `PORTALS_NATIVE_VISIBLE=1` lets a
`none` driver use the caller's `DISPLAY` and authority instead of Xvfb. This
explicit choice may display a window over the desktop; it is rejected for
`dashboard` so synthetic input remains isolated. The default is `0`; any other
value is rejected. `launch.log` records the choice and actual display. Private
Xvfb timings describe an offscreen X11 server and must not be labelled as WSLg
desktop timings.

XTest provides server-side keyboard, pointer motion and button press/release.
The fixture does not enable `allowSendEvents` or relax `allowMouseOps`: xterm's
`allowSendEvents` disables `allowXXXOps` and would suppress mouse reporting even
when the application correctly enables modes 1000/1006. A green driver stage
proves request submission; confirm actual mouse delivery in the dashboard trace
(two `event=input ... input=mouse` records) and inspect the exported selection,
Unicode note, theme and resize data for the complete acceptance gate.

The launcher stops only its own terminal/server and supervised child PIDs,
removes only its private authority/readiness files, and records cleanup. A child
watchdog, forced terminal termination, failed driver stage or remaining process
fails the run; cleanup does not convert a deadline into a pass.

Primary references: [XTest](https://xorg.freedesktop.org/archive/X11R7.6/doc/libXtst/xtestlib.html),
[xterm security resources](https://invisible-island.net/xterm/manpage/xterm.html),
[CMake FindX11](https://cmake.org/cmake/help/latest/module/FindX11.html).

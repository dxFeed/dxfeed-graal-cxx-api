# Limitations {#limitations}

Portals is an evolving C++23 terminal UI library. Its public headers document
checked access and component errors; neither version 0.2 nor CMake version
selection promises a stable API/ABI.

## Platforms, toolchains and delivery

Native terminal implementations target Windows and POSIX/Linux. This MVP does
not establish macOS support. Use a C++23 compiler with a compatible standard
library providing `<expected>` and `<format>`, plus a C99 compiler for
utf8proc. A compiler's language flag alone does not prove library support.
The supplied presets select MSVC on Windows and GCC on Linux; Clang requires
a compatible chosen standard library and a separately configured build.

Installed static archives require matching architecture, compiler, standard
library and runtime choices. Release does not imply static MSVC runtime linkage.
Headers/static archives are delivered; portable module BMI files are not.
See [package](package.md) for relocation and configuration rules. Source build
commands are available in [getting started](getting-started.md); runnable archive
assembly and validation remain a separate delivery step.

Native interactive execution needs terminal input and output. Redirected streams
are not a replacement interactive backend. The finite example `--check` modes
use an in-memory transport. A passing replay proves neither native restoration
nor the appearance of a real terminal. Width policy is explicit, and terminal
fonts/Unicode support can still disagree with it; see [text](text.md).
ANSI16 and ASCII examples are explicit fallbacks, not terminal authentication.

## Service and ownership boundaries

One creating UI thread owns Application, tree, router and callbacks. Models,
bindings and providers belong to the scene/caller; destroy them before Application.
Worker publication uses bounded scheduler endpoints and immutable payloads,
not direct widget access. Cancellation blocks future callbacks but cannot unwind
an active one or join arbitrary producers. The scene's `stop` hook must cancel
and join its own producers.

Queue capacities bound retained owners, not arbitrary payload size or callback
execution time. A slow user callback can delay the UI. The dashboard aggregates
every generated market tick before coalescing display snapshots; its provider
runs on the UI owner, not on an independent worker. Configured tick rate is a
schedule, not measured throughput. Delayed virtual ranges and logical row counts
do not materialize every row.

The runner uses bounded terminal reads for waiting; it has no universal OS wait
that combines terminal and scheduler wakeups. Input cadence bounds worker service
delay. Frames require invalidation, respect the configured cap and do not catch
up in bursts. The application and each accepted run are single-use.

## EOF, output and shutdown

EOF finishes decoding, delivers admitted CONTROL and flushes already submitted
output. It does not wait for future frame deadlines, timers or DATA. A delivered
edit can therefore remain dirty without a final visible frame. Finite replay
must leave enough time before EOF when it asserts a rendered result. Cancellation
can stop earlier and need not deliver remaining CONTROL.

Pending output retains its own storage. Retryable partial output may continue,
but any transport error ends the run. Windows `OUTPUT_UNCERTAIN` is explicit:
the runner never guesses committed cells, reopens the transport or replays
uncertain bytes. Ordinary exit and caught failures attempt restoration, with a
separate restoration error retained alongside the first failure. A stalled native
host/driver can extend close; there is no hard native shutdown deadline.
Abrupt process termination is outside orderly cleanup guarantees.

Successful transport acceptance is not proof of physical display. Diagnostics
records bounded metadata, excludes input/text/VT payloads and reports overwritten
or contended records. Missing evidence cannot prove that an action did not happen.
See [Application](application.md), [terminal](terminal.md), [diff](diff.md) and
[diagnostics](diagnostics.md) for the precise contracts.

## Work after the MVP

Coroutines, general Undo/Redo, system clipboard adapters and Drag-and-Drop remain
future work. LineEdit has an injected clipboard boundary; that is not a system
clipboard implementation. The module/shared-library spike is an experiment,
not the installed core's distribution contract. No second event loop or implicit
provider thread is supplied by these planned extensions.

# MVP measurements

`portals_mvp_benchmark` runs the production `Application::run` and dashboard once
with real steady time. It does not implement another event loop. Build with
`PORTALS_BUILD_TOOLS=ON`; tests additionally use `PORTALS_BUILD_TESTS=ON`.

```powershell
.\portals_mvp_benchmark.exe --transport headless --rate 1000 --seed 42 --rows 1000000 --fps 30 --warmup-ms 5000 --duration-ms 30000 --diagnostics disabled --output run.json
.\portals_mvp_benchmark.exe --transport native --rate 10000 --warmup-ms 0 --duration-ms 5000 --delay-ms 250 --ascii --ansi16 --output acceptance.json
.\portals_mvp_benchmark.exe --encoding-only --output encoding.json
```

Linux uses the same arguments with `./portals_mvp_benchmark`. The caller must
provide an existing output directory. Run primary profiles in Release, at a
stable native viewport of 120 by 40 cells, with rates 0/100/1000/10000 and three
separate runs per rate. Headless results describe only the in-process transport.
Native sessions use actual standard terminal handles and close before report I/O.
The benchmark does not inject keyboard/mouse events or resize a terminal.

`--duration-ms` is the observation duration; warmup is additional. A scheduler
timer records the actual start and a timer at the rounded-up real end deadline
requests orderly stop. Neither timer invalidates the tree. Thus idle observation
contains no periodic repaint. In the primary profile the initial full frame must
have completed during warmup. `--warmup-ms 0` is useful for interactive acceptance
and may not have an initial completed frame when observation begins.

Exit 0 means a successful complete observation with valid CPU, transport byte
accounting and OHLC. Exit 2 means successful application shutdown with an incomplete
observation (for example F10/EOF before its deadline); `application_ok` remains
true and `exit_reason` identifies the cutoff. Exit 1 means an error. An external
native acceptance driver must distinguish orderly early exit from a performance
baseline. Resized runs record their final viewport and have
`fixed_viewport_baseline=false`; they cannot prove a fixed-size performance profile.
EOF never requests a forced final repaint or bypasses the frame cap.

The schema-1 JSON retains cumulative begin/end scheduler, provider and output
counters. The window excludes warmup. Process CPU uses kernel plus user time from
[GetProcessTimes](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getprocesstimes)
or [CLOCK_PROCESS_CPUTIME_ID](https://man7.org/linux/man-pages/man3/clock_gettime.3.html).
CPU percent is relative to **one** CPU and may exceed 100; terminal-host CPU is
outside this process scope. CPU-read OS errors are explicit. Configured tick rate
is a target; actual throughput derives from generated ticks and real wall duration.
Scheduled, generated and backlog counters identify delayed bounded provider work.
The provider has a 512-tick catchup bound per callback. OHLC is independently
reduced again from every actual deterministic index/seed tick after run.

Samples keep the first `--sample-capacity` values per named series (default 65536).
They are reserved before run and append without allocation. Sampling uses phase
completion within the observation; STOP and CLOSE are retained separately after
it. Percentiles use nearest rank: `ceil(n*p/100)-1` in sorted order. Empty series
and any series with overflow have null p50/p95/max, not invented zeroes. Raw values
and overflow counts are exported so complete percentiles can be reproduced.
Sorting, formatting, CPU normalization and OHLC verification occur after close.

PREPARE measures scene preparation; RENDER measures combined layout, paint and
full cell-frame construction. SUBMIT includes writer admission/ownership and diff
encoding; it is not labeled pure encode time. FLUSH includes transport service.
The existing observer does not emit CREATE/OPEN/INPUT/DISPATCH phase durations;
these phase series remain N/A. `transport_read_ns` independently measures the
actual transport read, including waits; native opening is `native_open_ns`.
CONTROL latency samples come only from the runner input/resize callback, expressed
in ns from its logical millisecond clock. DATA latency samples come only from the
dashboard display-ingress callback and are expressed in ms. Private model channels
are represented by scheduler aggregate counters, not by invented p95 samples.

`high_water_lifetime` and `max_latency_lifetime_ms` include warmup and are never
subtracted. Saturating scheduler counters must not be used to derive a delta or
mean once they equal UINT64_MAX. `encoded_bytes` counts only accepted nonempty
transport submissions; `confirmed_bytes` counts each flush's completed bytes once.
Completed transport frames require a successful consistent flush with no pending
suffix, and do not prove physical presentation. `accepted_frames` includes unchanged
writer frames. Confirmed byte and frame boundary snapshots permit separate rate
and per-frame calculations without losing partial-frame information.

STOP includes scene provider stop; the provider is a synchronous timer, with no
worker to join. `provider_synchronous_timer_stop_ns` measures its cancellation.
CLOSE is explicitly join plus restore combined. Pure OS restore time is null,
since no independent backend measurement point exists. Diagnostic ring capacity,
retained records, overflow, contention, saturation and incomplete evidence are
reported separately. Diagnostic metadata is not a timing source. Compare enabled
and disabled diagnostic runs with identical workload/profile and separate output.

`acceptance` includes theme, note text, stable row selection, first row, viewport,
model range state, coherent revision, bounded prepared rows, pending ranges and
lossless OHLC. `range_state` uses the public `RangeState` ordinal (0=PENDING,
1=READY, 2=FAILED). Inputs accepted near EOF need not become a final visible frame.

`--encoding-only` performs a separate finite dashboard replay through the same
Application entry point, saving at most 16 complete VT transactions. An independent
VT oracle reconstructs owning cell frames and text scenes. Full and diff encodings
are verified against each captured semantic screen **before** timing, and unchanged
diff must be empty. Twenty passes measure pure `encodeDiff(nullptr, frame)` and
`encodeDiff(previous, frame)` on exactly the same frames. Saved cell scenes are
rendered separately; that metric is labeled `render_saved_cell_scene_ns` and does
not substitute for live dashboard layout/paint. It uses an ASCII fixture alphabet
and the selected color mode. No full shadow encoding occurs in the live workload.

The focused CTest names are `portals.mvp-acceptance` and `portals.mvp-measurement`.
Existing `portals.dashboard` and `portals.application` retain coverage for lossless
saturation, bounded catchup/backlog, delayed/out-of-order range completion, Unicode,
resize, input ordering, writer failures and shutdown. Real Windows Terminal/xterm,
ConPTY pressure and native restoration checks are separate verification gates.

## Compact construction benchmark

`portals_compact_benchmark` takes no arguments and writes JSON to stdout. It
opens no native terminal. Build with `PORTALS_BUILD_TOOLS=ON`; run the Release
executable for the measured configuration. Three warmups precede 30 samples.
Equivalent managed and description-based three-label scenes report mount/theme,
initial prepare/layout, idle prepare and one-frame run/cleanup. Separate explicit
and compact column cases measure ten callback constructions and ten cell formats.
The column comparison uses explicit identities on both paths and does not measure
aggregate reflection or automatic schema assembly.

Nearest-rank p50/p95 timings use steady-clock nanoseconds. Allocation counters cover
ordinary `new`/`new[]` calls, excluding malloc, aligned allocation and allocated
bytes. Application creation and JSON reporting are outside counted phases. The
workload checks one submitted frame and terminal cleanup. These measurements do
not establish universal allocation budgets or zero-cost abstractions; see
[compact examples](../../docs/compact-examples.md) for API constraints.

Native runs use the same managed terminal factory as Application. Startup queries and
mouse setup are included in native_open_ns. Restoration is included in close timing.
These lifecycle protocol bytes bypass frame counters; throughput measures rendered
frame submissions only. The terminal must support the documented native mouse policy.

# Compact examples and typed text {#compact_examples}

Start with the [runnable examples](examples.md), then read
[declarative composition](declarative-ui.md) for ownership and mount errors.
The conveniences below are additive: retained widgets, `GraphemeText::from`,
explicit projections, and the low-level model/view API remain available.

## Format text without discarding validation

Include `<portals/text_format.hpp>`. `formatText("Price {:.2f}", value)` checks
the format specification at compile time and returns
`expected<GraphemeText, TextError>`. It validates the resulting UTF-8 and preserves
the original error category and byte offset. Use
`formatText(policy, "Price {:.2f}", value)` for an explicit display policy.
Allocation and custom standard-formatter exceptions can still propagate.

For a table callback, `cellFormat<double>("{:.2f}", policy)` owns its pattern
and policy and accepts a borrowed `const double &`. It returns exactly owning
`GraphemeText`, following the existing callback contract. Direct invocation can
throw `bad_expected_access<TextError>` for invalid formatted bytes; table
preparation translates thrown exceptions to its existing `CALLBACK_FAILED`.
Use checked `formatText` when the application needs the individual text error.
This convenience does not introduce a second callback error protocol.

## Fixed schemas without manual column IDs

```cpp
const auto price = portals::cellFormat<double>("{:.2f}", policy);
auto columns = portals::tableColumns(policy,
  portals::tableColumn("Open", &Ohlc::open, price, 9),
  portals::tableColumn("Close", &Ohlc::close, price, 9));

if (!columns) {
  // Report columns.error(): the original header TextError.
}
```

The record type is inferred from the member pointer. Recipes own captions and
callbacks; assembly validates captions and assigns IDs 1..N with initial generation
in argument order, independently for each table. Assemble a fixed schema once.
Reordering the resulting columns retains their IDs. Rebuilding recipes in a new
order creates a different positional schema; dynamic replacement uses explicit
`ColumnId` through the original factories. No global counter or caption-derived
identity is introduced. The existing table validates widths during preparation. The formatter
must use the same width policy as the header and table. No domain row is copied.
`columnOptions(id, caption, width, policy)` returns checked metadata for aggregate
fields or explicit lambda projections. Display captions may be runtime strings:
they are not used as identities or assumed to be translation keys.

## Optional aggregate fields

Link `portals::aggregate` and include `<portals/aggregate_columns.hpp>` only where
aggregate reflection is useful. This opt-in target links the existing core and
supplies the standalone pinned Boost.PFR headers and C++23 configuration.
The ordinary `portals::core` target does not add PFR to its include search path
or compile definitions. Building and consuming the installed adapter require no
network, Boost superproject, or Git submodule.

`aggregateColumns<Record>(std::array{ColumnCaption{"Bid", 9}, ColumnCaption{"Ask", 9}},
formatter)` validates captions and assigns fixed-schema identities automatically.
The overload taking `TableColumnOptions` retains explicit identities for dynamic schemas.
Both forms project every field in aggregate
declaration order, with an independent owning formatter in each existing column.
Metadata count must equal field count, and the formatter must accept every const
field type and return exactly `GraphemeText`. `aggregateColumn<I, Record>` selects
a single field. Explicit projections remain the path for nested records,
nonaggregates, and different formatting of different fields.

PFR's portable SimpleAggregate domain excludes base classes, reference/const
members, and C array members. Unsupported shapes are not promised to produce
a friendly constraint diagnostic. Field positions are not stable schema IDs:
keep explicit column identities stable when record declarations change.
Reflection does not provide captions, units, validation rules, edit commands,
resources, or i18n. It does not copy datasets or change model ownership.

The target sets `BOOST_PFR_USE_CPP26=0` to select the C++23 backend consistently.
Keep PFR version and configuration consistent across translation units; do not
mix another Boost.PFR version in the same program without checking compatibility.
The [dependency notice](../vendor/pfr/README.md) records provenance and hashes.

The complete SDK-free installed adapter consumer is below:

\include{lineno} compact.cpp

## Matching keys without variant boilerplate

Include `<portals/input_match.hpp>` for the pure C++23 constexpr predicates:

```cpp
if (keyPressed(event, InputKey::F2)) {
  // Switch theme, preserving its typed failure handling.
}

if (!keyPressed(event, {InputKey::LEFT, InputKey::RIGHT})) {
  return false;
}

const auto step = keyPressed(event, InputKey::RIGHT) ? LEVEL_STEP : -LEVEL_STEP;
const auto next = std::clamp(level_ + step, MIN_LEVEL, MAX_LEVEL);
```

Matching accepts PRESS and REPEAT, excludes RELEASE and non-key input, and does
not consume events or track physical held-key state. The single-key and list
overloads ignore modifiers, preserving the meter's original behavior. For an exact
shortcut use `keyPressed(event, InputKey::F2, {.control = true})`; all modifier
fields must match, including false fields. CHARACTER matches identity only:
inspect `KeyEvent::scalar` or `TextInputEvent` for character content. The key list
allocates nothing and is searched linearly. Routing and focus still belong to the
existing router; this convenience adds no key-binding subsystem.

## One composition, explicit customization

Gallery uses `ui::column`, standard managed state and bindings; it already had
this short form before these helpers. The custom final meter uses the existing
`ManagedState<int>` for its scalar level, instead of a one-row dataset provider.
State owns immutable snapshots and maintains its revisions internally; the input
handler does not build `LevelSnapshot` or increment a revision. Dataset providers
still use the low-level model API, as demonstrated in dashboard. The meter's
presentation adapter uses checked `formatText` and C++23 `expected::and_then`.
Each continuation runs only after success; the first typed error propagates to
the caller. State snapshots retain their value while the adapter consumes it.
`transform_error` adds managed input context without formatting or flattening the
original `TreeError`. The adapter reads the snapshot directly, so rejected observer
notifications cannot prevent its next preparation. Equal/clamped `set` is a no-op:
unlike the old forced snapshot publication, it does not redraw unchanged output.
Tests compare the old and new terminal bytes/frame counts across every theme,
and separately check this intentional no-redraw improvement at the upper bound.
No implicit widget discovery or separate reactive runtime is added.

Dashboard now mounts its controls, table and chart together through `ui::grid`.
Observers are published after successful mount. Grid tracks and identity-based
placements remain explicit through `setGridOptions`, before running the scene.
There is no implicit placement DSL: this is the low-level customization escape
hatch, with the same four fixed header tracks and two flexible view tracks.
Provider work, immutable model pins, delayed ranges, theme switching and cleanup
keep their original contracts. Continuous-scroll loading is a separate data
provider issue, not repaired by this example migration.

## Full comparable programs

The existing complete composed form and the compact version below perform the
same edit/commit, click, self-removal, custom event, list/table/chart preparation,
theme, hide/disable/remove, resize and cleanup scenario. Installed-package tests
compare their summaries and complete terminal byte streams. The shared support
contains domain rendering, terminal replay and assertions; construction and
bindings remain in the programs. Both support files are presented on the
[declarative composition page](declarative-ui.md).

### Before: explicit text and typed columns

\include{lineno} composed_named.cpp

### After: checked captions, inferred records and owning formatters

\include{lineno} composed_compact.cpp

These are full programs, not a claim that removing every line is desirable.
Their shared `example::take` is a fail-fast demonstration helper: it turns setup
failure into a generic exception and does not preserve the complete original error.
It is not part of the public API or a recommended production error policy. Public
factories return their typed results so applications can report the original cause.
General builder error composition remains future work; this step does not add
TRY macros, an exception-based public error protocol, or a result coroutine runtime.
The three interactive programs are also compiled and run as relocated installed
consumers. Their CLI options, exit codes and defaults are documented on the
[examples page](examples.md). This step does not implement runtime reconciliation,
compile-time scene construction, resources/i18n, or a general reflection schema.

## Costs and measurements

`portals_compact_benchmark` compares equivalent three-label managed and owning
description constructions. It samples mount/theme, initial prepare/layout,
idle preparation, and a finite one-frame runner including cleanup: 30 samples
after three warmups, nearest-rank p50/p95 in nanoseconds. The viewport is 80x24.
Allocation counts cover ordinary C++ `new`/`new[]` calls in each phase, excluding
malloc, aligned allocations and bytes. Application/scene creation and benchmark
reporting are outside the counters. Counts are an instrumented workload, not
the library's universal allocation budget. The runner assertion requires exactly
one frame and a closed in-memory terminal; no native window is opened.

The existing managed lifecycle benchmark remains the broader scaling workload.
PFR instantiation and format checking can add compile time. A shorter source
does not imply faster compilation, fewer allocations or faster rendering.

### Local C++23 measurements

Measured on 10 October 2026 with MSVC 19.51 x64 and GCC 14.3 in WSL,
Release configuration. These small workloads are diagnostic snapshots, not
portable performance thresholds. Timing noise and standard-library differences
prevent comparing the two toolchains as a speed ranking.

| Phase / allocation calls | MSVC explicit / compact | GCC explicit / compact |
| --- | --- | --- |
| Three-label mount and theme | 49 / 63 | 48 / 62 |
| Initial prepare and layout | 48 / 48 | 47 / 47 |
| Idle prepare | 8 / 8 | 8 / 8 |
| One-frame run and cleanup | 70 / 70 | 68 / 68 |
| Ten column callback constructions | 51 / 61 | 41 / 51 |
| Ten cell formats | 50 / 50 | 40 / 40 |

The description path adds 14 allocation calls at mount in this workload;
owning cell formatters add ten during construction. Neither adds calls to the
measured steady prepare/frame phases. The column comparison uses explicit IDs
on both paths; it does not measure automatic schema assembly or PFR at runtime.

A representative two-double-column translation unit was compiled to an object
with `/O2 /std:c++latest` or `-O2 -std=c++23`, one warmup and three samples per
variant. Filesystem caches were warm; linking and a complete clean project build
were excluded. Median wall times were:

| Column construction | MSVC milliseconds | GCC milliseconds |
| --- | --- | --- |
| Original explicit metadata and lambda | 2258 | 3201 |
| Automatic IDs and cellFormat | 2382 | 3267 |
| Optional aggregateColumns / PFR | 2216 | 3693 |

These samples do not demonstrate a compiler-independent speedup. On GCC this
particular PFR instantiation cost more than the explicit member-pointer variant.

Full files also retain provider code and customization. Nonblank, noncomment
line counts changed as follows (text counting, not a complexity metric): gallery
59 to 59, dashboard 423 to 438, custom meter 140 to 119, installed composed form
167 to 165. Dashboard removes manual mount/binding bookkeeping but adds explicit
observers and readable descriptions; its complete file became longer. General
layout shorthand and resource/i18n text are future work, not hidden helpers in
this comparison.

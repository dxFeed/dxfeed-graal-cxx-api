# Runnable examples {#examples}

For the complete form using typed columns and named view settings, see
[declarative composition](declarative-ui.md).
For checked formatted text, member deduction, optional aggregate columns and
full comparable programs, see [compact examples](compact-examples.md).

For a complete form combining an editor, custom final widget, list, table and chart,
see the [declarative before/after consumers](declarative-ui.md). Their installed-package
tests compare identical interactions and complete terminal output.

Build with `PORTALS_BUILD_EXAMPLES=ON`. Each program runs through the same
[Application runner](application.md) and has a finite `--check` mode for automation.
The check modes use an in-memory terminal and logical clock; they do not prove
native terminal restoration or visual compatibility.

| Program | What to try |
| --- | --- |
| `portals_gallery` | Declarative managed form: Tab through editor, checkbox, action and Quit; Enter commits; F2 cycles six themes. |
| `portals_dashboard` | Synthetic market, managed table/chart lifecycle with one shared model, selection and delayed virtual ranges. |
| `portals_custom_widget` | A custom final meter with a managed presentation adapter and scalar managed state; Left/Right change its level, F2 cycles themes. |
| `portals_replay_example` | The dashboard with the built-in recorded price tape selected by default. |

F10 or Ctrl+C exits every interactive example. Mouse routing is provided by the
existing bindings where supported by the terminal. The examples install no signal
handlers and do not promise cleanup after abrupt process termination.

## Gallery and custom widget options

```text
portals_gallery --theme nord
portals_custom_widget --theme catppuccin-latte
portals_gallery --check --theme gruvbox --ansi16
```

Both programs accept `--theme NAME`, `--ansi16`, `--check` and `--help`.
`--theme` selects the initial palette: `modern` (default), `turbo`, `nord`,
`gruvbox`, `tokyo-night`, or `catppuccin-latte`. F2 cycles in that order,
including wraparound. `--ansi16` quantizes actual emitted colors to sixteen
terminal colors; the terminal's configured ANSI palette determines their final
appearance. `--check` uses a finite in-memory keyboard/resize replay and exits;
it opens no native window. `--help` prints these options and exits. Unknown
options or theme names and missing theme values return exit code 2. Runtime
failures return 1 after the runner attempts terminal restoration; success is 0.
The gallery also retains the historical `initial`, `keyboard`, `mouse` and
`disabled` finite fixture modes. They use the same replay runner afterward.

The interactive gallery uses `gallery::Form`: its managed scene owns the state,
nodes and scoped registrations. The separate `gallery::Scene` is a finite
low-level regression fixture, where binding tokens and presentation sync remain
explicit to exercise the retained API. It runs only in the check/stage paths.
The example-only `take` helper stops setup with a generic exception and loses
the original error detail; it is not a production error-handling policy.

The four adapted [Textual palettes](styles.md) change colors
through existing style transactions. They do not reproduce Textual's complete
widget design or introduce a CSS cascade. The dashboard/replay example uses
Modern/Turbo with managed table and chart preparation.

For nested tables and charts, the [layout preparation examples](virtual-preparation.md)
show complete C++23 consumers of the installed package, from explicit extents
through layout snapshots to managed models and views. These finite examples
compare terminal output and resize behavior. The interactive dashboard exercises
the same managed factories with delayed provider work.

## Dashboard configuration

```text
portals_dashboard --rate 10000 --fps 30 --rows 1000000 --seed 42
portals_dashboard --rate 0 --delay-ms 250 --ansi16 --ascii
portals_dashboard --mode replay --trace trace.json --duration-ms 5000
portals_dashboard --check --mode replay --delay-ms 250
```

`--rate` accepts 0..10000 scheduled ticks/second; zero demonstrates idle and delayed
range navigation without market updates. `--fps` accepts 1..1000 frames/second.
`--rows` sets 1..1000000000 logical instruments without creating a widget per row.
`--seed` controls the deterministic synthetic tape. `--mode synthetic|replay`
selects the data source. The replay tape lives in `examples/replay/data.hpp` and is
compiled into the executable, so running from another directory needs no data file
or network. Replay loops its 16 recorded prices in order.

`--delay-ms` delays first acquisition of an uncached viewport range (0..60000 ms).
The initial 256-row page is immediately ready. Each acquired completion prefetches
up to 128 rows on either side of its exact request, clipped to the logical model.
The views reuse those immutable pages for nearby windows within the same market
revision. This keeps held-arrow navigation from replacing work on every step,
including `--rate 0`; a true cache miss still displays loading placeholders.
Two independent bounded range slots serve table and chart, with intentionally
reversed completion order. An uncached range replaces its old request. Once
acquired, a page can be regenerated with new immutable market revisions, so a
continuous market does not permanently starve a delayed viewport. Each snapshot
owns the initial page plus at most two prefetched pages; each of those contains
at most the exact request count plus 256 rows. No full-model cache is built.

`--pump-budget` sets 1..10000 ticks per provider callback, default 512. The source
aggregates every generated tick into OHLC before publishing display snapshots.
Only immutable display updates coalesce. A late timer retains the next ungenerated
ordinal and catches up in bounded callbacks; scheduled ticks and actually generated
`ticks` are distinct. Stop cancels remaining scheduled generation rather than
claiming that ungenerated data was processed. This demo provider runs on the UI
owner; it is not a benchmark of an independent market worker.

The managed scene pins the shared model once per frame for both table and candle
chart, including any bounded layout retries. Logical instruments
share the market OHLC with a deterministic price offset. Table formatting covers
only visible rows; preparation retains visible rows plus two overscan rows per
side. No million-row materialization is performed. The title and editable note
exercise Unicode. Dashboard candles use one-cell Unicode wicks and filled bodies
by default; `--ascii` selects ASCII labels and candle brushes. `--ansi16` selects
actual ANSI16 encoding, not an advertised capability flag. Both themes are
compiled typed equivalents of the distributed PSS themes and require no runtime
theme-file lookup. Themes change controls, table colors and the chart background;
rising/falling market colors remain distinct.

Tab switches focus, arrows/Home/End select/scroll the table, Enter commits the
note, and F2 switches Modern/Turbo. The Inspect button deliberately sets the note
to its existing value. With `--rate 0 --trace trace.json`, focus Inspect and press
Enter: the metadata trace distinguishes handler return, `value_unchanged`, and
`frame_not_requested`. Text values and terminal bytes are not exported. File errors
are reported after terminal restoration, independently of a prior run failure.

`--duration-ms` stops an interactive run after at most the next provider service
opportunity (up to 3600000 ms); zero leaves it interactive. Output/host stalls can
still extend real cleanup, as documented by `TerminalSession`. `--check` instead
replays a finite keyboard/resize script for one logical second and prints observed
frame/tick/byte/queue totals after close.

## Where to start in the source

`examples/gallery/form.hpp` shows a complete owning `ui::column` form. Managed
state, standard bindings, presentation synchronization and style registration
are owned by `ManagedScene`; the application retains only business behavior,
initial focus and scene-level keyboard commands. The older
`gallery.hpp` finite fixture remains for existing semantic replay tests.
`examples/custom_widget/meter.hpp` shows an alternative immutable provider and a
final widget whose `.onPrepare` adapter formats changed presentation before
rendering. This custom preparation remains explicit.
[The complete dashboard scene](../examples/dashboard/dashboard.hpp) delegates
ordinary and virtual-view preparation to `ManagedScene`. Its managed factories
own model ranges, input bindings and standard styles, while `Grid` determines
the actual view extents. The scene no longer retains its own frame pin or calls
per-view preparation helpers.

Provider work remains visible in the dashboard. The table's preparation handler
reads its first row before the chart's origin callback runs; their registration
order keeps the chart aligned with the table. Both handlers receive the same
borrowed frame pin and copy new requests into the existing bounded provider
slots. The owning `pendingLatest` slot is cleared only after the model publisher
accepts it. The provider keeps its timer, retry, cancellation and prefetch policy;
managed preparation adds no second scheduler or cache. On stop, the scene cancels
provider work before closing the managed subtree and model. No example requires
private work files.

For measurements, `dashboard::Options::ingressObserver` can collect bounded
logical queue-latency samples at entry to the snapshot ingress callback. It covers
that ingress only, not the model's separate DATA endpoints. Null disables the
sampling branch. `tickCount`, `scheduledTicks`, `pendingTicks` and `stoppedAt`
expose observed provider progress for an external benchmark without inventing
throughput from the configured rate. Observer storage remains caller-owned.

## Complete gallery before and after

These are complete form implementations, not abbreviated pseudocode. The
frozen low-level form is compiled by `portals.gallery-managed`; the test replays
keyboard editing, checkbox/action activation, mouse, Unicode, resize and the
original Modern-to-Turbo switch through both implementations. It compares all
terminal bytes, action counts and frame counts. A separate idle check requires
exactly one frame. The shared `common.hpp` provides ordinary error reporting and
text construction; it does not hide binding tokens or synchronization.

### Before: explicit ownership and synchronization

[Open the complete source](../tests/support/gallery_before.hpp).
The generated HTML reference embeds it below with line numbers.

\include{lineno} gallery_before.hpp

### After: managed declarative form

[Open the complete source](../examples/gallery/form.hpp).

\include{lineno} gallery/form.hpp

The CLI parser and managed error reporter are additional application support,
not costs silently attributed to the composition API. The complete installed
consumer comparison remains available in [declarative composition](declarative-ui.md).
Runtime costs and scaling limits have a separate [measurement guide](managed-performance.md).

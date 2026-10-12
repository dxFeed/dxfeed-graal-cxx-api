# Control gallery

Start with [`Form`](form.hpp): it mounts a managed form, binds the name and
checkbox state, and registers scoped actions. Enter commits the name; Tab
changes focus. Run increments an observable action count without changing a
model. F2 cycles the six theme presets; F10 or Ctrl+C exits. The application
must outlive the form and all calls use its owning UI thread.

```powershell
.\portals_gallery.exe --theme nord
.\portals_gallery.exe --check
.\portals_gallery.exe --help
```

`--check` runs a finite in-memory replay and closes its transport automatically.
It also exercises the low-level [`Scene`](gallery.hpp). That fixture deliberately
owns binding tokens and synchronizes presentation explicitly: it preserves
coverage of the retained API, rather than demonstrating the shortest form API.
Its positional stages (`initial`, `keyboard`, `mouse`, `disabled`) select
cumulative semantic checks; they imply `--check` and do not open a native window.

[`options.hpp`](options.hpp) documents parsed settings and theme order.
`--ansi16` requests the terminal's sixteen-color output mode; the default is
true color. [`common.hpp`](common.hpp) contains shared setup helpers and typed
application diagnostics; [`managed_report.hpp`](managed_report.hpp) reports
managed failures after terminal restoration. The example-only `take` helper
throws a generic setup exception and loses the original error detail. Production
applications should retain their typed error policy.

Build and installation instructions are in the project's
[examples guide](../../docs/examples.md).

# Release notes

Versions are listed newest first. Entries describe implemented changes;
planned work is not included. An **Unreleased** heading has no publication date.

## Unreleased

## v0.2.0 (2026-10-10)

- [Added] C++23 retained terminal UI core with UI-thread ownership, typed
  errors, bounded scheduling, event routing, focus and an application runner.
- [Added] Managed control lifecycle and owning `ui` descriptions for forms,
  layouts and custom widgets. Scenes manage bindings and preparation while
  the low-level tree and control APIs remain available.
- [Added] Properties and immutable model snapshots, virtual lists and tables,
  scrolling, Canvas and charts. Managed virtual views prepare from captured
  layout geometry and share pinned model snapshots within a frame.
- [Added] UTF-8 grapheme text and explicit terminal cell-width policies,
  including wide characters and combining sequences.
- [Added] Styles, selectable theme palettes and bounded causal diagnostics
  for input, scheduling and frame processing.
- [Added] Checked `formatText`, reusable `cellFormat`, typed table accessors
  and `tableColumns` recipes with automatic table-local column identities
  for fixed schemas. Explicit identities remain available for dynamic schemas.
- [Added] Optional `portals::aggregate` target for positional aggregate columns
  using standalone Boost.PFR headers; consumers do not need the full Boost
  distribution or C++26 reflection.
- [Added] `ui::grid` composition and constexpr `keyPressed` helpers.
- [Added] Minimal `portals::legacy` streaming compatibility for the original
  dxFeed `MarketDepthModelSample`, including its historical umbrella-header
  aliases. This is a limited migration boundary, not the complete 0.1.x API.
  See [compatibility and cleanup requirements](docs/legacy.md).
- [Added] Relocatable installed CMake package, vendored dependencies for offline
  source builds, headless tests, opt-in fuzz targets and installed-package
  consumer checks.
- [Changed] Gallery, dashboard and custom-widget examples use managed APIs;
  the gallery also retains an explicit low-level scene fixture. Documentation
  now has a reading path, complete examples, API contracts and offline Mermaid
  diagrams.
- [Fixed] Text-field caret visibility and orderly Windows terminal screen
  restoration in the example applications. After cancelled output, native cleanup
  waits for the console host before restoring the caller's screen and cursor.
- [Fixed] MSVC builds interpret project sources and string literals as UTF-8,
  independent of the Windows host code page.
- [Changed] Naming checks omit statistics about suppressed diagnostics while
  retaining project diagnostics and failure exit codes.
- [Added] Tag-triggered and manual release automation, version-specific release
  descriptions, release candidates, verified bundles and GitHub Pages deployment
  through Actions. Standalone PowerShell and Python commands prepare release PRs;
  merging a release PR automatically starts tag creation and publication.
  See [release process](docs/releasing.md).

### Known limitations

The managed and declarative APIs remain experimental; version 0.2.0 does not
promise a stable API or ABI. Native backends target Windows and POSIX/Linux;
macOS support has not been established.

With delayed range delivery, sustained scrolling can keep a virtual table
showing loading placeholders until the key is released. Continuous-scroll
loading improvements remain pending.

See [current limitations](docs/limitations.md) for platform, ownership,
shutdown and delivery boundaries.

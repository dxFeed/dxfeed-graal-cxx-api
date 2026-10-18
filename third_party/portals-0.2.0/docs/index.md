# Portals documentation

Start here to build an application with Portals. The library is under
development; managed and declarative APIs are experimental. Read the
[current limitations](limitations.md) before relying on a platform or
lifecycle guarantee.

See [release notes](../ReleaseNotes.md) for changes by version and publication
status.

## First visit: read in this order

1. [Getting started](getting-started.md): build Portals and launch the gallery.
2. [Runnable examples](examples.md): controls, options and complete source
   examples. Try the gallery before the larger dashboard.
3. [Declarative composition](declarative-ui.md): describe an ordinary form
   with `ui` and understand what its state and callbacks own.
4. [Managed controls](managed-controls.md): lifecycle, typed failures and
   custom widgets. Read this when a form needs more customization.
5. [Installed CMake package](package.md): link Portals into your own project.

You do not need to read the renderer, scheduler or every API declaration
before writing a form. Consult the chapters below as needed.

## Build an application

| Need | Read |
| --- | --- |
| Compose a form and manage its lifetime | [Descriptions](declarative-ui.md), [managed controls](managed-controls.md), [application runner](application.md) |
| Choose and arrange controls | [Basic controls](controls.md), [LineEdit](line-edit.md), [Row and Column](layout.md), [Grid and Overlay](grid.md) |
| Apply colors and themes | [Styles and themes](styles.md) |
| Connect state and data | [Properties](property.md), [immutable models](model.md) |
| Display large datasets | [Layout-driven preparation](virtual-preparation.md), [virtual lists](virtual-list.md), [virtual tables](virtual-table.md), [scrolling](scroll.md), [charts](chart.md) |
| Write shorter checked text and typed columns | [Compact examples](compact-examples.md) |
| Explain an action that did nothing | [Diagnostics](diagnostics.md), [terminal probe](terminal-probe.md) |
| Keep an existing streaming MarketDepthModelSample building | [Minimal legacy compatibility](legacy.md) |

## Understand the core

Read [architecture](architecture.md) first for component and event-flow
diagrams. These chapters describe the retained core and its low-level contracts.

- Ownership and events: [widget tree](widget-tree.md), [focus and routing](focus.md),
  [scheduler](scheduler.md).
- Text and coordinates: [UTF-8 and cell width](text.md), [geometry](geometry.md).
- Input and output: [input decoder](input.md), [terminal session](terminal.md),
  [cell buffer](cell-buffer.md), [renderer](render.md), [diff and output](diff.md).

## API reference

The generated HTML site has a separate **API reference** menu for types,
concepts, namespaces, members and headers. In a checkout, public contracts
live under `include/portals`; start with `ui.hpp`, `managed.hpp` and
`application.hpp` for ordinary application code. See
[getting started](getting-started.md) to build
and open the complete reference locally.

## Develop and evaluate Portals

The [contributor guide](development.md) collects tools, experiments, documentation
maintenance and dependency notices.

See [releasing](releasing.md) for tags, release candidates, artifacts and
documentation deployment through GitHub Actions.

- [Testing](testing.md) and [fuzzing](../fuzz/README.md): correctness tools.
- [Managed API measurements](managed-performance.md): reproducible costs and
  known scaling limits, not a promise of linear performance.
- [API design scenarios](api-design.md) and the
  [module experiment](../experiments/module-spike/README.md): design/experiment
  material, not the recommended first application or a stable API contract.
- Dependency notices: [utf8proc](../vendor/utf8proc/README.md) and
  [Textual palette attribution](../vendor/textual/README.md).
  [Private doctest dependency](../vendor/doctest/README.md) is used only by tests.

## Reading on GitHub and as HTML

This index and its relative Markdown links work in a checkout and on GitHub.
Complete example sections link to the real, compiled source files. Their
Doxygen `\include` lines expand into numbered listings in generated HTML;
GitHub shows those lines literally, so use the source links there. Header
labels such as `{#examples}` are Doxygen identifiers, not part of a title.

The HTML site adds structured navigation and API reference to the same guides.
It is generated into a build directory, not maintained as a second copy.
A future GitHub Pages deployment can publish that directory. This repository
does not assume an existing hosted documentation URL.

# Developing Portals

This chapter is for library contributors. To write an application instead,
start with the [documentation guide](index.md).

## Verify changes

Use [headless testing](testing.md) for the normal test suite, naming checks
and terminal fixtures. [Fuzzing](../fuzz/README.md) describes the bounded input
and text targets and sanitizer setup. Automated checks must finish without
requiring someone to dismiss a window.

## Evaluate changes to the API

[Managed API measurements](managed-performance.md) records equivalent workloads,
raw samples and known scaling limits. [Design scenarios](api-design.md) and the
[module experiment](../experiments/module-spike/README.md) are exploratory
material, not the entry point for current applications.

## Maintain the documentation

Keep the [reading guide](index.md) and `docs/DoxygenLayout.xml` in agreement.
Use relative Markdown links that resolve in the checkout. When embedding a
compiled example with a Doxygen command, also provide a source link for GitHub
readers. Build `portals_docs` and run `tools/docs/check_links.py` as described in
[getting started](getting-started.md). Inspect the generated navigation as well
as checking links: a valid link does not establish a useful reading order.

Use fenced `mermaid` blocks for flow and ownership diagrams. The same Markdown
renders on GitHub and in Doxygen; do not maintain a separate hand-aligned ASCII
copy. Give each diagram an `accTitle` and an `accDescr`, keep labels short, and
prefer vertical flow or several smaller diagrams to one very wide graph.

The documentation build requires Mermaid CLI and renders SVG locally. Its
shared theme is configured in `docs/mermaid.json`. Diagrams use a light neutral
canvas inside either documentation theme, and scale down to the page width.
Check the generated images as well as parser success: valid Mermaid can still
produce an unreadable layout. Existing architecture SVG illustrations remain
ordinary image assets.

## Licenses and provenance

Portals uses the Boost Software License 1.0 (`LICENSE.txt` in the repository root).
The [utf8proc provenance](../vendor/utf8proc/README.md) and
[license](../vendor/utf8proc/LICENSE.md) cover the private Unicode dependency.
The [Textual notice](../vendor/textual/README.md) identifies the adapted palette
colors and their MIT license. These notices also belong in distributed help
and packages.


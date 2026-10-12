# Textual palette attribution

Portals adapts the Nord, Gruvbox, Tokyo Night, and Catppuccin Latte RGB palette
values from [Textual](https://github.com/Textualize/textual), commit
`06dbeef4bb70fb718236aa418ed658ef4667a126`:

- [Source theme definitions](https://github.com/Textualize/textual/blob/06dbeef4bb70fb718236aa418ed658ef4667a126/src/textual/theme.py)
- [Upstream MIT license](https://raw.githubusercontent.com/Textualize/textual/06dbeef4bb70fb718236aa418ed658ef4667a126/LICENSE)
- [Retained complete license notice](LICENSE), copyright (c) 2021 Will McGugan

The adaptation is in `src/styles/style.cpp`. Textual foreground and background
remain unchanged; primary maps to the Portals focus/caret accent, secondary to
disabled text, panel to selection fill, and accent to header text. See
[the style guide](../../docs/styles.md) for state behavior and
limitations. Tests repeat these source RGB values as expected results.

No Textual Python code, runtime, generated color system, or widget styles are
bundled. The MIT notice applies to the adapted palette material; Portals' own
implementation retains its existing license.

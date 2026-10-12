# utf8proc provenance

Private Unicode adapter dependency: [utf8proc v2.12.0](https://github.com/JuliaStrings/utf8proc/releases/tag/v2.12.0),
commit `79cdd5ab40c79afa559f689ffc13b76812dee1ac`, Unicode **18.0.0**.
`manifest.json` records exact downloaded file sizes and SHA-256 hashes. Each file
origin is `https://raw.githubusercontent.com/JuliaStrings/utf8proc/79cdd5ab40c79afa559f689ffc13b76812dee1ac/<name>`.
Sources are kept byte-for-byte. CMake verifies them at configure time and through
`portals.unicode-provenance`, offline. Git text conversion is disabled here.

Only `utf8proc.c` is compiled as C99 (it includes `utf8proc_data.c`). The target
is static and private to Portals. `UTF8PROC_STATIC` and the upstream include path
are private. No upstream types appear in Portals public headers. The only
transitive runtime dependency is the C runtime; no data generator or download
is needed to build or consume Portals.

Code is MIT/Expat (`MIT`); embedded Unicode data have the Unicode permission
notice. Both complete notices are in [LICENSE.md](LICENSE.md) and must accompany
redistribution. Portals' own BSL-1.0 license is unchanged. The independently
published conformance fixture and its license live in `tests/fixtures/unicode`.

Alternative ICU adds a much larger required component. Maintaining our own
Unicode tables and grapheme state machine would duplicate specialist work.
utf8proc supplies decoding, UAX #29 boundaries, properties and scalar widths;
Portals owns its deliberately limited terminal cluster-width policy.

To update: choose an exact release/commit and Unicode version, obtain all four
files from that same commit, preserve upstream notices, record new bytes/hashes
and origins, update the independent matching Unicode fixture, and run all text,
provenance, compiler and offline-consumer checks. Do not format upstream files.
Version changes require review of width fixtures and documented policy.

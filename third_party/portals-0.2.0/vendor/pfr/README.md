# Boost.PFR snapshot

This directory contains the unmodified `include/` tree (44 headers) and
`LICENSE_1_0.txt` from [boostorg/pfr](https://github.com/boostorg/pfr), tag
`boost-1.92.0`, commit `401385c240027423acbb1eb6dea2abe0043db5aa`.
`manifest.json` records the archive and per-file SHA-256 hashes. No Boost
superproject, submodules, or network access are needed at build time.

The opt-in Portals adapter is `<portals/aggregate_columns.hpp>`. Its C++23
consumers should use `BOOST_PFR_USE_CPP26=0` until upstream PFR's feature
detection no longer selects Clang's C++26 extension in C++23 mode. Keep that
definition consistent across translation units that include Boost.PFR.

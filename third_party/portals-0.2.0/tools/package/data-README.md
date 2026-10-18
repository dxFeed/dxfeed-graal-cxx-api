# Bundled reference data

traces contains the repository's versioned terminal replay fixtures. These are
useful for inspecting deterministic resize/input, wide-cell-edge and writer
failure cases. They are not required by the four runnable executables.

data.hpp is the exact recorded price tape used by the replay example, also
available in examples/source/replay/data.hpp. The prices are compiled into the
executable. This copy explains the replay data and lets you adapt the source;
changing it here does not change the existing binary.

themes/modern.pss and themes/turbo.pss (beside this data directory) are shipped
stylesheet examples. Demonstration defaults are compiled into the executables;
the files are reference inputs for applications using the stylesheet API.

Repository code and these project fixtures use BSL-1.0; third-party and Unicode
notices are collected in licenses and inside the installed SDK.

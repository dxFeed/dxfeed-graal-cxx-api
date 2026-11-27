* **\[MDAPI-644]\[C++]** Added the CMake option `DXFCXX_GRAAL_NATIVE_SDK_VARIANT` (`ce`, `oracle`, `oracle-g1`) that selects
  the variant of the Graal Native SDK: the `oracle` variant (Oracle GraalVM, `-O3`) publishes ~20 % more events per
  second and spends ~20 % less CPU per received event on Windows x64 (see `HOW_TO_BUILD.md`).
    * The installed package sets `dxFeedGraalCxxApi_GRAAL_NATIVE_SDK_VARIANT`.

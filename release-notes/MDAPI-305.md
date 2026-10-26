* **\[MDAPI-305]\[C++]** The Windows `-static-mt` archives need no Visual C++ Redistributable, and a single exe or
  dll can be built from them. See "Windows without the Visual C++ Redistributable" in `README.md`.
    * The archives take the SDK DLL built with the static C runtime, and the C++ API DLL is built with `/MT` (`/MTd`).
    * The archives add `lib\DxFeedGraalNativeSdk.lib`, `lib\DxFeedGraalNativeSdk_static.lib` and
      `lib\link-flags.txt`: `dxFeedGraalCxxApi_static.lib` links either with the SDK DLL or with the static SDK
      library into one exe or dll that depends only on Windows DLLs.
    * Code built with `/MT` shall not link with `dxFeedGraalCxxApi.dll` of these archives: the DLL and the code each
      have their own C runtime, and the C++ API passes standard library objects between them.
    * The Windows archives are built with Visual Studio 2022 17.14 (v8.1.0 was built with Visual Studio 2026), so
      their static libraries link with Visual Studio 2022 17.14 and newer.
    * `StringLike`, `StringHash`, `EventFlagsMask`, `operator""_c` and `DXFeedTimeSeriesSubscription` are exported
      from `dxFeedGraalCxxApi.dll`: a program that used them with the DLL failed to link.
    * The release workflow checks the archives: no DLL imports the dynamic C runtime, and a program linked in each of
      the two ways runs in Windows Server Core without the Redistributable.

* **\[MDAPI-461]\[C++]** **\[BREAKING]** Build and packaging: the shared library exports only its public API and
  has a version, the packages contain a CMake package, the library targets keep their build options to themselves.
    * Linux and macOS: the shared library is built with hidden visibility and exports only the classes and functions
      of the public headers. The symbols of the internal classes and of the embedded `fmt`, `date` and Boost are no
      longer exported; applications that used them link their own copy.
    * The CMake target `dxfcxx` passes `DXFCPP_USE_DLLS` to the targets that link it: an application that uses the
      shared library through CMake no longer defines it. Without CMake, define `DXFCPP_USE_DLLS` as before.
    * The lists of the exported symbols (`abi/symbols-linux.txt`, `abi/symbols-macos.txt`, `abi/symbols-windows.txt`)
      are checked on every change, so a change of the ABI is visible in the pull request.
    * The shared library has a version: `libdxFeedGraalCxxApi.so.9` (SONAME) on Linux, `libdxFeedGraalCxxApi.9.dylib`
      on macOS, with the unversioned name as a link to it. Programs need the versioned file next to them or in the
      library path.
    * The `Lib` archives of a release for Linux and macOS are `.tar.gz` instead of `.zip`: tar keeps the links of the
      versioned library names for every unpacking tool. Windows archives stay `.zip`.
    * The packages contain a CMake package: `find_package(dxFeedGraalCxxApi CONFIG)` gives the targets `dxfcxx::dxfcxx`
      (the shared library), `dxfcxx::static` and `dxfcxx::graal`; the same targets exist when the API is added by
      `add_subdirectory` or `FetchContent`. The targets require C++20 from the targets that link them. A package of
      another major version is not found.
    * The library targets no longer pass their build options to the targets that link them (`/bigobj`, `/utf-8`,
      `/W4`, `-fPIC`, `-Wall`, `-Wextra`, `-pedantic`, `-Wa,--noexecstack`, `DXFCXX_VERSION`,
      `_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR` and the other definitions of the build). An application that relied on them
      sets them itself.
    * Release builds of the libraries are hardened: `/guard:cf`, `/sdl`, `/CETCOMPAT` with MSVC; `_FORTIFY_SOURCE=3`,
      stack protector, full RELRO and immediate binding with GCC and Clang (the `DXFCXX_HARDENING` option).
    * Added as a subproject, the API no longer adds install rules or a CPack configuration to the project and no
      longer overrides its `BOOST_RUNTIME_LINK`; `-DDXFCXX_INSTALL=ON` installs it with the project.
    * The public headers contain only ASCII characters.
    * Within a major version the shared library keeps its ABI on Linux: each release is checked against the previous
      one of the major and publishes its ABI as the asset `abi-x86_64-linux.xml`.
    * The Samples packages have all the samples directly in `bin/`, with one `dxfeed.system.properties`; the samples
      that were in subdirectories of `bin/` no longer need `LD_LIBRARY_PATH` to start on Linux. OnDemandSample sets
      the address of the on-demand service in code.

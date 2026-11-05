* **\[MDAPI-461]\[C++]** **\[BREAKING]** The shared library exports only its public API.
    * Linux and macOS: the shared library is built with hidden visibility and exports only the classes and functions
      of the public headers. The symbols of the internal classes and of the embedded `fmt`, `date` and Boost are no
      longer exported; applications that used them link their own copy.
    * The CMake target `dxfcxx` passes `DXFCPP_USE_DLLS` to the targets that link it: an application that uses the
      shared library through CMake no longer defines it. Without CMake, define `DXFCPP_USE_DLLS` as before.
    * The lists of the exported symbols (`abi/symbols-linux.txt`, `abi/symbols-macos.txt`, `abi/symbols-windows.txt`)
      are checked on every change, so a change of the ABI is visible in the pull request.

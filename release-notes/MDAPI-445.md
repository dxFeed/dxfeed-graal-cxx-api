* **\[MDAPI-445]\[C++]** Fixed: the Linux packages (library, Samples, Tools; x64 and aarch64) did not load on
  Ubuntu 22.04, Debian 12 and Oracle Linux / RHEL 8 and 9: they needed glibc 2.38 and the `libstdc++` of GCC 13.
  * The packages are built on Oracle Linux 8 with GCC Toolset 14 and need glibc 2.28 and the `libstdc++` of GCC 8.
  * The release workflow checks that the packages load on Oracle Linux 8 and Ubuntu 22.04.
  * The library targets link the thread library (`Threads::Threads`, public): with glibc older than 2.34 the
    programs that use the library did not link (`libpthread.so.0: error adding symbols: DSO missing from command line`).
  * With CMake older than 3.28, the installation no longer contains the private `bits` headers and CMake package.

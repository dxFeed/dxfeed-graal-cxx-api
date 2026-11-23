* **\[MDAPI-632]\[C++]** `System::getBuildInfo` tells how the Graal Native SDK library was built and what runs it: the
  version of QDS, the vendor and version of the runtime, the target machine and the garbage collectors
  (`System::BUILD_INFO_QD_VERSION`, `BUILD_INFO_VENDOR`, `BUILD_INFO_MARCH`, `BUILD_INFO_GC`).
    * The Graal Native SDK 3.19.0 is the minimum version: the library calls its `dxfg_v4_system_get_build_info`.

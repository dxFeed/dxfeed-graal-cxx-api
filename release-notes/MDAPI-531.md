* **\[MDAPI-531]\[C++]** `System::getRuntimeStatistics()` returns the runtime statistics of the Graal isolate and of its
  process as `RuntimeStatistics`: the heap (used, committed, maximum, and the eden, survivor and old pools), the counts
  and the times of the young and full collections, the threads, the CPU time of the process, the uptime, the allocated
  bytes and the available processors.
    * A value that the platform or the garbage collector does not give is `-1` ("not given"): the pool and GC values
      with a GC other than the Serial GC of the SDK builds, `allocatedBytes` with GraalVM 25.5.
    * With a Graal Native SDK library that does not support the runtime statistics the call throws `RuntimeException`
      that names the feature and the version of the library.
    * The Graal Native SDK 3.17.0 is the default and the minimum version.

* **\[MDAPI-440]\[C++]** Fixed: the process hung at exit after a live `InstrumentProfileConnection` was opened (with
  or without `close()`), and could hang or crash at exit with an open endpoint (with the DLL on Windows as well).
    * The library no longer tears down the Graal isolate at exit (the tear-down waits for every Java thread): the
      process ends the Java threads. The library's registries are never destroyed, so the listener calls during the
      exit do not use destroyed objects.
    * The listeners are not called after the library's exit handler has run (it is registered when the library is
      first used), but a listener call in progress is not awaited, and with the Windows DLL the handler runs after
      the static objects of the executable are destroyed. Close the endpoints and the connections, or call
      `dxfcpp::shutdown()`, before the static objects that the listeners use are destroyed.
    * Added `dxfcpp::shutdown()` (`System.hpp`): closes all the endpoints and the instrument profile connections,
      waits for the termination of the endpoints and the listener calls in progress, and releases the Java objects.
      Calling it is optional. Afterwards the listeners are not called, the functions that need Java fail (most throw
      `GraalException`), and the objects of the library can still be destroyed.

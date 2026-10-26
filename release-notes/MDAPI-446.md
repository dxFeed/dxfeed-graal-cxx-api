* **\[MDAPI-446]\[C++]** Fixed: a process that uses `dxFeedGraalCxxApi.dll` on Windows hung at exit after any call
  that needs the Graal isolate (the fix is the exit model of MDAPI-440). The static initialization test checks the
  Graal-dependent constants with the DLL on Windows too.

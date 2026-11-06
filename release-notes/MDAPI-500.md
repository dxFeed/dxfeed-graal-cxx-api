* **\[MDAPI-500]\[C++]** macOS x86-64: the Graal isolate installs no handlers of `SIGPIPE` and `SIGXFSZ` (the SDK
  library for this platform is built with an older GraalVM), so a write to a closed socket may kill the process. Set
  `SIG_IGN` for `SIGPIPE` before the first call of the API. The documentation of `System::setIsolateArguments()`
  says so.

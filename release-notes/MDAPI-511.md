* **\[MDAPI-511]\[C++]** **\[BREAKING]** The Graal Native SDK 3.19.0 is the minimum version: the configuration fails
  with an older `DXFEED_GRAAL_NATIVE_SDK_VERSION`, also with `DXFG_LOCAL_ARCHIVE_PATH`.
    * macOS x86-64: the Graal isolate installs its handlers of `SIGPIPE` and `SIGXFSZ`, as on the other platforms
      (SDK 3.14.0+, MDAPI-500, SDK defect MDAPI-501): a write to a closed socket by a connector of an endpoint fails
      with `EPIPE` instead of killing the process.

* **\[MDAPI-428]\[C++]** Arguments of the Graal isolate and the error of its creation.
    * Added `System::setIsolateArguments()`: the arguments that the isolate is created with (`-Dmy.prop=value`,
      `-Xmx512m`, `-XX:-EnableSignalHandling`). A repeated call replaces them, an empty vector clears them. It does not
      create the isolate and returns `false` once the creation has started.
    * Added the `DXFCPP_ISOLATE_ARGUMENTS` environment variable with the isolate arguments, split as `JDK_JAVA_OPTIONS`
      is (whitespace, quotes, no escapes). Its arguments go after those of `System::setIsolateArguments()`. The only
      way to give arguments to the isolate in the C API.
    * If the isolate cannot be created (for example, `-Xmx64q`, an unknown `-XX:` option, an unmatched quote in the
      variable), the first call that needs Java and every next one throw `GraalException` with the numeric code, its
      text and the reason; the isolate is not created again. The error used to be lost and showed up later, in
      another call. The values of the arguments are not in the messages.
    * `System::getProperty()` and `System::setProperty()` throw `GraalException` when the isolate cannot be created,
      instead of returning an empty string or `false`.
    * `dxfc_system_set_property()`, `dxfc_system_get_property()` and `dxfc_dxendpoint_new_builder()`,
      `dxfc_dxendpoint_get_instance()`, `dxfc_dxendpoint_get_instance2()`, `dxfc_dxendpoint_create()`,
      `dxfc_dxendpoint_create2()` return `DXFC_EC_ERROR` when the isolate cannot be created: the exception does not
      leave the C function.

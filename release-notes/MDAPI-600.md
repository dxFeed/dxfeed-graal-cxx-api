* **\[MDAPI-600]\[C++]** `Tools NetTest` follows the NetTest specification `nettest/1`: the command line, the error
  messages and the exit codes are the same as in the NetTest of the Graal Native SDK.
    * The producer mode `p` publishes generated `Trade` and `TimeAndSale` events.
    * `Tools help NetTest` prints the usage with the line `spec_version nettest/1`.
    * The scheme of `Tools` is `ext:opt:sysprops,opt:dxprops,resource:dxlink.xml`, as in the Tools of the .NET API: the
      scheme properties of the endpoint builders and of the `-p` option, such as `dxscheme.nanoTime`, take effect.
    * The options of the native runtime (`-X`, `-XX:`), `--legacy`, the flags of the QD library (`/r`, `{len,max}`,
      `;<time>`) and the wildcard are not supported and end with `unsupported option` or `unsupported QD flag`.

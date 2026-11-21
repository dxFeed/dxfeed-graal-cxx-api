* **\[MDAPI-600]\[C++]** `Tools NetTest` follows the NetTest specification `nettest/1`: the command line, the error
  messages and the exit codes are the same as in the NetTest of the Graal Native SDK.
    * The producer mode `p` publishes generated `Trade` and `TimeAndSale` events and prints the statistics lines
      (`{*NetTestProducerSide-<name>*}`, one line per instance) and the runtime line (`{*NetTestRuntime-<name>*}`) once
      per period `-s`, aligned to the system clock.
    * The consumer mode `c` receives the events, `-C` instances sharing the symbols, and prints the end-to-end lag of
      the records (`{*NetTestConsumerSide-<name>*}`) from their time and nanosecond part; a negative lag (a clock skew
      between the hosts) is kept with its sign. The consumer exits with 1 and `no_data` when the open connections bring
      no records for 30 s after the last record (60 s after the connection), and with `nano_missing` when the first
      1000 events have no nanosecond part (`-Ddxscheme.nanoTime=false` receives without it).
    * `--summary <file>` writes the summary of the run in the format `nettest-summary/1`: the intervals of the
      statistics, the end-to-end lag percentiles of the consumer (an HdrHistogram with the layout of the Java one),
      the publishing time and the schedule slip of the producer, the runtime and the statistics of the connectors. The
      file is written through `<file>.tmp` and a rename, also when the run fails. `--warmup <time>` marks the intervals
      from the first batch or record as warm-up: they stay out of the percentiles of the summary.
    * `Tools help NetTest` prints the usage with the line `spec_version nettest/1`.
    * The scheme of `Tools` is `ext:opt:sysprops,opt:dxprops,resource:dxlink.xml`, as in the Tools of the .NET API: the
      scheme properties of the endpoint builders and of the `-p` option, such as `dxscheme.nanoTime`, take effect. The
      dxLink example of the README and the `dxfeed.system.properties` of the samples show the same scheme.
    * The options of the native runtime (`-X`, `-XX:`), `--legacy`, the flags of the QD library (`/r`, `{len,max}`,
      `;<time>`) and the wildcard are not supported and end with `unsupported option` or `unsupported QD flag`.

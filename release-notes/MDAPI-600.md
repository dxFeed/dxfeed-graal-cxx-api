* **\[MDAPI-600]\[C++]** `Tools NetTest` follows the NetTest specification `nettest/1`: the command line, the error
  messages and the exit codes are the same as in the NetTest of the Graal Native SDK.
    * The producer mode `p` publishes generated `Trade` and `TimeAndSale` events and prints the statistics lines
      (`{*NetTestProducerSide-<name>*}`, one line per instance) and the runtime line (`{*NetTestRuntime-<name>*}`, `n/a`
      for a value the runtime does not give) once per period `-s`, aligned to the system clock.
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
    * `--metrics-port [<host>:]<port>` serves `/metrics` in the Prometheus text format over HTTP (127.0.0.1 without a
      host, a free port for 0): the records and the connections per instance, the publishing calls and the schedule
      slip of the producer, the end-to-end lag histogram of the consumer, the statistics of the connectors and the
      runtime. The tool prints `{*NetTestMetrics-<name>*} http://<host>:<port>/metrics` and warns when the host is not a
      loopback one, since the endpoint has no authentication and no TLS; a port in use ends the run with 1.
    * `Tools help NetTest` prints the usage with the line `spec_version nettest/1`, as `--help`, `-h` or `-help`
      anywhere in the arguments of `Tools NetTest` (except as the value of an option) does, also after an invalid
      argument.
    * The scheme of `Tools` is `ext:opt:sysprops,opt:dxprops,resource:dxlink.xml`, as in the Tools of the .NET API: the
      scheme properties of the endpoint builders and of the `-p` option, such as `dxscheme.nanoTime`, take effect. The
      dxLink example of the README and the `dxfeed.system.properties` of the samples show the same scheme.
    * The options `-Xmx<size>`, `-Xms<size>`, `-Xmn<size>`, `-Xss<size>` and `-XX:<option>` go to the Graal isolate
      when it is created, in the command line order, and the summary lists them in `config.isolate_arguments`. An
      option that the isolate rejects ends the tool with 2 and the line `NetTest: the isolate rejected one of the
      options: ...`; other `-X` arguments are command line errors.
    * Ctrl+C while the symbols load still writes the summary, with the status `stopped_by_signal`; a second Ctrl+C ends
      the process at once.
    * `--legacy`, the flags of the QD library (`/r`, `{len,max}`, `;<time>`) and the wildcard are not supported and end
      with `unsupported option` or `unsupported QD flag`.

* **\[MDAPI-476]\[C++]** Documentation: a troubleshooting guide and notes on the data model.
    * `TROUBLESHOOTING.md`: the first steps when there is no data (the `Diagnose` command of the Tools, the versions in
      the log, the QD log in a file), a table "log line or exception, likely cause, first check" for the connection
      errors, the causes of "no data", how to measure the delay of the events, and the reconnect contract of the C++
      API: what is restored after a drop (the connection, the subscriptions) and what is not guaranteed.
    * `DATA_MODEL.md`: conflated events versus `TimeAndSale`, `Trade` versus `TradeETH`, event flags and snapshots,
      `NaN` values, the precision of the time, order sources and scope, why an order book can be crossed, candle
      symbols.
    * Both pages are linked from the README and included in the Doxygen documentation.
    * Recipes: the samples for frequent tasks take an address and `--duration <seconds>`, stop by themselves and tell by
      the exit code whether they got data. `ReconnectSample` connects to a list of addresses and logs the state of the
      connectors with the last connection error, `SimpleOrderBookSample` shows the scope of each order and filters by
      scope, `ScheduleSample` tells whether the market is open at a time, `IpfLiveSample` also reads an IPF file, and
      `CandleSample` stops after the given time. The section "Recipes" of `TROUBLESHOOTING.md` lists the commands.

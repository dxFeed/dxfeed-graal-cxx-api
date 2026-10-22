* **\[MDAPI-452]\[C++]** Tools and samples:
    * `Connect` and `Dump`: the quiet option is `-q, --quiet` as in Java. The former `--quite` is still accepted.
    * `LatencyTest`: `--ignore-exchanges <list>` skips the events of the specified exchange codes (e.g. `Q,Z`):
      `Quote` by the bid or ask exchange, `Trade`, `TradeETH` and `TimeAndSale` by the exchange.
    * Added `CandleSample`: subscribes to `Candle` events of a symbol from a time, with the candle symbol attributes
      given as arguments.

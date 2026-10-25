* **\[MDAPI-459]\[C++]** Fixed `toString()` of the events to print as the Java API does. Only the text changes: the
  event data, publishing and receiving are not affected.
    * Numbers are printed as Java `Double.toString()` does: `1.0E-5`, `9.0E-4`, `1.2345678E7`, `2.5E8` (previously
      `1e-05`, `0.0009`, `12345678.0`, `250000000.0`).
    * An unset string field is printed as `null` rather than `<null>`: `ESC` and `optionSymbol` of `OptionSale`,
      `description` and `statusReason` of `Profile`, `marketMaker` of `Order`, `AnalyticOrder` and `OtcMarketsOrder`,
      `spreadSymbol` of `SpreadOrder`.
    * `marketMaker` and `spreadSymbol` are quoted: `marketMaker='NSDQ'`.
    * `Summary` prints `dayLow` without quotes.

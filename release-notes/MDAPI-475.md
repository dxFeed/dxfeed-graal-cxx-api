* **\[MDAPI-475]\[C++]** `CandleSymbol::valueOf` warns about the typical mistakes in a candle symbol, and the
  documentation has a page on how to build one.
    * A candle symbol string without a period (`AAPL`) gives tick candles: `CandleSymbol::valueOf(string)` and the
      `_c` literal report a warning through `ErrorManager` with the symbols to use instead (`AAPL{=1m}`, `AAPL{=d}`,
      or `AAPL{=t}` for tick candles); the default handler prints it to `stderr`. The symbol is created as before.
    * A period of the type `M` (`AAPL{=5M}`) is a minute, a month is `mo`; `PR` is a price, `pr` a Renko price:
      such a period gets a warning too.
    * A warning is reported once per string, for the first 1000 strings in the process. The symbols of the received
      candles, the overloads of `CandleSymbol::valueOf` with attributes and a symbol with the `gr` attribute get none.
    * The page "Candle symbols" of the documentation describes the period, the price, the session, the alignment, the
      exchange, the price level, the data type and the typical mistakes.

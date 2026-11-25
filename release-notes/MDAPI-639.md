* **\[MDAPI-639]\[C++]** Fixed a build with AddressSanitizer by MSVC 19.52: `CandleSymbol::valueOf` gave a broken
  symbol instead of throwing `InvalidArgumentException` for a price, period, alignment or price level attribute that
  cannot be parsed.

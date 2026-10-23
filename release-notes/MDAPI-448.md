* **\[MDAPI-448]\[C++]** **\[BREAKING]** Added the `CandleDataType` candle attribute (the `dt` key) and
  `CandleSymbol::getDataType()`, as in the Java API.
    * The constants are `SOURCE`, `THO`, `ALL`, `RTH`, `ETH` and `UNDEFINED` (`DEFAULT`). Any other string is a valid
      data type: `CandleDataType::valueOf()` and `parse()` never throw, and the value is case-sensitive. Such a data
      type refers to a copy of its value that the library keeps until the program exits (one per distinct value).
    * `CandleSymbol` normalization removes the empty data type (`A{dt=}` gives `A`) and keeps it when removing it would
      change the base symbol (`A{=t}{dt=}`; Java gives `A{=t}`), as for the other attributes.
    * **\[BREAKING]** `CandleSymbolAttributeVariant` has the new alternative `CandleDataType`: a `std::visit` with an
      overload for each alternative needs one for it.
    * **\[BREAKING]** The layout of `CandleSymbol` changed (ABI).

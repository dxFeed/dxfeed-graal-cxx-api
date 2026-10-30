* **\[MDAPI-431]\[C++]** **\[BREAKING]** Event fields, defaults and `toString()` as in Java.
    * **\[BREAKING]** Added `getTradeId()` and `setTradeId()` to `Trade`, `TradeETH`, `TimeAndSale` (and so to
      `NuamTrade` and `NuamTimeAndSale`). The trade id is transferred to and from the native SDK (it was lost) and is
      printed by `toString()` as in Java. The layout of these classes has changed, so code that uses them must be
      recompiled.
    * The default day volume and day turnover of `Trade`, `TradeETH` and `NuamTrade` are `NaN`, as in Java (they
      were 0).
    * **\[BREAKING]** The text of `TextMessage` is null by default, as in Java (it was an empty string), and a null text
      received from the native SDK stays null instead of becoming the text `"<null>"`. `getText()` returns `"<null>"`
      for a null text; the new `getTextOpt()` returns `std::nullopt`. `toString()` prints the text in quotes, as in
      Java (`text='null'`). The layout of `TextMessage` has changed, so code that uses it must be recompiled.
    * **\[BREAKING]** The text `"<null>"` in a string field of an event, including the event symbol, is passed to the
      native SDK as text, as in Java (it was passed as null). Only an unset field is passed as null, and a null event
      symbol received from the native SDK stays null. `getX()` still returns `"<null>"` for a null field; `getXOpt()`
      tells null from the text.
    * The string setters of events (except `setEventSymbol()`) and their `with*()` methods also take
      `std::optional<std::string>` and `std::nullopt`, which sets null; `setX(other.getXOpt())` copies a field, null
      included.
    * Exchange codes from 0x80 to 0xFF set as `char` are kept as Latin-1 characters, as in Java (they became 0).
    * `System::setProperty()` and `System::getProperty()` cut a key or a value that is not valid UTF-8 to its longest
      valid prefix (the native SDK replaced the invalid bytes with U+FFFD).
    * `OptionSeries::toString()` prints the expiration and the last trade day as `yyyyMMdd` and the multiplier and
      the SPC as Java does (`1.0E7`); they were printed as day ids and in the C++ notation.
    * `day_util::getYearMonthDayByDayId()` and `day_util::getDayIdByYearMonthDay()` give the results of Java for any
      `int32` value, the limits included (an overflow was undefined behaviour). `day_util::getDayIdByYearMonthDay()` is
      exported from the shared library (code linked to it could not call the function).
    * `Profile::toString()` prints `shares` and `freeFloat` as Java does (`shares=1E7, freeFloat=0.0009`), and a
      negative `index` of `OptionSale` and `Series` is printed as an unsigned hexadecimal number
      (`index=0xffffffffffffffff`), as in Java.

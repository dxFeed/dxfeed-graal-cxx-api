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

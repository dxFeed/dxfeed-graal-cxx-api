* **\[MDAPI-473]\[C++]** `MarketDepthModel` can warn when its book is crossed, and its documentation explains why
  books cross.
    * `MarketDepthModel::Builder::withCrossedBookWarning()` enables a check after each transaction: when the best buy
      order is above the best sell order, a warning is reported through `ErrorManager` with the symbol, the sources
      and both prices. Equal prices are not reported. The check is off by default.
    * A model reports at most one warning per minute (the second parameter changes the period, `0` reports every
      crossing); the next warning tells how many transactions with a crossed book were since the previous one.
    * The section "Crossed book" of the `MarketDepthModel` documentation lists the usual causes (lost packets,
      buffer overflow, conflation, one-sided or combined sources) and states that the model shows what the endpoint
      received.

* **\[MDAPI-486]\[C++]** Samples are fixed:
    * `OptionChainSample` prints the put bid and ask under their own headers (they were swapped).
    * `QuoteAndTradeSample` prints that a quote did not arrive instead of crashing on a null result.
    * `MultipleMarketDepthSample` stops at the end of the input instead of printing the book in a loop.
    * `PriceLevelBookSample` removes all the levels of a cleared source (it skipped some and could increment the end
      iterator).
    * `MarketDepthModelSample` and `PriceLevelBookSample` show the bids under "BID" and the asks under "ASK".
    * `MarketDepthModelSample` hides the cursor while drawing the book.

* **\[MDAPI-646]\[C++]** Fewer heap allocations in conversions of lists:
    * `PriceLevelChecker::validate` converts the orders to the native list in one arena.
    * Lists of symbols and event sources from the native library reserve their size, and the symbols of
      `ObservableSubscriptionChangeListener` are moved into a set sized for them.

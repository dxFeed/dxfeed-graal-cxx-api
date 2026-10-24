* **\[MDAPI-256]\[C++]** Added the `OrderImbalance` event of QDS 3.356 (experimental, as in the Java API): the
  imbalance between buy and sell orders during auctions, with the reference, near and far prices, the paired and
  imbalance sizes, `ImbalanceSide` and `AuctionType` (`EventTypeEnum::ORDER_IMBALANCE`).
    * The event has an order source (`getSource()`): `OrderSource::DEFAULT`, `NTV`, `ARCA` and `NUAM` publish it. A
      subscription by symbol receives the events of all sources. The event is not indexed, as in the Java API.
    * `OrderSource::isPublishable(EventTypeEnum::ORDER_IMBALANCE)` and
      `OrderSource::publishable(EventTypeEnum::ORDER_IMBALANCE)` no longer throw.
    * `toString()` prints the event as the Java API does.
* **\[MDAPI-256]\[C++]** Added the `NuamOrder`, `NuamTrade` and `NuamTimeAndSale` events of QDS 3.356
  (experimental, as in the Java API) for the NUAM exchange, with the enums `NuamOrderType` and `NuamTimeInForceType`
  (`EventTypeEnum::NUAM_ORDER`, `NUAM_TRADE`, `NUAM_TIME_AND_SALE`).
    * `NuamOrder` extends `Order` with the actor, participant and submitter ids, the client order id, the customer
      account and info, the exchange info, the time in force and its data, the trigger order book id, price and session
      type, the order type and the order, display, refresh, leaves and matched quantities. The strings are `null` until
      set (`getClientOrderIdOpt()` and the like). `OrderSource::DEFAULT`, `NUAM` and `nuam` publish the event:
      `OrderSource::isPublishable(EventTypeEnum::NUAM_ORDER)` and `OrderSource::publishable(EventTypeEnum::NUAM_ORDER)`
      no longer throw and answer these three sources.
    * `NuamTrade` extends `Trade` with the trade stat time, the last significant price, the last price for all, the
      number of trades and the VWAP; `NuamTimeAndSale` extends `TimeAndSale` with the match id. Code that handles
      `Trade` and `TimeAndSale` handles them too (`is<Trade>()`, `std::dynamic_pointer_cast<Trade>()`), so `Trade` and
      `TimeAndSale` are no longer `final`.
    * `toString()` prints the events as the Java API does, except for what comes from the C++ base classes: there is no
      `tradeId` (the C++ `Trade` and `TimeAndSale` do not have it), and the inherited numbers keep the format of
      `Order`, `Trade` and `TimeAndSale` (`250000000.0` where Java prints `2.5E8`).
* **\[MDAPI-256]\[C++]** `TimeAndSale::toString()` prints unset values as the Java API does: `ESC='null'` (was
  `ESC='<null>'`), and `buyer` and `seller` only when they are set, an empty string included (it printed
  `buyer='<null>'` for an unset buyer and skipped an empty one).
* **\[MDAPI-256]\[C++]** Added the `MarketMaker` and `TextConfiguration` events of QDS 3.356
  (`EventTypeEnum::MARKET_MAKER`, `TEXT_CONFIGURATION`).
    * `MarketMaker` is the bid and ask of a market maker: the bid and ask times, prices, sizes and counts. The index
      packs the exchange code and the market maker id, a QD short string that keeps the last 4 characters up to U+00FF
      (`getMarketMakerOpt()` is `std::nullopt` for an unset or empty id); `setMarketMaker()` throws
      `InvalidArgumentException` for more than 8 characters, a character above U+00FF or invalid UTF-8, as the Java API
      does. The default scheme transfers the bid and ask times in seconds.
    * `TextConfiguration` is a lasting event with a text, a version, time and sequence. The text is `null` until set
      (`getTextOpt()`).
    * `toString()` prints the events as the Java API does.
* **\[MDAPI-256]\[C++]** `toString()` of the events prints an exchange code from U+8000 to U+FFFF as `\uXXXX`, as the
  Java API does (it printed a negative number).

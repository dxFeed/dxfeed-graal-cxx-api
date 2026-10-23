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

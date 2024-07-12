
* **\[MDAPI-26]\[C++]** Migrate to Graal SDK v1.1.21.
  * Added `Day::getSessions` method.
  * New order sources added: `CEDX` and `cedx`.
  * Added the ability to use dxFeed Graal SDK stub. In other words, now API can be built for any 32 and 64 platforms.
  * Added `TimePeriod` class.
  * Added `DXFeedSubscription::getAggregationPeriod` and `DXFeedSubscription::setAggregationPeriod` methods.
  * Added `DXFeedSubscription::getEventsBatchLimit` and `DXFeedSubscription::setEventsBatchLimit` methods.
  * Added `AuthToken` class.
  * Added `InstrumentProfileReader::readFromFile(address, token)` method.

## v2.0.0
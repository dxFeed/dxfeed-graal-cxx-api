* **\[MDAPI-113]\[C++]\[Tools]** Tools should report invalid event type
  *  Added classes: `RuntimeException`, `InvalidArgumentException`
  *  `InvalidArgumentException`, `GraalException`, `JavaException` are now descendants of the `RuntimeException` class, which can collect stacktrace.
  *  Now an `InvalidArgumentException` exception is thrown instead of the `std::invalid_argument` exception. 
  *  `Tools` now reports incorrect event types specified by the user.
* **\[MDAPI-80]\[C++]\[IPF]** Implement custom fields in InstrumentProfile
  * The API was migrated to Graal SDK v1.1.22
  * Added methods:
    * `InstrumentProfile::getField`
    * `InstrumentProfile::setField`
    * `InstrumentProfile::getNumericField`
    * `InstrumentProfile::setNumericField`
    * `InstrumentProfile::getDateField`
    * `InstrumentProfile::setDateField`
    * `InstrumentProfile::getNonEmptyCustomFieldNames`
  * **\[BREAKING]**: All `toString` methods can now throw exceptions.
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
* **\[MDAPI-599]\[C++]** The Graal Native SDK 3.18.0 is the minimum version.
    * `MessageConnectorInfo::ioStatistics` has the I/O statistics of a message connector (`IoStatistics`): the data
      and subscription records read and written, the bytes of the QTP messages, the sums of the lags and of the
      round-trip times of the records in microseconds, and the counts of the open and the closed connections. It is
      `std::nullopt` with a library that does not support them.
    * The SDK deserializes only the classes of an allow-list (the dxFeed and QD classes, the JDK collections, boxed
      values and exceptions): in the attachments of `Message`, the object fields of the records and the results of
      `PriceLevelService`. An attachment of another class reads as `std::nullopt`, a result of `PriceLevelService` of
      another class fails the call.

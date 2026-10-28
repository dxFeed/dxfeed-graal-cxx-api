* **\[MDAPI-467]\[C++]** `InstrumentProfileCollector::addUpdateListener()` throws `InvalidArgumentException` when the
  listener ids of the collector are exhausted (about 2^32 listeners added on a 64-bit platform). Before, the
  notifications of such a listener silently went to another listener.

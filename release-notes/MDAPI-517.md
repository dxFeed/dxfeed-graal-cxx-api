* **\[MDAPI-517]\[C++]** Instrument profiles, `InstrumentProfileReader`, `InstrumentProfileCollector`,
  `InstrumentProfileConnection`, promises (`Promise`, `Promises::allOf`) and the transactional models
  (`IndexedTxModel`, `TimeSeriesTxModel`, `IndexedEventTxModel`, `MarketDepthModel`) call the `dxfg_v4_` functions of
  the Graal Native SDK instead of the deprecated `dxfg_` ones.
  * The library checks the version of the Graal Native SDK library when the GraalVM isolate is created: with a library
    older than 3.15.0 the creation fails with the code 13 (`ISOLATE_INITIALIZATION_FAILED`), and `GraalException`
    names the found and the required versions. A library older than 3.13.0 lacks the `dxfg_v4_` functions and fails
    to load before the check.
  * The transactional models use the experimental functions of the SDK: `newBuilder()` and the creation of a model
    listener throw `RuntimeException` with both revisions when the SDK library has another advanced revision than the
    one the library is built for.

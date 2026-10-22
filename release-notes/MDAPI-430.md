* **\[MDAPI-430]\[C++]** **\[BREAKING]** Candle attributes are parsed and printed as in Java.
    * **\[BREAKING]** `CandlePeriod::parse()` and `CandlePriceLevel::parse()` follow Java `Double.parseDouble`: a sign
      (`+5m`), the `d`/`f` suffix and hexadecimal values (`0x1p3`) of a price level are accepted, and parsing no longer
      depends on the C locale. An invalid number throws `InvalidArgumentException`.
    * `CandlePeriod::toString()` and `CandlePriceLevel::toString()` print values as Java does (`1.0E19m`, `1.0E-5m`,
      `NaNm`, `Infinitym`). Values beyond the range of `std::int64_t` previously printed as wrong numbers.
    * `CandlePeriod::getPeriodIntervalMillis()` saturates as the Java `(long)` cast does (NaN gives 0) instead of
      causing undefined behaviour for large periods such as `1e30m`.
    * Fixed a data race in `CandlePeriod::toString()`: the string is computed on each call instead of being cached.
    * **\[BREAKING]** `CandlePeriod::valueOf()` and the `CandlePeriod` default constructor are no longer `noexcept`.
    * `CandleSession()` is `CandleSession::ANY`: comparing a default-constructed `CandleSession` dereferenced a null
      pointer.
    * `CandleSymbol` normalization is idempotent: a default attribute is kept when removing it would leave a base symbol
      that itself ends with attributes (`A{=t}{=t}`, `A{=t}{=1t}` gives `A{=t}{=t}`). The session attribute is normalized
      as it is read (Java `Boolean.parseBoolean`): `A{tho=t}` gives `A` (any session) instead of `A{tho=true}`.
      Unlike Java, normalization still removes the default attributes (`IBM{=t}` gives `IBM`).
    * **\[BREAKING]** Copying and moving a `CandleSymbol` copy its fields instead of parsing the symbol again; these
      operations are no longer `noexcept`.
    * `RuntimeException` (and so every exception of the API) formats its stacktrace on the first `getStackTrace()` call
      instead of when it is thrown: with `DXFCXX_FEATURE_STACKTRACE` each exception resolved the names of all frames
      (on Linux by running `addr2line`), which made each throw slow (up to tens of seconds in a Debug build with
      ASan). The copies of an exception share the stacktrace.
    * A stacktrace frame without a path (`??:0`, a relative file name) prints its location instead of the whole frame.

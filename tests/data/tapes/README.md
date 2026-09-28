# Golden tapes

QD text tapes that the tests use as the reference for the events of all types. The text format is QD's own (the
format of `tape:<file>[format=text]` and `file:<file>`), so it does not depend on the C++ API. The Java API reads and
writes these files the same way.

| File | Content | Used by |
|---|---|---|
| `all-events.txt` | One event of each of the 18 event types, every transferred field has its own value (the events of `test::createTapeEvents()`, plus the Java-only fields: `TradeId` of `Trade`, `TradeETH`, `TimeAndSale` and the `Message` attachment) | `tests/event/TapeRoundTripTest.cpp` |
| `defaults.txt` | Default-constructed events of the 18 types, written by the Java API | `tests/event/TapeRoundTripTest.cpp` |
| `edge-values.txt` | The edge values of `test::createEdgeEvents()` (NaN, ±0.0, infinities, number limits, special strings, all enum values, all 256 values of the event flags byte), written by the Java API from the same values | `tests/event/EdgeValuesTapeTest.cpp` |
| `edge-values-round-trip.txt` | `edge-values.txt` read by the Java API (`STREAM_FEED`) and written back (`PUBLISHER`): QD does not read back every value it writes, e.g. the day ids of the int32 limits become 0 | `tests/event/EdgeValuesTapeTest.cpp` |

## Format

- `==DXP3 ...` is the header, `=<Record>` lines list the columns of a record, the other lines are data.
- Times are `yyyyMMdd-HHmmss[.SSS]` with an explicit UTC offset. The files use `+0000`; a tape written on a machine in
  another time zone has a different offset but the same instants, so the tests compare instants, not text.
- `Sequence` of time series events packs `<milliseconds>:<sequence>`; `Flags` packs the enum and boolean fields.
- `\NULL` is a null string, `\0` is an empty exchange code.
- `EventTime` is written because the tests enable `dxendpoint.eventTime`.
- Event flags follow the columns as `EventFlags=TX_PENDING,SNAPSHOT_END`.
- QD writes the UTC offset in whole minutes. For times before a zone switched to standard time (local mean time with
  seconds, e.g. `+02:30:17` in Europe/Moscow until 1919) a tape written in that zone is up to 59 seconds off, so the
  tests compare such times with this tolerance.

## Edge values

The edge-value tapes are written by the Java API, so they show what Java passes to QD, including what QD changes:
- the QD short strings (the market maker of `Order#NTV` and `OtcMarketsOrder#pink`, the sale conditions of
  `TimeAndSale` and `OptionSale`) keep the last 4 characters, although the publisher accepts 8 (`" spaces "` is
  written as `"ces "`, `<null>` as `ull>`);
- the event flags `0x20` and `REMOVE_SYMBOL` (`0x80`) are not written;
- times are stored in whole seconds of an int32 (`Long.MAX_VALUE` is `20380119-031407`), day ids as `yyyyMMdd`
  (the int32 limits give numbers that QD does not read back).

The values that the Java API rejects on publishing are not in the tapes (the tests check that C++ rejects them too):
an order with the UNDEFINED side that is not empty, QD short strings (market maker, sale conditions) of more than
8 characters or with characters above U+00FF.

## Updating

Edit the files by hand. When a value or a column changes, keep `test::createTapeEvents()` and
`test::createDefaultEvents()` in sync. The files were checked against the Java API (QD 3.355, the version in Graal SDK
3.5.0): Java reads `all-events.txt` as the intended events (including the packed flags) and writes `defaults.txt`
for default-constructed events. A new QD version needs no change unless QD changes the tape format or the scheme.

`edge-values.txt` and `edge-values-round-trip.txt` are regenerated with the Java API when `test::createEdgeEvents()`
changes: a Java program sets the same values on the Java events and writes them with a `PUBLISHER` to
`tape:edge-values.txt[format=text]`, then reads that file with a `STREAM_FEED` (`file:...[speed=max]`) and writes the
events back to `tape:edge-values-round-trip.txt[format=text]`. Run it with `-Duser.timezone=UTC` and convert the line
endings to LF (on Windows Java writes CRLF).

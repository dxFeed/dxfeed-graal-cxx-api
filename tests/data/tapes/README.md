# Golden tapes

QD text tapes that the tests use as the reference for the events of all types. The text format is QD's own (the
format of `tape:<file>[format=text]` and `file:<file>`), so it does not depend on the C++ API. The Java API reads and
writes these files the same way.

| File | Content | Used by |
|---|---|---|
| `all-events.txt` | One event of each of the 18 event types, every transferred field has its own value (the events of `test::createTapeEvents()`, plus the Java-only fields: `TradeId` of `Trade`, `TradeETH`, `TimeAndSale` and the `Message` attachment) | `tests/event/TapeRoundTripTest.cpp` |
| `defaults.txt` | Default-constructed events of the 18 types, written by the Java API | `tests/event/TapeRoundTripTest.cpp` |

## Format

- `==DXP3 ...` is the header, `=<Record>` lines list the columns of a record, the other lines are data.
- Times are `yyyyMMdd-HHmmss[.SSS]` with an explicit UTC offset. The files use `+0000`; a tape written on a machine in
  another time zone has a different offset but the same instants, so the tests compare instants, not text.
- `Sequence` of time series events packs `<milliseconds>:<sequence>`; `Flags` packs the enum and boolean fields.
- `\NULL` is a null string, `\0` is an empty exchange code.
- `EventTime` is written because the tests enable `dxendpoint.eventTime`.

## Updating

Edit the files by hand. When a value or a column changes, keep `test::createTapeEvents()` and
`test::createDefaultEvents()` in sync. The files were checked against the Java API (QD 3.355, the version in Graal SDK
3.5.0): Java reads `all-events.txt` as the intended events (including the packed flags) and writes `defaults.txt`
for default-constructed events. A new QD version needs no change unless QD changes the tape format or the scheme.

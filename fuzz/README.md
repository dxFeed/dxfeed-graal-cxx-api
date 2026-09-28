# Fuzz targets

libFuzzer targets for the parts of the library that do not call the Graal isolate: parsers, string and time utilities,
packed event fields. Each target checks that the code does not crash, has no memory errors or undefined behavior (with
ASan and UBSan) and, where Java defines it, that parsing the result of `toString()` gives the same value.

| Target | What it fuzzes |
|---|---|
| `fuzz_candle_symbol` | `CandleSymbol::valueOf`, all getters, `valueOf(toString())` is the same symbol |
| `fuzz_candle_attrs` | `CandlePeriod`, `CandlePriceLevel`, `CandleType`, `CandlePrice`, `CandleAlignment` parsers and their round trips |
| `fuzz_market_event_symbols` | `MarketEventSymbols` (exchange code, base symbol, attributes); input: `[exchange][symbol]\0[key]\0[value]\0[new base]` |
| `fuzz_string_utils` | string, UTF-8/UTF-16 and time formatting utilities |
| `fuzz_cmd_args` | `CmdArgsUtils::parseProperties`, `CmdArgsUtils::parseEventSources` |
| `fuzz_order_source` | `OrderSource::valueOf(name)`, `OrderSource::valueOf(id)` and the name-id codec |
| `fuzz_order_fields` | packed fields of `Order` (index and source, time and sequence, exchange code) and `day_util` |

`corpus/<target>/` holds the seed inputs.

## Build

LLVM Clang only (libFuzzer is not shipped with GCC, MSVC or AppleClang):

```sh
cmake -S . -B build-fuzz -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
  -DDXFCXX_BUILD_FUZZERS=ON -DDXFCXX_ENABLE_ASAN=ON -DDXFCXX_ENABLE_UBSAN=ON \
  -DDXFCXX_BUILD_UNIT_TESTS=OFF -DDXFCXX_BUILD_SAMPLES=OFF -DDXFCXX_BUILD_TOOLS=OFF -DDXFCXX_BUILD_DOC=OFF
cmake --build build-fuzz
```

`DXFCXX_BUILD_FUZZERS` instruments the static library for coverage (`-fsanitize=fuzzer-no-link`) and builds the
targets in `build-fuzz/fuzz/`.

## Run

```sh
mkdir -p corpus-work/fuzz_candle_symbol
build-fuzz/fuzz/fuzz_candle_symbol corpus-work/fuzz_candle_symbol fuzz/corpus/candle_symbol -max_total_time=60 -max_len=256
```

The first directory is the working corpus (new inputs are added there), the others are read. A failing input is saved
as `crash-<hash>` in the current directory; run the target with that file as the only argument to reproduce it.

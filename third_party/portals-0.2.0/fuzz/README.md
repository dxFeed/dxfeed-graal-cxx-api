# Bounded input and text fuzzing

These optional Clang/libFuzzer targets exercise the production input decoder and
Unicode text code under AddressSanitizer and UndefinedBehaviorSanitizer. Core
and vendored Unicode objects receive coverage and sanitizer instrumentation too.
The default build is unchanged. No target opens a terminal or clipboard.

From a Linux shell with Clang 20 and a C++23 standard library:

```sh
cmake -S . -B build/fuzz -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_C_COMPILER=clang-20 -DCMAKE_CXX_COMPILER=clang++-20 \
  -DPORTALS_BUILD_FUZZERS=ON -DPORTALS_BUILD_TESTS=OFF \
  -DPORTALS_BUILD_EXAMPLES=OFF -DPORTALS_BUILD_TOOLS=OFF
cmake --build build/fuzz --target portals_fuzz_input portals_fuzz_text
mkdir -p build/fuzz/corpus-input build/fuzz/corpus-text build/fuzz/artifacts
cp fuzz/corpus/input/* build/fuzz/corpus-input/
cp fuzz/corpus/text/* build/fuzz/corpus-text/
export ASAN_OPTIONS=detect_leaks=1:abort_on_error=1
export UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1
build/fuzz/fuzz/portals_fuzz_input build/fuzz/corpus-input \
  -dict=fuzz/input.dict -max_len=4096 -max_total_time=60 -timeout=5 \
  -rss_limit_mb=1024 -seed=211 -artifact_prefix=build/fuzz/artifacts/
build/fuzz/fuzz/portals_fuzz_text build/fuzz/corpus-text \
  -max_len=4096 -max_total_time=60 -timeout=5 -rss_limit_mb=1024 \
  -seed=212 -artifact_prefix=build/fuzz/artifacts/
```

Use writable build corpora: libFuzzer extends them during a run. Preserve its
complete output (compiler/version, command, seed, executions, coverage, corpus,
and any failure artifact). A fixed seed helps reproduction but does not make a
time-limited campaign identical across machines. Reproduce a failure by passing
the saved artifact path directly to the same executable. Minimize it before
adding a regression; do not weaken assertions to make a campaign pass.

`input` varies bounded decoder options and compares complete owning event
sequences from whole, single-byte and 37-byte chunks at identical timestamps
and reply-window schedules. It checks consumed prefixes, issue offsets,
bounded progress and finish/reset ownership. It covers both immediate finish
and an expired deadline. This is not an exhaustive clock-schedule search.

`text` validates grapheme coverage, cursor boundaries at every byte, roundtrips
at cluster boundaries, cell totals and clipping against a simple linear oracle.
Both width policies and bounded text construction are exercised. This checks
internal consistency, not Unicode conformance against a second Unicode engine.

Inputs above 4096 bytes are intentionally ignored by both harnesses. Successful
runs establish evidence for the sampled inputs, not the absence of all bugs.
Model/lifecycle, renderer and full application fuzz campaigns are separate work.
See the primary [libFuzzer documentation](https://llvm.org/docs/LibFuzzer.html).

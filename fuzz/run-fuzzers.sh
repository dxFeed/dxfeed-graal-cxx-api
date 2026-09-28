#!/usr/bin/env bash
# Copyright (c) 2026 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0

# Runs every fuzz target of a build directory for the given time, in report mode: a failing input does not stop the
# run and does not fail the script (the targets still find known defects that are not fixed yet); the script fails only
# when a target cannot be run. Writes a Markdown summary to stdout.
#
# Usage: run-fuzzers.sh <directory with fuzz_* executables> <seconds per target> <working corpus dir> <artifacts dir>
#   The working corpus keeps the inputs found by earlier runs (<working corpus dir>/<target>); the seed inputs of
#   fuzz/corpus/<name> are read too. Failing inputs and logs go to <artifacts dir>/<target>/ and <artifacts dir>/<target>.log.
set -u

BIN_DIR=$1
SECONDS_PER_TARGET=$2
CORPUS_DIR=$3
ARTIFACTS_DIR=$4
SEED_DIR=$(cd "$(dirname "$0")" && pwd)/corpus

export ASAN_OPTIONS=${ASAN_OPTIONS:-detect_leaks=1}
export UBSAN_OPTIONS=${UBSAN_OPTIONS:-print_stacktrace=1:halt_on_error=1}

status=0

echo "### Fuzzing: ${SECONDS_PER_TARGET} s per target"
echo
echo "| Target | Failing inputs | Leaks, timeouts, OOMs | Corpus |"
echo "|---|---:|---:|---:|"

for target in "$BIN_DIR"/fuzz_*; do
    [ -x "$target" ] || continue

    name=$(basename "$target")
    log=$ARTIFACTS_DIR/$name.log

    mkdir -p "$CORPUS_DIR/$name" "$ARTIFACTS_DIR/$name"

    # -fork runs the inputs in child processes, so that a failing input is saved and fuzzing goes on
    # (-ignore_crashes); the time limit of the parent covers the whole run.
    timeout $((SECONDS_PER_TARGET + 300)) "$target" "$CORPUS_DIR/$name" "$SEED_DIR/${name#fuzz_}" \
        -max_total_time="$SECONDS_PER_TARGET" -max_len=256 -timeout=10 -rss_limit_mb=2048 \
        -fork=2 -ignore_crashes=1 -ignore_ooms=1 -ignore_timeouts=1 \
        -artifact_prefix="$ARTIFACTS_DIR/$name/" > "$log" 2>&1

    failing=$(find "$ARTIFACTS_DIR/$name" -maxdepth 1 -name 'crash-*' | wc -l)
    others=$(find "$ARTIFACTS_DIR/$name" -maxdepth 1 \( -name 'leak-*' -o -name 'timeout-*' -o -name 'oom-*' \) | wc -l)
    corpus=$(find "$CORPUS_DIR/$name" -maxdepth 1 -type f | wc -l)

    # The fork mode prints this line when it has started to fuzz; without it the target did not run.
    if grep -q 'starting to fuzz' "$log"; then
        echo "| \`$name\` | $failing | $others | $corpus |"
    else
        echo "| \`$name\` | **did not run** (see \`$name.log\`) | | |"
        status=1
    fi
done

exit $status

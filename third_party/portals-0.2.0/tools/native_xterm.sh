#!/bin/sh
# Copyright (c) 2026 ttldtor.
# SPDX-License-Identifier: BSL-1.0
# Usage: native_xterm.sh XTERM SUPERVISOR LOGDIR DEADLINE_MS EXPECTED_EXIT DRIVE QUIT_MS CHILD [ARGS...]
set -eu
if [ "$#" -lt 8 ]; then exit 2; fi
xterm=$1 supervisor=$2 logdir=$3 deadline=$4 expected=$5 drive=$6 quit=$7
shift 7
case "$drive" in none|dashboard) ;; *) exit 2 ;; esac
# Automated runs use a private server; visible none runs require explicit opt-in.
visible=${PORTALS_NATIVE_VISIBLE:-0}
case "$visible" in 0|1) ;; *) exit 2 ;; esac
if [ "$visible" = 1 ] && [ "$drive" != none ]; then exit 2; fi
case "$deadline" in ''|*[!0-9]*) exit 2 ;; esac
mkdir "$logdir"
logdir=$(realpath "$logdir")
title="portals-native-$(date +%s)-$$"
terminalPid= serverPid= privateDir=
forced=0

# Signal only exact PIDs created by this invocation. A forced termination is
# failure evidence even when cleanup subsequently succeeds.
stopPid() {
  ownPid=$1
  if ! kill -0 "$ownPid" 2>/dev/null; then return; fi
  kill -TERM "$ownPid" 2>/dev/null || true
  attempts=0
  while kill -0 "$ownPid" 2>/dev/null && [ "$attempts" -lt 20 ]; do
    sleep 0.05
    attempts=$((attempts + 1))
  done
  if kill -0 "$ownPid" 2>/dev/null; then
    kill -KILL "$ownPid" 2>/dev/null || true
  fi
  wait "$ownPid" 2>/dev/null || true
}

cleanup() {
  if [ -f "$logdir/supervisor.log" ]; then
    for ownPid in $(sed -n 's/^\(harness\|child\)_pid=\([0-9]*\).*$/\2/p' "$logdir/supervisor.log"); do
      stopPid "$ownPid"
    done
  fi
  if [ -n "$terminalPid" ]; then stopPid "$terminalPid"; fi
  if [ -n "$serverPid" ]; then
    stopPid "$serverPid"
    if kill -0 "$serverPid" 2>/dev/null; then return 1; fi
    printf 'xvfb_gone=1\n' >> "$logdir/launch.log"
  fi
  if [ -n "$privateDir" ]; then
    # Remove only this mktemp directory's two known private files.
    rm -f "$privateDir/auth" "$privateDir/display" || return 1
    rmdir "$privateDir" || return 1
    printf 'private_auth_removed=1\n' >> "$logdir/launch.log"
  fi
}
trap 'code=$?; trap - 0; cleanup || code=1; exit "$code"' 0
trap 'exit 1' HUP INT TERM

sha256sum "$xterm" "$supervisor" "$1" > "$logdir/hashes.txt"
printf 'title=%s\nparent_DISPLAY=%s\nTERM=%s\nexpected_child_exit=%s\n' "$title" "${DISPLAY-}" "${TERM-}" "$expected" > "$logdir/launch.log"
printf 'visible_opt_in=%s\n' "$visible" >> "$logdir/launch.log"
if [ "$visible" = 0 ]; then
  # Automated runs must never open windows on the caller's X server. Select a
  # free nonzero display and require readiness from our exact server process.
  xvfb=${PORTALS_XVFB:-Xvfb}
  command -v "$xvfb" >/dev/null
  command -v xauth >/dev/null
  privateDir=$(mktemp -d "${TMPDIR:-/tmp}/portals-native-x11.XXXXXX")
  chmod 700 "$privateDir"
  XAUTHORITY=$privateDir/auth
  export XAUTHORITY
  : > "$XAUTHORITY"
  chmod 600 "$XAUTHORITY"
  cookie=$(od -An -N16 -tx1 /dev/urandom | tr -d ' \n')
  candidate=$((100 + $(od -An -N2 -tu2 /dev/urandom) % 60000))
  serverDeadline=$(($(date +%s) + 10))
  ready=0
  while [ "$(date +%s)" -lt "$serverDeadline" ]; do
    DISPLAY=:$candidate
    candidate=$((candidate + 1))
    if [ -e "/tmp/.X${DISPLAY#:}-lock" ] || [ -S "/tmp/.X11-unix/X${DISPLAY#:}" ]; then continue; fi
    export DISPLAY
    xauth -f "$XAUTHORITY" add "$DISPLAY" . "$cookie"
    : > "$privateDir/display"
    (
      if [ -n "${PORTALS_XVFB_WORKDIR-}" ]; then cd "$PORTALS_XVFB_WORKDIR"; fi
      exec "$xvfb" "$DISPLAY" -auth "$XAUTHORITY" -nolisten tcp -screen 0 1280x800x24 \
        -displayfd 3 3> "$privateDir/display"
    ) >> "$logdir/xvfb.stdout" 2>> "$logdir/xvfb.stderr" &
    serverPid=$!
    while kill -0 "$serverPid" 2>/dev/null && [ ! -s "$privateDir/display" ] && \
          [ "$(date +%s)" -lt "$serverDeadline" ]; do sleep 0.05; done
    if kill -0 "$serverPid" 2>/dev/null && [ "$(cat "$privateDir/display")" = "${DISPLAY#:}" ]; then
      ready=1
      break
    fi
    stopPid "$serverPid"
    serverPid=
  done
  if [ "$ready" -ne 1 ]; then exit 1; fi
  PORTALS_ISOLATED_X11_DISPLAY=$DISPLAY
  PORTALS_ISOLATED_X11_TITLE=$title
  export PORTALS_ISOLATED_X11_DISPLAY PORTALS_ISOLATED_X11_TITLE
  printf 'xvfb_pid=%s\nprivate_auth=%s\n' "$serverPid" "$XAUTHORITY" >> "$logdir/launch.log"
fi
printf 'DISPLAY=%s\n' "${DISPLAY-}" >> "$logdir/launch.log"
# Each string action emits one byte: space, CJK, combining acute, and emoji.
# The translation belongs to this child instance, not the X server keymap.
unicode_translation='XTerm*VT100.translations: #override <Key>F8: string(0x20) string(0xe7) string(0x95) string(0x8c) string(0x20) string(0x65) string(0xcc) string(0x81) string(0x20) string(0xf0) string(0x9f) string(0x99) string(0x82)'
"$xterm" -iconic -u8 -geometry 120x40 -title "$title" \
  -xrm "$unicode_translation" \
  -e "$supervisor" --log "$logdir/supervisor.log" --title "$title" --deadline-ms "$deadline" \
  --quit-ms "$quit" --expected-exit "$expected" --drive "$drive" -- "$@" \
  > "$logdir/xterm.stdout" 2> "$logdir/xterm.stderr" &
terminalPid=$!
printf 'terminal_pid=%s\n' "$terminalPid" >> "$logdir/launch.log"
limit=$(( ($(date +%s) * 1000 + deadline + 12000) / 1000 ))
while kill -0 "$terminalPid" 2>/dev/null; do
  if [ "$(date +%s)" -ge "$limit" ]; then
    forced=1
    stopPid "$terminalPid"
    break
  fi
  sleep 0.1
done
wait "$terminalPid" || true
alive=0
if [ -f "$logdir/supervisor.log" ]; then
  for ownPid in $(sed -n 's/^\(harness\|child\)_pid=\([0-9]*\).*$/\2/p' "$logdir/supervisor.log"); do
    if kill -0 "$ownPid" 2>/dev/null; then alive=$((alive + 1)); fi
  done
  cat "$logdir/supervisor.log"
fi
printf 'forced_cleanup=%s\nterminal_gone=1\nown_pid_alive=%s\n' "$forced" "$alive" >> "$logdir/launch.log"
if [ "$forced" -eq 0 ] && [ "$alive" -eq 0 ] && grep -q '^PASS: native application supervisor' "$logdir/supervisor.log"; then exit 0; fi
exit 1

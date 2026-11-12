#!/usr/bin/env bash
# Copyright (c) 2026 Devexperts LLC.
# SPDX-License-Identifier: MPL-2.0
#
# The local QDS server of the network tests, a CTest fixture (tests/CMakeLists.txt):
#   local-server.sh start <Tools> <tape> <port> <dir>   starts the server that sends the tape on 127.0.0.1:<port>
#   local-server.sh stop <dir>                          stops the server started with the same <dir>
# The PID file and the log of the server are in <dir>.

set -u

portIsOpen() {
    (exec 3<>"/dev/tcp/127.0.0.1/$1") 2>/dev/null
}

# Whether the PID is alive and is the server (not another process that got the PID of a dead server).
isServer() {
    kill -0 "$1" 2>/dev/null && grep -qa 'Qds' "/proc/$1/cmdline" 2>/dev/null
}

start() {
    local tools=$1 tape=$2 port=$3 dir=$4
    local pidFile=$dir/local-server.pid log=$dir/local-server.log

    mkdir -p "$dir"

    if [ -f "$pidFile" ] && isServer "$(cat "$pidFile")"; then
        echo "The server of a previous run is alive (PID $(cat "$pidFile")): run '$0 stop $dir'"
        return 1
    fi

    if portIsOpen "$port"; then
        echo "The port $port is busy: free it or configure another DXFCXX_NETWORK_TEST_PORT"
        return 1
    fi

    # The Tools split the arguments of QDS on spaces.
    case $tape in
    *' '*)
        echo "The path of the tape has a space: $tape"
        return 1
        ;;
    esac

    if [ ! -f "$tape" ]; then
        echo "No tape: $tape"
        return 1
    fi

    # -c all-se: the server keeps all the data of the tape, which is read once before any client subscribes.
    # -p scheme: the Tools set the dxLink scheme, which has no records of the candles of the default scheme (Trade.Day);
    # the server must use the default scheme, as the C++ API does.
    # setsid and the redirection of all the standard streams detach the server from CTest, which otherwise waits for
    # the streams that the server inherits.
    setsid "$tools" Qds "Feed -c all-se file:$tape[format=text,time=none] :$port[bindaddr=127.0.0.1]" \
        -p scheme=com.dxfeed.api.impl.DXFeedScheme </dev/null >"$log" 2>&1 &
    local pid=$!

    echo "$pid" >"$pidFile"

    # The port opens before the whole tape is read: the server is ready when the reader stops.
    for _ in $(seq 1 300); do
        if ! kill -0 "$pid" 2>/dev/null; then
            echo "The server has exited:"
            tail -n 30 "$log"
            rm -f "$pidFile"
            return 1
        fi

        if portIsOpen "$port" && grep -q 'Reading stopped' "$log"; then
            echo "The server (PID $pid) sends $tape on 127.0.0.1:$port"
            return 0
        fi

        sleep 0.1
    done

    echo "The server is not ready in 30 seconds:"
    tail -n 30 "$log"
    stop "$dir"

    return 1
}

stop() {
    local dir=$1
    local pidFile=$dir/local-server.pid

    if [ ! -f "$pidFile" ]; then
        return 0
    fi

    local pid
    pid=$(cat "$pidFile")

    if isServer "$pid"; then
        kill -TERM "$pid"

        for _ in $(seq 1 50); do
            isServer "$pid" || break
            sleep 0.1
        done

        if isServer "$pid"; then
            kill -KILL "$pid"
        fi
    fi

    rm -f "$pidFile"
    echo "The server (PID $pid) is stopped"
}

case "${1:-}" in
start)
    [ $# -eq 5 ] || { echo "Usage: $0 start <Tools> <tape> <port> <dir>"; exit 2; }
    start "$2" "$3" "$4" "$5"
    ;;
stop)
    [ $# -eq 2 ] || { echo "Usage: $0 stop <dir>"; exit 2; }
    stop "$2"
    ;;
*)
    echo "Usage: $0 start <Tools> <tape> <port> <dir> | stop <dir>"
    exit 2
    ;;
esac

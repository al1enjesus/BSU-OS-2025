#!/usr/bin/env bash

pid=$$
chain=""

while :; do
    comm=$(<"/proc/$pid/comm" 2>/dev/null)

    if [ -z "$chain" ]; then
        chain="${comm}(${pid})"
    else
        chain="${comm}(${pid}) ← ${chain}"
    fi

    if [ "$pid" -le 1 ]; then
        break
    fi

    ppid=$(awk '/^PPid:/ {print $2}' "/proc/$pid/status" 2>/dev/null)
    if [ -z "$ppid" ] || [ "$ppid" -eq 0 ] || [ "$ppid" -eq "$pid" ]; then
        break
    fi
    pid=$ppid
done

echo "$chain"

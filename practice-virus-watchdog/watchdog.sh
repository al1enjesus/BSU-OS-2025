#!/bin/bash
while true; do
    pid=$(pgrep firefox)
    [ -n "$pid" ] && kill $pid &&  echo "hohoho"
    sleep 2
done

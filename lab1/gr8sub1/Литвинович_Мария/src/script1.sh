#!/bin/bash

LOGFILE="/var/log/syslog"

cat "$LOGFILE" \
  | tr -s '[:space:]' '\n' \
  | tr -d '[:punct:]' \
  | tr '[:upper:]' '[:lower:]' \
  | grep -v '^$' \
  | sort \
  | uniq -c \
  | sort -nr \
  | head -n 5

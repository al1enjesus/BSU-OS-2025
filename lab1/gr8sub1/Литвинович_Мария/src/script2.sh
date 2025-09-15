#!/bin/bash

LOGFILE="/var/log/auth.log"

if [ ! -f "$LOGFILE" ]; then
  echo "Файл $LOGFILE не найден!"
  exit 1
fi

sudo grep -E "Failed|Invalid" "$LOGFILE" \
  | awk '{print $0}' \
  | sort \
  | uniq -c \
  | sort -nr \
  | head -n 10

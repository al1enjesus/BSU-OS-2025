#!/bin/bash

LOGFILE="/var/log/dpkg.log"

if [ ! -f "$LOGFILE" ]; then
  echo "Файл $LOGFILE не найден!"
  exit 1
fi

sudo grep " install " "$LOGFILE" \
  | awk '{print $4}' \
  | sort \
  | uniq -c \
  | sort -nr \
  | head -n 10

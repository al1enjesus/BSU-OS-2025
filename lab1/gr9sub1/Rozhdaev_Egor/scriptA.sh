#!/bin/bash
if [ ! -f /var/log/syslog ]; then
  echo "Ошибка: файл /var/log/syslog не найден"
  exit 1
fi
tr -cs '[:alnum:]' '\n' < /var/log/syslog | \
tr '[:upper:]' '[:lower:]' | \
sort | \
uniq -c | \
sort -nr | \
head -n 5

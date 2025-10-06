#!/bin/bash
if [ ! -f /var/log/syslog ]; then
  echo "Ошибка: файл /var/log/syslog не найден"
  exit 1
fi
grep 'install ' /var/log/dpkg.log | \
awk '{print $4}' | \
sort | uniq -c | sort -nr | head -n 10

#!/bin/bash
if [ ! -f /var/log/syslog ]; then
  echo "Ошибка: файл /var/log/syslog не найден"
  exit 1
fi
grep -E '(Failed|Invalid)' /var/log/auth.log | \
grep -oE '([0-9]{1,3}\.){3}[0-9]{1,3}' | \
sed -E 's/([0-9]+\.[0-9]+\.[0-9]+\.)[0-9]+/\1x/g' | \
sort | uniq -c | sort -nr | head -n 10

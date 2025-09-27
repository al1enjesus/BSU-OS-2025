#!/bin/bash

if [ ! -f "/var/log/syslog" ]; then
	echo "файл /var/log/syslog не найден!"
	exit 1
fi

cat /var/log/syslog \
| tr -cs '[:alnum:]' '\n' \
| tr '[:upper:]' '[:lower:]' \
| sort \
| uniq -c \
| sort -nr \
| head -n 5

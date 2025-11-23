#!/bin/bash

if [ ! -f "/var/log/dpkg.log" ]; then
        echo "файл /var/log/dpkg.log не найден!"
        exit 1
fi

cat /var/log/dpkg.log \
| cut -c21- \
| grep -E '^install' \
| sort \
| uniq -c \
| sort -n -r \
| head -n 10

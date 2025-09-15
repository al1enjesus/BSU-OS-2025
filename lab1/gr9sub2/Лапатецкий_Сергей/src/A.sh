#!/bin/bash
cat ../logs/syslog \
| tr -cs '[:alnum:]' '\n' \
| tr '[:upper:]' '[:lower:]' \
| sort \
| uniq -c \
| sort -nr \
| head -n 5

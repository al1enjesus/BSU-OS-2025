#!/bin/bash

tr '[:upper:]' '[:lower:]' < /var/log/syslog | tr -cs '[:alnum:]' '\n' | grep -v '^$' | sort | uniq -c | sort -nr | head -n 5

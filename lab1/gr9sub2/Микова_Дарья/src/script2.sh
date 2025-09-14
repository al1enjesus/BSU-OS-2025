#!/bin/bash
cat /var/log/auth.log \
| grep -a -E "(Failed|Invalid)" \
| grep -oE '[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}' \
| sed -E 's/([0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.)[0-9]{1,3}/\1x/g' \
| sort | uniq -c | sort -nr | head -n 10
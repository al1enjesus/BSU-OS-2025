#!/bin/bash
cat ../logs/auth.log \
| grep -E "(Failed|Invalid)" \
| grep -Eo "\b([0-9]{1,3}\.){3}[0-9]{1,3}\b" \
| sort \
| uniq -c \
| sort -nr \
| head -n 10 \
| sed -E 's/([0-9]+\.[0-9]+\.[0-9]+\.)[0-9]+/\1x/g' 

#!/bin/bash

grep -E 'Failed|Invalid' /var/log/auth.log \
  | awk '{for(i=1;i<=NF;i++) if ($i ~ /^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$/) print $i}' \
  | sed -E 's/([0-9]+\.[0-9]+\.[0-9]+\.)[0-9]+/\1x/g' \
  | sort | uniq -c | sort -nr | head -10



#!/bin/bash

LOG_FILE="/var/log/pacman.log"
OUTPUT="top5_packages.txt"
grep "installed" "$LOG_FILE" > installs.txt

awk '{print $4}' installs.txt | sort | uniq -c | sort -nr | head -n 5 > "$OUTPUT"

cat "$OUTPUT"

rm installs.txt

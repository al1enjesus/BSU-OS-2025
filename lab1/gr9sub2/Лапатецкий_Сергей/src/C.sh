#!/bin/bash
cat ../logs/dpkg.log \
| grep "install " \
| awk '{print $4}' \
| sort \
| uniq -c \
| sort -nr \
| head -n 10

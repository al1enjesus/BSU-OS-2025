#!/bin/bash
cat /var/log/dpkg.log \
| cut -c21- \
| grep -E '^install' \
| sort \
| uniq -c\
| sort -n -r\
| head -n 10
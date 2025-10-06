tr -cs '[:alnum:]' '\n' < /var/log/syslog | \
tr '[:upper:]' '[:lower:]' | \
sort | \
uniq -c | \
sort -nr | \
head -n 5

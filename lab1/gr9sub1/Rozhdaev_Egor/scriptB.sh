grep -E '(Failed|Invalid)' /var/log/auth.log | \
grep -oE '([0-9]{1,3}\.){3}[0-9]{1,3}' | \
sed -E 's/([0-9]+\.[0-9]+\.[0-9]+\.)[0-9]+/\1x/g' | \
sort | uniq -c | sort -nr | head -n 10

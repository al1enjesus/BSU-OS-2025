cat /var/log/auth.log \
| grep -a -E -i "failed|invalid"

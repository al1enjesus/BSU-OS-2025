cat /var/log/auth.log \
  | grep "Failed|Invalid" \
  | head -n 5

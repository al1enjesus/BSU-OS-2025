cat /var/log/dpkg.log \
  | tr '[:upper:]' '[:lower:]' \
  | cut -c21- \
  | grep -w "^install" \
  | sort \
  | uniq -c \
  | sort -nr \
  | head -n 10\
  | awk '{gsub(/install/, ""); print}'

grep 'install ' /var/log/dpkg.log \
  | awk '{print $4}' \
  | cut -d: -f1 \
  | sort \
  | uniq -c \
  | sort -nr \
  | head -n 10
  

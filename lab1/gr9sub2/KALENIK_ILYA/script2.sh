cat /var/log/auth.log \
 |grep -E "(Failed|Invalid)"\
 | awk '{print $NF}'\
 | grep -E "[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+"\
 | sort\
 | uniq -c\
 | sort -nr\
 | head -10\
 | sed -E 's/([0-9]+\.[0-9]+\.[0-9]+\.)[0-9]+/\1x/g'

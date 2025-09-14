mkdir -p artifacts

awk '{
  for (i=1;i<=NF;i++) {
    str=tolower($i)
    gsub(/[^a-z]/,"",str)
    if (str != "") print str
  }
}' src/lab-logs/syslog |
  sort |
  uniq -c |
  sort -nr |
  head -n 5 >artifacts/top5_syslog.txt

grep -Ei 'Failed|Invalid|authentication failure' src/lab-logs/auth.log |
  grep -Eoi 'from[[:space:]]+([0-9]{1,3}\.){3}[0-9]{1,3}|from[[:space:]]+([0-9a-f:]+)|rhost=([0-9]{1,3}\.){3}[0-9]{1,3}|rhost=([0-9a-f:]+)' |
  sed -E 's/^[^0-9a-f:]*//I' |
  sed -E 's/(([^.]*\.){3})[^.]+/\1x/; s/(:[^:]+)$/:xxxx/' |
  sort | uniq -c | sort -nr | head -n 10 >artifacts/top10_auth_ips_masked.txt

if [ ! -s artifacts/top10_auth_ips_masked.txt ]; then
  echo "За последние данные нет записей с внешними IP для Failed/Invalid/authentication failure." >artifacts/top10_auth_ips_masked.txt
fi

grep -Ei 'Failed password|authentication failure' src/lab-logs/auth.log |
  grep -Ei 'pam_unix\((sudo|hyprlock):auth\)' |
  grep -c 'user=wresq' >artifacts/failed_count_user_wresq.txt

awk '/\[ALPM\] installed /{print $4}' src/lab-logs/dpkg.log |
  sort |
  uniq -c |
  sort -nr |
  head -n 10 >artifacts/top10_installed.txt

awk '/\[ALPM\] upgraded /{print $4}' src/lab-logs/dpkg.log |
  sort |
  uniq -c |
  sort -nr |
  head -n 10 >artifacts/top10_upgraded.txt

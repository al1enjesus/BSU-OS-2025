mkdir -p src/lab-logs

sudo journalctl --since -7d --no-pager >src/lab-logs/syslog

: >src/lab-logs/auth.log

sudo journalctl --since -7d --no-pager SYSLOG_FACILITY=4 >>src/lab-logs/auth.log
sudo journalctl --since -7d --no-pager SYSLOG_FACILITY=10 >>src/lab-logs/auth.log
sudo journalctl --since -7d --no-pager -t sshd >>src/lab-logs/auth.log
sudo journalctl --since -7d --no-pager -t sudo >>src/lab-logs/auth.log
sudo journalctl --since -7d --no-pager -t polkitd >>src/lab-logs/auth.log
sudo journalctl --since -7d --no-pager _SYSTEMD_UNIT=systemd-logind.service >>src/lab-logs/auth.log

sort -u src/lab-logs/auth.log -o src/lab-logs/auth.log

cp /var/log/pacman.log src/lab-logs/dpkg.log

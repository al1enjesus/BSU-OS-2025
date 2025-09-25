set -euo pipefail
apt update && apt install -y build-essential psmisc sysstat htop iotop strace ltrace gdb >/dev/null 2>&1 || true
echo "== Build =="; make -C "$(dirname "$0")" -j
echo "== fork_example (C) ==";  "$(dirname "$0")/bin/fork_example" | tee -a "$(dirname "$0")/fork_c.log"
echo "== fork_example (Py) =="; python3 "$(dirname "$0")/src/fork_example.py" | tee -a "$(dirname "$0")/fork_py.log"
echo "== ptree (C) ==";        "$(dirname "$0")/bin/ptree"
echo "== ptree (Py) ==";       python3 "$(dirname "$0")/src/ptree.py"
echo "== ps tree ==";          ps -ef --forest | head -n 30 | tee "$(dirname "$0")/ps_forest.log"
echo "== pstree ==";           pstree -p | head -n 50 | tee "$(dirname "$0")/pstree.log"
PID=$$; {
  echo "Shell PID is $$"
  echo "[cmdline]"; tr '\0' ' ' < /proc/$PID/cmdline; echo
  echo "[status]";  head -n 20 /proc/$PID/status
  echo "[fd]";      ls -l /proc/$PID/fd
} | tee "$(dirname "$0")/proc_shell.log"
echo "== Top snapshots =="; top -b -n 1 | head -n 20 | tee "$(dirname "$0")/top_head.log"
ps -eo pid,ppid,comm,state,%cpu,%mem,etime --sort=-%cpu | head -n 15 | tee "$(dirname "$0")/ps_cpu_top.log"
ps -eo pid,ppid,comm,state,%cpu,%mem,rss --sort=-%mem | head -n 15 | tee "$(dirname "$0")/ps_mem_top.log"
echo "== pidstat =="; pidstat -u -r -d 1 5 | tee "$(dirname "$0")/pidstat.log" || true
if command -v iotop >/dev/null 2>&1; then iotop -b -n 5 | head -n 30 | tee "$(dirname "$0")/iotop.log" || true; fi
echo "Done."
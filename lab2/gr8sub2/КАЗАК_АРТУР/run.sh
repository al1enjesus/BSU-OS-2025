#!/usr/bin/env bash
set -euo pipefail

require_cmd(){ command -v "$1" >/dev/null 2>&1 || { echo "ERROR: '$1' required"; exit 1; }; }
optional_cmd(){ command -v "$1" >/dev/null 2>&1; }

require_cmd python3; require_cmd ps; require_cmd top; require_cmd pidstat; require_cmd pstree

echo "== fork_example (Python) =="
python3 src/fork_example.py | tee fork_py.log

echo "== ptree (Python) =="
python3 src/ptree.py | tee ptree_py.log

echo "== ps tree =="
ps -ef --forest | head -n 30 | tee ps_forest.log
pstree -p | head -n 50 | tee pstree.log

echo "== /proc of current shell =="
PID=$$
{ echo "Shell PID=$PID"
  echo "[cmdline]"; tr '\0' ' ' < "/proc/$PID/cmdline"; echo
  echo "[status]";  head -n 20 "/proc/$PID/status"
  echo "[fd]";      ls -l "/proc/$PID/fd"; } | tee proc_shell.log

echo "== Top snapshots =="
top -b -n 1 | head -n 20 | tee top_head.log
ps -eo pid,ppid,comm,state,%cpu,%mem,etime --sort=-%cpu | head -n 15 | tee ps_cpu_top.log
ps -eo pid,ppid,comm,state,%cpu,%mem,rss --sort=-%mem | head -n 15 | tee ps_mem_top.log

echo "== pidstat (5 x 1s) =="
pidstat -u -r -d 1 5 | tee pidstat.log || true

if optional_cmd iotop; then
  echo "== iotop (optional) =="; iotop -b -n 5 | head -n 30 | tee iotop.log || true
else
  echo "NOTE: 'iotop' not found; skipping IO snapshot"
fi

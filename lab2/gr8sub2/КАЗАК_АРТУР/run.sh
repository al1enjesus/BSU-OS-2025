#!/usr/bin/env bash
set -euo pipefail

# -- helpers ---------------------------------------------------------------

require_cmd() {
  # usage: require_cmd <name> [hint]
  local name="${1:-}"; local hint="${2:-"please install it"}"
  if ! command -v "$name" >/dev/null 2>&1; then
    echo "ERROR: required command '$name' not found; $hint" >&2
    exit 1
  fi
}

optional_cmd() {
  # usage: optional_cmd <name> [note]
  local name="${1:-}"; local note="${2:-"skipping optional step"}"
  if ! command -v "$name" >/dev/null 2>&1; then
    echo "NOTE: optional command '$name' not found; $note" >&2
    return 1
  fi
  return 0
}

# -- prerequisites ---------------------------------------------------------

require_cmd python3 "install Python 3"
require_cmd ps      "usually in procps package"
require_cmd top     "usually in procps/top package"
require_cmd pidstat "install 'sysstat' package (e.g. apt install sysstat)"
require_cmd pstree  "install 'psmisc' package (e.g. apt install psmisc)"

# -- run steps -------------------------------------------------------------

echo "== fork_example (Python) =="
python3 src/fork_example.py | tee fork_py.log

echo "== ptree (Python) =="
python3 src/ptree.py | tee ptree_py.log

echo "== ps tree (forest) =="
ps -ef --forest | head -n 30 | tee ps_forest.log

echo "== pstree =="
pstree -p | head -n 50 | tee pstree.log

echo "== /proc of current shell =="
PID=$$
{
  echo "Shell PID is $PID"
  echo "[cmdline]"; tr '\0' ' ' < "/proc/$PID/cmdline"; echo
  echo "[status]";  head -n 20 "/proc/$PID/status"
  echo "[fd]";      ls -l "/proc/$PID/fd"
} | tee proc_shell.log

echo "== Top snapshots =="
top -b -n 1 | head -n 20 | tee top_head.log
ps -eo pid,ppid,comm,state,%cpu,%mem,etime --sort=-%cpu | head -n 15 | tee ps_cpu_top.log
ps -eo pid,ppid,comm,state,%cpu,%mem,rss --sort=-%mem | head -n 15 | tee ps_mem_top.log

echo "== pidstat (5 x 1s) =="
pidstat -u -r -d 1 5 | tee pidstat.log || true

# Optional: iotop (if present and user has permissions)
if optional_cmd iotop "apt install iotop (optional)"; then
  echo "== iotop (optional) =="
  iotop -b -n 5 | head -n 30 | tee iotop.log || true
fi

echo "Done."

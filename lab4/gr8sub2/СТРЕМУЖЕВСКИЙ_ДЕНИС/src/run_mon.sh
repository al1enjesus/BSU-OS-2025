#!/usr/bin/env bash
set -euo pipefail

# Параметры (можно задать через env)
DURATION="${DURATION:-20}"      # сколько секунд мониторить
WITH_IOTOP="${WITH_IOTOP:-0}"   # 1 — добавить iotop (нужен root), 0 — нет

if [[ $# -lt 1 ]]; then
  echo "Usage: $0 <command> [args...]"
  echo "Env: DURATION=20 WITH_IOTOP=0"
  exit 2
fi

CMDLINE="$*"

# --- Шапка: инфо о ФС/диске/планировщике (всё в stdout) ---
echo "====== DISK/FS INFO ====== $(date -Is)"
df -P . | tail -1
DEV=$(awk 'NR==2{print $1}' <(df -P .))
DISK=$(lsblk -no PKNAME "$DEV" 2>/dev/null || true)
echo "--- lsblk ---"
lsblk -o NAME,ROTA,SIZE,MODEL | grep -E "^NAME|^$DISK" || true
echo "--- scheduler ---"
[[ -n "${DISK:-}" && -r "/sys/block/$DISK/queue/scheduler" ]] \
  && cat "/sys/block/$DISK/queue/scheduler" || echo "n/a"
echo

# --- Запускаем нагрузку в фоне (без stdin, чтобы не подвис) ---
echo "====== RUN ====== $(date -Is)"
echo "CMD: $CMDLINE"
"$@" </dev/null & CMDPID=$!
echo "PID: $CMDPID"
echo

# --- Снэпшоты /proc/<PID>/io до/после ---
read_io_field(){ # $1=PID $2=key
  [[ -r /proc/$1/io ]] && awk -v k="$2" '$1==k{print $2}' /proc/$1/io || echo 0
}
R0=$(read_io_field "$CMDPID" read_bytes)
W0=$(read_io_field "$CMDPID" write_bytes)
T0=$(date +%s.%N)

# --- Мониторы в stdout (помечаем префиксами, отключаем буферизацию) ---
echo "====== IOSTAT (every 1s, ${DURATION}s) ======"
stdbuf -oL iostat -x 1 "$DURATION" | sed -u 's/^/[iostat] /' & IOP=$!

echo "====== PIDSTAT (PID=$CMDPID, every 1s, ${DURATION}s) ======"
stdbuf -oL pidstat -d -p "$CMDPID" 1 "$DURATION" | sed -u 's/^/[pidstat] /' & PDP=$!

if [[ "$WITH_IOTOP" == "1" ]]; then
  echo "====== IOTOP (every 1s, ${DURATION}s) ======"
  sudo stdbuf -oL iotop -b -d 1 -n "$DURATION" | sed -u 's/^/[iotop] /' & TOPP=$!
else
  TOPP=
fi

# --- Ждём или DURATION, или завершение процесса ---
elapsed=0
while kill -0 "$CMDPID" 2>/dev/null && [[ $elapsed -lt $DURATION ]]; do
  sleep 1
  elapsed=$((elapsed+1))
done

# Останавливаем мониторы
kill "$IOP" 2>/dev/null || true
kill "$PDP" 2>/dev/null || true
[[ -n "${TOPP:-}" ]] && kill "$TOPP" 2>/dev/null || true

# Если процесс ещё жив — дожидаемся
wait "$CMDPID" 2>/dev/null || true

# --- Финальные снэпшоты и сводка ---
T1=$(date +%s.%N)
R1=$(read_io_field "$CMDPID" read_bytes)
W1=$(read_io_field "$CMDPID" write_bytes)

dur=$(awk -v a="$T0" -v b="$T1" 'BEGIN{print (b-a)}')
dR=$(( R1>R0 ? R1-R0 : 0 ))
dW=$(( W1>W0 ? W1-W0 : 0 ))

to_mb(){ awk -v x="$1" 'BEGIN{printf "%.2f", x/1024/1024}'; }
mbps(){ awk -v bytes="$1" -v s="$2" 'BEGIN{printf "%.2f", (bytes/1024/1024)/s}'; }

echo
echo "====== SUMMARY ====== $(date -Is)"
echo "Elapsed_s: $dur"
echo "read_bytes_delta:  $dR  ($(to_mb "$dR") MB)  => $(mbps "$dR" "$dur") MB/s"
echo "write_bytes_delta: $dW  ($(to_mb "$dW") MB)  => $(mbps "$dW" "$dur") MB/s"

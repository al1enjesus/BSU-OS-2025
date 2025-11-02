set -Eeuo pipefail

DURATION="${DURATION:-20}"
WITH_IOTOP="${WITH_IOTOP:-0}"
REDIRECT_STDIN="${REDIRECT_STDIN:-1}"

if [[ $# -lt 1 ]]; then
  echo "Usage: $0 <command> [args...]"
  echo "Env: DURATION=20 WITH_IOTOP=0 REDIRECT_STDIN=1"
  exit 2
fi

CMDLINE="$*"

cleanup() {
  [[ -n "${IOP:-}"  ]] && kill "$IOP"  2>/dev/null || true
  [[ -n "${PDP:-}"  ]] && kill "$PDP"  2>/dev/null || true
  [[ -n "${TOPP:-}" ]] && kill "$TOPP" 2>/dev/null || true
  [[ -n "${CMDPID:-}" ]] && kill "$CMDPID" 2>/dev/null || true
}
trap cleanup INT TERM EXIT

echo "====== DISK/FS INFO ====== $(date -Is)"
df -P . | tail -1
DEV=$(df -P . | awk 'END{print $1}')
DISK=$(lsblk -no PKNAME "$DEV" 2>/dev/null || true)
echo "--- lsblk ---"
lsblk -o NAME,ROTA,SIZE,MODEL | awk -v d="$DISK" 'NR==1 || $1==d'
echo "--- scheduler ---"
if [[ -n "${DISK:-}" && -r "/sys/block/$DISK/queue/scheduler" ]]; then
  cat "/sys/block/$DISK/queue/scheduler"
else
  echo "n/a"
fi
echo

echo "====== RUN ====== $(date -Is)"
echo "CMD: $CMDLINE"
if [[ "$REDIRECT_STDIN" == "1" ]]; then
  "$@" </dev/null & CMDPID=$!
else
  "$@" & CMDPID=$!
fi
echo "PID: $CMDPID"
echo

read_io_field(){
  [[ -r "/proc/$1/io" ]] && awk -v k="$2" '$1==k{print $2}' "/proc/$1/io" || echo 0
}
R0=$(read_io_field "$CMDPID" read_bytes)
W0=$(read_io_field "$CMDPID" write_bytes)
T0=$(date +%s.%N)

has(){ command -v "$1" >/dev/null 2>&1; }

echo "====== IOSTAT (every 1s, ${DURATION}s) ======"
if has iostat; then
  stdbuf -oL -eL iostat -y -x 1 "$DURATION" | sed -u 's/^/[iostat] /' & IOP=$!
else
  echo "[iostat] not found — skipping"
  IOP=
fi

echo "====== PIDSTAT (PID=$CMDPID, every 1s, ${DURATION}s) ======"
if has pidstat; then
  stdbuf -oL -eL pidstat -d -p "$CMDPID" 1 "$DURATION" | sed -u 's/^/[pidstat] /' & PDP=$!
else
  echo "[pidstat] not found — skipping"
  PDP=
fi

if [[ "$WITH_IOTOP" == "1" ]]; then
  echo "====== IOTOP (every 1s, ${DURATION}s) ======"
  if has iotop; then
    sudo -n stdbuf -oL -eL iotop -b -d 1 -n "$DURATION" 2>/dev/null | sed -u 's/^/[iotop] /' & TOPP=$! || {
      echo "[iotop] need root (NOPASSWD) — skipping"; TOPP=
    }
  else
    echo "[iotop] not found — skipping"
    TOPP=
  fi
else
  TOPP=
fi

elapsed=0
while kill -0 "$CMDPID" 2>/dev/null && [[ $elapsed -lt $DURATION ]]; do
  sleep 1
  elapsed=$((elapsed+1))
done

[[ -n "${IOP:-}"  ]] && kill "$IOP"  2>/dev/null || true
[[ -n "${PDP:-}"  ]] && kill "$PDP"  2>/dev/null || true
[[ -n "${TOPP:-}" ]] && kill "$TOPP" 2>/dev/null || true

wait "$CMDPID" 2>/dev/null || true

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

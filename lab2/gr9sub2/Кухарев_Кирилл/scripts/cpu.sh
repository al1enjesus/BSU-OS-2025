#!/bin/bash

echo "=== TOP‑5 по CPU ==="
ps -eo pid,comm,%cpu,etime --sort=-%cpu | head -n 6

echo
echo "=== TOP‑5 по памяти ==="
ps -eo pid,comm,%mem,rss --sort=-%mem | \
    awk 'NR==1 {printf "%-6s %-20s %-6s %-10s\n", $1,$2,$3,"RSS(MiB)"; next}
         NR<=6 {printf "%-6s %-20s %-6s %-10.1f\n", $1,$2,$3,$4/1024}'

echo
echo "=== TOP‑5 по IO (rKB/s, wKB/s) ==="
if command -v pidstat >/dev/null; then
    pidstat -d 1 1 | \
        awk 'NR>3 {print $0}' | sort -k4 -nr | head -n 5
else
    echo "pidstat не найден. Установите пакет sysstat."
fi

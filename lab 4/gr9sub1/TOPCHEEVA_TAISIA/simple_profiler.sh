if [ -z "$1" ]; then
    echo "Использование: $0 <PID>"
    exit 1
fi

PID=$1

echo "=== Memory Profiler - PID: $PID ==="
echo ""


echo "📊 Основные метрики:"
ps -o pid,comm,vsz,rss,pmem --no-headers -p $PID

echo ""
echo "📈 Детальная информация:"
cat /proc/$PID/status 2>/dev/null | grep -E "^(Vm|Pid)" | head -10

echo ""
echo "🔄 Page Faults:"
cat /proc/$PID/stat 2>/dev/null | awk '{
    printf "Minor faults: %d\n", $10
    printf "Major faults: %d\n", $12
}'

echo ""
echo "🗺️  Типы памяти:"
cat /proc/$PID/maps 2>/dev/null | head -10 | while read line; do
    path=$(echo "$line" | awk '{print $6}')
    if [ -n "$path" ]; then
        echo "$path"
    else
        echo "[anonymous]"
    fi
done

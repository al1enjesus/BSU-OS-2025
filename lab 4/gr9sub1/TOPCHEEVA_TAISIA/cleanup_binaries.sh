echo "Очистка бинарных файлов из истории Git..."

git rm -f memory_profiler memory_test io_benchmark disk_stress simple_profiler 2>/dev/null || true
git rm -f *.bin 2>/dev/null || true
git rm -f gnuplot_*.gnu 2>/dev/null || true

git reset --hard


echo "=== Статус после очистки ==="
git status

echo "=== Файлы в рабочей директории ==="
ls -la | grep -E "(memory_profiler|memory_test|io_benchmark|disk_stress|\.bin$|\.gnu$)"

echo "Очистка завершена!"
EOF

chmod +x cleanup_binaries.sh
./cleanup_binaries.sh

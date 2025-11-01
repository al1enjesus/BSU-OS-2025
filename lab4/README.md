# Lab4 — Виртуальная память, I/O и профилировщик памяти

Этот проект содержит четыре C++‑программы:
- `A_mem.cpp` — анализ виртуальной памяти процесса
- `B_io_bench.cpp` — бенчмарк буферизованного/небуферизованного I/O
- `C_io_monitor.cpp` — мониторинг дисковой активности
- `memory_profiler.cpp` — профилировщик памяти

## Быстрая компиляция одной программы

```bash
g++ -std=c++17 -O2 -Wall -Wextra -o program_name source.cpp
```

Примеры:
```bash
# из каталога lab4/
g++ -std=c++17 -O2 -Wall -Wextra -o bin/A_mem src/A_mem.cpp
g++ -std=c++17 -O2 -Wall -Wextra -o bin/B_io_bench src/B_io_bench.cpp
g++ -std=c++17 -O2 -Wall -Wextra -o bin/C_io_monitor src/C_io_monitor.cpp
g++ -std=c++17 -O2 -Wall -Wextra -o bin/memory_profiler src/memory_profiler.cpp
```

## Автоматическая сборка (Makefile)

Соберите все программы одной командой:

```bash
make            # сборка в режиме Release (-O2)
make debug      # сборка с -O0 -g
make clean      # удалить bin/ и промежуточные файлы
make help       # краткая справка по целям
```

- Исходники ожидаются в `src/`.
- Готовые бинарники попадают в `bin/`.

## Примеры запуска

```bash
# A_mem
./bin/A_mem --size-mb=1 --show-maps

# Бенчмарк I/O
./bin/B_io_bench --size-mb=100 --buffers=512,4096,65536

# Монитор диска (пример для nvme0n1)
./bin/C_io_monitor --dev=nvme0n1 --iters=5 --interval=0.5

# Профилировщик памяти
PID=$(pidof -s firefox)
./bin/memory_profiler --watch --interval=1 --samples=10 --graph "$PID"
```

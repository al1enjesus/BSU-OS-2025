# Лабораторная работа 4
## Управление памятью и файловый ввод-вывод

### Компиляция и запуск

```bash
# Сборка всех программ
make all

# Задание A: Анализ виртуальной памяти
make run_analysis

# Задание B: Тестирование I/O
make run_io_test

# Задание C: Дисковый I/O
./disk_load

# Memory Profiler
./memory_profiler <PID>
./memory_profiler --watch <PID>
./memory_profiler --compare <PID1> <PID2>

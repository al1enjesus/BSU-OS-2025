```markdown
# Лабораторная работа 4: Управление памятью и файловый ввод-вывод

Набор утилит для анализа управления памятью, производительности I/O операций и мониторинга процессов в Linux.

## Программы

- **memory_info** - анализ виртуальной памяти процесса
- **io_benchmark** - бенчмарк методов файлового ввода-вывода  
- **mmap_vs_read** - сравнение производительности mmap() и read()
- **page_faults_demo** - демонстрация работы page faults
- **memory_profiler** - продвинутый мониторинг памяти процессов

## Требования

- **ОС**: Linux (требуется доступ к /proc файловой системе)
- **Компилятор**: GCC

## Сборка

```bash
# Сборка всех программ
make all

# Сборка отдельных утилит
make memory_info
make io_benchmark
make mmap_vs_read  
make page_faults_demo
make memory_profiler

# Очистка
make clean
```

## Использование

### memory_info
```bash
# Демонстрационный режим
./memory_info

# Анализ конкретного процесса
./memory_info <PID>
```

### io_benchmark
```bash
# Стандартный тест (100 MB файл)
./io_benchmark

# Тест с указанным размером
./io_benchmark --size 500
```

### mmap_vs_read
```bash
# Создать тестовый файл и сравнить методы
./mmap_vs_read test.bin --create-file 100

# Сравнить для существующего файла
./mmap_vs_read /path/to/existing/file.bin
```

### page_faults_demo
```bash
./page_faults_demo
```

### memory_profiler
```bash
# Базовая информация о процессе
./memory_profiler <PID>

# Мониторинг в реальном времени
./memory_profiler <PID> --watch 2

# Сравнение двух процессов
./memory_profiler <PID1> --compare <PID2>

# Детальная карта памяти
./memory_profiler <PID> --map
```

## Примеры вывода

### memory_info
```
Memory Metrics for PID 10159:
  VSZ (Virtual):  64196 KB (62.7 MB)
  RSS (Resident): 16880 KB (16.5 MB)
  PSS (Proportional): 15495 KB (15.1 MB)
```

### io_benchmark
```
=== Summary ===
Method        Time (s)    Throughput (MB/s)
--------------------------------------------
fwrite()      0.026       388.64  
write()       0.070       143.40  
mmap()        0.102       98.21   
```

## Особенности Makefile

```bash
# Профилирование процесса сборки
make run_profiler

# Справка по целям
make help
```

**Примечание**: В правиле `run_profiler` используется `$$PPID` для экранирования - это позволяет профилировать сам процесс сборки.

---

**Разработано для изучения механизмов управления памятью в ОС Linux**
```
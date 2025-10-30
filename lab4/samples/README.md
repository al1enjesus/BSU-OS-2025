# Lab 4 Samples - Скелеты и примеры

Этот каталог содержит **скелеты** (templates) для выполнения Лабораторной работы 4.

## ВАЖНО

⚠️ **Это НЕ готовые решения!** Все файлы содержат `TODO` места, которые студент обязан реализовать самостоятельно.

⚠️ **Нельзя сдавать код без изменений.** Скопируйте в свою папку и доработайте.

---

## Содержимое

### 1. `memory_info.c`
Базовый пример чтения информации о памяти процесса.

**Что демонстрирует:**
- Чтение VSZ, RSS из `/proc/[PID]/status`
- Разные типы выделения памяти (stack, heap, mmap)
- Отображение карты памяти из `/proc/[PID]/maps`

**Использование:**
```bash
make memory_info
./memory_info          # Демонстрационный режим
./memory_info <PID>    # Анализ указанного процесса
```

**TODO для студента:**
- Реализовать парсинг `/proc/[PID]/status`
- Реализовать парсинг `/proc/[PID]/maps`
- Добавить вывод PSS из `/proc/[PID]/smaps_rollup`

---

### 2. `mmap_vs_read.c`
Сравнение производительности memory mapping vs традиционного I/O.

**Что демонстрирует:**
- Чтение файла через `read()`
- Чтение файла через `mmap()`
- Замер времени и page faults

**Использование:**
```bash
make mmap_vs_read
./mmap_vs_read testfile.bin --create-file 100
```

**TODO для студента:**
- Реализовать метод чтения через `read()`
- Реализовать метод чтения через `mmap()`
- Добавить замер page faults
- Сравнить производительность

**Эксперименты:**
```bash
# Очистить page cache для чистого эксперимента
sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'

# Запустить бенчмарк
./mmap_vs_read testfile.bin

# С трассировкой системных вызовов
strace -c ./mmap_vs_read testfile.bin
```

---

### 3. `page_faults_demo.c`
Демонстрация page faults при работе с памятью.

**Что демонстрирует:**
- Minor page faults при первом обращении к памяти
- Разница между последовательным и случайным доступом
- Влияние `malloc` vs `calloc`

**Использование:**
```bash
make page_faults_demo
./page_faults_demo
```

**TODO для студента:**
- Реализовать все демонстрации (6 сценариев)
- Добавить замер через `getrusage()`
- Проанализировать результаты

**Проверка:**
```bash
# Подробная статистика
/usr/bin/time -v ./page_faults_demo

# Только page faults
./page_faults_demo 2>&1 | grep -i fault
```

---

### 4. `io_benchmark.c`
Бенчмарк различных методов файлового I/O.

**Что демонстрирует:**
- `fwrite()` (stdio, буферизованный)
- `write()` (системный вызов)
- `mmap()` (memory-mapped I/O)
- Влияние размера буфера

**Использование:**
```bash
make io_benchmark
./io_benchmark --size 100
```

**TODO для студента:**
- Реализовать все три метода I/O
- Добавить замер времени
- Реализовать тест разных размеров буфера

**Эксперименты:**
```bash
# Разные размеры файла
./io_benchmark --size 50
./io_benchmark --size 500

# С трассировкой
strace -c -e trace=write ./io_benchmark --size 10

# Очистка cache между запусками
sync && sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'
```

---

### 5. `memory_profiler.c` ⭐ **Главная утилита**
Профилировщик памяти процессов - основная практическая работа.

**Возможности:**
- Анализ VSZ, RSS, PSS, USS
- Отображение карты памяти
- Мониторинг в реальном времени
- Сравнение процессов

**Использование:**
```bash
make memory_profiler

# Анализ процесса
./memory_profiler <PID>

# Мониторинг
./memory_profiler <PID> --watch

# Сравнение
./memory_profiler <PID1> --compare <PID2>

# С картой памяти
./memory_profiler <PID> --map
```

**TODO для студента:**

**Обязательная часть:**
1. Реализовать чтение метрик из `/proc`
2. Парсинг карты памяти
3. Расчёт PSS, USS
4. Красивый вывод

**Дополнительная часть (бонус):**
5. Режим мониторинга с дельтами
6. Сравнение процессов
7. ASCII-график изменения памяти
8. Анализ разделяемых библиотек
9. Цветной вывод
10. Интерактивный режим

**Примеры:**
```bash
# Профилирование текущей оболочки
./memory_profiler $$

# Профилирование systemd
./memory_profiler 1

# Мониторинг Firefox
firefox &
./memory_profiler $(pgrep firefox) --watch
```

---

### 6. `memory_profiler.py` (альтернатива)
Python версия профилировщика для тех, кто предпочитает Python.

**Требования:**
```bash
pip3 install psutil
```

**Использование:**
```bash
python3 memory_profiler.py <PID>
python3 memory_profiler.py <PID> --watch
```

**Преимущества Python версии:**
- Проще в реализации
- Кросс-платформенность
- Легко добавить визуализацию (matplotlib)

**TODO для студента:**
- Реализовать все методы класса `MemoryProfiler`
- Добавить визуализацию (опционально)
- Сравнить с C версией

---

## Сборка и запуск

### Сборка всех программ
```bash
make all
```

### Сборка отдельной программы
```bash
make memory_profiler
make io_benchmark
```

### Запуск тестов
```bash
make test
```

### Запуск примеров
```bash
make run_memory_info
make run_page_faults
make run_io_benchmark
make run_profiler PID=1234
```

### Бенчмарк
```bash
make benchmark
```

### Очистка
```bash
make clean
```

### Помощь
```bash
make help
```

---

## Рекомендации

### Порядок выполнения

1. **Начните с `memory_info.c`**
   - Самый простой пример
   - Научитесь читать `/proc` файлы
   - Понять структуру виртуальной памяти

2. **Перейдите к `page_faults_demo.c`**
   - Увидеть page faults в действии
   - Понять, когда они происходят
   - Научиться их измерять

3. **Изучите `mmap_vs_read.c`**
   - Сравнить разные методы I/O
   - Понять преимущества mmap
   - Замерить производительность

4. **Попробуйте `io_benchmark.c`**
   - Углубиться в файловый I/O
   - Понять влияние буферизации
   - Сравнить методы

5. **Реализуйте `memory_profiler.c`** ⭐
   - Основная работа
   - Объединяет все знания
   - Практическая утилита

### Отладка

**Проверка системными утилитами:**
```bash
# Ваша программа
./memory_profiler 1234

# Сравнить с:
ps -o pid,vsz,rss,comm -p 1234
cat /proc/1234/status | grep ^Vm
sudo smem -p | grep <process_name>
```

**Трассировка:**
```bash
# Системные вызовы
strace ./program

# Только I/O
strace -e trace=open,read,write,mmap ./program

# Статистика
strace -c ./program
```

**Замер производительности:**
```bash
# Базовый
time ./program

# Детальный (включая page faults)
/usr/bin/time -v ./program

# Профилирование
perf stat ./program
```

### Очистка page cache

Для чистых экспериментов с I/O:
```bash
# Сбросить буферы на диск
sync

# Очистить page cache (требует root)
sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'
```

**Что означают значения:**
- `1` - освободить pagecache
- `2` - освободить dentries и inodes
- `3` - освободить всё (1 + 2)

---

## Частые ошибки

### 1. Не проверяете возвращаемые значения
```c
// ❌ Плохо
FILE *f = fopen("/proc/123/status", "r");
fgets(line, sizeof(line), f);  // Segfault если f == NULL!

// ✅ Хорошо
FILE *f = fopen("/proc/123/status", "r");
if (!f) {
    perror("fopen");
    return -1;
}
```

### 2. Неправильный парсинг /proc файлов
```c
// ❌ Плохо - не учитывает формат
fscanf(f, "%lu", &vm_rss);

// ✅ Хорошо
if (sscanf(line, "VmRSS: %lu kB", &vm_rss) == 1) {
    // Успешно распарсили
}
```

### 3. Утечки памяти
```c
// ❌ Плохо
char *data = mmap(...);
return 0;  // Забыли munmap!

// ✅ Хорошо
char *data = mmap(...);
// ... работа с data ...
munmap(data, size);
```

### 4. Неправильные единицы измерения
```c
// VmRSS в /proc/status в килобайтах!
printf("RSS: %lu bytes", vm_rss);  // ❌ Неверно

printf("RSS: %lu KB", vm_rss);     // ✅ Правильно
printf("RSS: %.1f MB", vm_rss / 1024.0);  // ✅ Ещё лучше
```

---

## Полезные ссылки

**Man pages:**
```bash
man 5 proc       # Документация по /proc
man 2 mmap       # Memory mapping
man 2 getrusage  # Статистика ресурсов
man 1 ps         # Информация о процессах
man 1 free       # Память системы
```

**Kernel документация:**
- `/usr/src/linux/Documentation/filesystems/proc.rst`
- https://www.kernel.org/doc/html/latest/filesystems/proc.html

**Утилиты:**
- `htop` - интерактивный просмотр процессов
- `smem` - показывает PSS/USS
- `vmstat` - статистика памяти
- `perf` - профилирование производительности

---

## Вопросы?

Если что-то непонятно:
1. Читайте комментарии в коде (там подробные объяснения)
2. Смотрите основной `lab4/README.md` (там теория)
3. Используйте `man` pages
4. Создайте Issue в репозитории
5. Спросите в комментариях к PR

---

**Удачи в выполнении лабораторной работы! 🚀**

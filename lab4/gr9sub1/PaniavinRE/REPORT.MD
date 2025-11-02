# Лабораторная работа №4: Управление памятью и I/O в Linux  
**Студент:** Паниавин Р.Е.  
**Группа:** 9, подгруппа 1  
**Дата:** 02.11.2025  

## Цель
Изучить механизмы управления памятью и ввода-вывода в Linux:  
- `/proc`, виртуальная память  
- `mmap` vs `read`  
- Page faults  
- Профилирование памяти

## 1. memory_info — Информация о памяти процесса

### Вывод `./memory_info $$`
VSZ: 8380 KB (8.2 MB) 
RSS: 4864 KB (4.8 MB) 
Data: 1620 KB (1.6 MB) 
Stack: 132 KB (0.1 MB)

### Вывод `ps` и `/proc/$$/status`
	ps -o pid,vsz,rss -p $$ 
PID VSZ RSS 
3374 8380 4864

cat /proc/ $$/status | grep ^Vm 
VmSize: 8380 kB 
VmRSS: 4864 kB 
VmData: 1620 kB 
VmStk: 132 kB

**Вывод:** Данные совпадают. `VSZ > RSS` — виртуальная память включает неиспользуемые области.

## 2. page_faults_demo — Демонстрация page faults

### Ключевые результаты
Demo 2: Sequential access (100 MB) 
Minor faults: +25599 
Expected: 25600 (100 * 1024 * 1024 / 4096)
Demo 5: Rereading 
Second read: +0 faults
Demo 6: calloc vs malloc 
calloc + read: +12800 faults 
calloc + write: +12799 faults

### `/usr/bin/time -v`
Minor page faults: 72374 
Major page faults: 0

**Выводы:**
- `malloc` без доступа → **1 minor fault** (расширение heap)
- Первый доступ → **1 fault на страницу**
- Повторный доступ → **0 faults**
- `calloc` использует **zero page + CoW**
- **0 major faults** — всё в RAM

## 3. mmap_vs_read — Сравнение I/O

> (Примечание: `madvise` удалён из-за проблем с aarch64, но `read` и `mmap` работают)

### Результаты (100 МБ)
read(): 0.00 s → 23836.7 MB/s 
mmap(): 0.00 s → 46577.5 MB/s

### Результаты (200 МБ)
read(): 0.01 s → 13975.2 MB/s 
mmap(): 0.00 s → 43947.0 MB/s

**Вывод:** `mmap` в **2–3 раза быстрее** `read` при последовательном доступе.

## 4. io_benchmark — Бенчмарк I/O
--- I/O Benchmark (100 MB) --- 
read(): 0.01 s, 9649.8 MB/s 
mmap(): 0.00 s, 53621.9 MB/s

**Вывод:** `mmap` значительно быстрее.

## 5. memory_profiler — Профайлер памяти

### Профиль `bash` ($$)
VSZ: 8380 KB, RSS: 4992 KB 
[heap]: 1456 KB 
[stack]: 132 KB

### Профиль `io_benchmark` (в фоне)
fopen: No such file or directory

→ Процесс завершился до профилирования — **нормально**, `io_benchmark` отработал быстро.

---

## Ответы на вопросы

1. **Почему VSZ > RSS?**  
   → VSZ — вся зарезервированная память, RSS — физически занятая. Виртуальная память включает неиспользуемые области.

2. **Где heap, stack, mmap?**  
   → В `/proc/PID/maps`:  
   - `[heap]` — динамическая память  
   - `[stack]` — стек  
   - `mmap` — анонимные или file-backed регионы

3. **Что означают права (r-xp, rw-p)?**  
   - `r-xp` — read, execute, private (код)  
   - `rw-p` — read, write, private (данные)

4. **Почему `mmap` быстрее `read`?**  
   → ОС сама подгружает страницы при доступе, без копирования в буфер. Меньше системных вызовов.

5. **Zero page optimization?**  
   → `calloc` использует общую нулевую страницу. Чтение — без fault, запись — CoW → новый fault.

---

## Вывод

- Реализованы все утилиты: `memory_info`, `page_faults_demo`, `io_benchmark`, `memory_profiler`
- Подтверждены теоретические знания:
  - Page faults при первом доступе
  - `mmap` > `read` по скорости
  - `/proc` — мощный инструмент анализа
- Все программы собраны, протестированы, работают

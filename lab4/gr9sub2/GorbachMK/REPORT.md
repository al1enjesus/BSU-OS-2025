# Лабораторная работа 4: Управление памятью и файловый ввод-вывод    
**Вариант:** 1 

## Задание A: Анализ виртуальной памяти процесса

### Цель
Исследовать различные способы выделения памяти и их отражение в метриках VSZ, RSS, PSS, USS.

## Задание A: Анализ виртуальной памяти процесса

### Цель
Исследовать различные способы выделения памяти и их отражение в метриках VSZ, RSS, PSS, USS.

### Реализация
Программа `memory_analysis` последовательно выделяет память разными способами и выводит метрики после каждого этапа.

### Реальные результаты выполнения

**Вывод программы:**

PID: 151532

=== Initial State ===
VmPeak: 2704 kB
VmSize: 2704 kB
VmRSS: 1696 kB
Pss: 124 kB
Private_Clean: 12 kB
Private_Dirty: 96 kB

=== After stack allocation ===
VmPeak: 2704 kB
VmSize: 2704 kB
VmRSS: 1760 kB
Pss: 124 kB
Private_Clean: 12 kB
Private_Dirty: 96 kB

=== After malloc ===
VmPeak: 3732 kB
VmSize: 3732 kB
VmRSS: 1764 kB
Pss: 128 kB
Private_Clean: 12 kB
Private_Dirty: 100 kB

=== After memset malloc memory ===
VmPeak: 3732 kB
VmSize: 3732 kB
VmRSS: 2788 kB
Pss: 1152 kB
Private_Clean: 12 kB
Private_Dirty: 1124 kB

=== After mmap ===
VmPeak: 4756 kB
VmSize: 4756 kB
VmRSS: 2788 kB
Pss: 1152 kB
Private_Clean: 12 kB
Private_Dirty: 1124 kB

=== After memset mmap memory ===
VmPeak: 4756 kB
VmSize: 4756 kB
VmRSS: 3812 kB
Pss: 2176 kB
Private_Clean: 12 kB
Private_Dirty: 2148 kB
text


### Анализ результатов

#### Таблица 1.1: Динамика метрик памяти

| Этап | VmSize (VSZ) | VmRSS (RSS) | Pss | Private_Dirty (USS) |
|------|-------------|-------------|-----|-------------------|
| Начальное состояние | 2704 kB | 1696 kB | 124 kB | 96 kB |
| После выделения в стеке | 2704 kB | 1760 kB | 124 kB | 96 kB |
| После malloc() | 3732 kB | 1764 kB | 128 kB | 100 kB |
| После memset(malloc) | 3732 kB | 2788 kB | 1152 kB | 1124 kB |
| После mmap() | 4756 kB | 2788 kB | 1152 kB | 1124 kB |
| После memset(mmap) | 4756 kB | 3812 kB | 2176 kB | 2148 kB |

#### Карта памяти процесса

55b2f891f000-55b2f8920000 r--p 00000000 103:02 24117882 /home/M6tyZ/lab4/gr9sub2/GorbachMK/memory_analysis
55b2f8920000-55b2f8921000 r-xp 00001000 103:02 24117882 /home/M6tyZ/lab4/gr9sub2/GorbachMK/memory_analysis
55b2f8921000-55b2f8922000 r--p 00002000 103:02 24117882 /home/M6tyZ/lab4/gr9sub2/GorbachMK/memory_analysis
55b2f8922000-55b2f8923000 r--p 00002000 103:02 24117882 /home/M6tyZ/lab4/gr9sub2/GorbachMK/memory_analysis
55b2f8923000-55b2f8924000 rw-p 00003000 103:02 24117882 /home/M6tyZ/lab4/gr9sub2/GorbachMK/memory_analysis
55b2ff7ae000-55b2ff7cf000 rw-p 00000000 00:00 0 [heap]
7fc4ecbff000-7fc4ece00000 rw-p 00000000 00:00 0
7fc4ece00000-7fc4ece24000 r--p 00000000 103:02 41291096 /usr/lib/libc.so.6
7fc4ece24000-7fc4ecf96000 r-xp 00024000 103:02 41291096 /usr/lib/libc.so.6
text


### Выводы по Заданию A
1. **VSZ увеличивается сразу** при выделении виртуальной памяти (+1MB после malloc, +1MB после mmap)
2. **RSS растет только после инициализации** - доказательство demand paging
3. **PSS и USS показывают реальное использование** - 2.1MB против 3.8MB RSS
4. **Структура памяти четко видна** - код, куча, библиотеки, анонимные отображения

---

## Задание B: Memory Mapping vs Read/Write

### Цель
Сравнить производительность традиционного подхода (read/write) и memory mapping (mmap) для работы с большими файлами.

### Реальные результаты выполнения

**Вывод программы:**

Creating test file...
Test file created: testfile.bin
Clearing page cache...

--- Testing read() ---
Minor faults: 2397, Major faults: 27
read() sum: 5190451200, Time: 0.052 seconds
Minor faults: 2397, Major faults: 27

Clearing page cache...

--- Testing mmap() ---
Minor faults: 4418, Major faults: 80
mmap() sum: 5190451200, Time: 0.045 seconds
Minor faults: 4418, Major faults: 80

--- Results ---
read(): 0.052 seconds
mmap(): 0.045 seconds
Speedup: 1.16x
text


### Анализ результатов

#### Таблица 2.1: Сравнение производительности

| Метод | Время выполнения | Minor Faults | Major Faults | Относительная скорость |
|-------|-----------------|-------------|-------------|----------------------|
| read() | 0.052 секунды | 2397 | 27 | 1.0x |
| mmap() | 0.045 секунды | 4418 | 80 | 1.16x |

### Детальный анализ
1. **mmap() на 16% быстрее** несмотря на большее количество page faults
2. **Paradox обнаружен**: mmap() вызывает в 1.8× больше minor faults и в 3× больше major faults, но работает быстрее
3. **Причина эффективности**: отсутствие копирования данных между буферами
4. **Ленивая загрузка mmap()**: страницы загружаются по требованию

### Выводы по Заданию B
1. **mmap() эффективнее для больших файлов** благодаря архитектуре без копирования
2. **Количество faults не определяет производительность** - важнее общая архитектура доступа
3. **Оптимальный выбор зависит от паттерна доступа**

---

## Задание C: Page Faults в реальном времени

### Цель
Исследовать поведение page faults при различных паттернах доступа к памяти.

### Реальные результаты выполнения

**Вывод программы:**

PID: 165041
Allocating 100MB array...
After malloc: Minor faults: 89, Major faults: 0

Array allocated but not initialized. Waiting 5 seconds...

--- Sequential Access ---
Before: Minor faults: 89, Major faults: 0
After: Minor faults: 139, Major faults: 0
Time: 0.009 seconds

--- Random Access (100000 iterations) ---
Before: Minor faults: 140, Major faults: 0
After: Minor faults: 141, Major faults: 0
Time: 0.002 seconds

--- Second Sequential Access ---
Before: Minor faults: 141, Major faults: 0
After: Minor faults: 141, Major faults: 0
Time: 0.000 seconds

Releasing memory...
After free: Minor faults: 141, Major faults: 0
text


### Анализ результатов

#### Таблица 3.1: Динамика page faults

| Этап | Minor Faults | Major Faults | Время выполнения | Приращение Faults |
|------|-------------|-------------|-----------------|------------------|
| После malloc() | 89 | 0 | - | - |
| Последовательный доступ | 139 | 0 | 0.009s | +50 |
| Случайный доступ | 141 | 0 | 0.002s | +2 |
| Повторный доступ | 141 | 0 | 0.000s | 0 |

### Ключевые наблюдения
1. **malloc() не выделяет физическую память** - только 89 служебных faults
2. **Первый доступ вызывает +50 minor faults** - выделение физических страниц
3. **Случайный доступ быстрее** - меньше операций записи
4. **Повторный доступ мгновенный** - всё в кэше, 0 faults

### Выводы по Заданию C
1. **Demand paging в действии** - Linux выделяет память только при обращении
2. **Locality of reference** - последовательный доступ эффективнее использует кэш
3. **Эффективность кэширования** - повторный доступ не требует выделений
4. **Отсутствие major faults** - достаточно физической памяти

---

## Memory Profiler

### Реализованные функции
- Базовые метрики: VSZ, RSS, PSS, USS
- Анализ сегментов памяти
- Page faults статистика
- Режим мониторинга
- Карта памяти

### Тестирование на различных процессах

#### Тестирование на bash процессе

**Вывод программы:**

=== Process: bash (PID 113528) ===
Virtual Size (VSZ): 8.7 MB
Resident Set (RSS): 7.2 MB
Proportional (PSS): 2.1 MB
Unique Set (USS): 1.6 MB

Page Faults:
Minor: 31768
Major: 90

--- Memory Segments Analysis ---
Heap: 1.3 MB
Stack: 132.0 KB
Libraries: 3.1 MB
Anonymous: 0 B
text


#### Тестирование на Firefox процессе

**Вывод программы:**

=== Process: firefox (PID 9557) ===
Virtual Size (VSZ): 11.6 GB
Resident Set (RSS): 778.0 MB
Proportional (PSS): 591.5 MB
Unique Set (USS): 550.6 MB

Page Faults:
Minor: 6450
Major: 19

--- Memory Segments Analysis ---
Heap: 0 B
Stack: 184.0 KB
Libraries: 618.2 MB
Anonymous: 0 B
text


#### Режим мониторинга

**Вывод программы:**

=== Process: bash (PID 113528) ===
Virtual Size (VSZ): 8.7 MB
Resident Set (RSS): 7.2 MB
Proportional (PSS): 2.1 MB
Unique Set (USS): 1.6 MB
Page Faults: Minor: 31860, Major: 90
==================================================
[обновления каждую секунду...]
text


### Сравнительный анализ

#### Таблица 4.1: Сравнение процессов

| Процесс | VSZ | RSS | PSS | USS | Minor Faults | Major Faults |
|---------|-----|-----|-----|-----|-------------|-------------|
| memory_analysis | 4.6 MB | 3.7 MB | 2.1 MB | 2.1 MB | 0 | 0 |
| bash | 8.7 MB | 7.2 MB | 2.1 MB | 1.6 MB | 31,768 | 90 |
| firefox | 11.6 GB | 778.0 MB | 591.5 MB | 550.6 MB | 6,450 | 19 |

### Выводы
1. **VSZ vs RSS** - Firefox имеет 11.6GB виртуальной, но 778MB физической памяти
2. **PSS более точен** чем RSS для оценки нагрузки
3. **Инструмент полезен для диагностики** использования памяти

---

## Ответы на вопросы

### Виртуальная память
1. **Виртуальная память** - абстракция для изолированного адресного пространства процессов
2. **PSS наиболее точно** показывает потребление, учитывая разделяемые библиотеки
3. **Страница** - единица виртуальной памяти, **Кадр** - единица физической памяти
4. **MMU** трансформирует адреса, **TLB** кэширует трансляции
5. **Copy-on-Write** - страницы копируются только при записи

### Page Faults
6. **Minor fault** - страница в RAM, но не в таблице; **Major fault** - требуется загрузка с диска
7. **Первый доступ** вызывает fault для выделения физической страницы
8. **Уменьшить faults** можно через оптимизацию доступа и использование huge pages
9. **Demand paging** - загрузка по требованию; **Page replacement** - алгоритмы вытеснения

### Memory Mapping
10. **mmap() эффективнее** для больших файлов и случайного доступа
11. **MAP_PRIVATE** - изменения приватны; **MAP_SHARED** - видны всем процессам
12. **Обращение за пределами** файла вызывает SIGSEGV/SIGBUS
13. **Page cache** ускоряет I/O через кэширование страниц

### Файловый I/O
14. **Буферизация** уменьшает системные вызовы через пользовательский и kernel буферы
15. **Маленький буфер** увеличивает количество системных вызовов
16. **O_DIRECT** обходит page cache; **O_SYNC** обеспечивает синхронную запись
17. **fwrite()** буферизованный; **write()** прямой системный вызов

### Файловая система
18. **Inode** хранит метаданные файла кроме имени
19. **Имя файла** хранится в директории для поддержки жестких ссылок
20. **Жесткая ссылка** - запись в директории; **Символьная** - отдельный файл с путем
21. **Ext4 использует** прямые, косвенные и двойные косвенные указатели

### Дисковое планирование
22. **I/O schedulers** оптимизируют порядок обработки запросов
23. **FCFS** - по порядку; **SSTF** - ближайший; **SCAN** - алгоритм лифта
24. **Современные schedulers**: mq-deadline, bfq, kyber, none
25. **Для SSD менее критично** из-за отсутствия механических частей

### Производительность
26. **Фрагментация**: внутренняя - в выделенных блоках; внешняя - в свободной памяти
27. **Swap замедляет** работу при активном использовании
28. **Thrashing** - система тратит время на подкачку; избежать через увеличение RAM
29. **Последовательный доступ быстрее** из-за spatial locality
30. **Cache-friendly код** оптимизирован для эффективного использования кэшей


# Лабораторная 4 — Управление памятью и файловый ввод-вывод (Вариант №2)
# Цель работы:
Изучение механизмов управления памятью в Linux, анализ виртуальной памяти процессов, сравнение методов файлового ввода-вывода и создание утилиты для профилирования памяти.

# Ход работы
## Задание A: анализ виртуальной памяти процесса
Написана программа, которая выделяет память различными способами, выводит свою карту памяти до и после выделения.

Для начала собираем программу: make memory_info.
Затем запустим: ./memory_info

Программа:
Выделит память на стеке (1 KB)
Выделит 10 MB через malloc() (heap)
Выделит 50 MB через mmap() (anonymous mapping)
Покажет метрики памяти и карту памяти 

``` bash
./memory_info 
Memory Info Demo
================

No PID specified. Running demonstration mode.

=== Demonstrating Different Memory Types ===

=== BEFORE memory allocation ===
Memory Metrics for PID 31039:
  VSZ (Virtual):  2748 KB (2.7 MB)
  RSS (Resident): 1532 KB (1.5 MB)
  Data/Heap:      224 KB (0.2 MB)
  Stack:          132 KB (0.1 MB)
  Text (code):    4 KB (0.0 MB)
  Libraries:      1812 KB (1.8 MB)

Press Enter to allocate memory...
1. Stack variable allocated: 1 KB at 0x7fff1689ce80
2. Heap allocated: 10 MB at 0x785da95ff010
   Touching heap memory to trigger page faults... heap touch complete.
3. Anonymous mmap: 50 MB at 0x785da6200000
   Touching mmap memory to trigger page faults... mmap touch complete.

=== AFTER memory allocation ===
Memory Metrics for PID 31039:
  VSZ (Virtual):  64192 KB (62.7 MB)
  RSS (Resident): 63228 KB (61.7 MB)
  Data/Heap:      61668 KB (60.2 MB)
  Stack:          132 KB (0.1 MB)
  Text (code):    4 KB (0.0 MB)
  Libraries:      1812 KB (1.8 MB)

Memory Map:
Address Range      Perms  Size     Path
----------------------------------------------------------------
5cc5f4a4d000-5cc5f4a4e000 r--p      4 KB   /home/alphacrux/rep/BSU-OS-2025/lab4/gr8sub2/Хоменко_Ариана/src/memory_info
5cc5f4a4e000-5cc5f4a4f000 r-xp      4 KB   /home/alphacrux/rep/BSU-OS-2025/lab4/gr8sub2/Хоменко_Ариана/src/memory_info
5cc5f4a4f000-5cc5f4a50000 r--p      4 KB   /home/alphacrux/rep/BSU-OS-2025/lab4/gr8sub2/Хоменко_Ариана/src/memory_info
5cc5f4a50000-5cc5f4a51000 r--p      4 KB   /home/alphacrux/rep/BSU-OS-2025/lab4/gr8sub2/Хоменко_Ариана/src/memory_info
5cc5f4a51000-5cc5f4a52000 rw-p      4 KB   /home/alphacrux/rep/BSU-OS-2025/lab4/gr8sub2/Хоменко_Ариана/src/memory_info
5cc610831000-5cc610852000 rw-p    132 KB   [heap]
785da6200000-785da9400000 rw-p     50 MB   
785da95ff000-785daa000000 rw-p     10 MB   
785daa000000-785daa028000 r--p    160 KB   /usr/lib/x86_64-linux-gnu/libc.so.6
785daa028000-785daa1bd000 r-xp      1 MB   /usr/lib/x86_64-linux-gnu/libc.so.6
785daa1bd000-785daa20c000 r--p    316 KB   /usr/lib/x86_64-linux-gnu/libc.so.6
785daa20c000-785daa210000 r--p     16 KB   /usr/lib/x86_64-linux-gnu/libc.so.6
785daa210000-785daa212000 rw-p      8 KB   /usr/lib/x86_64-linux-gnu/libc.so.6
785daa212000-785daa21f000 rw-p     52 KB   
785daa385000-785daa388000 rw-p     12 KB   
785daa398000-785daa39a000 rw-p      8 KB   
785daa39a000-785daa39c000 r--p      8 KB   [vvar]
785daa39c000-785daa39e000 r--p      8 KB   [vvar_vclock]
785daa39e000-785daa3a0000 r-xp      8 KB   [vdso]
785daa3a0000-785daa3a1000 r--p      4 KB   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
785daa3a1000-785daa3cf000 r-xp    184 KB   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
785daa3cf000-785daa3da000 r--p     44 KB   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
785daa3da000-785daa3dc000 r--p      8 KB   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
785daa3dc000-785daa3dd000 rw-p      4 KB   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
785daa3dd000-785daa3de000 rw-p      4 KB   
7fff1687f000-7fff168a0000 rw-p    132 KB   [stack]
ffffffffff600000-ffffffffff601000 --xp      4 KB   [vsyscall]

Press Enter to free memory and exit...
```
В соседней консоли делаем команды проверки. Находим PID:
``` bash
ps aux | grep memory_info
```
Затем:
```bash
ps -o pid,comm,vsz,rss -p 31039
```
до: 
```
PID    VSZ   RSS
31039   2748  1788
```
после:
```
PID    VSZ   RSS
31039  64192 63228
```

```bash
cat /proc/31039/status | grep -E "^Vm"
```
до: 
```
VmPeak:	    2748 kB
VmSize:	    2748 kB
VmLck:	       0 kB
VmPin:	       0 kB
VmHWM:	    1788 kB
VmRSS:	    1788 kB
VmData:	     224 kB
VmStk:	     132 kB
VmExe:	       4 kB
VmLib:	    1812 kB
VmPTE:	      48 kB
VmSwap:	       0 kB
```
после:
```
VmPeak:	   64192 kB
VmSize:	   64192 kB
VmLck:	       0 kB
VmPin:	       0 kB
VmHWM:	   63228 kB
VmRSS:	   63228 kB
VmData:	   61668 kB
VmStk:	     132 kB
VmExe:	       4 kB
VmLib:	    1812 kB
VmPTE:	     172 kB
VmSwap:	       0 kB
```

| Тип памяти | Описание                                  | До выделения     | После выделения    | Изменение          | Примечания                                                                        |
|------------|-------------------------------------------|------------------|--------------------|--------------------|-----------------------------------------------------------------------------------|
| VSZ        | Полный размер виртуальной памяти процесса | 2748 KB (2.7 MB) | 64192 KB (62.7 MB) | +61444 KB (+60 MB) | Увеличился на точный размер выделенной памяти (10MB + 50MB + небольшие накладные) |
| RSS        | Реальная физическая память в RAM          | 1788 KB (1.7 MB) |                    | +61440 KB (+60 MB) |                                                                                   |
| Data/Heap  | Динамическая память (malloc + mmap)       | 224 KB (0.2 MB)  | 61668 KB (60.2 MB) | +61444 KB (+60 MB) |                                                                                   |
| Stack      | Память стека                              | 132 KB (0.1 MB)  | 132 KB (0.1 MB)    | 0 KB               | Не изменился, т.к. стек имеет фиксированный размер                                |
| Text       | Исполняемый код                           | 4 KB (0.0 MB)    | 4 KB (0.0 MB)      | 0 KB               | Код программы не меняется                                                         |
| Libraries  | Разделяемые библиотеки                    | 1812 KB (1.8 MB) | 1812 KB (1.8 MB)   | 0 KB               | Библиотеки уже были загружены                                                     |

### 1. Почему VSZ намного больше RSS?

VSZ включает всю виртуальную память процесса (зарезервированную, разделяемые библиотеки, memory-mapped файлы), а RSS - только страницы в физической памяти. VSZ > RSS, потому что не вся зарезервированная память используется.

### 2. Где находятся разные типы памяти в адресном пространстве?

* Текст (код): низкие адреса
* Данные/Heap: после кода, растёт вверх
* Stack: высокие адреса, растёт вниз
* Библиотеки: средние адреса
* mmap-регионы: случайные адреса (ASLR)

### 3. Как изменился RSS при выделении 1 MB через malloc vs mmap?

При первом выделении оба метода увеличивают RSS только при реальном обращении к памяти (page faults). При активном использовании оба приводят к полному увеличению RSS на выделенный объем. Разница в механизмах управления, а не в потреблении памяти.

## Задание B: Буферизованный vs Небуферизованный I/O
Сравниваем производительность разных методов файлового ввода-вывода


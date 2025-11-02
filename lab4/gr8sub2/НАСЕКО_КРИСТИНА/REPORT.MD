# Лабораторная 4 — Управление памятью и файловый ввод-вывод
Практика по управлению памятью в Linux: изучим виртуальную память процессов, страничную организацию, page faults, memory mapping, файловый ввод-вывод и буферизацию. Реализуем утилиту для профилирования памяти процессов.

## Цели
- Анализ виртуальной памяти процесса.
- Сравнение производительности методов файлового I/O.
- Изучение механизма Page Faults.
- Анализ работы Page Cache.
- Разработка утилиты мониторинга памяти.
- Исследование планировщика I/O.

Ubuntu 24.04.3 на VirtualBox.

Варинат для лабораторной работы - 2.

## Задания

### A)  Анализ виртуальной памяти процесса
Цель: реализовать программу на C для анализа виртуальной памяти процесса, демонстрирующую различные типы выделения памяти и их отображение в адресном пространстве с использованием /proc файловой системы.



Шаги/решения:
    - Реализовать memory_info.c
    - Компиляция и тестирование файла.
    
Код:
    Код на С представлен в файле memory_info.c, сборка и запуск файла с помощью Makefile.

Команда:
~~~
    ulimit -s unlimited
~~~
  
Цель:
    Измнение размера буфера, чтобы программа корректно отработала.

Команда:
~~~
    make memory_info
    ./memory_info
~~~
  
Результат:
~~~
    Memory Info Demo
================
No PID specified. Running demonstration mode.
This will allocate different types of memory and show the results.


=== Demonstrating Different Memory Types ===

1. Stack variable allocated: 1 KB at 0x7fffa609ab00
2. Heap allocated: 10 MB at 0x737f41ff010
3. Anonymous mmap: 50 MB at 0x737f0e00000
4. File-backed mmap: 5 MB at 0x737f0900000

Memory allocated. Check /proc/8090/maps to see different regions.
Press Enter to see memory info and map...

Memory Metrics for PID 8090:
  VSZ (Virtual):     67.6 MB
  RSS (Resident):    66.4 MB
  Data/Heap:         60.2 MB
  Stack:             132.0 KB
  Text (code):       4.0 KB
  Libraries:         1.7 MB
  Ratio RSS/VSZ:     98.1%

Memory Map:
Address Range      Perms        Size  Path
----------------------------------------------------------------
737f0900000-737f0e00000 rw-s   5.0 MB  /media/sf_Shared/test_mmap_file.bin
737f0e00000-737f4000000 rw-p   50.0 MB  (anonymous)
737f41ff000-737f4c00000 rw-p   10.0 MB  (anonymous)
737f4c00000-737f4c28000 r--p   160.0 KB  /usr/lib/x86_64-linux-gnu/libc.so.6
737f4c28000-737f4db0000 r-xp   1.5 MB  /usr/lib/x86_64-linux-gnu/libc.so.6
737f4db0000-737f4dff000 r--p   316.0 KB  /usr/lib/x86_64-linux-gnu/libc.so.6
737f4dff000-737f4e03000 r--p   16.0 KB  /usr/lib/x86_64-linux-gnu/libc.so.6
737f4e03000-737f4e05000 rw-p   8.0 KB  /usr/lib/x86_64-linux-gnu/libc.so.6
737f4e05000-737f4e12000 rw-p   52.0 KB  (anonymous)
737f4fb3000-737f4fb6000 rw-p   12.0 KB  (anonymous)
737f4fc6000-737f4fc8000 rw-p   8.0 KB  (anonymous)
737f4fc8000-737f4fca000 r--p   8.0 KB  [vvar]
737f4fca000-737f4fcc000 r--p   8.0 KB  [vvar_vclock]
737f4fcc000-737f4fce000 r-xp   8.0 KB  [vdso]
737f4fce000-737f4fcf000 r--p   4.0 KB  /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
737f4fcf000-737f4ffa000 r-xp   172.0 KB  /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
737f4ffa000-737f5004000 r--p   40.0 KB  /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
737f5004000-737f5006000 r--p   8.0 KB  /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
737f5006000-737f5008000 rw-p   8.0 KB  /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
62daf7340000-62daf7341000 r--p   4.0 KB  /media/sf_Shared/memory_info
... (showing first 20 segments)

Total mapped memory: 67.3 MB in 20 segments

Press Enter to free memory and exit...
~~~

Команда:
~~~
    ./memory_info $$
~~~
  
Результат:
~~~
    Memory Info Demo
================
Analyzing process 8031

Memory Metrics for PID 8031:
  VSZ (Virtual):     8.8 MB
  RSS (Resident):    5.6 MB
  Data/Heap:         1.6 MB
  Stack:             132.0 KB
  Text (code):       956.0 KB
  Libraries:         1.8 MB
  Ratio RSS/VSZ:     63.5%

Memory Map:
Address Range      Perms        Size  Path
----------------------------------------------------------------
56f4d2ab2000-56f4d2ae2000 r--p   192.0 KB  /usr/bin/bash
56f4d2ae2000-56f4d2bd1000 r-xp   956.0 KB  /usr/bin/bash
56f4d2bd1000-56f4d2c06000 r--p   212.0 KB  /usr/bin/bash
56f4d2c06000-56f4d2c0a000 r--p   16.0 KB  /usr/bin/bash
56f4d2c0a000-56f4d2c13000 rw-p   36.0 KB  /usr/bin/bash
56f4d2c13000-56f4d2c1e000 rw-p   44.0 KB  (anonymous)
56f4fb108000-56f4fb274000 rw-p   1.4 MB  [heap]
7cf3d5800000-7cf3d5aec000 r--p   2.9 MB  /usr/lib/locale/locale-archive
7cf3d5c00000-7cf3d5c28000 r--p   160.0 KB  /usr/lib/x86_64-linux-gnu/libc.so.6
7cf3d5c28000-7cf3d5db0000 r-xp   1.5 MB  /usr/lib/x86_64-linux-gnu/libc.so.6
7cf3d5db0000-7cf3d5dff000 r--p   316.0 KB  /usr/lib/x86_64-linux-gnu/libc.so.6
7cf3d5dff000-7cf3d5e03000 r--p   16.0 KB  /usr/lib/x86_64-linux-gnu/libc.so.6
7cf3d5e03000-7cf3d5e05000 rw-p   8.0 KB  /usr/lib/x86_64-linux-gnu/libc.so.6
7cf3d5e05000-7cf3d5e12000 rw-p   52.0 KB  (anonymous)
7cf3d5f24000-7cf3d5f44000 r--p   128.0 KB  /usr/share/locale-langpack/ru/LC_MESSAGES/bash.mo
7cf3d5f44000-7cf3d5f73000 r--p   188.0 KB  /usr/share/locale-langpack/ru/LC_MESSAGES/libc.mo
7cf3d5f73000-7cf3d5f76000 rw-p   12.0 KB  (anonymous)
7cf3d5f76000-7cf3d5f84000 r--p   56.0 KB  /usr/lib/x86_64-linux-gnu/libtinfo.so.6.4
7cf3d5f84000-7cf3d5f97000 r-xp   76.0 KB  /usr/lib/x86_64-linux-gnu/libtinfo.so.6.4
7cf3d5f97000-7cf3d5fa5000 r--p   56.0 KB  /usr/lib/x86_64-linux-gnu/libtinfo.so.6.4
... (showing first 20 segments)

Total mapped memory: 8.3 MB in 20 segments
~~~

Команда:
~~~
   ps -o pid,comm,vsz,rss -p 8031
cat /proc/8031/status | grep -E "^Vm"
cat /proc/8031/smaps_rollup
~~~

Резальтат:
~~~
    PID COMMAND            VSZ   RSS
   8031 bash              8984  5704
VmPeak:	    8984 kB
VmSize:	    8984 kB
VmLck:	       0 kB
VmPin:	       0 kB
VmHWM:	    5704 kB
VmRSS:	    5704 kB
VmData:	    1628 kB
VmStk:	     132 kB
VmExe:	     956 kB
VmLib:	    1824 kB
VmPTE:	      60 kB
VmSwap:	       0 kB
56f4d2ab2000-7ffcadae1000 ---p 00000000 00:00 0                          [rollup]
Rss:                5816 kB
Pss:                3625 kB
Pss_Dirty:          1648 kB
Pss_Anon:           1648 kB
Pss_File:           1977 kB
Pss_Shmem:             0 kB
Shared_Clean:       2704 kB
Shared_Dirty:          0 kB
Private_Clean:      1464 kB
Private_Dirty:      1648 kB
Referenced:         5816 kB
Anonymous:          1648 kB
KSM:                   0 kB
LazyFree:              0 kB
AnonHugePages:         0 kB
ShmemPmdMapped:        0 kB
FilePmdMapped:         0 kB
Shared_Hugetlb:        0 kB
Private_Hugetlb:       0 kB
Swap:                  0 kB
SwapPss:               0 kB
Locked:                0 kB
~~~


Вопросы для анализа:
1. VSZ большк RSS из-за отложенного выделения (demand paging) и разделяемых ресурсов.
2. Память организована иерархически от низких адресов (программа) к высоким (стек).
3. malloc и mmap по-разному влияют на RSS - mmap создает отдельные сегменты, malloc расширяет heap.

Выводы:
  Без синхронизации результат многопточного инкремента отличается от ожидаемого (expected > actual).
  Оба подхода: `mutex` и `atomic` - показали верный результат, т.е. expected равняется actual, однако для одинаковых входных данных `atomic` работает быстрее `mutex`.
    
Использование ИИ:
    Помощь в реализации кода и анализа результатов.


### B) Буферизованный vs Небуферизованный I/O
Цель: Сравнить производительность стандартной библиотеки C (fopen/fwrite/fread) и системных вызовов (open/write/read) с разными размерами буфера.

Шаги/решения:
    - Доработать код на С в файле io_benchmark.c
    - Проверка корректности реализации.

Код:
    Код на С представлен в файле io_benchmark.c, сборка и запуск файла с помощью Makefile.

Команда:
~~~
   make io_benchmark.c
    ./io_benchmark --size 100
~~~

Резальтат:
~~~
    I/O Benchmark - Variant 2
==========================
Test file size: 100 MB

========================================
Benchmark: I/O Methods Comparison
========================================
File size: 100 MB

--- Testing with 512 byte buffer ---
=== fwrite() with buffer=512 bytes ===
Time: 1.073 seconds
Throughput: 93.20 MB/s
Operations: 204800
=== write() with buffer=512 bytes ===
Time: 15.434 seconds
Throughput: 6.48 MB/s
Syscalls: 204800

--- Testing with 4096 byte buffer ---
=== fwrite() with buffer=4096 bytes ===
Time: 1.113 seconds
Throughput: 89.88 MB/s
Operations: 25600
=== write() with buffer=4096 bytes ===
Time: 1.997 seconds
Throughput: 50.08 MB/s
Syscalls: 25600

--- Testing with 65536 byte buffer ---
=== fwrite() with buffer=65536 bytes ===
Time: 0.357 seconds
Throughput: 280.26 MB/s
Operations: 1600
=== write() with buffer=65536 bytes ===
Time: 0.262 seconds
Throughput: 381.51 MB/s
Syscalls: 1600

--- Testing Read Methods ---
=== write() with buffer=65536 bytes ===
Time: 0.253 seconds
Throughput: 395.39 MB/s
Syscalls: 1600
=== fread() ===
Time: 0.221 seconds
Throughput: 452.89 MB/s
Checksum: 6920601600
=== read() ===
Time: 0.228 seconds
Throughput: 437.88 MB/s
Checksum: 6920601600

========================================
Benchmark: Buffer Size Impact
========================================
Testing write() with different buffer sizes:
File size: 100 MB

Buffer:     512 bytes -> === write() with buffer=512 bytes ===
Time: 13.706 seconds
Throughput: 7.30 MB/s
Syscalls: 204800
Buffer:    1024 bytes -> === write() with buffer=1024 bytes ===
Time: 6.843 seconds
Throughput: 14.61 MB/s
Syscalls: 102400
Buffer:    4096 bytes -> === write() with buffer=4096 bytes ===
Time: 2.021 seconds
Throughput: 49.47 MB/s
Syscalls: 25600
Buffer:    8192 bytes -> === write() with buffer=8192 bytes ===
Time: 1.070 seconds
Throughput: 93.44 MB/s
Syscalls: 12800
Buffer:   16384 bytes -> === write() with buffer=16384 bytes ===
Time: 0.578 seconds
Throughput: 172.98 MB/s
Syscalls: 6400
Buffer:   65536 bytes -> === write() with buffer=65536 bytes ===
Time: 0.244 seconds
Throughput: 409.28 MB/s
Syscalls: 1600
Buffer: 1048576 bytes -> === write() with buffer=1048576 bytes ===
Time: 0.169 seconds
Throughput: 591.07 MB/s
Syscalls: 100

========================================
Benchmark completed!
========================================
~~~

Команда:
~~~
   strace -c ./io_benchmark --size 100
~~~

Резальтат:
~~~
   % time     seconds  usecs/call     calls    errors syscall
------ ----------- ----------- --------- --------- ----------------
 99,46   17,177898          27    616188           write
  0,32    0,055425          17      3203           read
  0,19    0,032176       16088         2           wait4
  0,01    0,001997         110        18           openat
  0,01    0,001302          93        14           unlink
  0,00    0,000748          41        18           close
  0,00    0,000477          36        13           clock_nanosleep
  0,00    0,000341         341         1           execve
  0,00    0,000200          50         4           munmap
  0,00    0,000147          18         8           rt_sigprocmask
  0,00    0,000145          72         2           clone3
  0,00    0,000121          15         8           rt_sigaction
  0,00    0,000120          10        11           mmap
  0,00    0,000049           6         8           fstat
  0,00    0,000040           3        11           fsync
  0,00    0,000032          10         3           brk
  0,00    0,000018           6         3           mprotect
  0,00    0,000006           3         2           pread64
  0,00    0,000006           6         1           newfstatat
  0,00    0,000004           4         1           getrandom
  0,00    0,000003           3         1           arch_prctl
  0,00    0,000003           3         1           set_tid_address
  0,00    0,000003           3         1           set_robust_list
  0,00    0,000003           3         1           prlimit64
  0,00    0,000003           3         1           rseq
  0,00    0,000000           0         1         1 access
------ ----------- ----------- --------- --------- ----------------
100,00   17,271267          27    619525         1 total

~~~

Выводы:
     граики сравнения буфер vs пропускная способность и буфер vs время находятся в паке screenshots в файлах buffer_vs_throughput.png и buffer_vs_time.png. Оптимальный размер буфера 64 KB - совпадение с размерами кешей (следовательно их эффективное использование), хороший баланс вызовов syscall'ов. По данным о syscall'ах: размеры буферов 512B - 4KB - 64KB - 1MB => разность количества вызовов меньше в: x1 - x8 - x128 - x2048.
    
    
Использование ИИ:
    Помощь в понимании работы кода на С в файле io_benchmark.c, также проверка правильности кода в io_benchmark.c при окончании его реализации.


### C) Дисковый I/O и планирование
Цель: Проанализировать дисковую активность системы при интенсивных операциях I/O, изучить метрики производительности и влияние планировщика ввода-вывода.
    

Шаги/решения:
    - Запустите программу, активно читающую/пишущую на диск.
    - Мониторить I/O.
    - Проанализировать результаты мониторинга.
    
Команда:
~~~
    mkdir -p /tmp/test_load && find /usr -type f -size +1M -exec cp {} /tmp/test_load/ \; 2>/dev/null &
CP_PID=$!
~~~

Цель: создание нагрузки для мониторинга.

Команда:
~~~
    iostat -x 1 10
~~~

Результат:
~~~
    avg-cpu:  %user   %nice %system %iowait  %steal   %idle
Device            r/s     rkB/s   rrqm/s  %rrqm r_await rareq-sz     w/s     wkB/s   wrqm/s  %wrqm w_await wareq-sz     d/s     dkB/s   drqm/s  %drqm d_await dareq-sz     f/s f_await  aqu-sz  %util
sda             10,51    610,12     2,81  21,10   12,47    58,03    6,57    726,70    12,65  65,81   19,92   110,57    0,00      0,00     0,00   0,00    0,00     0,00    2,54    7,81    0,28   6,54
avg-cpu:  %user   %nice %system %iowait  %steal   %idle
Device            r/s     rkB/s   rrqm/s  %rrqm r_await rareq-sz     w/s     wkB/s   wrqm/s  %wrqm w_await wareq-sz     d/s     dkB/s   drqm/s  %drqm d_await dareq-sz     f/s f_await  aqu-sz  %util
sda              0,00      0,00     0,00   0,00    0,00     0,00    0,00      0,00     0,00   0,00    0,00     0,00    0,00      0,00     0,00   0,00    0,00     0,00    0,00    0,00    0,00   0,00
avg-cpu:  %user   %nice %system %iowait  %steal   %idle
Device            r/s     rkB/s   rrqm/s  %rrqm r_await rareq-sz     w/s     wkB/s   wrqm/s  %wrqm w_await wareq-sz     d/s     dkB/s   drqm/s  %drqm d_await dareq-sz     f/s f_await  aqu-sz  %util
sda              0,00      0,00     0,00   0,00    0,00     0,00    0,00      0,00     0,00   0,00    0,00     0,00    0,00      0,00     0,00   0,00    0,00     0,00    0,00    0,00    0,00   0,00
avg-cpu:  %user   %nice %system %iowait  %steal   %idle
Device            r/s     rkB/s   rrqm/s  %rrqm r_await rareq-sz     w/s     wkB/s   wrqm/s  %wrqm w_await wareq-sz     d/s     dkB/s   drqm/s  %drqm d_await dareq-sz     f/s f_await  aqu-sz  %util
sda              0,00      0,00     0,00   0,00    0,00     0,00   10,00    264,00    56,00  84,85   27,70    26,40    0,00      0,00     0,00   0,00    0,00     0,00    8,00   34,50    0,55  27,50
avg-cpu:  %user   %nice %system %iowait  %steal   %idle
Device            r/s     rkB/s   rrqm/s  %rrqm r_await rareq-sz     w/s     wkB/s   wrqm/s  %wrqm w_await wareq-sz     d/s     dkB/s   drqm/s  %drqm d_await dareq-sz     f/s f_await  aqu-sz  %util
sda              0,00      0,00     0,00   0,00    0,00     0,00    0,00      0,00     0,00   0,00    0,00     0,00    0,00      0,00     0,00   0,00    0,00     0,00    0,00    0,00    0,00   0,00
avg-cpu:  %user   %nice %system %iowait  %steal   %idle
Device            r/s     rkB/s   rrqm/s  %rrqm r_await rareq-sz     w/s     wkB/s   wrqm/s  %wrqm w_await wareq-sz     d/s     dkB/s   drqm/s  %drqm d_await dareq-sz     f/s f_await  aqu-sz  %util
sda              0,00      0,00     0,00   0,00    0,00     0,00   37,00  12680,00     4,00   9,76    6,84   342,70    0,00      0,00     0,00   0,00    0,00     0,00    0,00    0,00    0,25   4,90
avg-cpu:  %user   %nice %system %iowait  %steal   %idle
Device            r/s     rkB/s   rrqm/s  %rrqm r_await rareq-sz     w/s     wkB/s   wrqm/s  %wrqm w_await wareq-sz     d/s     dkB/s   drqm/s  %drqm d_await dareq-sz     f/s f_await  aqu-sz  %util
sda              0,00      0,00     0,00   0,00    0,00     0,00    0,00      0,00     0,00   0,00    0,00     0,00    0,00      0,00     0,00   0,00    0,00     0,00    0,00    0,00    0,00   0,00
avg-cpu:  %user   %nice %system %iowait  %steal   %idle
Device            r/s     rkB/s   rrqm/s  %rrqm r_await rareq-sz     w/s     wkB/s   wrqm/s  %wrqm w_await wareq-sz     d/s     dkB/s   drqm/s  %drqm d_await dareq-sz     f/s f_await  aqu-sz  %util
sda              0,00      0,00     0,00   0,00    0,00     0,00    0,00      0,00     0,00   0,00    0,00     0,00    0,00      0,00     0,00   0,00    0,00     0,00    0,00    0,00    0,00   0,00
avg-cpu:  %user   %nice %system %iowait  %steal   %idle
Device            r/s     rkB/s   rrqm/s  %rrqm r_await rareq-sz     w/s     wkB/s   wrqm/s  %wrqm w_await wareq-sz     d/s     dkB/s   drqm/s  %drqm d_await dareq-sz     f/s f_await  aqu-sz  %util
sda              0,00      0,00     0,00   0,00    0,00     0,00    0,96     42,31     9,62  90,91    0,00    44,00    0,00      0,00     0,00   0,00    0,00     0,00    0,00    0,00    0,00   0,10
avg-cpu:  %user   %nice %system %iowait  %steal   %idle
Device            r/s     rkB/s   rrqm/s  %rrqm r_await rareq-sz     w/s     wkB/s   wrqm/s  %wrqm w_await wareq-sz     d/s     dkB/s   drqm/s  %drqm d_await dareq-sz     f/s f_await  aqu-sz  %util
sda              0,00      0,00     0,00   0,00    0,00     0,00    1,00      4,00     0,00   0,00  288,00     4,00    0,00      0,00     0,00   0,00    0,00     0,00    2,00  143,50    0,57  28,80
~~~

Команда:
~~~
    pidstat -d 1 10
~~~

Результат:
~~~
    inux 6.14.0-29-generic (chris-VirtualBox-pc) 	02.11.2025 	_x86_64_	(8 CPU)

18:25:32      UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command
18:25:33     1000      9593      0,00 506930,69      0,00       0  bash
18:25:33     1000     10739      0,00      3,96      0,00       0  iostat

18:25:33      UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command
18:25:34     1000      3334      0,00    124,00      0,00       0  tracker-miner-f
18:25:34     1000     10739      0,00      4,00      0,00       0  iostat

18:25:34      UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command
18:25:35     1000      3334      0,00      4,00      0,00       0  tracker-miner-f
18:25:35     1000     10739      0,00      4,00      0,00       0  iostat

18:25:35      UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command
18:25:36     1000     10739      0,00      4,00      0,00       0  iostat

18:25:36      UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command
18:25:37     1000     10739      0,00      7,92      0,00       0  iostat
~~~

Команда:
~~~
    sudo iotop -b -n 10
~~~

Результат:
~~~
    Total DISK READ:         0.00 B/s | Total DISK WRITE:         4.79 K/s
Current DISK READ:       0.00 B/s | Current DISK WRITE:       0.00 B/s
    TID  PRIO  USER     DISK READ  DISK WRITE  SWAPIN      IO    COMMAND
      1 be/4 root        0.00 B/s    0.00 B/s ?unavailable?  systemd --system --deserialize=33 splash
      2 be/4 root        0.00 B/s    0.00 B/s ?unavailable?  [kthreadd]
      3 be/4 root        0.00 B/s    0.00 B/s ?unavailable?  [pool_workqueue_release]
      4 be/0 root        0.00 B/s    0.00 B/s ?unavailable?  [kworker/R-rcu_gp]
      5 be/0 root        0.00 B/s    0.00 B/s ?unavailable?  [kworker/R-sync_wq]
      6 be/0 root        0.00 B/s    0.00 B/s ?unavailable?  [kworker/R-kvfree_rcu_reclaim]
      7 be/0 root        0.00 B/s    0.00 B/s ?unavailable?  [kworker/R-slub_flushwq]
      8 be/0 root        0.00 B/s    0.00 B/s ?unavailable?  [kworker/R-netns]
     11 be/0 root        0.00 B/s    0.00 B/s ?unavailable?  [kworker/0:0H-events_highpri]
     12 be/4 root        0.00 B/s    0.00 B/s ?unavailable?  [kworker/u32:0-ipv6_addrconf]
     13 be/0 root        0.00 B/s    0.00 B/s ?unavailable?  [kworker/R-mm_percpu_wq]
     14 be/4 root        0.00 B/s    0.00 B/s ?unavailable?  [rcu_tasks_kthread]
     15 be/4 root        0.00 B/s    0.00 B/s ?unavailable?  [rcu_tasks_rude_kthread]
     16 be/4 root        0.00 B/s    0.00 B/s ?unavailable?  [rcu_tasks_trace_kthread]
     17 be/4 root        0.00 B/s    0.00 B/s ?unavailable?  [ksoftirqd/0]
     18 be/4 root        0.00 B/s    0.00 B/s ?unavailable?  [rcu_preempt]
     19 be/4 root        0.00 B/s    0.00 B/s ?unavailable?  [rcu_exp_par_gp_kthread_worker/0]

~~~

Команда:
~~~
    cat /proc/11189/io
~~~

Результат:
~~~
    rchar: 0
  wchar: 0
  syscr: 1
  syscw: 0
  read_bytes: 0
  write_bytes: 0
  cancelled_write_bytes: 0
  
  rchar: 5262344
  wchar: 5243053
  syscr: 40
  syscw: 9
  read_bytes: 110592
  write_bytes: 5242880
  cancelled_write_bytes: 0
  
  rchar: 10525627
  wchar: 10486106
  syscr: 77
  syscw: 18
  read_bytes: 110592
  write_bytes: 10485760
  cancelled_write_bytes: 0
  
  rchar: 15788910
  wchar: 15729159
  syscr: 114
  syscw: 27
  read_bytes: 110592
  write_bytes: 15728640
  cancelled_write_bytes: 0
  
  rchar: 21052193
  wchar: 20972212
  syscr: 151
  syscw: 36
  read_bytes: 110592
  write_bytes: 20971520
  cancelled_write_bytes: 0
  
  rchar: 26315476
  wchar: 26215265
  syscr: 188
  syscw: 45
  read_bytes: 110592
  write_bytes: 26214400
  cancelled_write_bytes: 0
  
  rchar: 31578759
  wchar: 31458318
  syscr: 225
  syscw: 54
  read_bytes: 110592
  write_bytes: 31457280
  cancelled_write_bytes: 0
  
  rchar: 36842042
  wchar: 36701371
  syscr: 262
  syscw: 63
  read_bytes: 110592
  write_bytes: 36700160
  cancelled_write_bytes: 0
  
  rchar: 42105325
  wchar: 41944424
  syscr: 299
  syscw: 72
  read_bytes: 110592
  write_bytes: 41943040
  cancelled_write_bytes: 0
  
  rchar: 47368608
  wchar: 47187476
  syscr: 336
  syscw: 81
  read_bytes: 110592
  write_bytes: 47185920
  cancelled_write_bytes: 0
~~~

Команда - Измените I/O scheduler:
~~~
    echo "bfq" | sudo tee /sys/block/sda/queue/scheduler
~~~

Результат тех же команд монитронига, но в одном блоке и для измененного I/O scheduler:
~~~
avg-cpu:  %user   %nice %system %iowait  %steal   %idle
Device            r/s     rkB/s   rrqm/s  %rrqm r_await rareq-sz     w/s     wkB/s   wrqm/s  %wrqm w_await wareq-sz     d/s     dkB/s   drqm/s  %drqm d_await dareq-sz     f/s f_await  aqu-sz  %util
sda              9,49    549,73     2,54  21,10   12,49    57,96   19,35    849,54    23,28  54,61    8,91    43,91    0,00      0,00     0,00   0,00    0,00     0,00    7,47    3,37    0,32   6,91
avg-cpu:  %user   %nice %system %iowait  %steal   %idle
Device            r/s     rkB/s   rrqm/s  %rrqm r_await rareq-sz     w/s     wkB/s   wrqm/s  %wrqm w_await wareq-sz     d/s     dkB/s   drqm/s  %drqm d_await dareq-sz     f/s f_await  aqu-sz  %util
sda            222,00  40048,00   104,00  31,90    6,70   180,40    1,00      4,00     0,00   0,00    2,00     4,00    0,00      0,00     0,00   0,00    0,00     0,00    2,00    0,50    1,49  93,30
avg-cpu:  %user   %nice %system %iowait  %steal   %idle
Device            r/s     rkB/s   rrqm/s  %rrqm r_await rareq-sz     w/s     wkB/s   wrqm/s  %wrqm w_await wareq-sz     d/s     dkB/s   drqm/s  %drqm d_await dareq-sz     f/s f_await  aqu-sz  %util
sda            279,00  65224,00   139,00  33,25    7,14   233,78    0,00      0,00     0,00   0,00    0,00     0,00    0,00      0,00     0,00   0,00    0,00     0,00    0,00    0,00    1,99  94,80
avg-cpu:  %user   %nice %system %iowait  %steal   %idle
Device            r/s     rkB/s   rrqm/s  %rrqm r_await rareq-sz     w/s     wkB/s   wrqm/s  %wrqm w_await wareq-sz     d/s     dkB/s   drqm/s  %drqm d_await dareq-sz     f/s f_await  aqu-sz  %util
sda            210,00  40816,00    82,00  28,08    6,34   194,36   10,00    252,00    53,00  84,13   37,10    25,20    0,00      0,00     0,00   0,00    0,00     0,00    8,00    6,62    1,75  94,70
avg-cpu:  %user   %nice %system %iowait  %steal   %idle
Device            r/s     rkB/s   rrqm/s  %rrqm r_await rareq-sz     w/s     wkB/s   wrqm/s  %wrqm w_await wareq-sz     d/s     dkB/s   drqm/s  %drqm d_await dareq-sz     f/s f_await  aqu-sz  %util
sda            280,00  49620,00     0,00   0,00    5,96   177,21    0,00      0,00     0,00   0,00    0,00     0,00    0,00      0,00     0,00   0,00    0,00     0,00    0,00    0,00    1,67  95,30
avg-cpu:  %user   %nice %system %iowait  %steal   %idle
Device            r/s     rkB/s   rrqm/s  %rrqm r_await rareq-sz     w/s     wkB/s   wrqm/s  %wrqm w_await wareq-sz     d/s     dkB/s   drqm/s  %drqm d_await dareq-sz     f/s f_await  aqu-sz  %util
sda            285,15  46867,33    54,46  16,03    5,31   164,36    0,00      0,00     0,00   0,00    0,00     0,00    0,00      0,00     0,00   0,00    0,00     0,00    0,00    0,00    1,51  93,47
avg-cpu:  %user   %nice %system %iowait  %steal   %idle
Device            r/s     rkB/s   rrqm/s  %rrqm r_await rareq-sz     w/s     wkB/s   wrqm/s  %wrqm w_await wareq-sz     d/s     dkB/s   drqm/s  %drqm d_await dareq-sz     f/s f_await  aqu-sz  %util
sda            652,00  41680,00     0,00   0,00    2,44    63,93   16,00   7408,00     3,00  15,79   74,31   463,00    0,00      0,00     0,00   0,00    0,00     0,00    0,00    0,00    2,78  92,20
avg-cpu:  %user   %nice %system %iowait  %steal   %idle
Device            r/s     rkB/s   rrqm/s  %rrqm r_await rareq-sz     w/s     wkB/s   wrqm/s  %wrqm w_await wareq-sz     d/s     dkB/s   drqm/s  %drqm d_await dareq-sz     f/s f_await  aqu-sz  %util
sda            265,00  46140,00     0,00   0,00    5,62   174,11    0,00      0,00     0,00   0,00    0,00     0,00    0,00      0,00     0,00   0,00    0,00     0,00    0,00    0,00    1,49  93,80
avg-cpu:  %user   %nice %system %iowait  %steal   %idle
Device            r/s     rkB/s   rrqm/s  %rrqm r_await rareq-sz     w/s     wkB/s   wrqm/s  %wrqm w_await wareq-sz     d/s     dkB/s   drqm/s  %drqm d_await dareq-sz     f/s f_await  aqu-sz  %util
sda             55,00   3080,00    18,00  24,66   11,02    56,00   35,00  27032,00    25,00  41,67  117,31   772,34    0,00      0,00     0,00   0,00    0,00     0,00    2,00   71,00    4,85  81,50
avg-cpu:  %user   %nice %system %iowait  %steal   %idle
Device            r/s     rkB/s   rrqm/s  %rrqm r_await rareq-sz     w/s     wkB/s   wrqm/s  %wrqm w_await wareq-sz     d/s     dkB/s   drqm/s  %drqm d_await dareq-sz     f/s f_await  aqu-sz  %util
sda              0,99      3,96     0,00   0,00  657,00     4,00   48,51  51552,48     0,00   0,00  348,57  1062,61    0,00      0,00     0,00   0,00    0,00     0,00    0,00    0,00   17,56  82,18
~~~

~~~
Linux 6.14.0-29-generic (chris-VirtualBox-pc) 	02.11.2025 	_x86_64_	(8 CPU)

18:41:41      UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command
18:41:42     1000     11276      0,00      3,92      0,00       0  iostat
18:41:42     1000     11279  19317,65  23780,39      0,00       0  find
18:41:42     1000     11289  22705,88  22211,76      0,00       0  cp

18:41:42      UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command
18:41:43     1000      3334      0,00    120,00      0,00       0  tracker-miner-f
18:41:43     1000     11276      0,00      4,00      0,00       0  iostat
18:41:43     1000     11279  87176,00  88480,00      0,00       0  find

18:41:43      UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command
18:41:44     1000      3334      0,00      4,00      0,00       0  tracker-miner-f
18:41:44     1000     11276      0,00      4,00      0,00       0  iostat
18:41:44     1000     11279  30892,00  30920,00      0,00       0  find
18:41:44     1000     11310  11520,00  13440,00      0,00       0  cp

18:41:44      UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command
18:41:45     1000     11276      0,00      4,00      0,00       0  iostat
~~~

~~~
Total DISK READ:         0.00 B/s | Total DISK WRITE:         4.65 K/s
Current DISK READ:       0.00 B/s | Current DISK WRITE:       0.00 B/s
    TID  PRIO  USER     DISK READ  DISK WRITE  SWAPIN      IO    COMMAND
      1 be/4 root        0.00 B/s    0.00 B/s ?unavailable?  systemd --system --deserialize=33 splash
      2 be/4 root        0.00 B/s    0.00 B/s ?unavailable?  [kthreadd]
      3 be/4 root        0.00 B/s    0.00 B/s ?unavailable?  [pool_workqueue_release]
      4 be/0 root        0.00 B/s    0.00 B/s ?unavailable?  [kworker/R-rcu_gp]
      5 be/0 root        0.00 B/s    0.00 B/s ?unavailable?  [kworker/R-sync_wq]
      6 be/0 root        0.00 B/s    0.00 B/s ?unavailable?  [kworker/R-kvfree_rcu_reclaim]
      7 be/0 root        0.00 B/s    0.00 B/s ?unavailable?  [kworker/R-slub_flushwq]
      8 be/0 root        0.00 B/s    0.00 B/s ?unavailable?  [kworker/R-netns]
     11 be/0 root        0.00 B/s    0.00 B/s ?unavailable?  [kworker/0:0H-events_highpri]
     12 be/4 root        0.00 B/s    0.00 B/s ?unavailable?  [kworker/u32:0-ipv6_addrconf]
     13 be/0 root        0.00 B/s    0.00 B/s ?unavailable?  [kworker/R-mm_percpu_wq]
     14 be/4 root        0.00 B/s    0.00 B/s ?unavailable?  [rcu_tasks_kthread]
     15 be/4 root        0.00 B/s    0.00 B/s ?unavailable?  [rcu_tasks_rude_kthread]
     16 be/4 root        0.00 B/s    0.00 B/s ?unavailable?  [rcu_tasks_trace_kthread]
     17 be/4 root        0.00 B/s    0.00 B/s ?unavailable?  [ksoftirqd/0]
     18 be/4 root        0.00 B/s    0.00 B/s ?unavailable?  [rcu_preempt]
     19 be/4 root        0.00 B/s    0.00 B/s ?unavailable?  [rcu_exp_par_gp_kthread_worker/0]
~~~

~~~
rchar: 0
wchar: 0
syscr: 1
syscw: 0
read_bytes: 0
write_bytes: 0
cancelled_write_bytes: 0

rchar: 5262344
wchar: 5243052
syscr: 40
syscw: 9
read_bytes: 0
write_bytes: 5242880
cancelled_write_bytes: 0

rchar: 10525627
wchar: 10486105
syscr: 77
syscw: 18
read_bytes: 0
write_bytes: 10485760
cancelled_write_bytes: 0

rchar: 15788910
wchar: 15729158
syscr: 114
syscw: 27
read_bytes: 0
write_bytes: 15728640
cancelled_write_bytes: 0

rchar: 21052193
wchar: 20972211
syscr: 151
syscw: 36
read_bytes: 0
write_bytes: 20971520
cancelled_write_bytes: 0

rchar: 26315476
wchar: 26215264
syscr: 188
syscw: 45
read_bytes: 0
write_bytes: 26214400
cancelled_write_bytes: 0

rchar: 31578759
wchar: 31458317
syscr: 225
syscw: 54
read_bytes: 0
write_bytes: 31457280
cancelled_write_bytes: 0

rchar: 36842042
wchar: 36701370
syscr: 262
syscw: 63
read_bytes: 0
write_bytes: 36700160
cancelled_write_bytes: 0

rchar: 42105325
wchar: 41944423
syscr: 299
syscw: 72
read_bytes: 0
write_bytes: 41943040
cancelled_write_bytes: 0

rchar: 47368608
wchar: 47187476
syscr: 336
syscw: 81
read_bytes: 0
write_bytes: 47185920
cancelled_write_bytes: 0
~~~

Выводы:
   | Метрика | До BFQ | После BFQ | Изменение |
|---------|--------|-----------|-----------|
| **Скорость чтения** | 0.6 MB/s | 65 MB/s | **+10,800%** |
| **Скорость записи** | 12.4 MB/s | 51.5 MB/s | **+315%** |
| **Утилизация диска** | 27.5% | 95.3% | **+246%** |
| **Задержка чтения** | 12.47 ms | 2.44 ms | **-80%** |
| **Задержка записи** | 19.92 ms | 348 ms | **+1,647%** |
| **Длина очереди** | 0.28 | 17.56 | **+6,171%** |

Анализ данных:
  плюсы BFQ: выше пропускная способность, более эффективное использование ресурсов, а вот минусы: сильно выросшие задержки записи, большая очередь запросов, есть пики и интервалы - неравномерная производительность.

Использование ИИ:
    Помощь в создании нагрузки для мониторинга. 


### Практика: Memory Profiler
Цель: Реализуйте утилиту memory_profiler, которая выводит: метрики памяти, карты памяти, динамический мониторинг, сравнение процессов.
    

Шаги/решения:
    - Реализизовать утилиту memory_profiler.c
    - Компиляция и тестирование утилиты.

Код:
  Код утилиты представлен в файле memory_profiler.c, сборка и запуск файла с помощью Makefile.

1. Компиляция программы
Команда:
~~~
  make memory_profiler
~~~


2. Тестирование на разных процессах
Команда:
~~~
  ./memory_profiler $$
~~~

Результат:
~~~
  Process: bash (PID 12382)
=====================================

Memory Metrics:
  VSZ (Virtual):        8.6 MB
  RSS (Resident):       5.4 MB
  Data/Heap:            1.7 MB
  Stack:              132 KB
  Executable:         956 KB
  Libraries:            1.8 MB
  Swap:                 0 KB

Detailed Memory (from smaps):
  PSS (Proportional):   3.6 MB
  Shared Clean:         2.6 MB
  Shared Dirty:         0 KB
  Private Clean:        1.4 MB
  Private Dirty:        1.8 MB

Page Faults:
  Minor: 3949
  Major: 0

Memory Map Summary (Top 10 segments):
=====================================
        2992 KB r--p     file   /usr/lib/locale/locale-archive
        1588 KB rw-p     heap   [heap]
        1568 KB r-xp     library /usr/lib/x86_64-linux-gnu/libc.so.6
         956 KB r-xp     file   /usr/bin/bash
         316 KB r--p     library /usr/lib/x86_64-linux-gnu/libc.so.6
         212 KB r--p     file   /usr/bin/bash
         192 KB r--p     file   /usr/bin/bash
         172 KB r-xp     library /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
         160 KB r--p     library /usr/lib/x86_64-linux-gnu/libc.so.6
         132 KB rw-p     stack  [stack]

Shared Libraries Analysis:
==========================
    2068 KB ( 5 seg) /usr/lib/x86_64-linux-gnu/libc.so.6
     232 KB ( 5 seg) /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
     208 KB ( 5 seg) /usr/lib/x86_64-linux-gnu/libtinfo.so.6.4
~~~
Скриншот цветов можно увидеть на картинке image.png - папка screenshots.


Команда:
~~~
  ./memory_profiler 1
~~~

Результат:
~~~
  Process: systemd (PID 1)
=====================================

Memory Metrics:
  VSZ (Virtual):       22.4 MB
  RSS (Resident):      13.6 MB
  Data/Heap:            3.8 MB
  Stack:              132 KB
  Executable:          44 KB
  Libraries:           11.9 MB
  Swap:                 0 KB

Page Faults:
  Minor: 75486
  Major: 109
Failed to open maps file: Permission denied
~~~

Команда:
~~~
  firefox &
./memory_profiler $(pgrep firefox)
~~~

Результат:
~~~
  [1] 12637
Process: firefox (PID 12637)
=====================================

Memory Metrics:
  VSZ (Virtual):      221.7 MB
  RSS (Resident):      32.1 MB
  Data/Heap:           11.6 MB
  Stack:              140 KB
  Executable:         596 KB
  Libraries:          134.1 MB
  Swap:                 0 KB

Detailed Memory (from smaps):
  PSS (Proportional):  29.3 MB
  Shared Clean:         5.2 MB
  Shared Dirty:         0 KB
  Private Clean:       18.8 MB
  Private Dirty:        8.5 MB

Page Faults:
  Minor: 11604
  Major: 177

Memory Map Summary (Top 10 segments):
=====================================
      107120 KB r-xp     library /snap/firefox/7177/usr/lib/firefox/libxul.so
       48740 KB r--p     library /snap/firefox/7177/usr/lib/firefox/libxul.so
        8192 KB rw-p     unknown [anonymous]
        6408 KB r-xp     library /snap/firefox/7177/usr/lib/firefox/libgkcodecs.so
        5472 KB r--p     library /snap/firefox/7177/usr/lib/firefox/libxul.so
        4016 KB r-xp     library /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libgtk-3.so.0.2406.32
        3832 KB r--p     library /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libgtk-3.so.0.2406.32
        2048 KB ---p     unknown [anonymous]
        1756 KB r--p     library /snap/firefox/7177/usr/lib/firefox/libgkcodecs.so
        1620 KB r-xp     library /usr/lib/x86_64-linux-gnu/libc.so.6

Shared Libraries Analysis:
==========================
  162208 KB ( 4 seg) /snap/firefox/7177/usr/lib/firefox/libxul.so
    8352 KB ( 4 seg) /snap/firefox/7177/usr/lib/firefox/libgkcodecs.so
    8468 KB ( 5 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libgtk-3.so.0.2406.32
    2160 KB ( 6 seg) /usr/lib/x86_64-linux-gnu/libc.so.6
    2072 KB ( 6 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libgio-2.0.so.0.7800.1
    1484 KB ( 5 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libharfbuzz.so.0.60830.0
    1428 KB ( 4 seg) /snap/firefox/7177/usr/lib/firefox/libmozsqlite3.so
    2212 KB ( 6 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libstdc++.so.6.0.30
    1424 KB ( 6 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libcairo.so.2.11800.0
    1268 KB ( 6 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libgcrypt.so.20.3.4
     980 KB ( 5 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libpixman-1.so.0.42.2
     828 KB ( 5 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libzstd.so.1.4.8
    1388 KB ( 5 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libglib-2.0.so.0.7800.1
    1036 KB ( 5 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libasound.so.2.0.0
    1280 KB ( 6 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libX11.so.6.4.0
    1132 KB ( 6 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libgdk-3.so.0.2406.32
     800 KB ( 6 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libfreetype.so.6.18.1
     792 KB ( 6 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libsystemd.so.0.32.0
     924 KB ( 5 seg) /usr/lib/x86_64-linux-gnu/libm.so.6
     732 KB ( 4 seg) /snap/firefox/7177/usr/lib/firefox/libnss3.so
     604 KB ( 5 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libpcre2-8.so.0.10.4
    1228 KB ( 6 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libepoxy.so.0.0.0
     516 KB ( 5 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libjpeg.so.8.2.2
     412 KB ( 4 seg) /snap/firefox/7177/usr/lib/firefox/libssl3.so
     456 KB ( 5 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libpango-1.0.so.0.5100.0
     420 KB ( 5 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libgobject-2.0.so.0.7800.1
     312 KB ( 6 seg) /snap/firefox/7177/gnome-platform/lib/x86_64-linux-gnu/libdbus-1.so.3.19.13
     272 KB ( 6 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libmount.so.1.1.0
     236 KB ( 5 seg) /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
     236 KB ( 5 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libpng16.so.16.37.0
     296 KB ( 6 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libfontconfig.so.1.12.0
     284 KB ( 5 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libxkbcommon.so.0.0.0
     216 KB ( 4 seg) /snap/firefox/7177/usr/lib/firefox/libnspr4.so
     220 KB ( 6 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libblkid.so.1.1.0
     196 KB ( 6 seg) /snap/firefox/7177/gnome-platform/lib/x86_64-linux-gnu/libexpat.so.1.8.7
     264 KB ( 6 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libatspi.so.0.0.1
     140 KB ( 5 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libbrotlicommon.so.1.0.9
     188 KB ( 4 seg) /snap/firefox/7177/usr/lib/firefox/liblgpllibs.so
     156 KB ( 5 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libgraphite2.so.3.2.1
     196 KB ( 6 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libgdk_pixbuf-2.0.so.0.4200.10
     180 KB ( 4 seg) /snap/firefox/7177/usr/lib/firefox/libmozsandbox.so
     172 KB ( 5 seg) /usr/lib/x86_64-linux-gnu/liblzma.so.5.2.5
     228 KB ( 5 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libatk-bridge-2.0.so.0.0.0
     168 KB ( 6 seg) /snap/firefox/7177/gnome-platform/lib/x86_64-linux-gnu/libselinux.so.1
     128 KB ( 6 seg) /usr/lib/x86_64-linux-gnu/liblz4.so.1.9.3
     176 KB ( 4 seg) /snap/firefox/7177/usr/lib/firefox/libsmime3.so
     140 KB ( 5 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libfribidi.so.0.4.0
     128 KB ( 5 seg) /usr/lib/x86_64-linux-gnu/libgcc_s.so.1
     152 KB ( 5 seg) /snap/firefox/7177/gnome-platform/lib/x86_64-linux-gnu/libgpg-error.so.0
     192 KB ( 4 seg) /snap/firefox/7177/usr/lib/firefox/libnssutil3.so
     168 KB ( 5 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libxcb.so.1.1.0
     112 KB ( 6 seg) /usr/lib/x86_64-linux-gnu/libz.so.1.2.11
     164 KB ( 5 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libatk-1.0.so.0.23809.1
     108 KB ( 6 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libpangoft2-1.0.so.0.5100.0
      92 KB ( 6 seg) /usr/lib/x86_64-linux-gnu/libbsd.so.0.11.5
      80 KB ( 5 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libXi.so.6.1.0
      84 KB ( 5 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libXext.so.6.4.0
      72 KB ( 5 seg) /usr/lib/x86_64-linux-gnu/libresolv.so.2
      72 KB ( 5 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libpangocairo-1.0.so.0.5100.0
      56 KB ( 5 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libbrotlidec.so.1.0.9
      76 KB ( 5 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libwayland-client.so.0.22.0
      52 KB ( 5 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libXrender.so.1.3.0
      52 KB ( 5 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libffi.so.7.1.0
      52 KB ( 5 seg) /usr/lib/x86_64-linux-gnu/libmd.so.0.0.5
      52 KB ( 5 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libXrandr.so.2.2.0
      48 KB ( 5 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libXcursor.so.1.0.2
      60 KB ( 6 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libxcb-render.so.0.0.0
      44 KB ( 5 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libthai.so.0.3.1
      44 KB ( 5 seg) /usr/lib/x86_64-linux-gnu/libcap.so.2.44
      48 KB ( 6 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libcairo-gobject.so.2.11800.0
      40 KB ( 5 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libwayland-cursor.so.0.22.0
      36 KB ( 5 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libdatrie.so.1.4.0
      36 KB ( 5 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libuuid.so.1.3.0
      32 KB ( 5 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libXfixes.so.3.1.0
      28 KB ( 4 seg) /snap/firefox/7177/usr/lib/firefox/libplc4.so
      24 KB ( 4 seg) /snap/firefox/7177/usr/lib/firefox/libmozwayland.so
      28 KB ( 5 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libgmodule-2.0.so.0.7800.1
      24 KB ( 4 seg) /snap/firefox/7177/usr/lib/firefox/libplds4.so
      32 KB ( 5 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libXdmcp.so.6.0.0
      24 KB ( 5 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libXau.so.6.0.0
      20 KB ( 5 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libX11-xcb.so.1.0.0
      20 KB ( 5 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libXinerama.so.1.0.0
      20 KB ( 5 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libXcomposite.so.1.0.0
      20 KB ( 5 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libXdamage.so.1.1.0
      20 KB ( 5 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libwayland-egl.so.1.22.0
      16 KB ( 4 seg) /snap/firefox/7177/usr/lib/firefox/libmozgtk.so
      20 KB ( 5 seg) /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libxcb-shm.so.0.0.0
      20 KB ( 5 seg) /snap/firefox/7177/gnome-platform/lib/x86_64-linux-gnu/bindtextdomain.so
~~~

3. Режим мониторинга (--watch)
Команда:
~~~
  ./memory_profiler $$ --watch
~~~

Результат:
~~~
  onitoring PID 12382 (update every 1 sec, Ctrl+C to stop)



========================================
Time: Sun Nov  2 19:47:27 2025
Iteration: 0
Process: bash (PID 12382)

VSZ:     9.2 MB
RSS:     5.9 MB
PSS:     4.2 MB

Page Faults:
  Minor: 4606
  Major: 0



========================================
Time: Sun Nov  2 19:47:28 2025
Iteration: 1
Process: bash (PID 12382)

VSZ:     9.2 MB
RSS:     5.9 MB
PSS:     4.2 MB

Page Faults:
  Minor: 4606
  Major: 0

Memory Usage Graph (RSS):
┌────────────────────────────────────────┐
│  │    5.9 MB
│  │    5.9 MB
│  │    5.9 MB
│  │    5.9 MB
│  │    5.9 MB
│  │    5.9 MB
│  │    5.9 MB
│  │    5.9 MB
│  │    5.9 MB
│██│    5.9 MB
└────────────────────────────────────────┘
  Time → (last 2 samples)


========================================
Time: Sun Nov  2 19:47:29 2025
Iteration: 2
Process: bash (PID 12382)

VSZ:     9.2 MB
RSS:     5.9 MB
PSS:     4.2 MB

Page Faults:
  Minor: 4606
  Major: 0

Memory Usage Graph (RSS):
┌────────────────────────────────────────┐
│   │    5.9 MB
│   │    5.9 MB
│   │    5.9 MB
│   │    5.9 MB
│   │    5.9 MB
│   │    5.9 MB
│   │    5.9 MB
│   │    5.9 MB
│   │    5.9 MB
│███│    5.9 MB
└────────────────────────────────────────┘
  Time → (last 3 samples)
~~~

4. Сравнение с системными утилитами
Команда:
~~~
  ps -o pid,vsz,rss,comm -p $$
~~~

Результат:
~~~
  PID    VSZ   RSS COMMAND
  12382   9444  6088 bash
~~~

Команда:
~~~
  cat /proc/$$/status | grep ^Vm
~~~

Результат:
~~~
  VmPeak:	    9444 kB
  VmSize:	    9444 kB
  VmLck:	       0 kB
  VmPin:	       0 kB
  VmHWM:	    6088 kB
  VmRSS:	    6088 kB
  VmData:	    2276 kB
  VmStk:	     132 kB
  VmExe:	     956 kB
  VmLib:	    1824 kB
  VmPTE:	      60 kB
  VmSwap:	       0 kB

~~~

Команда:
~~~
   sudo smem -p | grep $(basename $SHELL)
~~~

Результат:
~~~
  13910 chris    grep --color=auto bash         0.00%    0.00%    0.01%    0.03% 
  12382 chris    bash                           0.00%    0.05%    0.05%    0.08% 
~~~

Сравнение с системными утилитами: memory_profiler работает корректно и предоставляет более полную и удобную информацию, чем стандартные утилиты. Все данные соответствуют системным показателям.

5. Режим сравнения (--compare)
Команда:
~~~
  ./memory_profiler $$ --compare $PPID
~~~
Результат:
~~~
omparing Processes:
===================
bash (PID 12382) vs gnome-terminal- (PID 12375)

Metric                  Process 1    Process 2   Difference
------                  ---------    ---------   ----------
VSZ (KB)                     9444       627080      -617636
RSS (KB)                     6088        60116       -54028
PSS (KB)                     4286        23546       -19260
~~~

6. Улучшенный вывод карты памяти
  Продемонстирован в пункте 2 - команда: ./memory_profiler $$

7. ASCII-график
  Продемонстирован в пункте 3 - команда: ./memory_profiler $$ --watch

8. Анализ разделяемых библиотек
 Продемонстирован в пункте 2 - команда: firefox & ./memory_profiler $(pgrep firefox)

9. Поиск процессов по имени
Команда:
~~~
  ./memory_profiler bash --watch 2
~~~
Результат:
~~~
  Monitoring PID 12382 (update every 2 sec, Ctrl+C to stop)



========================================
Time: Sun Nov  2 20:00:35 2025
Iteration: 0
Process: bash (PID 12382)

VSZ:     9.2 MB
RSS:     5.9 MB
PSS:     4.2 MB

Page Faults:
  Minor: 5117
  Major: 0


========================================
Time: Sun Nov  2 20:00:37 2025
Iteration: 1
Process: bash (PID 12382)

VSZ:     9.2 MB
RSS:     5.9 MB
PSS:     4.2 MB

Page Faults:
  Minor: 5117
  Major: 0

Memory Usage Graph (RSS):
┌────────────────────────────────────────┐
│  │    5.9 MB
│  │    5.9 MB
│  │    5.9 MB
│  │    5.9 MB
│  │    5.9 MB
│  │    5.9 MB
│  │    5.9 MB
│  │    5.9 MB
│  │    5.9 MB
│██│    5.9 MB
└────────────────────────────────────────┘
  Time → (last 2 samples)
~~~

10.  Интерактивный режим
Команда:
~~~
  ./memory_profiler --interactive
~~~
Результат:
~~~
nteractive Process Selection
=============================

Available processes:
 1. PID     1: systemd
 2. PID     2: kthreadd
 3. PID     3: pool_workqueue_release
 4. PID     4: kworker/R-rcu_gp
 5. PID     5: kworker/R-sync_wq
 6. PID     6: kworker/R-kvfree_rcu_reclaim
 7. PID     7: kworker/R-slub_flushwq
 8. PID     8: kworker/R-netns
 9. PID    11: kworker/0:0H-events_highpri
10. PID    12: kworker/u32:0-ipv6_addrconf
11. PID    13: kworker/R-mm_percpu_wq
12. PID    14: rcu_tasks_kthread
13. PID    15: rcu_tasks_rude_kthread
14. PID    16: rcu_tasks_trace_kthread
15. PID    17: ksoftirqd/0
16. PID    18: rcu_preempt
17. PID    19: rcu_exp_par_gp_kthread_worker/0
18. PID    20: rcu_exp_gp_kthread_worker
19. PID    21: migration/0
20. PID    22: idle_inject/0

Enter process number (1-20): 13

Process: rcu_tasks_rude_kthread (PID 15)
=====================================

Memory Metrics:
  VSZ (Virtual):        0 KB
  RSS (Resident):       0 KB
  Data/Heap:            0 KB
  Stack:                0 KB
  Executable:           0 KB
  Libraries:            0 KB
  Swap:                 0 KB

Detailed Memory (from smaps):
  PSS (Proportional):   0 KB
  Shared Clean:         0 KB
  Shared Dirty:         0 KB
  Private Clean:        0 KB
  Private Dirty:        0 KB

Page Faults:
  Minor: 0
  Major: 0

Memory Map Summary (Top 10 segments):
=====================================

Shared Libraries Analysis:
==========================

Press Enter to continue...
~~~

Выводы:
memory_profiler успешно реализует все поставленные задачи и превосходит стандартные утилиты по функциональности. Утилита предоставляет профессиональный инструмент для анализа памяти с уникальными возможностями анализа PSS и карты памяти, сохраняя при этом точность и надежность системных метрик.

  Ответы на впоросы:
  1. VSZ включает всю виртуальную память (загруженные библиотеки, отображенные файлы, зарезервированную память), а RSS - только физически загруженные в RAM страницы.
  2. PSS (Proportional Set Size) делит разделяемую память между процессами. Если 4 процесса используют библиотеку 1MB, каждый получит 0.25MB в PSS, тогда как RSS покажет 1MB для каждого.
  3. USS (Unique Set Size) - только приватная, неразделяемая память процесса. Полезен для определения реального потребления памяти конкретным процессом.
  4. Когда процессы модифицируют разделяемые страницы (например, через shared memory), они становятся "dirty" и должны быть записаны на диск при вытеснении.
  5. r-x: read/execute (код программ и библиотек), rw-: read/write (данные, heap, stack), r--: read-only (константы, read-only данные).
  6. Heap растет вверх от данных (обычно в середине адресного пространства), Stack растет вниз от верхней границы (ближе к 0x7fff...).
  7. Minor faults возникают при обращении к страницам, уже загруженным в память но не отображенным в таблице страниц процесса. Это нормально для работающих программ.
  8. Когда требуемая страница отсутствует в RAM и должна быть загружена с диска (swap или файлы).
  9. Одна копия библиотеки в RAM используется множеством процессов, вместо загрузки отдельной копии для каждого.
  10. Используется Copy-on-Write: память разделяется до тех пор, пока один из процессов не попытается изменить страницу, тогда создается приватная копия.

Использование ИИ:
  Реализация утилиты и объяснение ее работы.

## Ответы на вопросы
1. Виртуальная память - абстракция, дающая каждому процессу собственное адресное пространство. Зачем: изоляция процессов, безопасность, возможность использования больше памяти чем физически есть (swap).

2. Метрики памяти:
VSZ - вся виртуальная память (зарезервированная)
RSS - физическая память в RAM
PSS - RSS с учетом разделяемой памяти (лучше для сравнения)
USS - только приватная память (лучше для реального потребления)
Наиболее точная: PSS для сравнения, USS для реального потребления.

3. Страница - блок виртуальной памяти (обычно 4KB). Кадр - блок физической памяти. Таблица страниц - структура, отображающая виртуальные страницы на физические кадры.

4. MMU (Memory Management Unit) - аппаратный модуль, переводящий виртуальные адреса в физические. TLB - кэш трансляций. При промахе TLB - обращение к таблице страниц в памяти (медленно).

5. Copy-on-Write - техника, когда при fork() память разделяется до записи, затем создается копия. Применение: fork(), mmap(MAP_PRIVATE).

6. Minor page fault - страница в памяти, но не в таблице страниц. Major page fault - страницы нет в памяти, нужно загружать с диска.

7. malloc() резервирует виртуальную память, но физическая выделяется при первом обращении (lazy allocation) - поэтому page fault.

8. Уменьшить page faults: предварительное выделение (mlock()), использование больших страниц, оптимизация доступа к памяти.

9. Demand paging - загрузка страниц по требованию. Page replacement - алгоритмы вытеснения страниц (LRU, Clock).

10. mmap() отображает файл в память, read()/write() - системные вызовы. mmap эффективнее для случайного доступа к большим файлам.

11. MAP_PRIVATE - изменения не видны другим процессам. MAP_SHARED - изменения синхронизируются с файлом и видны другим процессам.

12. Выход за границы mmap → SIGSEGV (segmentation fault).

13. Page cache - кэш страниц файлов в памяти. Зачем: ускорение доступа к часто используемым файлам.

14. Буферизация уменьшает количество системных вызовов. Уровни: пользовательский буфер (stdio), кэш ядра (page cache), буфер диска.

15. Маленький буфер → много системных вызовов → накладные расходы.

16. O_DIRECT - минует кэш ядра. O_SYNC - синхронная запись. Использовать: базы данных (O_DIRECT), критичные данные (O_SYNC).

17. fwrite() - буферизованный ввод-вывод (пользовательский буфер). write() - небуферизованный системный вызов.

18. inode - структура метаданных файла (права, размер, timestamps, указатели на данные). Не хранит имя файла.

19. Имя файла хранится в директории для поддержки жестких ссылок (один inode - много имен).

20. Жесткая ссылка - другое имя того же inode. Символьная ссылка - файл с путем к целевому файлу.

21. ext4 для больших файлов: прямые указатели → косвенные → двойные косвенные → тройные косвенные.

22. I/O schedulers упорядочивают запросы для минимизации перемещений головки диска.

23. FCFS - в порядке поступления. SSTF - ближайший запрос. SCAN - "лифт" (вперед-назад).

24. Современные Linux schedulers: mq-deadline, bfq, kyber (для SSD и HDD соответственно).

25. Для SSD нет механических частей → нет задержек на позиционирование → планирование менее критично.

26. Фрагментация:

Внутренняя - неиспользуемая память внутри выделенного блока

Внешняя - свободная память разбита на маленькие фрагменты

27. Swap использует диск как расширение RAM → медленно при активном использовании.

28. Thrashing - система тратит больше времени на своппинг чем на работу. Избегать: больше RAM, ограничение процессов, оптимизация памяти.

29. Последовательный доступ использует prefetcher и кэш. Случайный доступ - промахи кэша, непредсказуемость.

30. Cache-friendly код - последовательный доступ, компактные структуры данных, локальность ссылок.

## Итоги
  Лабораторная 4 успешно выполнена: успешно выполнен анализ вирутальной памяти процесса и анализ измеения итогов работы при разных размерах буфера, проанализирована активность на устройстве и сделаны выводы, также сделан анализ виртуальной памяти процесса.

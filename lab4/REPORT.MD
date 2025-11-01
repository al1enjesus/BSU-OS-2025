# Лабораторная работа по анализу виртуальной памяти, I/O и профилированию памяти
##Вариант 2

## Введение

Данная работа посвящена глубокому изучению механизмов управления памятью и ввода-вывода в операционной системе **Linux**. Были реализованы и протестированы инструменты для анализа виртуальной памяти процесса, сравнения производительности буферизованного и небуферизованного ввода-вывода, мониторинга дисковой активности, а также создания полноценного **профилировщика памяти** с поддержкой динамического мониторинга, сравнения процессов и визуализации.

**Цели работы**:

- Понять, как ОС управляет виртуальной памятью
- Изучить механизмы page fault, demand paging, CoW
- Сравнить производительность `stdio` и системных вызовов
- Освоить мониторинг I/O через `/proc`, `iostat`, `iotop`
- Реализовать утилиту профилирования памяти

**Окружение**:

- ОС: Ubuntu 22.04
- Компилятор: `g++ -std=c++17`

**Исходный код**:

- `A_mem.cpp` — анализ виртуальной памяти
- `B_io_bench.cpp` — бенчмарк I/O
- `C_io_monitor.cpp` — мониторинг диска
- `memory_profiler.cpp` — профилировщик памяти

**Результаты**: `out_log.txt`, `io_results.csv`

---

## Задание A: Анализ виртуальной памяти процесса

### Цель

Изучить, как процесс использует память: стек, куча, `mmap`, и как это отражается в `/proc`.

### Реализация (`A_mem.cpp`)

Программа выполняет следующие шаги:

1. **Чтение метрик до выделения памяти**:
   - `/proc/self/status` → `VmSize`, `VmRSS`
   - `/proc/self/smaps_rollup` → `Pss`, `Private_Clean`, `Private_Dirty`
   - `USS = Private_Clean + Private_Dirty`

2. **Выделение памяти**:
   - **Стек**: `volatile char stack_var[1024]` с касанием
   - **Heap**: `new char[sz]` (sz = N × 1 МБ)
   - **mmap**: `mmap(NULL, sz, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0)`
   - **Касание страниц**:
     ```cpp
     const size_t page = sysconf(_SC_PAGESIZE);
     for (size_t i = 0; i < bytes; i += page)
         ptr[i] = static_cast<char>(i);
     ```

3. **Чтение метрик после → вывод дельты**

4. **Опция `--show-maps` → полный дамп `/proc/self/maps`**

**Аргументы**:
```
--size-mb=N (по умолчанию 1)
--show-maps
```

### Запуск:

```bash
./A_mem --size-mb=1 --show-maps
```

### Вывод:

```
discessit@discessit-MCLF-XX:~/EDU/ospractise/lab1/lab4/src$ ./A_mem --size-mb=1 --show-maps
--- Before allocations ---
VmSize: 6364 kB
VmRSS : 3244 kB
PSS   : 307 kB
USS   : 236 kB (Private_Clean 4, Private_Dirty 232)
--- After allocations (touched) ---
VmSize: 8416 kB
VmRSS : 5420 kB
PSS   : 2372 kB
USS   : 2292 kB (Private_Clean 4, Private_Dirty 2288)

ΔVmRSS: 2176 kB for 1 MB heap+mmap touched
ΔPSS  : 2065 kB
ΔUSS  : 2056 kB
===== /proc/self/maps =====
5844a2e99000-5844a2e9b000 r--p 00000000 103:05 1062512                   /home/discessit/EDU/ospractise/lab1/lab4/src/A_mem
5844a2e9b000-5844a2e9d000 r-xp 00002000 103:05 1062512                   /home/discessit/EDU/ospractise/lab1/lab4/src/A_mem
5844a2e9d000-5844a2e9e000 r--p 00004000 103:05 1062512                   /home/discessit/EDU/ospractise/lab1/lab4/src/A_mem
5844a2e9e000-5844a2e9f000 r--p 00004000 103:05 1062512                   /home/discessit/EDU/ospractise/lab1/lab4/src/A_mem
5844a2e9f000-5844a2ea0000 rw-p 00005000 103:05 1062512                   /home/discessit/EDU/ospractise/lab1/lab4/src/A_mem
5844cbdb2000-5844cbdd3000 rw-p 00000000 00:00 0                          [heap]
7a75ea700000-7a75ea800000 rw-p 00000000 00:00 0 
7a75ea800000-7a75ea828000 r--p 00000000 103:05 4365034                   /usr/lib/x86_64-linux-gnu/libc.so.6
7a75ea828000-7a75ea9b0000 r-xp 00028000 103:05 4365034                   /usr/lib/x86_64-linux-gnu/libc.so.6
7a75ea9b0000-7a75ea9ff000 r--p 001b0000 103:05 4365034                   /usr/lib/x86_64-linux-gnu/libc.so.6
7a75ea9ff000-7a75eaa03000 r--p 001fe000 103:05 4365034                   /usr/lib/x86_64-linux-gnu/libc.so.6
7a75eaa03000-7a75eaa05000 rw-p 00202000 103:05 4365034                   /usr/lib/x86_64-linux-gnu/libc.so.6
7a75eaa05000-7a75eaa12000 rw-p 00000000 00:00 0 
7a75eaaff000-7a75eac00000 rw-p 00000000 00:00 0 
7a75eac00000-7a75eac9d000 r--p 00000000 103:05 4337397                   /usr/lib/x86_64-linux-gnu/libstdc++.so.6.0.33
7a75eac9d000-7a75eade5000 r-xp 0009d000 103:05 4337397                   /usr/lib/x86_64-linux-gnu/libstdc++.so.6.0.33
7a75eade5000-7a75eae6c000 r--p 001e5000 103:05 4337397                   /usr/lib/x86_64-linux-gnu/libstdc++.so.6.0.33
7a75eae6c000-7a75eae77000 r--p 0026b000 103:05 4337397                   /usr/lib/x86_64-linux-gnu/libstdc++.so.6.0.33
7a75eae77000-7a75eae7a000 rw-p 00276000 103:05 4337397                   /usr/lib/x86_64-linux-gnu/libstdc++.so.6.0.33
7a75eae7a000-7a75eae7e000 rw-p 00000000 00:00 0 
7a75eaf21000-7a75eaf26000 rw-p 00000000 00:00 0 
7a75eaf26000-7a75eaf36000 r--p 00000000 103:05 4365037                   /usr/lib/x86_64-linux-gnu/libm.so.6
7a75eaf36000-7a75eafb5000 r-xp 00010000 103:05 4365037                   /usr/lib/x86_64-linux-gnu/libm.so.6
7a75eafb5000-7a75eb00d000 r--p 0008f000 103:05 4365037                   /usr/lib/x86_64-linux-gnu/libm.so.6
7a75eb00d000-7a75eb00e000 r--p 000e7000 103:05 4365037                   /usr/lib/x86_64-linux-gnu/libm.so.6
7a75eb00e000-7a75eb00f000 rw-p 000e8000 103:05 4365037                   /usr/lib/x86_64-linux-gnu/libm.so.6
7a75eb00f000-7a75eb013000 r--p 00000000 103:05 4337004                   /usr/lib/x86_64-linux-gnu/libgcc_s.so.1
7a75eb013000-7a75eb037000 r-xp 00004000 103:05 4337004                   /usr/lib/x86_64-linux-gnu/libgcc_s.so.1
7a75eb037000-7a75eb03b000 r--p 00028000 103:05 4337004                   /usr/lib/x86_64-linux-gnu/libgcc_s.so.1
7a75eb03b000-7a75eb03c000 r--p 0002b000 103:05 4337004                   /usr/lib/x86_64-linux-gnu/libgcc_s.so.1
7a75eb03c000-7a75eb03d000 rw-p 0002c000 103:05 4337004                   /usr/lib/x86_64-linux-gnu/libgcc_s.so.1
7a75eb04f000-7a75eb051000 rw-p 00000000 00:00 0 
7a75eb051000-7a75eb053000 r--p 00000000 00:00 0                          [vvar]
7a75eb053000-7a75eb055000 r--p 00000000 00:00 0                          [vvar_vclock]
7a75eb055000-7a75eb057000 r-xp 00000000 00:00 0                          [vdso]
7a75eb057000-7a75eb058000 r--p 00000000 103:05 4365031                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
7a75eb058000-7a75eb083000 r-xp 00001000 103:05 4365031                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
7a75eb083000-7a75eb08d000 r--p 0002c000 103:05 4365031                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
7a75eb08d000-7a75eb08f000 r--p 00036000 103:05 4365031                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
7a75eb08f000-7a75eb091000 rw-p 00038000 103:05 4365031                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
7fff7430d000-7fff7432e000 rw-p 00000000 00:00 0                          [stack]
ffffffffff600000-ffffffffff601000 --xp 00000000 00:00 0                  [vsyscall]
===== end maps =====
PID: 73453. Press ENTER to exit...
```
##Проверка через ps и cat /proc/[PID]/smaps_rollup:
```
$ ps -p 73453 -o pid,comm,vsz,rss
  PID COMM           VSZ   RSS
73453 A_mem         8416  5420

$ cat /proc/73453/smaps_rollup
Pss:             2372 kB
Private_Clean:      4 kB
Private_Dirty:   2288 kB
```

### Проверка через `ps` и `cat /proc/[PID]/smaps_rollup`:

```bash
$ ps -p 73453 -o pid,comm,vsz,rss
  PID COMM           VSZ   RSS
73453 A_mem         8416  5420

$ cat /proc/73453/smaps_rollup
Pss:             2372 kB
Private_Clean:      4 kB
Private_Dirty:   2288 kB
```

### Анализ

- VSZ вырос на ~2052 KB → 2 МБ (1 МБ heap + 1 МБ mmap)  
- RSS вырос на 2176 KB → чуть больше из-за overhead (page tables, alignment)  
- USS ≈ 2056 KB → почти точно 2 МБ (private dirty)  
- PSS < RSS → часть памяти shared (libc, но не в heap/mmap)

### Ответы на вопросы задания A

**1) Почему VSZ намного больше RSS?**

- **VSZ** (Virtual Size) — сумма *всех* виртуальных регионов процесса: код, данные, стек (часто 8–16 МБ с guard page), анонимные `mmap`, отображенные файлы, разделяемые библиотеки, незакоммиченные зоны (lazy allocation), `vdso/vvar`.  
- **RSS** — только страницы, фактически находящиеся в RAM *прямо сейчас*.  
- Разрыв VSZ−RSS возникает из‑за: (а) неактивных/неиспользованных страниц; (б) ленивого выделения (demand paging); (в) разделяемых библиотек (их *полный размер* входит в VSZ, но RSS у процесса учитывает лишь реально загруженные страницы).  
- **Дополнительно:** таблицы страниц и метаданные аллокатора добавляют небольшой RSS‑overhead; guard‑страницы увеличивают VSZ, но почти не влияют на RSS.

**2) Где находятся разные типы памяти в адресном пространстве?**

| Тип памяти | Адрес (пример) | Описание | Примечание |
|---|---|---|---|
| Текст (code) | `0x5555...` | Исполняемый код (`r-xp`) | Отображение бинарника |
| Данные/BSS | `0x5555...` | Иниц./неиниц. данные | `rw-p` |
| Heap | `[heap]` | `malloc/new` | Обычно растёт вверх через `brk/sbrk` |
| `mmap` (anon) | `0x7f..` | Выделения страницами | Гибкая гранулярность, CoW |
| Shared libs | `0x7f..` | `libc`, `libstdc++` и т. д. | Разделяются между процессами |
| Stack | `[stack]` | Локальные переменные | Часто 8–16 МБ с guard |
| `vdso/vvar` | `0x7fff..` | Помощники ядра | Доступ к таймерам/вызовам |

**3) Как изменился RSS при 1 MB через `malloc` vs `mmap`?**

| Способ | ΔRSS (пример) | Как это устроено |
|---|---:|---|
| `malloc` | +1024–1030 КБ | Часто расширяет heap через `brk`; добавляется метаданные аллокатора и возможная фрагментация |
| `mmap` | +1024–1028 КБ | Анонимное отображение, страницы физически появляются по касанию (minor faults) |

**Практические нюансы:**  
- На **малых** блоках `malloc` быстрее и экономичнее по системным вызовам; на **крупных** (`≥128–256 КБ`) многие аллокаторы *всё равно* идут через `mmap`.  
- **THP/Transparent Huge Pages** могут менять картину: рост RSS будет идти 2 МБ‑страницами (если включены и есть доступные паттерны доступа).  
- **CoW** при форке: RSS у потомка низкий до первой записи; PSS отражает «долю» общих страниц.

### Вывод по заданию

- Метрики **USS/PSS/RSS** дают разные грани использования памяти: RSS — «сейчас в RAM», PSS — «справедливое разделение shared», USS — «строго приватная нагрузка».  
- **mmap** и **malloc** при «касании» страниц приводят к сходному росту RSS, но `mmap` даёт лучшую сегментацию больших блоков и экономит `brk`‑область.  
- Разница VSZ↔RSS — норма: большие VSZ — это *не* проблема сами по себе, если RSS/USS остаются под контролем.

---

## Задание B: Буферизованный vs Небуферизованный I/O

### Цель

Сравнить производительность `stdio` и системных вызовов при записи/чтении 100 МБ.

### Реализация (`B_io_bench.cpp`)

| Режим | Метод | Буфер |
|---|---|---|
| `stdio_char` | `fputc('A', f)` | 1 байт |
| `stdio_buf` | `fwrite(buf, 1, n, f) + setvbuf(_IOFBF)` | 512, 4K, 64K |
| `sys` | `write(fd, buf, n)` | 512, 4K, 64K |

Замер времени: `clock_gettime(CLOCK_MONOTONIC)`  
Подсчёт вызовов `write/fwrite`  
`fsync` для `sys`  
`fflush` для `stdio`

### Запуск и вывод

```
discessit@discessit-MCLF-XX:~/EDU/ospractise/lab1/lab4/src$ ./B_io_bench --size-mb=100 --buffers=512,4096,65536 | column -s, -t
mode        buf    size_mb  write_sec  read_sec   write_calls  read_calls
stdio_char  1      100      0.688811   0.0410098  104857600    25600
stdio_buf   512    100      1.02745    0.0433162  204800       204800
stdio_buf   4096   100      0.196086   0.036875   25600        25600
stdio_buf   65536  100      0.0843926  0.0291157  1600         1600
sys         512    100      1.0722     0.17577    204800       204800
sys         4096   100      0.249285   0.0314038  25600        25600
sys         65536  100      0.188179   0.0236734  1600         1600
discessit@discessit-MCLF-XX:~/EDU/ospractise/lab1/lab4/src$ strace -c ./B_io_bench --size-mb=100 --buffers=65536 --no-stdio-char
mode,buf,size_mb,write_sec,read_sec,write_calls,read_calls
stdio_buf,65536,100,0.134294,0.0629481,1600,1600
sys,65536,100,0.204931,0.0699485,1600,1600
% time     seconds  usecs/call     calls    errors syscall
------ ----------- ----------- --------- --------- ----------------
 57.39    0.182123          56      3203           write
 19.08    0.060558          18      3206           read
 15.21    0.048279       24139         2           unlink
  7.73    0.024541       24541         1           fsync
  0.25    0.000796         796         1           execve
  0.13    0.000413          45         9           openat
  0.11    0.000340          15        22           mmap
  0.04    0.000125          13         9           close
  0.02    0.000053          10         5           brk
  0.01    0.000046           6         7           fstat
  0.01    0.000041           6         6           mprotect
  0.00    0.000013           6         2           pread64
  0.00    0.000013          13         1         1 access
  0.00    0.000007           7         1           arch_prctl
  0.00    0.000007           7         1           set_tid_address
  0.00    0.000007           7         1           rseq
  0.00    0.000006           6         1           set_robust_list
  0.00    0.000000           0         1           munmap
  0.00    0.000000           0         1           futex
  0.00    0.000000           0         1           prlimit64
  0.00    0.000000           0         1           getrandom
------ ----------- ----------- --------- --------- ----------------
100.00    0.317368          48      6482         1 total
```

### Анализ

- Оптимальный буфер: **4 КБ** — совпадает с размером страницы и шагом page cache.  
- Малый буфер (1 B/512 B) приводит к взрывному росту числа syscalls → переключения контекста и системные накладные расходы.  
- `stdio` выигрывает у «голого» `write()` за счёт пользовательского буфера и агрегации.  
- Для 64 КБ разница с 4 КБ мала: выигрывает тот, где выше эффективность копирования/слияния запросов на блоковом слое.

### Ответы на вопросы

**1) Почему оптимальный размер — около 4 КБ? Можно ли больше?**  
- 4 КБ соответствует странице памяти и базовой единице работы page cache, что минимизирует частичные попадания и переработку.  
- Большие блоки (64 КБ–1 МБ) часто **тоже** эффективны, особенно при последовательном I/O: блоковый слой и устройство смогут сливать запросы; выигрывает уменьшение количества системных вызовов.  
- При очень больших буферах упираемся в **копирование** между user↔kernel и в политику readahead/writeback. Баланс: 4–64 КБ — практически «золотая середина» для POSIX‑I/O на современных системах.

**2) Как соотносятся количество `write()` и время?**  
- Время растёт примерно линейно с числом системных вызовов до насыщения CPU/IOPS.  
- `stdio` снижает число `write()` благодаря буферизации в userspace, особенно критично для мелких записей.  
- При последовательной записи *без* `O_DIRECT` вы выигрываете от page cache и write‑combining ядра.

**Дополнительно — нюансы синхронизации:**  
- `fsync`/`fdatasync` гарантируют сброс на носитель и *существенно* увеличивают время. Для честных бенчмарков нужно явно указывать политику сброса.  
- `O_DIRECT` (как в примере с `dd oflag=direct`) обходит page cache и показывает «сырые» возможности диска — полезно для проверки устройства, но не характерно для типичных приложений.

### Вывод по заданию

- **Буферизация критична**: уменьшение числа системных вызовов — прямая экономия времени.  
- Оптимальный размер зависит от паттерна: 4–64 КБ устойчиво хороши; единичные байты — всегда плохо.  
- Обязательно фиксируйте политику синхронизации (fflush/fsync) и режимы (`O_DIRECT`), иначе сравнение будет некорректным.

---

## Задание C: Дисковый I/O и планирование

### Цель

Мониторить дисковую активность во время I/O‑нагрузки.

### Реализация (`C_io_monitor.cpp`)

- `/proc/diskstats` с периодической выборкой  
- Метрики:
  - `read_MBps = (rsectors * 512) / 1e6 / sec`
  - `write_MBps = (wsectors * 512) / 1e6 / sec`
  - `util% = ioms / (10 * sec)` (1 tick = 1 ms)
  - `await_ms = (rms + wms) / (reads + writes)`
- Фильтр: `--dev=nvme0n1`
- `--pid=PID` → `/proc/PID/io`
- Вывод планировщиков (schedulers)

### Запуск и вывод

```
discessit@discessit-MCLF-XX:~/EDU/ospractise/lab1/lab4/src$ ./C_io_monitor --dev=nvme0n1 --iters=5 --interval=0.5 --pid="$(pidof -s firefox)" | column -s, -t
time                          dev      read_MBps  write_MBps  util_%  await_ms
0.5                           nvme0n1  0.0        0.0         0.0     0.0
1.0                           nvme0n1  0.0        0.0         0.0     0.0
1.5                           nvme0n1  0.0        0.0         0.0     0.0
2.0                           nvme0n1  0.0        0.0         0.0     0.0
2.5                           nvme0n1  0.0        0.0         0.0     0.0
Schedulers:                                                           
loop1: [none] mq-deadline                                             
loop29: [none] mq-deadline                                            
loop19: [none] mq-deadline                                            
nvme0n1: [none] mq-deadline                                           
loop27: [none] mq-deadline                                            
loop17: [none] mq-deadline                                            
loop8: [none] mq-deadline                                             
loop25: [none] mq-deadline                                            
loop15: [none] mq-deadline                                            
loop6: [none] mq-deadline                                             
loop23: [none] mq-deadline                                            
loop13: [none] mq-deadline                                            
loop4: [none] mq-deadline                                             
loop21: [none] mq-deadline                                            
loop11: [none] mq-deadline                                            
loop2: [none] mq-deadline                                             
loop0: [none] mq-deadline                                             
loop28: [none] mq-deadline                                            
loop18: [none] mq-deadline                                            
loop9: [none] mq-deadline                                             
loop26: [none] mq-deadline                                            
loop16: [none] mq-deadline                                            
loop7: [none] mq-deadline                                             
loop24: [none] mq-deadline                                            
loop14: [none] mq-deadline                                            
loop5: [none] mq-deadline                                             
loop22: [none] mq-deadline                                            
loop12: [none] mq-deadline                                            
loop3: [none] mq-deadline                                             
loop30: [none] mq-deadline                                            
loop20: [none] mq-deadline                                            
loop10: [none] mq-deadline                                            
--- /proc/21498/io ---                                                
rchar: 128027065                                                      
wchar: 1773057                                                        
syscr: 671555                                                         
syscw: 777861                                                         
read_bytes: 109209600                                                 
write_bytes: 0                                                        
cancelled_write_bytes: 0  
```
Диск мог не использоваться, поэтому для убеждения в роботоспособности программы была создана искусственная нагрузка:
```
discessit@discessit-MCLF-XX:~/EDU/ospractise/lab1/lab4/src$ DIR=.; BLK=$(df -P "$DIR" | awk 'NR==2{print $1}'); DEV=$(lsblk -no PKNAME "$BLK" 2>/dev/null | head -1); [ -z "$DEV" ] && DEV=$(basename "$BLK"); \
dd if=/dev/zero of="$DIR/io_test.bin" bs=1M count=10000 oflag=direct status=progress & LOAD_PID=$!; \
./C_io_monitor --dev="$DEV" --iters=6 --interval=0.5 --pid="$LOAD_PID" | column -s, -t; \
wait "$LOAD_PID" || true; sync; rm -f "$DIR/io_test.bin"
[1] 107660
3722444800 bytes (3.7 GB, 3.5 GiB) copied, 3 s, 1.2 GB/stime                          dev      read_MBps  write_MBps  util_%  await_ms
0.5                           nvme0n1  0.0        1132.0      44.0    0.3
1.0                           nvme0n1  0.1        1196.0      44.0    0.3
1.5                           nvme0n1  0.0        1170.0      37.6    0.3
2.0                           nvme0n1  0.0        1208.0      38.2    0.3
2.5                           nvme0n1  0.0        1191.8      47.4    0.3
3.0                           nvme0n1  0.0        1216.2      42.2    0.3
Schedulers:                                                           
loop1: [none] mq-deadline                                             
loop29: [none] mq-deadline                                            
loop19: [none] mq-deadline                                            
nvme0n1: [none] mq-deadline                                           
loop27: [none] mq-deadline                                            
loop17: [none] mq-deadline                                            
loop8: [none] mq-deadline                                             
loop25: [none] mq-deadline                                            
loop15: [none] mq-deadline                                            
loop6: [none] mq-deadline                                             
loop23: [none] mq-deadline                                            
loop13: [none] mq-deadline                                            
loop4: [none] mq-deadline                                             
loop21: [none] mq-deadline                                            
loop11: [none] mq-deadline                                            
loop2: [none] mq-deadline                                             
loop0: [none] mq-deadline                                             
loop28: [none] mq-deadline                                            
loop18: [none] mq-deadline                                            
loop9: [none] mq-deadline                                             
loop26: [none] mq-deadline                                            
loop16: [none] mq-deadline                                            
loop7: [none] mq-deadline                                             
loop24: [none] mq-deadline                                            
loop14: [none] mq-deadline                                            
loop5: [none] mq-deadline                                             
loop22: [none] mq-deadline                                            
loop12: [none] mq-deadline                                            
loop3: [none] mq-deadline                                             
loop30: [none] mq-deadline                                            
loop20: [none] mq-deadline                                            
loop10: [none] mq-deadline                                            
--- /proc/107660/io ---                                               
rchar: 3731889072                                                     
wchar: 3730833579                                                     
syscr: 3570                                                           
syscw: 3564                                                           
read_bytes: 4096                                                      
write_bytes: 3731881984                                               
cancelled_write_bytes: 0                                              
9687793664 bytes (9.7 GB, 9.0 GiB) copied, 12 s, 807 MB/s
10000+0 records in
10000+0 records out
10485760000 bytes (10 GB, 9.8 GiB) copied, 12.611 s, 831 MB/s
[1]+  Done                    dd if=/dev/zero of="$DIR/io_test.bin" bs=1M count=10000 oflag=direct status=progress
```

### Анализ

- При `oflag=direct` запись идёт в обход page cache → хорошо видно «сырые» возможности NVMe (сотни–тысячи MB/s), а также влияние размеров блоков.  
- **`util%`** отражает долю времени, когда устройство занято обработкой запросов. Для NVMe 100% часто достижимо на последовательной записи.  
- **`await_ms`** — среднее ожидание завершения I/O (очередь + сервис). Для SSD типично <1–2 мс на мелких запросах; на больших последовательных запросах могут быть десятые доли мс.  
- Планировщик `mq-deadline` — дефолт для блочного слоя с многоочередным NVMe; разница между `none/mq-deadline/bfq/kyber` на NVMe чаще заметна при смешанных паттернах (random RW) и многопоточности.

**Замечание по интерпретации:**  
- `await` ≠ «чистый сервис‑тайм»; это метрика «всё включено» (учитывает очередь). При высоких нагрузках рост `await` сигнализирует о переполнении очередей.  
- При оценке «узких мест» полезно смотреть ещё на **IOPS**, **средний размер запроса**, **queue depth** 

### Вывод по заданию

- Инструмент подтверждает корректную диагностику реальных/искусственных нагрузок и позволяет **быстро понять**, кто генерирует I/O и насколько устройство загружено.  
- Для NVMe на современных ядрах планировщик имеет **умеренное значение** при последовательных потоках, но важен при смешанных RW‑нагрузках и множестве конкурирующих потоков.  
- Различайте «пропускную способность носителя» и «эффективность стека ввода‑вывода ОС» — режимы `O_DIRECT`, размеры блоков, синхронизация, page cache меняют картину.

---

## Практика: Memory Profiler

### Реализация (`memory_profiler.cpp`)

Функции программы:

- Чтение VSZ, RSS, PSS, USS, page faults
- Парсинг `/proc/PID/maps` и `/proc/PID/smaps`
- Группировка: heap, stack, anon, libs
- `--watch`: мониторинг с delta
- `--graph`: ASCII‑график RSS
- `--csv=file.csv`
- `--compare PID1 PID2`
- `--share-scan`: сколько процессов используют `.so`

### Запуски и выводы

— **Первый запуск**: статический снимок метрик и топ разделяемых библиотек.  
— **Второй запуск**: `--watch` + ASCII‑график стабильного процесса (нулевые дельты).  
— **Третий запуск**: `--share-scan` — оценка того, сколько процессов разделяют `.so`.  
— **Четвёртый запуск**: `--compare PID1 PID2` — сравнение Firefox и VS Code.  
— **Динамический паттерн**: синтетическая нагрузка с ростом и спадом RSS по 20 МБ «ступеньками».

```
Первый:
discessit@discessit-MCLF-XX:~/EDU/ospractise/lab1/lab4/src$ ./memory_profiler "$PID"
Process: forkserver (PID 21498)
VSZ:  292.8 MB
RSS:  22.6 MB
PSS:  2.7 MB
USS:  0.2 MB
Shared:  22.2 MB    Private: 0.2 MB

Page Faults:
  Minor: 3761
  Major: 9

Memory Map (categories):
  anonymous   size 6.2 MB | RSS 0.5 MB | PSS 0.2 MB | Private 0.1 MB | Shared 0.4 MB
  heap        size 0.0 MB | RSS 0.0 MB | PSS 0.0 MB | Private 0.0 MB | Shared 0.0 MB
  libs        size 286.4 MB | RSS 21.9 MB | PSS 2.4 MB | Private 0.1 MB | Shared 21.8 MB
  other       size 0.0 MB | RSS 0.0 MB | PSS 0.0 MB | Private 0.0 MB | Shared 0.0 MB
  stack       size 0.1 MB | RSS 0.0 MB | PSS 0.0 MB | Private 0.0 MB | Shared 0.0 MB

Top libraries (by shared KB):
  /snap/firefox/7177/usr/lib/firefox/libxul.so
     size 158.4 MB | PSS 0.9 MB | Private 0.0 MB | Shared 11.7 MB
  /usr/lib/x86_64-linux-gnu/libc.so.6
     size 2.1 MB | PSS 0.1 MB | Private 0.0 MB | Shared 1.4 MB
  /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libglib-2.0.so.0.7800.1
     size 1.4 MB | PSS 0.0 MB | Private 0.0 MB | Shared 0.7 MB
  /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libpixman-1.so.0.42.2
     size 1.0 MB | PSS 0.2 MB | Private 0.0 MB | Shared 0.5 MB
  /snap/firefox/7177/usr/lib/firefox/libmozsqlite3.so
     size 1.4 MB | PSS 0.2 MB | Private 0.0 MB | Shared 0.4 MB
  /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libepoxy.so.0.0.0
     size 1.2 MB | PSS 0.1 MB | Private 0.0 MB | Shared 0.4 MB
  /snap/firefox/7177/usr/lib/firefox/firefox
     size 0.8 MB | PSS 0.0 MB | Private 0.0 MB | Shared 0.4 MB
  /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libgobject-2.0.so.0.7800.1
     size 0.4 MB | PSS 0.0 MB | Private 0.0 MB | Shared 0.4 MB
  /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libgio-2.0.so.0.7800.1
     size 2.0 MB | PSS 0.0 MB | Private 0.0 MB | Shared 0.3 MB
  /usr/lib/x86_64-linux-gnu/libm.so.6
     size 0.9 MB | PSS 0.0 MB | Private 0.0 MB | Shared 0.3 MB
  /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libstdc++.so.6.0.30
     size 2.2 MB | PSS 0.0 MB | Private 0.0 MB | Shared 0.2 MB
  /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
     size 0.2 MB | PSS 0.0 MB | Private 0.0 MB | Shared 0.2 MB
  /snap/firefox/7177/usr/lib/firefox/libnss3.so
     size 0.7 MB | PSS 0.0 MB | Private 0.0 MB | Shared 0.2 MB
  /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libX11.so.6.4.0
     size 1.2 MB | PSS 0.0 MB | Private 0.0 MB | Shared 0.2 MB
  /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libgdk-3.so.0.2406.32
     size 1.1 MB | PSS 0.0 MB | Private 0.0 MB | Shared 0.2 MB
```
```Второй:
discessit@discessit-MCLF-XX:~/EDU/ospractise/lab1/lab4/src$ ./memory_profiler --watch --interval=1 --samples=10 --graph "$PID"
Watching: forkserver (PID 21498), interval 1s, samples 10

t(s)    RSS         PSS         USS         ΔRSS         ΔPSS         ΔUSS         minfltΔ        majfltΔ        
0.0     22.6 MB     2.7 MB      0.2 MB      +             0+             0+             0+               0+               0
1.0     22.6 MB     2.7 MB      0.2 MB      +             0kB+             0kB+             0kB+               0+               0
2.0     22.6 MB     2.7 MB      0.2 MB      +             0kB+             0kB+             0kB+               0+               0
3.0     22.6 MB     2.7 MB      0.2 MB      +             0kB+             0kB+             0kB+               0+               0
4.0     22.6 MB     2.7 MB      0.2 MB      +             0kB+             0kB+             0kB+               0+               0
5.0     22.6 MB     2.7 MB      0.2 MB      +             0kB+             0kB+             0kB+               0+               0
6.0     22.6 MB     2.7 MB      0.2 MB      +             0kB+             0kB+             0kB+               0+               0
7.0     22.6 MB     2.7 MB      0.2 MB      +             0kB+             0kB+             0kB+               0+               0
8.0     22.6 MB     2.7 MB      0.2 MB      +             0kB+             0kB+             0kB+               0+               0
9.0     22.6 MB     2.7 MB      0.2 MB      +             0kB+             0kB+             0kB+               0+               0

RSS ASCII graph (10 samples, min=22.6 MB, max=22.6 MB):
0    | ##### 22.6 MB
1    | ##### 22.6 MB
2    | ##### 22.6 MB
3    | ##### 22.6 MB
4    | ##### 22.6 MB
5    | ##### 22.6 MB
6    | ##### 22.6 MB
7    | ##### 22.6 MB
8    | ##### 22.6 MB
9    | ##### 22.6 MB
```
```Третий:
discessit@discessit-MCLF-XX:~/EDU/ospractise/lab1/lab4/src$ PID=$(pidof -s firefox)
discessit@discessit-MCLF-XX:~/EDU/ospractise/lab1/lab4/src$ ./memory_profiler --share-scan "$PID"
Process: forkserver (PID 21498)
VSZ:  292.8 MB
RSS:  22.6 MB
PSS:  2.7 MB
USS:  0.2 MB
Shared:  22.2 MB    Private: 0.2 MB

Page Faults:
  Minor: 3761
  Major: 9

Memory Map (categories):
  anonymous   size 6.2 MB | RSS 0.5 MB | PSS 0.2 MB | Private 0.1 MB | Shared 0.4 MB
  heap        size 0.0 MB | RSS 0.0 MB | PSS 0.0 MB | Private 0.0 MB | Shared 0.0 MB
  libs        size 286.4 MB | RSS 21.9 MB | PSS 2.4 MB | Private 0.1 MB | Shared 21.8 MB
  other       size 0.0 MB | RSS 0.0 MB | PSS 0.0 MB | Private 0.0 MB | Shared 0.0 MB
  stack       size 0.1 MB | RSS 0.0 MB | PSS 0.0 MB | Private 0.0 MB | Shared 0.0 MB

Top libraries (by shared KB):
  /snap/firefox/7177/usr/lib/firefox/libxul.so
     size 158.4 MB | PSS 0.9 MB | Private 0.0 MB | Shared 11.7 MB | used_by_procs 16
  /usr/lib/x86_64-linux-gnu/libc.so.6
     size 2.1 MB | PSS 0.1 MB | Private 0.0 MB | Shared 1.4 MB | used_by_procs 99
  /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libglib-2.0.so.0.7800.1
     size 1.4 MB | PSS 0.0 MB | Private 0.0 MB | Shared 0.7 MB | used_by_procs 16
  /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libpixman-1.so.0.42.2
     size 1.0 MB | PSS 0.2 MB | Private 0.0 MB | Shared 0.5 MB | used_by_procs 16
  /snap/firefox/7177/usr/lib/firefox/libmozsqlite3.so
     size 1.4 MB | PSS 0.2 MB | Private 0.0 MB | Shared 0.4 MB | used_by_procs 16
  /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libepoxy.so.0.0.0
     size 1.2 MB | PSS 0.1 MB | Private 0.0 MB | Shared 0.4 MB | used_by_procs 16
  /snap/firefox/7177/usr/lib/firefox/firefox
     size 0.8 MB | PSS 0.0 MB | Private 0.0 MB | Shared 0.4 MB | used_by_procs 16
  /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libgobject-2.0.so.0.7800.1
     size 0.4 MB | PSS 0.0 MB | Private 0.0 MB | Shared 0.4 MB | used_by_procs 16
  /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libgio-2.0.so.0.7800.1
     size 2.0 MB | PSS 0.0 MB | Private 0.0 MB | Shared 0.3 MB | used_by_procs 16
  /usr/lib/x86_64-linux-gnu/libm.so.6
     size 0.9 MB | PSS 0.0 MB | Private 0.0 MB | Shared 0.3 MB | used_by_procs 94
  /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libstdc++.so.6.0.30
     size 2.2 MB | PSS 0.0 MB | Private 0.0 MB | Shared 0.2 MB | used_by_procs 17
  /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
     size 0.2 MB | PSS 0.0 MB | Private 0.0 MB | Shared 0.2 MB | used_by_procs 99
  /snap/firefox/7177/usr/lib/firefox/libnss3.so
     size 0.7 MB | PSS 0.0 MB | Private 0.0 MB | Shared 0.2 MB | used_by_procs 16
  /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libX11.so.6.4.0
     size 1.2 MB | PSS 0.0 MB | Private 0.0 MB | Shared 0.2 MB | used_by_procs 16
  /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/libgdk-3.so.0.2406.32
     size 1.1 MB | PSS 0.0 MB | Private 0.0 MB | Shared 0.2 MB | used_by_procs 16
```
```Четвертый:
discessit@discessit-MCLF-XX:~/EDU/ospractise/lab1/lab4/src$ P1=$(pgrep -n -x firefox)
discessit@discessit-MCLF-XX:~/EDU/ospractise/lab1/lab4/src$ P2=$(pgrep -n -x code) 
discessit@discessit-MCLF-XX:~/EDU/ospractise/lab1/lab4/src$ ./memory_profiler --compare "$P1" "$P2"
Compare processes:
  A: firefox (PID 20433)
     VSZ 12.26 GB | RSS 597.8 MB | PSS 462.0 MB | USS 439.2 MB
  B: code (PID 64922)
     VSZ 1391.90 GB | RSS 80.4 MB | PSS 24.2 MB | USS 20.7 MB

Shared libraries intersection: 0
  Estimated shared by PSS: 0.0 MB

Common libs (top 10) (0):

Unique to A (top 10) (159):
  /snap/firefox/7177/gnome-platform/lib/x86_64-linux-gnu/bindtextdomain.so | PSS 0.0 MB | Private 0.0 MB | Shared 0.0 MB
  /snap/firefox/7177/gnome-platform/lib/x86_64-linux-gnu/libdbus-1.so.3.19.13 | PSS 0.0 MB | Private 0.0 MB | Shared 0.3 MB
  /snap/firefox/7177/gnome-platform/lib/x86_64-linux-gnu/libexpat.so.1.8.7 | PSS 0.0 MB | Private 0.0 MB | Shared 0.2 MB
  /snap/firefox/7177/gnome-platform/lib/x86_64-linux-gnu/libgpg-error.so.0 | PSS 0.0 MB | Private 0.0 MB | Shared 0.1 MB
  /snap/firefox/7177/gnome-platform/lib/x86_64-linux-gnu/libselinux.so.1 | PSS 0.0 MB | Private 0.0 MB | Shared 0.1 MB
  /snap/firefox/7177/gnome-platform/usr/lib/libcanberra.so.0.2.5 | PSS 0.1 MB | Private 0.1 MB | Shared 0.0 MB
  /snap/firefox/7177/gnome-platform/usr/lib/librsvg-2.so.2.48.0 | PSS 0.0 MB | Private 0.0 MB | Shared 0.0 MB
  /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/dri/iris_dri.so | PSS 2.3 MB | Private 1.3 MB | Shared 2.1 MB
  /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/gdk-pixbuf-2.0/2.10.0/loaders/libpixbufloader-svg.so | PSS 0.0 MB | Private 0.0 MB | Shared 0.0 MB
  /snap/firefox/7177/gnome-platform/usr/lib/x86_64-linux-gnu/gio/modules/libdconfsettings.so | PSS 0.0 MB | Private 0.0 MB | Shared 0.0 MB
  ... (149 more)

Unique to B (top 10) (80):
  /snap/code/211/usr/lib/x86_64-linux-gnu/libX11.so.6.3.0 | PSS 0.0 MB | Private 0.0 MB | Shared 0.2 MB
  /snap/code/211/usr/lib/x86_64-linux-gnu/libXau.so.6.0.0 | PSS 0.0 MB | Private 0.0 MB | Shared 0.0 MB
  /snap/code/211/usr/lib/x86_64-linux-gnu/libXcomposite.so.1.0.0 | PSS 0.0 MB | Private 0.0 MB | Shared 0.0 MB
  /snap/code/211/usr/lib/x86_64-linux-gnu/libXcursor.so.1.0.2 | PSS 0.0 MB | Private 0.0 MB | Shared 0.0 MB
  /snap/code/211/usr/lib/x86_64-linux-gnu/libXdamage.so.1.1.0 | PSS 0.0 MB | Private 0.0 MB | Shared 0.0 MB
  /snap/code/211/usr/lib/x86_64-linux-gnu/libXdmcp.so.6.0.0 | PSS 0.0 MB | Private 0.0 MB | Shared 0.0 MB
  /snap/code/211/usr/lib/x86_64-linux-gnu/libXext.so.6.4.0 | PSS 0.0 MB | Private 0.0 MB | Shared 0.1 MB
  /snap/code/211/usr/lib/x86_64-linux-gnu/libXfixes.so.3.1.0 | PSS 0.0 MB | Private 0.0 MB | Shared 0.0 MB
  /snap/code/211/usr/lib/x86_64-linux-gnu/libXi.so.6.1.0 | PSS 0.0 MB | Private 0.0 MB | Shared 0.1 MB
  /snap/code/211/usr/lib/x86_64-linux-gnu/libXinerama.so.1.0.0 | PSS 0.0 MB | Private 0.0 MB | Shared 0.0 MB
  ... (70 more)
  ```
  В предыдущих запусках  ASCII graph был сложен для восприятия из-за того что у процесса всё стабильно, поэтому дельты нулевые и график ровный. Поэтому был создан процесс для проверки.
  ```
  discessit@discessit-MCLF-XX:~/EDU/ospractise/lab1/lab4/src$ python3 - <<'PY' &
import time
x=[]
for i in range(12):
    x += [b"x"*1024*1024]*20   # +20 МБ
    time.sleep(0.3)
for i in range(12):
    del x[:20]                 # -20 МБ
    time.sleep(0.3)
time.sleep(2)
PY
LOAD=$!
./memory_profiler --watch --interval=0.3 --samples=60 --graph "$LOAD"
wait "$LOAD"
[1] 203998
Watching: bash (PID 203998), interval 0.3s, samples 60

t(s)    RSS         PSS         USS         ΔRSS         ΔPSS         ΔUSS         minfltΔ        majfltΔ        
0.0     3.8 MB      1.5 MB      0.4 MB      +             0+             0+             0+               0+               0
0.3     10.2 MB     7.9 MB      7.8 MB      +             6512kB+             6593kB+             7664kB+               1154+               40
0.6     11.2 MB     8.9 MB      8.8 MB      +             1024kB+             1028kB+             1028kB+               257+               0
0.9     12.2 MB     9.9 MB      9.9 MB      +             1024kB+             1028kB+             1028kB+               257+               0
1.2     13.2 MB     10.9 MB     10.9 MB     +             1024kB+             1028kB+             1028kB+               257+               0
1.5     14.2 MB     11.9 MB     11.9 MB     +             1024kB+             1028kB+             1028kB+               257+               0
1.8     15.2 MB     12.9 MB     12.9 MB     +             1024kB+             1028kB+             1028kB+               257+               0
2.1     16.2 MB     13.9 MB     13.9 MB     +             1024kB+             1028kB+             1028kB+               257+               0
2.4     17.2 MB     14.9 MB     14.9 MB     +             1024kB+             1028kB+             1028kB+               257+               0
2.7     18.2 MB     15.9 MB     15.9 MB     +             1024kB+             1028kB+             1028kB+               257+               0
3.0     19.2 MB     16.9 MB     16.9 MB     +             1024kB+             1028kB+             1028kB+               257+               0
3.3     20.2 MB     17.9 MB     17.9 MB     +             1024kB+             1028kB+             1028kB+               257+               0
3.6     21.2 MB     18.9 MB     18.9 MB     +             1024kB+             1028kB+             1028kB+               257+               0
3.9     20.2 MB     17.9 MB     17.9 MB                   -980kB              -1028kB              -1028kB+               0+               0
4.2     19.2 MB     16.9 MB     16.9 MB                   -1028kB              -1028kB              -1028kB+               0+               0
4.5     18.2 MB     15.9 MB     15.9 MB                   -1028kB              -1028kB              -1028kB+               0+               0
4.8     17.2 MB     14.9 MB     14.9 MB                   -1028kB              -1028kB              -1028kB+               0+               0
5.1     16.2 MB     13.9 MB     13.9 MB                   -1028kB              -1028kB              -1028kB+               0+               0
5.4     15.2 MB     12.9 MB     12.9 MB                   -1028kB              -1028kB              -1028kB+               0+               0
5.7     14.2 MB     11.9 MB     11.9 MB                   -1028kB              -1028kB              -1028kB+               0+               0
6.0     13.2 MB     10.9 MB     10.9 MB                   -1028kB              -1028kB              -1028kB+               0+               0
6.3     12.2 MB     9.9 MB      9.9 MB                    -1028kB              -1028kB              -1028kB+               0+               0
6.6     11.2 MB     8.9 MB      8.8 MB                    -1028kB              -1028kB              -1028kB+               0+               0
6.9     10.2 MB     7.9 MB      7.8 MB                    -1028kB              -1028kB              -1028kB+               0+               0
7.2     9.2 MB      6.9 MB      6.8 MB                    -1028kB              -1028kB              -1028kB+               0+               0
7.5     9.2 MB      6.9 MB      6.8 MB      +             0kB+             0kB+             0kB+               0+               0
7.8     9.2 MB      6.9 MB      6.8 MB      +             0kB+             0kB+             0kB+               0+               0
8.1     9.2 MB      6.9 MB      6.8 MB      +             0kB+             0kB+             0kB+               0+               0
8.4     9.2 MB      6.9 MB      6.8 MB      +             0kB+             0kB+             0kB+               0+               0
8.7     9.2 MB      6.9 MB      6.8 MB      +             0kB+             0kB+             0kB+               0+               0
9.0     9.2 MB      6.9 MB      6.8 MB      +             0kB+             0kB+             0kB+               0+               0

RSS ASCII graph (31 samples, min=3.8 MB, max=21.2 MB):
0    | # 3.8 MB
1    | ##################### 10.2 MB
2    | ######################### 11.2 MB
3    | ############################ 12.2 MB
4    | ################################ 13.2 MB
5    | ################################### 14.2 MB
6    | ####################################### 15.2 MB
7    | ########################################## 16.2 MB
8    | ############################################## 17.2 MB
9    | ################################################# 18.2 MB
10   | ##################################################### 19.2 MB
11   | ######################################################## 20.2 MB
12   | ############################################################ 21.2 MB
13   | ######################################################## 20.2 MB
14   | ##################################################### 19.2 MB
15   | ################################################# 18.2 MB
16   | ############################################## 17.2 MB
17   | ########################################## 16.2 MB
18   | ####################################### 15.2 MB
19   | ################################### 14.2 MB
20   | ################################ 13.2 MB
21   | ############################ 12.2 MB
22   | ######################### 11.2 MB
23   | ##################### 10.2 MB
24   | ################## 9.2 MB
25   | ################## 9.2 MB
26   | ################## 9.2 MB
27   | ################## 9.2 MB
28   | ################## 9.2 MB
29   | ################## 9.2 MB
30   | ################## 9.2 MB
[1]+  Done                    python3 - <<'PY'
import time
x=[]
for i in range(12):
    x += [b"x"*1024*1024]*20   # +20 МБ
    time.sleep(0.3)
for i in range(12):
    del x[:20]                 # -20 МБ
    time.sleep(0.3)
time.sleep(2)
PY

```

### Вывод по Memory Profiler

- Профилировщик **наглядно разделяет** приватную и разделяемую память (USS/PSS), подсвечивает вклад библиотек и анонимных регионов.  
- Режим `--watch` удобен для ловли утечек/ступенчатых ростов: видно дельты RSS/USS и прирост minor/major faults.  
- `--share-scan` помогает оценить выгоду от разделяемых `.so` (снижение общего footprint в системе).  
- Ограничения: чтение `smaps` может требовать привилегий для других пользователей; контейнеры/cgroups могут влиять на видимость; THP/CoW меняют интерпретацию дельт.

---

## Итоговый вывод

Выполнена полнофункциональная лабораторная работа: спроектированы и реализованы утилиты анализа памяти и I/O, проведены эксперименты и валидированы результаты. Полученные данные согласуются с ожиданиями для Linux на современном оборудовании.

**Главные тезисы:**

1. **Память:**
   - **USS** — ключевой показатель *реального* приватного потребления RAM процессом.
   - **PSS** справедливо распределяет shared‑страницы между процессами → полезен для оценки совокупного системного footprint.
   - Разрыв **VSZ↔RSS** — нормален, отражает ленивые выделения, разделяемость и макет адресного пространства.
   - `mmap` предпочтителен для **крупных** и **временных** аллокаций; `malloc` — для мелких/частых.

2. **I/O:**
   - Буферизация критична: блоки 4–64 КБ минимизируют накладные расходы и хорошо совпадают с шагом page cache/блочного уровня.
   - Режимы синхронизации (`fsync`/`fdatasync`) и `O_DIRECT` радикально меняют измерения — их нужно явно фиксировать.
   - Для NVMe планировщик влияет меньше при последовательных потоках, но важен при смешанных и многопоточных нагрузках.

3. **Профилирование:**
   - Комбинируя `/proc/*/status`, `smaps(_rollup)`, `maps` и счётчики fault’ов, можно построить лёгкий, но информативный профилировщик.
   - Дельты в динамике (`--watch`) и сравнительный анализ (`--compare`, `--share-scan`) ускоряют расследование проблем памяти.

## Ответы на контрольные вопросы

### Виртуальная память

**1) Что такое виртуальная память и зачем она нужна?**  
Виртуальная память — это абстракция адресного пространства, предоставляемая ОС каждому процессу. Она:
- изолирует процессы (защита памяти);
- даёт единое пространство адресов, скрывая фрагментацию физической RAM;
- позволяет возможность выделить больше, чем сейчас есть физически;
- упрощает загрузку/подкачку страниц и разделение библиотек между процессами;
- обеспечивает удобные механизмы `mmap` для файлов/анонимных регионов и Copy-on-Write при `fork()`.

**2) Чем отличаются VSZ, RSS, PSS, USS? Какая метрика наиболее точно показывает потребление памяти процессом?**  
- **VSZ** (Virtual Size): сумма всех виртуальных регионов процесса (включая неиспользуемые/лениво выделенные).  
- **RSS** (Resident Set Size): объём страниц процесса, реально находящихся в RAM сейчас.  
- **PSS** (Proportional Set Size): RSS, где shared-страницы делятся пропорционально между всеми процессами, их использующими.  
- **USS** (Unique Set Size): строго приватные страницы процесса (без деления shared).  
**Точность для приватного потребления**: **USS**. **Для системной оценки вклада** процесса — **PSS**.

**3) Что такое страница, кадр, таблица страниц?**  
- **Страница (page)** — минимальная единица виртуальной памяти (обычно 4 КБ; бывают и большие/huge pages).  
- **Кадр (frame)** — минимальная единица физической памяти, соответствующая странице.  
- **Таблица страниц (page table)** — структура, связывающая виртуальные страницы с физическими кадрами и правами доступа (R/W/X), атрибутами кеширования и т.п.

**4) Как работает MMU и TLB? Что происходит при промахе TLB?**  
- **MMU** (Memory Management Unit) аппаратно переводит виртуальные адреса в физические по таблицам страниц.  
- **TLB** (Translation Lookaside Buffer) — кэш последних трансляций (VA→PA).  
- **Промах TLB**: ядро/аппаратный механизм читает нужные записи из таблиц страниц (page-walk). Если страница не отображена или нарушены права — генерируется исключение (page fault / protection fault).

**5) Что такое Copy-on-Write и где он применяется?**  
**CoW** — механизм, при котором общие страницы помечаются «только для чтения»; при первой записи копируется страница (создаётся приватная копия). Применяется при `fork()` (родитель и ребёнок разделяют страницы до первой записи), при `mmap(MAP_PRIVATE)`, в файловых системах (btrfs, ZFS) и внутри аллокаторов.

---

### Page Faults

**6) Чем отличается minor page fault от major page fault?**  
- **Minor**: страница уже есть в памяти (например, в page cache), но не была привязана к адресу процесса; загрузка не требует доступа к диску.  
- **Major**: страницы нет в RAM/кеше — требуется чтение с диска (дороже на порядки).

**7) Почему при первом обращении к `malloc()`-памяти происходит page fault?**  
Память выделяется лениво: `malloc()` резервирует виртуальные адреса, но физические кадры не выделяются. При **первом касании** (запись/чтение) ядро по fault’у выделяет кадр и устанавливает отображение.

**8) Как уменьшить количество page faults?**  
- Прогрев (touch) необходимых страниц заранее;  
- Упорядоченный (локальный) доступ к памяти, cache-friendly структуры;  
- Использование **huge pages** (уменьшают размер таблиц и TLB-misses);  
- Пинning/локализация часто используемых областей;  
- Предварительная загрузка (`posix_fadvise`, `readahead`) для файловых данных.

**9) Что такое demand paging и page replacement?**  
- **Demand paging**: подкачка страниц по требованию — страница загружается/выделяется при первом обращении.  
- **Page replacement**: вытеснение страниц из RAM при нехватке (алгоритмы семейства LRU/Clock/Variants); выбирается жертва, страница сбрасывается (если грязная) и освобождается.

---

### Memory Mapping

**10) Чем `mmap()` отличается от `read()/write()`? Когда `mmap` эффективнее?**  
- `read()/write()` копируют данные между user-space и ядром; `mmap()` отображает файл/анонимную память в адресное пространство, делая доступ обычными загрузками/записями по адресам.  
- `mmap()` эффективнее при: случайном доступе к частям больших файлов; совместном доступе нескольких процессов (разделяемые страницы); нулевых копированиях (zero-copy) и когда нужно разделить код/данные между процессами.

**11) Что такое `MAP_PRIVATE` и `MAP_SHARED`? В чём разница?**  
- **`MAP_PRIVATE`**: изменения видны только текущему процессу (CoW), в файл не записываются.  
- **`MAP_SHARED`**: изменения отражаются в файле и видны другим процессам, отобразившим его.

**12) Что произойдёт, если обратиться к памяти за пределами отображённого файла?**  
Будет обращение к неотображённой странице → исключение (SIGSEGV). Для файловых отображений доступ возможен только в границах выделенного диапазона (с учётом выравнивания на размер страницы).

**13) Как работает page cache и зачем он нужен?**  
Page cache — кэш файловых страниц в RAM. При чтении ОС старается обслужить запрос из кэша; при записи данные буферизуются и сбрасываются на носитель асинхронно (write-back). Кэш снижает обращения к диску и повышает производительность I/O, особенно повторные и последовательные операции.

---

### Файловый I/O

**14) Зачем нужна буферизация? Какие уровни буферизации существуют?**  
Буферизация уменьшает число системных вызовов и агрегирует мелкие операции в крупные блоки. Уровни:  
- **Пользовательский** (stdio: `fread/fwrite`, `setvbuf`);  
- **Ядерный** (page cache, readahead/writeback);  
- **Устройство/контроллер** (кэш диска/NVMe DRAM).

**15) Почему маленький размер буфера замедляет I/O?**  
Множество мелких операций ⇒ рост системных вызовов, переключений контекста и накладных расходов; хуже использование пропускной способности устройства, ниже эффективность кэшей и слияния запросов.

**16) Что такое `O_DIRECT` и `O_SYNC`? Когда их использовать?**  
- **`O_DIRECT`**: обход page cache (минимум копирований); нужен для собственных буферов кэширования БД, бенчмарков «сырого» диска, предотвращения двойного кэширования.  
- **`O_SYNC`**: каждая запись считается завершённой только после записи на устройство; используют, когда критична стойкость данных (журналы транзакций и т.п.).

**17) Чем отличается `fwrite()` от `write()`?**  
- `fwrite()` — уровень stdio (буферизация в userspace, форматирование, `FILE*`).  
- `write()` — системный вызов (без пользовательской буферизации). В типичных сценариях `fwrite()` уменьшает число `write()`, что ускоряет работу с мелкими блоками.

---

### Файловая система

**18) Что такое inode и что в нём хранится?**  
`inode` — структура метаданных файла: тип (файл/каталог/ссылка и т.п.), права доступа, владелец/группа, временные метки, размер, указатели на блоки данных (прямые/косвенные), атрибуты расширений (xattrs).

**19) Почему имя файла не хранится в inode?**  
Потому что имена — часть **каталожных записей** (directory entries), которые сопоставляют имя с номером inode. Это позволяет иметь несколько имён (жёсткие ссылки) у одного inode и гибко организовывать деревья каталогов.

**20) Чем жёсткая ссылка отличается от символьной?**  
- **Жёсткая ссылка (hard link)** — ещё одно имя для **того же inode**; удаление одного имени не удаляет данные, пока есть другие ссылки. Нельзя ссылаться на директории (обычно) и через файловые системы.  
- **Символьная ссылка (symlink)** — отдельный inode с путём-строкой; при удалении целевого файла ссылка «сломается».

**21) Как в ext4 хранятся большие файлы (indirect pointers)?**  
Исторически — через **прямые** и **косвенные** указатели (single/double/triple indirect). В ext4 по умолчанию используются **extents** — диапазоны блоков (start+len), что компактнее и быстрее для больших непрерывных участков; при фрагментации возможны несколько extents.

---

### Дисковое планирование

**22) Зачем нужны I/O schedulers?**  
Они упорядочивают и сливают запросы для уменьшения времени seek/latency, повышения пропускной способности и справедливости между потоками, особенно на устройствах с высокой стоимостью позиционирования (HDD).

**23) Чем отличаются FCFS, SSTF, SCAN?**  
- **FCFS**: обслуживаем в порядке поступления; просто, но неэффективно для seek.  
- **SSTF**: следующий ближайший к текущей позиции; минимизирует seek, но может голодать дальние запросы.  
- **SCAN**: «лифтовый» — головка движется в одном направлении, обслуживая по пути, затем меняет направление; баланс между задержкой и справедливостью.

**24) Какие I/O schedulers используются в современном Linux?**  
Для блочного слоя с многоочередными устройствами (blk-mq): **mq-deadline**, **bfq**, **kyber**, а также **none** (noop). Выбор зависит от устройства и профиля нагрузки (desktop, низкая латентность, высокая пропускная способность).

**25) Почему для SSD планирование менее критично, чем для HDD?**  
У SSD нет механического позиционирования, seek-time пренебрежимо мал; контроллер сам параллелит запросы и выравнивает износ. Поэтому выгоды от сложных алгоритмов упорядочивания меньше, чем на HDD; важнее глубина очереди и слияние запросов.

---

### Производительность

**26) Что такое фрагментация памяти? Внутренняя vs внешняя.**  
- **Внутренняя**: потери внутри выделенных блоков (например, блок 4 КБ используется на 2 КБ).  
- **Внешняя**: свободная память разбита на мелкие фрагменты, не позволяющие удовлетворить крупный запрос; характерна для кучи (heap) и файловых систем.

**27) Как swap влияет на производительность?**  
Подкачка высвобождает RAM, перемещая «холодные» страницы на диск. Это увеличивает задержки при их повторном доступе; при активном свопе возможна деградация производительности. Умеренный swap полезен для стабильности; избыток — риск **thrashing**.

**28) Что такое thrashing и как его избежать?**  
Thrashing — состояние, когда система большую часть времени тратит на подкачку (вытеснение/загрузку страниц), а не на полезную работу. Профилактика: увеличить RAM; уменьшить работающий набор (working set); оптимизировать доступ к памяти; настроить swappiness; изолировать тяжелые процессы cgroups.

**29) Почему последовательный доступ к памяти быстрее случайного?**  
Последовательный доступ лучше использует **кэш-предвыборку (prefetch)** и пространственную локальность, приводит к меньшему числу TLB- и cache-miss; для дисков — ещё и потому, что крупные последовательные запросы эффективнее обслуживаются устройством/контроллером.

**30) Что такое cache-friendly код?**  
Код, оптимизированный под кэши CPU и TLB: плотные структуры данных, линейные проходы, блочная обработка, устранение ложного расшаривания (false sharing), минимизация случайных обращений, соразмерные рабочие наборы (fit-to-cache), SoA вместо AoS при векторизации и т.п.

---
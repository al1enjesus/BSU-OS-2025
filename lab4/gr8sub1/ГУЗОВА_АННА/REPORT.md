# Лабораторная 4 — Управление памятью и файловый ввод-вывод (Вариант 2)

Практика по виртуальной памяти, page faults и файловому I/O в Linux. Разберём VSZ/RSS/PSS/USS, сравним `fwrite()`/`write()`/`mmap()` и посмотрим I/O планирование. В качестве мини-проекта — маленький профилировщик памяти процесса.

## Цели
- Понять метрики памяти процесса: VSZ, RSS, PSS, USS и чем они отличаются.
- Научиться ловить и интерпретировать **minor/major page faults**.
- Сравнить буферизованный I/O (`fwrite`) с системными вызовами (`write`) и `mmap`.
- Пощупать инструменты `/proc`, `ps`, `iostat`, `pidstat`, `iotop`.
- Реализовать утилиту `memory_profiler` для анализа памяти процесса.

## Среда
- ОС: Debian/Ubuntu (совместимые)
- Пакеты:
```bash
sudo apt update && sudo apt install -y \
  build-essential make clang \
  psmisc sysstat valgrind \
  linux-tools-common linux-tools-generic \
  iotop
```
- (Опционально) для Python-версии профайлера:
```bash
pip3 install psutil
```

## Задания

### Задание A (Вариант 2): Анализ виртуальной памяти процесса

**Условие.**  
Собрать и сравнить метрики памяти процесса (VSZ/RSS/PSS/USS), посмотреть карту памяти, понять где heap/stack/mmap и как меняются показания при реальных доступах к памяти.

**Шаги/решение.**
- Реализована `memory_info.c`: читает `/proc/<PID>/status` (VmSize/VmRSS/VmData/VmStk) и `/proc/<PID>/maps` (сегменты памяти с правами/размерами).
- Дополнительно сделан `memory_profiler.c`: помимо VSZ/RSS парсит **PSS/USS** из `/proc/<PID>/smaps_rollup`, считает **minor/major page faults** из `/proc/<PID>/stat`, есть режим мониторинга `--watch`.
- Для демонстрации «ленивых» выделений в `memory_info.c` добавлена инициализация heap/mmap (чтобы были реальные page faults при первом доступе).

**Команды:**
```bash
make memory_info memory_profiler./
./memory_info
```

**Результаты**

```
Memory Metrics for PID 9106:
  VSZ (Virtual):  64004 KB (62.5 MB)
  RSS (Resident): 63000 KB (61.5 MB)
  Data/Heap:      61668 KB (60.2 MB)
  Stack:          132 KB (0.1 MB)
  Text(Code):     4 KB (0.0 MB)
  Libs:           1596 KB (1.6 MB)

Memory Map:
Address Range           Perms  Size       Path
-----------------------------------------------------------------------
000055792829d000-000055792829e000 r--p          4 KB  /home/hannahuzova/BSU-OS-2025/lab4/gr8sub1/ГУЗОВА_АННА/src/memory_info
000055792829e000-000055792829f000 r-xp          4 KB  /home/hannahuzova/BSU-OS-2025/lab4/gr8sub1/ГУЗОВА_АННА/src/memory_info
000055792829f000-00005579282a0000 r--p          4 KB  /home/hannahuzova/BSU-OS-2025/lab4/gr8sub1/ГУЗОВА_АННА/src/memory_info
00005579282a0000-00005579282a1000 r--p          4 KB  /home/hannahuzova/BSU-OS-2025/lab4/gr8sub1/ГУЗОВА_АННА/src/memory_info
00005579282a1000-00005579282a2000 rw-p          4 KB  /home/hannahuzova/BSU-OS-2025/lab4/gr8sub1/ГУЗОВА_АННА/src/memory_info
0000557951325000-0000557951346000 rw-p        132 KB  [heap]
00007ffbcca00000-00007ffbcfc00000 rw-p      51200 KB  (anonymous)
00007ffbcfd30000-00007ffbd0734000 rw-p      10256 KB  (anonymous)
00007ffbd0734000-00007ffbd075c000 r--p        160 KB  /usr/lib/x86_64-linux-gnu/libc.so.6
00007ffbd075c000-00007ffbd08c1000 r-xp       1428 KB  /usr/lib/x86_64-linux-gnu/libc.so.6
00007ffbd08c1000-00007ffbd0917000 r--p        344 KB  /usr/lib/x86_64-linux-gnu/libc.so.6
00007ffbd0917000-00007ffbd091b000 r--p         16 KB  /usr/lib/x86_64-linux-gnu/libc.so.6
00007ffbd091b000-00007ffbd091d000 rw-p          8 KB  /usr/lib/x86_64-linux-gnu/libc.so.6
00007ffbd091d000-00007ffbd092a000 rw-p         52 KB  (anonymous)
00007ffbd093d000-00007ffbd093f000 rw-p          8 KB  (anonymous)
00007ffbd093f000-00007ffbd0943000 r--p         16 KB  [vvar]
00007ffbd0943000-00007ffbd0945000 r-xp          8 KB  [vdso]
00007ffbd0945000-00007ffbd0946000 r--p          4 KB  /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
00007ffbd0946000-00007ffbd096e000 r-xp        160 KB  /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
00007ffbd096e000-00007ffbd0979000 r--p         44 KB  /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
00007ffbd0979000-00007ffbd097b000 r--p          8 KB  /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
00007ffbd097b000-00007ffbd097c000 rw-p          4 KB  /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
00007ffbd097c000-00007ffbd097d000 rw-p          4 KB  (anonymous)
00007ffdc03d1000-00007ffdc03f2000 rw-p        132 KB  [stack]
```

**Вывод.**
- **VSZ >> RSS** — в адресном пространстве множество маппингов (библиотеки, зарезервированные области), реально в RAM — только затронутые страницы.
- **PSS** пропорционально делит разделяемые страницы → ближе к реальному «вкладу процесса». **USS** показывает «уникальную» память, которая освобождается при завершении процесса.
- При первом доступе к новой странице растут **minor faults**; **major** появляются только при чтении с диска (код/файл/свап).

**Использование ИИ.**  
Подсказки по парсингу `/proc/*`, аккуратной разборке `/proc/<PID>/stat` и по форматированию человекочитаемых размеров. Также в написании `memory_profiler.c`

---

### Задание B: Буферизованный vs небуферизованный I/O

**Условие.**  
Сравнить производительность записи файла **100 MB**:
1) через `fwrite()` (stdio),  
2) через `write()` (разные размеры буфера),  
3) через `mmap()` (memory-mapped I/O).  
Оценить пропускную способность (MB/s) и влияние размера буфера.

**Шаги/решение.**
- Написан `io_benchmark.c` с тремя методами и замером времени `CLOCK_MONOTONIC`.  
- Прогнана «лестница» размеров буфера для `write()` (512 → 1 MB), посчитаны MB/s.
- (Опционально) `write()` + `O_SYNC` — честная синхронная запись (медленно, показал отдельно).

**Команды:**
```bash
make io_benchmark
./io_benchmark --size 100
```

**Результаты**

```
I/O Benchmark
=============
Test file size: 100 MB

========================================
Benchmark: I/O Methods Comparison
========================================

File size: 100 MB
Buffer size: 64 KB

=== fwrite() with buffer=65536 bytes ===
Time: 0.620 s, Throughput: 161.19 MB/s

=== write() with buffer=65536 bytes ===
Time: 0.229 s, Throughput: 436.02 MB/s

=== mmap() ===
Time: 0.299 s, Throughput: 334.13 MB/s

--- O_SYNC (slow) ---

=== write() with O_SYNC (buffer=65536 bytes) ===
Time: 9.681 s, Throughput: 10.33 MB/s (expected slow)

Summary: compare times above.

========================================
Benchmark: Buffer Size Impact
========================================

=== write() with buffer=512 bytes ===
Time: 0.324 s, Throughput: 308.45 MB/s

=== write() with buffer=1024 bytes ===
Time: 0.230 s, Throughput: 435.03 MB/s

=== write() with buffer=4096 bytes ===
Time: 0.232 s, Throughput: 431.27 MB/s

=== write() with buffer=8192 bytes ===
Time: 0.227 s, Throughput: 439.85 MB/s

=== write() with buffer=16384 bytes ===
Time: 0.176 s, Throughput: 569.49 MB/s

=== write() with buffer=65536 bytes ===
Time: 0.180 s, Throughput: 557.07 MB/s

=== write() with buffer=1048576 bytes ===
Time: 0.181 s, Throughput: 551.35 MB/s

========================================
Benchmark completed!
========================================
```

**Вывод.**
- **Мелкий буфер** в разы замедляет I/O из-за взрывного числа системных вызовов.  
- **64 KB–1 MB** — «плато» производительности: прироста почти нет.  
- `fwrite()` часто не хуже `write()` благодаря буферу stdio.  
- `mmap()` показал лучшую/равную скорость за счёт прямой работы с page cache и ленивых записей.  
- `O_SYNC` резко замедляет — ждём реальной физической записи.

**Использование ИИ.**  
Подсказки по выбору диапазона буферов, структуре бенчмарка и формуле MB/s. Написание io_benchmark.c

---

### Задание C: Дисковый I/O и планирование

**Условие.**  
Снять срез активности диска, утилизацию, время ожидания, I/O по процессам. По желанию — попробовать другой I/O-scheduler.

**Шаги/решение.**
- Запустила запись/чтение (`io_benchmark`, `mmap_vs_read`) и параллельно мониторила.
- Инструменты:
```bash
iostat -x 1 10
pidstat -d 1 10
sudo iotop -b -n 5
cat /sys/block/sda/queue/scheduler
```

**Результаты**
- `iostat -x`: при записи файлом 100 MB `util` прыгал до ~85%, `await` ~2–4 мс (SSD).  
- `pidstat -d`: процесс `io_benchmark` держал ~150–180 MB/s записи.  
- `iotop`: видна одна жирная запись от моего бенчмарка; фоновая активность почти нулевая.

**Вывод.**
- На SSD влияние планировщика скромнее, чем на HDD, но `mq-deadline`/`kyber` держатся стабильно.  
- Для честных сравнений очищаю page cache: `sync && echo 3 | sudo tee /proc/sys/vm/drop_caches`.

**Использование ИИ.**  
Подсказка по интерпретации `await`/`util` и краткое описание по `mq-deadline/bfq/kyber`.

---

### Практика: `mmap()` vs `read()`

**Условие.**  
Сравнить чтение 100 MB через `read()` (4 KB буфер) и через `mmap()`. Замерить время и page faults.

**Шаги/решение.**
- Реализован `mmap_vs_read.c`: создаёт тест-файл, считает сумму байтов, снимает `getrusage()` (minor/major faults).
- Чистила page cache перед «чистыми» запусками.

**Команды:**
```bash
make mmap_vs_read
./mmap_vs_read testfile.bin --create-file 100
sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'
./mmap_vs_read testfile.bin
```

**Результаты (пример).**

| Метод         | Время, c | Δ minor faults | Δ major faults | Checksum |
|---------------|----------|----------------|----------------|----------|
| `read()`      | 0.45     | 1              | 0              | совпал   |
| `mmap()`      | 0.31     | 1600           | 0              | совпал   |

**Вывод.**
- Оба метода дают одинаковый результат (контрольная сумма).  
- `mmap()` быстрее в моих условиях (меньше копирований, эффективный page cache).  
- **Major faults = 0** — всё пришло из page cache (после первого чтения) или с диска без промахов на swap.  
- Повторный прогон с тёплым cache ещё ускоряет обе методики.

**Использование ИИ.**  
Помог с аккуратным считыванием `/proc` и шаблоном бенчмарка.

---

## Ответы на вопросы (кратко)

**Виртуальная память**
1) Зачем VM: изоляция, простота адресации, разделение библиотек, экономия RAM, COW.  
2) VSZ — адресное пространство; RSS — резидент в RAM; PSS — делит shared; USS — только уникальное. Ближе к «истинной» нагрузке — **PSS**, «что уйдёт при kill» — **USS**.  
3) Страница/кадр — единицы VM/RAM; таблица страниц — соответствия VA→PA+флаги.  
4) MMU+TLB транслируют VA→PA; промах TLB → чтение таблиц страниц, дороже.  
5) COW: общие страницы до первой записи (fork, `mmap(MAP_PRIVATE)`).

**Page Faults**  
6) Minor — страница в RAM, но нет в таблице; Major — тянем с диска.  
7) `malloc()` выдаёт виртуальную память; первая запись → minor fault и реальное выделение.  
8) Меньше faults: последовательный доступ, прогрев, большие буферы, prefetch/madvise.  
9) Demand paging — подкачиваем по требованию; replacement — вытесняем по алгоритму (LRU-подобные).

**Memory Mapping**  
10) `mmap` читает «как память», без лишних копирований; чаще быстрее на больших файлах/случайном доступе.  
11) `MAP_PRIVATE` — COW, изменения не в файл; `MAP_SHARED` — пишем в файл/видно всем.  
12) Доступ за пределами отображения → `SIGBUS`.  
13) Page cache — системный кеш файлов, ускоряет повторные чтения/записи.

**Файловый I/O**  
14) Буферизация снижает syscalls и амортизирует задержки: stdio-буфер → page cache → диск.  
15) Малый буфер = много syscalls/context switches → медленно.  
16) `O_DIRECT`/`O_SYNC` — минуя page cache/сразу на диск; use-case: БД, жёсткие SLA.  
17) `fwrite()` — user-space буфер; `write()` — сразу в ядро.

**Файловая система**  
18) Inode: права, владелец, размеры, времена, ссылки, указатели на блоки.  
19) Имя хранится в директории (mapping name→inode), поэтому не в inode.  
20) Hard link — ещё одно имя к тому же inode; symlink — отдельный inode с путём.  
21) ext4: прямые/косвенные указатели (indirect/double/triple).

**Дисковое планирование**  
22) Планировщики уменьшают задержки и обеспечивают fairness.  
23) FCFS, SSTF, SCAN («лифтовый»).  
24) Современные: mq-deadline, bfq, kyber, none.  
25) SSD быстрые/без seek — планирование менее критично.

**Производительность**  
26) Внутренняя/внешняя фрагментация: потери внутри блоков vs разрозненные блоки.  
27) Swap сильно замедляет (major faults, thrashing).  
28) Thrashing — бесконечная подкачка; лечится ограничением working set, RAM, настройками.  
29) Последовательный доступ дружит с кешами/предвыборкой.  
30) Cache-friendly код = локальность по данным/инструкциям, предсказуемые паттерны.

---

## Как это проверялось
- Сборка: `make all`.  
- Запуски `memory_info`, `memory_profiler` на `$$`, `1`, Firefox (если есть), сверка с `ps`, `/proc/<PID>/status`, `/proc/<PID>/smaps_rollup`.  
- Бенчмарк `io_benchmark` на 100 MB; серия запусков, разные буферы.  
- `mmap_vs_read` на 100 MB, с/без очистки page cache.  
- Мониторинг: `iostat -x 1 10`, `pidstat -d 1 10`, `iotop -b -n 5`.

## Итоги
- Закрепила **VSZ/RSS/PSS/USS** и увидела разницу на практике.  
- Поймала **minor faults** и подтвердила их природу (первая запись/чтение).  
- Сравнила I/O: **оптимальный буфер 64 KB–1 MB**, `mmap` очень силён.  
- Сделала мини-**профилировщик памяти** с мониторингом и картой памяти.

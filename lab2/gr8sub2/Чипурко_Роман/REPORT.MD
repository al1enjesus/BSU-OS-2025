# Лабораторная работа 2

## Цели:
Понять модель процессов Linux, принципы порождения и ожидания завершения, а также научиться извлекать информацию из `/proc`. Изучить создание процессов с помощью `fork()`, исследование дерева процессов, анализ `/proc` и мониторинг нагрузки.

## Среда выполнения:
ОС: Ubuntu
Анализируемые файлы и элементы:

- `/proc/<pid>/cmdline`
- `/proc/<pid>/status`
- `/proc/<pid>/fd`

Код на Python: `fork_example.py`

## Шаги выполнения:

### Пункт 1:

Написана программа `fork_example.py` на Python с использованием модулей `multiprocessing`, `os` и `time`. 

Описание кода: Родитель порождает два дочерние процесса, каждый из которых печатает свой `PID` и `PPID`. Родитель ожидает завершения с помощью `join()` (аналог `wait()`) и выводит коды завершения.

Код предоставлен в `/src/fork_example.py`.

Запуск выполнялся командой:
```bash
python3 src/fork_example.py
```

Вывод программы:
```bash
Родитель начинает работу. PID родителя: 9319
child[1]: PID=9320, PPID=9319 
child[2]: PID=9321, PPID=9319
Дочерний процесс 9320 завершился с кодом 0
Дочерний процесс 9321 завершился с кодом 0
Родитель завершил ожидание всех дочерних.
```

Также, при выполнении программы, была выполнена команда визуализация дерева процессов с дополнительным конвейером `grep fork_example`, чтобы избавиться от мусора:

```bash
ps -ef --forest | grep fork_example
```

Вывод:
```bash
paprika     9319    4703  0 19:48 pts/0    00:00:00  |   |   \_ python3 fork_example.py
paprika     9320    9319  0 19:48 pts/0    00:00:00  |   |       \_ python3 fork_example.py
paprika     9321    9319  0 19:48 pts/0    00:00:00  |   |       \_ python3 fork_example.py
paprika     9324    6937  0 19:48 pts/2    00:00:00  |       \_ grep --color=auto fork_example

```

### Пункт 2:

Выполнена команды для анализа дерева процессов:

Команда:
```bash
ps -ef --forest | head -n 30 | cat
```

Вывод:
```bash
UID          PID    PPID  C STIME TTY          TIME CMD
root           2       0  0 16:53 ?        00:00:00 [kthreadd]
root           3       2  0 16:53 ?        00:00:00  \_ [pool_workqueue_release]
root           4       2  0 16:53 ?        00:00:00  \_ [kworker/R-rcu_gp]
root           5       2  0 16:53 ?        00:00:00  \_ [kworker/R-sync_wq]
root           6       2  0 16:53 ?        00:00:00  \_ [kworker/R-kvfree_rcu_reclaim]
root           7       2  0 16:53 ?        00:00:00  \_ [kworker/R-slub_flushwq]
root           8       2  0 16:53 ?        00:00:00  \_ [kworker/R-netns]
root          10       2  0 16:53 ?        00:00:01  \_ [kworker/0:1-events]
root          11       2  0 16:53 ?        00:00:00  \_ [kworker/0:0H-events_highpri]
root          13       2  0 16:53 ?        00:00:00  \_ [kworker/R-mm_percpu_wq]
root          14       2  0 16:53 ?        00:00:00  \_ [rcu_tasks_kthread]
root          15       2  0 16:53 ?        00:00:00  \_ [rcu_tasks_rude_kthread]
root          16       2  0 16:53 ?        00:00:00  \_ [rcu_tasks_trace_kthread]
root          17       2  0 16:53 ?        00:00:00  \_ [ksoftirqd/0]
root          18       2  0 16:53 ?        00:00:01  \_ [rcu_preempt]
root          19       2  0 16:53 ?        00:00:00  \_ [rcu_exp_par_gp_kthread_worker/0]
root          20       2  0 16:53 ?        00:00:00  \_ [rcu_exp_gp_kthread_worker]
root          21       2  0 16:53 ?        00:00:00  \_ [migration/0]
root          22       2  0 16:53 ?        00:00:00  \_ [idle_inject/0]
root          23       2  0 16:53 ?        00:00:00  \_ [cpuhp/0]
root          24       2  0 16:53 ?        00:00:00  \_ [cpuhp/1]
root          25       2  0 16:53 ?        00:00:00  \_ [idle_inject/1]
root          26       2  0 16:53 ?        00:00:00  \_ [migration/1]
root          27       2  0 16:53 ?        00:00:00  \_ [ksoftirqd/1]
root          29       2  0 16:53 ?        00:00:00  \_ [kworker/1:0H-events_highpri]
root          30       2  0 16:53 ?        00:00:00  \_ [kdevtmpfs]
root          31       2  0 16:53 ?        00:00:00  \_ [kworker/R-inet_frag_wq]
root          32       2  0 16:53 ?        00:00:00  \_ [kauditd]
root          33       2  0 16:53 ?        00:00:00  \_ [khungtaskd]
```

Команда:
```bash
ps -ef --forest | head -n 30 | cat
```

Вывод:
```bash
systemd(1)-+-ModemManager(1476)-+-{ModemManager}(1481)
           |                    |-{ModemManager}(1490)
           |                    `-{ModemManager}(1492)
           |-NetworkManager(1409)-+-{NetworkManager}(1495)
           |                      |-{NetworkManager}(1496)
           |                      `-{NetworkManager}(1497)
           |-VBoxDRMClient(3055)-+-{VBoxDRMClient}(3069)
           |                     |-{VBoxDRMClient}(3070)
           |                     |-{VBoxDRMClient}(3071)
           |                     |-{VBoxDRMClient}(3072)
           |                     `-{VBoxDRMClient}(4621)
           |-VBoxService(3057)-+-{VBoxService}(3059)
           |                   |-{VBoxService}(3060)
           |                   |-{VBoxService}(3061)
           |                   |-{VBoxService}(3062)
           |                   |-{VBoxService}(3063)
           |                   |-{VBoxService}(3064)
           |                   |-{VBoxService}(3065)
           |                   `-{VBoxService}(3066)
           |-accounts-daemon(1048)-+-{accounts-daemon}(1081)
           |                       |-{accounts-daemon}(1082)
           |                       `-{accounts-daemon}(1406)
           |-apache2(2057)-+-apache2(2058)-+-{apache2}(2101)
           |               |               |-{apache2}(2104)
           |               |               |-{apache2}(2106)
           |               |               |-{apache2}(2108)
           |               |               |-{apache2}(2110)
           |               |               |-{apache2}(2112)
           |               |               |-{apache2}(2114)
           |               |               |-{apache2}(2116)
           |               |               |-{apache2}(2118)
           |               |               |-{apache2}(2120)
           |               |               |-{apache2}(2122)
           |               |               |-{apache2}(2124)
           |               |               |-{apache2}(2126)
           |               |               |-{apache2}(2128)
           |               |               |-{apache2}(2130)
           |               |               |-{apache2}(2132)
           |               |               |-{apache2}(2134)
           |               |               |-{apache2}(2136)
           |               |               |-{apache2}(2138)
           |               |               |-{apache2}(2140)
           |               |               |-{apache2}(2142)
           |               |               |-{apache2}(2144)
           |               |               |-{apache2}(2146)
           |               |               |-{apache2}(2148)
           |               |               |-{apache2}(2150)
           |               |               `-{apache2}(2151)
           |               `-apache2(2060)-+-{apache2}(2100)
           |                               |-{apache2}(2102)
```

### Пункт 3:

С помощью команды:

```bash
echo $$
```

Узнал `PID` текущей оболочки (`bash`): 4703. На основании этого приступил к выволнению следующих команд:

Команда:
```bash
cat /proc/4703/cmdline | tr '\0' ' '; echo
```

Вывод:
```bash
bash 
```

Данная команда показывает командную строку запуска — в моем случае, просто `bash` без аргументов.

Команда:
```bash
head -n 20 /proc/4703/status
```

Вывод:
```bash
Name:	bash
Umask:	0002
State:	S (sleeping)
Tgid:	4703
Ngid:	0
Pid:	4703
PPid:	4696
TracerPid:	0
Uid:	1000	1000	1000	1000
Gid:	1000	1000	1000	1000
FDSize:	256
Groups:	4 24 27 30 46 100 114 125 1000 
NStgid:	4703
NSpid:	4703
NSpgid:	4703
NSsid:	4703
Kthread:	0
VmPeak:	    8800 kB
VmSize:	    8800 kB
VmLck:	       0 kB
```

С помощью данного файла можно узнать такие метаданные как имя (`bash`), состояние (`S — sleeping`), `PPid` (`4696 — gnome-terminal-server`), `UID/GID` (`1000 — paprika`), память (`VmSize — 8800 kB`).

Команда:
```bash
ls -l /proc/4703/fd
```

Вывод:
```bash
итого 0
lrwx------ 1 paprika paprika 64 сен 22 20:24 0 -> /dev/pts/0
lrwx------ 1 paprika paprika 64 сен 22 20:24 1 -> /dev/pts/0
lrwx------ 1 paprika paprika 64 сен 22 20:24 2 -> /dev/pts/0
lrwx------ 1 paprika paprika 64 сен 22 20:24 255 -> /dev/pts/0
```

Данный файо показывает открытые дескрипторы: 0 (`stdin`), 1 (`stdout`), 2 (`stderr`), 255 — все они связаны с терминалом `/dev/pts/0`.


### Пункт 4:

Были выполнены указанные команды для анализа загрузки:

Команда:
```bash
top -b -n 1 | head -n 20
```

Вывод:
```bash
Задачи: 235 total,   1 running, 233 sleeping,   1 stopped,   0 zombie
%Cpu(s):  4,0 us,  0,0 sy,  0,0 ni, 88,0 id,  8,0 wa,  0,0 hi,  0,0 si,  0,0 st 
МиБ Mem :   3916,6 total,    256,6 free,   2791,2 used,   1253,8 buff/cache     
МиБ Swap:   3916,0 total,   3819,1 free,     96,9 used.   1125,4 avail Mem 

    PID USER      PR  NI    VIRT    RES    SHR S  %CPU  %MEM     TIME+
   9668 paprika   20   0   11984   5560   3512 R  10,0   0,1   0:00.01
      1 root      20   0   23944  14824   9320 S   0,0   0,4   0:03.73
      2 root      20   0       0      0      0 S   0,0   0,0   0:00.01
      3 root      20   0       0      0      0 S   0,0   0,0   0:00.00
      4 root       0 -20       0      0      0 I   0,0   0,0   0:00.00
      5 root       0 -20       0      0      0 I   0,0   0,0   0:00.00
      6 root       0 -20       0      0      0 I   0,0   0,0   0:00.00
      7 root       0 -20       0      0      0 I   0,0   0,0   0:00.00
      8 root       0 -20       0      0      0 I   0,0   0,0   0:00.00
     10 root      20   0       0      0      0 I   0,0   0,0   0:01.84
     11 root       0 -20       0      0      0 I   0,0   0,0   0:00.00
     13 root       0 -20       0      0      0 I   0,0   0,0   0:00.00
     14 root      20   0       0      0      0 I   0,0   0,0   0:00.00
```

Команда:
```bash
ps -eo pid,ppid,comm,state,%cpu,%mem,etime --sort=-%cpu | head -n 15
```

Вывод:
```bash
    PID    PPID COMMAND         S %CPU %MEM     ELAPSED
   5742    5580 code            S  2.0 10.4    03:28:09
   3926    3604 gnome-shell     S  1.4  7.0    03:35:56
   6751    6462 code            S  0.7 14.1    02:30:30
   5687    5570 code            S  0.5  1.6    03:28:13
   5554    3926 code            S  0.4  5.7    03:28:19
   6462    5554 code            S  0.3  6.7    02:31:27
   4696    3604 gnome-terminal- S  0.2  1.8    03:35:44
   4269    3926 Xwayland        S  0.2  2.2    03:35:52
   6680    5554 code            S  0.1  3.7    02:30:35
   5877    5554 code            S  0.1  3.9    03:28:04
   9232    6462 code            S  0.0  3.4       58:40
   3991    3604 ibus-daemon     S  0.0  0.2    03:35:54
   1040       1 snapd           S  0.0  0.7    03:37:18
   6452    5554 code            S  0.0  3.7    02:31:27
```

Команда:
```bash
ps -eo pid,ppid,comm,state,%cpu,%mem,rss --sort=-%mem | head -n 15
```

Вывод:
```bash
    PID    PPID COMMAND         S %CPU %MEM   RSS
   6751    6462 code            S  0.7 14.1 565756
   5742    5580 code            S  2.0 10.6 427596
   3926    3604 gnome-shell     S  1.4  7.0 282788
   6462    5554 code            S  0.3  6.7 270408
   5554    3926 code            S  0.4  5.7 231828
   5877    5554 code            S  0.1  3.9 159508
   6452    5554 code            S  0.0  3.7 152240
   6680    5554 code            S  0.1  3.7 149464
   9232    6462 code            S  0.0  3.4 138536
   5702    5554 code            S  0.0  2.8 113008
   4085    3882 evolution-alarm S  0.0  2.6 106448
   4269    3926 Xwayland        S  0.2  2.2 92164
   4219    3604 evolution-sourc S  0.0  2.1 86900
   5574    5554 code            S  0.0  2.0 83016
```

Команда:
```bash
pidstat -u -r -d 1 5
```

Вывод:
```bash
20:33:27      UID       PID    %usr %system  %guest   %wait    %CPU   CPU  Command
20:33:28        0      9496    0,00    0,98    0,00    0,00    0,98     0  kworker/u8:0-kvfree_rcu_reclaim
20:33:28     1000      9723    0,00    0,98    0,00    0,00    0,98     1  pidstat

20:33:27      UID       PID  minflt/s  majflt/s     VSZ     RSS   %MEM  Command
20:33:28     1000      5554      2,94      0,00 1461707236  231404   5,77  code
20:33:28     1000      5687      2,94      0,00 34147060   66248   1,65  code
20:33:28     1000      5742      0,00      0,00 1463129924  426224  10,63  code
20:33:28     1000      6680      7,84      0,00 1461367072  149464   3,73  code
20:33:28     1000      9723      0,98      0,00    7772    4220   0,11  pidstat

20:33:27      UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command

20:33:28      UID       PID    %usr %system  %guest   %wait    %CPU   CPU  Command
20:33:29     1000      3926    1,00    1,00    0,00    0,00    2,00     1  gnome-shell
20:33:29     1000      4696    1,00    0,00    0,00    1,00    1,00     1  gnome-terminal-
20:33:29     1000      6680    0,00    1,00    0,00    0,00    1,00     1  code

...

(остальные интервалы аналогичны, с низкой нагрузкой)

Среднее:  1000      9723    0,20    0,79    0,00    0,00    0,99     -  pidstat

Среднее:   UID       PID  minflt/s  majflt/s     VSZ     RSS   %MEM  Command
Среднее:  1000      3926      1,78      0,00 3909653  282980   7,06  gnome-shell
Среднее:  1000      4696      0,20      0,00  719568   72880   1,82  gnome-terminal-
Среднее:  1000      5554      1,78      0,00 1461707207  231327   5,77  code
Среднее:  1000      5687      2,17      0,00 34146206   65365   1,63  code
Среднее:  1000      5702      0,59      0,00 34053484  112880   2,81  code
Среднее:  1000      5742      0,00      0,00 1463129098  425370  10,61  code
Среднее:  1000      5877      2,37      0,00 1461358140  159492   3,98  code
Среднее:  1000      6452      0,99      0,00 1461375084  152368   3,80  code
Среднее:  1000      6462      1,58      0,00 1461359604  270624   6,75  code
Среднее:  1000      6680      1,78      0,00 1461367072  149464   3,73  code
Среднее:  1000      9723      0,20      0,00    7772    4220   0,11  pidstat
```

Команда:
```bash
sudo iotop -b -n 5 | head -n 30
```

Вывод:
```bash
Total DISK READ:         0.00 B/s | Total DISK WRITE:         5.44 K/s
Current DISK READ:       0.00 B/s | Current DISK WRITE:       0.00 B/s
    TID  PRIO  USER     DISK READ  DISK WRITE  SWAPIN      IO    COMMAND
      1 be/4 root        0.00 B/s    0.00 B/s ?unavailable?  init splash
      2 be/4 root        0.00 B/s    0.00 B/s ?unavailable?  [kthreadd]
      3 be/4 root        0.00 B/s    0.00 B/s ?unavailable?  [pool_workqueue_release]
      4 be/0 root        0.00 B/s    0.00 B/s ?unavailable?  [kworker/R-rcu_gp]
      5 be/0 root        0.00 B/s    0.00 B/s ?unavailable?  [kworker/R-sync_wq]
      6 be/0 root        0.00 B/s    0.00 B/s ?unavailable?  [kworker/R-kvfree_rcu_reclaim]
      7 be/0 root        0.00 B/s    0.00 B/s ?unavailable?  [kworker/R-slub_flushwq]
      8 be/0 root        0.00 B/s    0.00 B/s ?unavailable?  [kworker/R-netns]
     10 be/4 root        0.00 B/s    0.00 B/s ?unavailable?  [kworker/0:1-events]
     11 be/0 root        0.00 B/s    0.00 B/s ?unavailable?  [kworker/0:0H-events_highpri]
     13 be/0 root        0.00 B/s    0.00 B/s ?unavailable?  [kworker/R-mm_percpu_wq]
     14 be/4 root        0.00 B/s    0.00 B/s ?unavailable?  [rcu_tasks_kthread]
     15 be/4 root        0.00 B/s    0.00 B/s ?unavailable?  [rcu_tasks_rude_kthread]
     16 be/4 root        0.00 B/s    0.00 B/s ?unavailable?  [rcu_tasks_trace_kthread]
     17 be/4 root        0.00 B/s    0.00 B/s ?unavailable?  [ksoftirqd/0]
     18 be/4 root        0.00 B/s    0.00 B/s ?unavailable?  [rcu_preempt]
     19 be/4 root        0.00 B/s    0.00 B/s ?unavailable?  [rcu_exp_par_gp_kthread_worker/0]
     20 be/4 root        0.00 B/s    0.00 B/s ?unavailable?  [rcu_exp_gp_kthread_worker]
     21 rt/4 root        0.00 B/s    0.00 B/s ?unavailable?  [migration/0]
     22 rt/4 root        0.00 B/s    0.00 B/s ?unavailable?  [idle_inject/0]
     23 be/4 root        0.00 B/s    0.00 B/s ?unavailable?  [cpuhp/0]
     24 be/4 root        0.00 B/s    0.00 B/s ?unavailable?  [cpuhp/1]
     25 rt/4 root        0.00 B/s    0.00 B/s ?unavailable?  [idle_inject/1]
     26 rt/4 root        0.00 B/s    0.00 B/s ?unavailable?  [migration/1]
     27 be/4 root        0.00 B/s    0.00 B/s ?unavailable?  [ksoftirqd/1]
     29 be/0 root        0.00 B/s    0.00 B/s ?unavailable?  [kworker/1:0H-events_highpri]
     30 be/4 root        0.00 B/s    0.00 B/s ?unavailable?  [kdevtmpfs]
```

Также дополнительно был открыт `htop`. Топ процессов предоставлен на скриншоте htop_proccess.

Выводы по выполненным командам:

- TOP-5 по CPU: PID=5742 (code, 2.0%, 03:28:09); PID=3926 (gnome-shell, 1.4%, 03:35:56); PID=6751 (code, 0.7%, 02:30:30); PID=5687 (code, 0.5%, 03:28:13); PID=5554 (code, 0.4%, 03:28:19).
- TOP-5 по памяти: PID=6751 (code, 14.1%, 552.8 MiB); PID=5742 (code, 10.6%, 417.6 MiB); PID=3926 (gnome-shell, 7.0%, 276.2 MiB); PID=6462 (code, 6.7%, 264.1 MiB); PID=5554 (code, 5.7%, 226.4 MiB).
- IO: Низкая активность (читание 0 B/s, запись 5.44 K/s от системных задач). Нет интенсивного IO.

Таким образом видно, что нагрузка низкая, доминирует VS Code (много процессов, высокая память из-за расширений) и GNOME (GUI). IO минимален.

## Ключевые выводы:

В ходе работы усвоена модель процессов Linux, создание через `fork/exec`, анализ через `/proc` и мониторинг ресурсов. Реализован код на Python. В VirtualBox поведение упрощённое, но стандартное.

## Ответы на вопросы:

1. Процесс — выполняющийся экземпляр программы с PID, контекстом, ресурсами; программа — статический файл.
2. Без wait() дочерний процесс станет зомби, засоряя систему, пока не усыновлён systemd.
3. Информация о процессах хранится в /proc/(pid)/(status — метаданные, cmdline — аргументы, fd — дескрипторы).
4. Exec заменяет образ процесса новой программой в том же самом PID, он нужен для запуска новых программ после fork.
5. Потому что /proc — виртуальная, она генерируется ядром на лету, а файлы — интерфейс к данным, не на диске.
6. %CPU — нагрузка CPU; %MEM — памяти; VIRT — виртуальная память; RES — реальная; SHR — общая; TIME+ — общее время CPU.
7. На системе с N ядрами общее 100 % × N. Если процесс использует два ядра полностью, его %CPU≈200 %.
8. %CPU — моментальный; load average — средняя очередь задач за 1/5/15 мин. Cpu(s): us — пользователь, sy — система, id — idle, wa — ожидание IO.
9. Тем, что IO — это ожидание диск/сеть (wa), а CPU — это вычисления. Увидеть их можно с помощью команд: pidstat -d (kB_rd/wr), iotop (активность), /proc/(pid)/io (байты).
10. nice (от –20 до +19) определяет «удобство» для планировщика CFS: если ниже nice (отрицательное), тогда выше приоритет, процесс получает больше квантов CPU, если наоборот выше nice, то процесс уступает ресурсы.
11. Потоки являются "легкими" процессами в одном адресном пространстве, они делят память и дескрипторы, но имеют собственный TID/стек/регистры. Смотреть их можно с помощью команд: ps -eLf, top -H, htop.
12. Зомби — завершившиеся процессы, чьи родители ещё не вызвали wait(). Существуют только как запись в таблице процессов. Сироты — процессы, чьи родители уже завершились. Их автоматически усыновляет init/systemd, который потом «прибирает» их при завершении.

## Как проверял:

Все команды и код выполнялись в терминале Ubuntu в VirtualBox. Для дерева использовал sleep в коде. Выводы проверены логами/скриншотами.

## Использование AI:

С помощью AI разобрался с теорией процессов, кодом на Python (с библиотеками fork/multiprocessing), а также некоторым анализом выводов (ps, top, /proc).

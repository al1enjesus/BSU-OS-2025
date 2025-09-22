# Цель: Понять модель процессов Linux, принципы порождения и ожидания завершения, а также научиться извлекать информацию из /proc

## Задание 1
Код(на Python):
```python
import os
import sys

print(f"parent(PID={os.getpid()})", flush = True)

children = []

for i in range (2):
        pid = os.fork()
        if pid == 0:
                print(f"child[{i}]:(PID={os.getpid()}),(PPID={os.getppid()})", flush = True)
                sys.exit(0)
        else:
                children.append(pid)

for pid in children:
        finished_pid, status = os.waitpid(pid, 0)
        exit_code = os.WEXITSTATUS(status)
        print(f"child (PID={os.getpid()}) finished with code: {exit_code}", flush = True)
```

### Вывод на консоль:
parent(PID=9896)
child[0]:(PID=9897),(PPID=9896)
child[1]:(PID=9898),(PPID=9896)
child (PID=9896) finished with code: 0
child (PID=9896) finished with code: 0


## Задание 2

### Команды чтоб найти строчки в которых есть PID программы code.py:
ps -ef --forest | grep -C 3 '10024' | cat

### Вывод на консоль:

ivan        3403    1363  0 сен21 ?     00:00:55  \_ /usr/libexec/gnome-terminal-server
ivan        3410    3403  0 сен21 pts/0 00:00:01  |   \_ bash
ivan       10108    3410 99 00:38 pts/0    00:00:00  |   |   \_ ps -ef --forest
ivan       10109    3410  0 00:38 pts/0    00:00:00  |   |   \_ grep --color=auto -C 3 10024
ivan       10110    3410  0 00:38 pts/0    00:00:00  |   |   \_ cat
ivan       10011    3403  0 00:32 pts/1    00:00:00  |   \_ bash
ivan       10024   10011  0 00:32 pts/1    00:00:00  |       \_ python3 src/code.py
ivan        4389    1363  0 сен21 ?     00:00:11  \_ /usr/bin/nautilus --gapplication-service
rtkit       1464       1  0 сен21 ?     00:00:00 /usr/libexec/rtkit-daemon
colord      1809       1  0 сен21 ?     00:00:00 /usr/libexec/colord

### Следующая команда:
pstree -p | grep -C 3 '10024' | cat

### Вывод на консоль:

           |               |-gnome-terminal-(3403)-+-bash(3410)-+-cat(10201)
           |               |                       |            |-grep(10200)
           |               |                       |            `-pstree(10199)
           |               |                       |-bash(10011)---python3(10024)
           |               |                       |-{gnome-terminal-}(3404)
           |               |                       |-{gnome-terminal-}(3405)
           |               |                       |-{gnome-terminal-}(3407)

# Задание 3
### cat /proc/<pid>/cmdline | tr '\0' ' '; echo
Показывает командную строку, с которой был запущен процесс.  
Символы `\0` заменяются на пробелы, чтобы аргументы выглядели как обычный ввод в терминале.  

### head -n 20 /proc/<pid>/status
Выводит первые 20 строк статуса процесса.  
Содержит имя, PID, PPid, состояние, UID/GID и базовую информацию об использовании памяти.  

### ls -l /proc/<pid>/fd
Показывает список открытых файловых дескрипторов процесса.  
Каждый дескриптор представлен как символьная ссылка на файл, сокет или устройство, с которым работает процесс.  

# Задание 4

## Пункт 1

### Команда:
top -b -n 1 | head -n 20 | cat

### Вывод на консоль:
top - 00:50:22 up  5:43,  2 users,  load average: 0,42, 0,67, 0,49
Задачи: 242 total,   1 running, 241 sleeping,   0 stopped,   0 zombie
%Cpu(s):  2,4 us,  2,4 sy,  0,0 ni, 95,2 id,  0,0 wa,  0,0 hi,  0,0 si,  0,0 st 
МиБ Mem :   8400,7 total,   3315,1 free,   2443,6 used,   2168,5 buff/cache     
МиБ Swap:   4096,0 total,   4096,0 free,      0,0 used.   5957,1 avail Mem 

    PID USER      PR  NI    VIRT    RES    SHR S  %CPU  %MEM     TIME+
  10300 ivan      20   0   10908   5612   3564 R  18,2   0,1   0:00.03
      1 root      20   0   25200  15568  10704 S   0,0   0,2   0:04.86
      2 root      20   0       0      0      0 S   0,0   0,0   0:00.14
      3 root      20   0       0      0      0 S   0,0   0,0   0:00.00
      4 root       0 -20       0      0      0 I   0,0   0,0   0:00.00
      5 root       0 -20       0      0      0 I   0,0   0,0   0:00.00
      6 root       0 -20       0      0      0 I   0,0   0,0   0:00.00
      7 root       0 -20       0      0      0 I   0,0   0,0   0:00.00
      8 root       0 -20       0      0      0 I   0,0   0,0   0:00.00
     11 root       0 -20       0      0      0 I   0,0   0,0   0:00.00
     12 root      20   0       0      0      0 I   0,0   0,0   0:00.00
     13 root       0 -20       0      0      0 I   0,0   0,0   0:00.00
     14 root      20   0       0      0      0 I   0,0   0,0   0:00.00

### Команда:
ps -eo pid,ppid,comm,state,%cpu,%mem,etime --sort=-%cpu | head -n 15 | cat

### Вывод на консоль:
PID    PPID COMMAND         S %CPU %MEM     ELAPSED
  10361    3410 ps              R  133  0.0       00:00
   2420    1676 firefox         S  7.9 10.5    05:44:39
   1676    1363 gnome-shell     R  7.9  5.3    05:44:47
   3330    2594 Isolated Web Co S  4.8  7.7    05:44:31
   3590    2594 Isolated Web Co S  1.3  4.5    05:41:29
   5399    2594 Isolated Web Co S  0.6  3.3    05:02:50
   3403    1363 gnome-terminal- S  0.3  0.7    05:44:26
   2628    2594 Privileged Cont S  0.2  2.1    05:44:35
   1867    1363 ibus-daemon     S  0.1  0.1    05:44:44
  10288       2 kworker/u19:2-k I  0.1  0.0       02:08
   5608    2594 Isolated Web Co S  0.1  2.2    05:00:39
   5668    2594 Isolated Web Co S  0.1  2.3    04:58:31
  10060       2 kworker/u18:3-e I  0.1  0.0       17:00
   8872       2 kworker/2:2-eve I  0.1  0.0    01:11:04

### Команда:
ps -eo pid,ppid,comm,state,%cpu,%mem,rss --sort=-%mem | head -n 15 | cat

### Вывод на консоль:
PID    PPID COMMAND         S %CPU %MEM   RSS
   2420    1676 firefox         S  7.9 10.5 909948
   3330    2594 Isolated Web Co S  4.7  7.5 651500
   1676    1363 gnome-shell     S  7.9  5.3 460492
   3590    2594 Isolated Web Co S  1.3  4.5 389532
   4389    1363 nautilus        S  0.0  3.8 330928
   5399    2594 Isolated Web Co S  0.6  3.3 285704
   5668    2594 Isolated Web Co S  0.1  2.3 203756
   5608    2594 Isolated Web Co S  0.1  2.2 190624
   2628    2594 Privileged Cont S  0.2  2.1 181284
   2836    2594 WebExtensions   S  0.0  1.2 105272
   6488    2594 Web Content     S  0.0  0.8 74204
   6174    2594 Web Content     S  0.0  0.8 74012
   7624    2594 Web Content     S  0.0  0.8 73120
   1923    1632 evolution-alarm S  0.0  0.7 65652

## Пункт 2

### Команда:
pidstat -u -r -d 1 5 | cat   # 5 интервалов по 1 сек

### Вывод на консоль:
Linux 6.14.0-29-generic (ivan-VirtualBox) 	22.09.2025 	_x86_64_	(4 CPU)

00:57:12      UID       PID    %usr %system  %guest   %wait    %CPU   CPU  Command
00:57:13     1000      1676    2,88    1,92    0,00    0,00    4,81     2  gnome-shell
00:57:13        0      9574    0,00    0,96    0,00    0,00    0,96     0  kworker/u17:1-events_unbound
00:57:13     1000     10428    0,00    0,96    0,00    0,00    0,96     1  pidstat

00:57:12      UID       PID  minflt/s  majflt/s     VSZ     RSS   %MEM  Command
00:57:13     1000      1676     75,96      3,85 5005780  460324   5,35  gnome-shell
00:57:13     1000     10428      3,85      0,00    7840    4440   0,05  pidstat

00:57:12      UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command

00:57:13      UID       PID    %usr %system  %guest   %wait    %CPU   CPU  Command
00:57:14      989       300    0,97    0,97    0,00    0,00    1,94     3  systemd-oomd
00:57:14     1000      1676    3,88    4,85    0,00    0,00    8,74     0  gnome-shell
00:57:14     1000      3403    0,97    0,00    0,00    0,00    0,97     0  gnome-terminal-
00:57:14     1000     10428    0,00    0,97    0,00    0,00    0,97     1  pidstat

00:57:13      UID       PID  minflt/s  majflt/s     VSZ     RSS   %MEM  Command
00:57:14     1000      1676     87,38     23,30 5005780  460324   5,35  gnome-shell
00:57:14     1000      3403      0,97      0,00  563336   62672   0,73  gnome-terminal-
00:57:14     1000     10428      0,97      0,00    7840    4440   0,05  pidstat

00:57:13      UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command

00:57:14      UID       PID    %usr %system  %guest   %wait    %CPU   CPU  Command
00:57:15        0        54    0,00    1,00    0,00    0,00    1,00     0  kcompactd0
00:57:15     1000      1676    6,00    5,00    0,00    0,00   11,00     2  gnome-shell
00:57:15        0      1853    1,00    0,00    0,00    0,00    1,00     2  upowerd
00:57:15     1000      2420    0,00    1,00    0,00    0,00    1,00     3  firefox
00:57:15     1000      3403    1,00    1,00    0,00    1,00    2,00     2  gnome-terminal-
00:57:15        0     10288    0,00    1,00    0,00    0,00    1,00     2  kworker/u19:2-events_freezable_pwr_efficient
00:57:15     1000     10428    1,00    1,00    0,00    0,00    2,00     1  pidstat

00:57:14      UID       PID  minflt/s  majflt/s     VSZ     RSS   %MEM  Command
00:57:15     1000      1676     73,00      7,00 5005780  460324   5,35  gnome-shell

00:57:14      UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command

00:57:15      UID       PID    %usr %system  %guest   %wait    %CPU   CPU  Command
00:57:16     1000      1676    1,98    0,99    0,00    0,99    2,97     0  gnome-shell
00:57:16     1000      2420    0,99    0,00    0,00    0,00    0,99     3  firefox
00:57:16        0     10415    0,00    0,99    0,00    0,00    0,99     2  kworker/2:1-events
00:57:16     1000     10428    0,00    1,98    0,00    0,00    1,98     1  pidstat

00:57:15      UID       PID  minflt/s  majflt/s     VSZ     RSS   %MEM  Command
00:57:16     1000      1676     68,32     13,86 5009940  460324   5,35  gnome-shell

00:57:15      UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command

00:57:16      UID       PID    %usr %system  %guest   %wait    %CPU   CPU  Command
00:57:17     1000      1676   15,00   22,00    0,00    2,00   37,00     0  gnome-shell
00:57:17     1000      2420    0,00    1,00    0,00    0,00    1,00     3  firefox
00:57:17     1000      3403    4,00    2,00    0,00    1,00    6,00     1  gnome-terminal-
00:57:17     1000      5608    0,00    1,00    0,00    0,00    1,00     3  Isolated Web Co
00:57:17        0     10288    0,00    1,00    0,00    1,00    1,00     2  kworker/u19:2-events_unbound
00:57:17     1000     10428    1,00    1,00    0,00    0,00    2,00     1  pidstat

00:57:16      UID       PID  minflt/s  majflt/s     VSZ     RSS   %MEM  Command
00:57:17     1000      1676    304,00     99,00 5009956  460324   5,35  gnome-shell
00:57:17     1000      4452      2,00      0,00   41832   30672   0,36  python3

00:57:16      UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command

Среднее:   UID       PID    %usr %system  %guest   %wait    %CPU   CPU  Command
Среднее:     0        54    0,00    0,20    0,00    0,20    0,20     -  kcompactd0
Среднее:   989       300    0,20    0,20    0,00    0,00    0,39     -  systemd-oomd
Среднее:  1000      1676    5,91    6,89    0,00    0,59   12,80     -  gnome-shell
Среднее:     0      1853    0,20    0,00    0,00    0,00    0,20     -  upowerd
Среднее:  1000      2420    0,20    0,39    0,00    0,00    0,59     -  firefox
Среднее:  1000      3403    1,18    0,59    0,00    0,59    1,77     -  gnome-terminal-
Среднее:  1000      5608    0,00    0,20    0,00    0,00    0,20     -  Isolated Web Co
Среднее:     0      9574    0,00    0,20    0,00    0,00    0,20     -  kworker/u17:1-events_unbound
Среднее:     0     10288    0,00    0,39    0,00    0,20    0,39     -  kworker/u19:2-events_unbound
Среднее:     0     10415    0,00    0,20    0,00    0,39    0,20     -  kworker/2:1-events
Среднее:  1000     10428    0,39    1,18    0,00    0,00    1,57     -  pidstat

Среднее:   UID       PID  minflt/s  majflt/s     VSZ     RSS   %MEM  Command
Среднее:  1000      1676    121,06     29,13 5007447  460324   5,35  gnome-shell
Среднее:  1000      3403      0,20      0,00  563336   62646   0,73  gnome-terminal-
Среднее:  1000      4452      0,39      0,00   41832   30672   0,36  python3
Среднее:  1000     10428      0,98      0,00    7840    4440   0,05  pidstat

Среднее:   UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command


## Пункт 5
## Топ 5 по CPU:
### Команда:
ps -eo pid,comm,pcpu,etime --sort=-pcpu | head -n 6

### Вывод на консоль:
PID COMMAND         %CPU     ELAPSED
  10524 ps               200       00:00
   1676 gnome-shell      8.0    05:58:01
   2420 firefox          7.9    05:57:53
   3330 Isolated Web Co  4.8    05:57:45
   3590 Isolated Web Co  1.3    05:54:42

## Топ 5 по памяти:
### Команда:
ps -eo pid,comm,pmem,rss --sort=-pmem | head -n 6

### Вывод на консоль:
PID COMMAND         %MEM   RSS
   2420 firefox         10.5 908524
   3330 Isolated Web Co  7.8 679260
   1676 gnome-shell      5.3 460904
   3590 Isolated Web Co  4.5 388780
   4389 nautilus         3.8 330928


### Короткие выводы
- **CPU:** основную нагрузку создают `gnome-shell` и `firefox`, что связано с графическим окружением и активным использованием браузера.  
- **Память:** больше всего RAM потребляют процессы браузера (`firefox` и его вкладки).  
- **Общая картина:** система нагружена в первую очередь пользовательскими приложениями (браузер, графическая оболочка), что характерно для настольной рабочей станции.


# Задание 5
Код на Python:
```python
import os

pid = os.getpid()
chain = []

while pid > 0:
    name, ppid = None, 0
    with open(f"/proc/{pid}/status") as f:
        for line in f:
            if line.startswith("Name:"):
                name = line.split()[1]
            elif line.startswith("PPid:"):
                ppid = int(line.split()[1])
    chain.append(f"{name}({pid})")
    pid = ppid

print(" ← ".join(chain))
```

### Вывод в консоль:
python3(10782) ← bash(3410) ← gnome-terminal-(3403) ← systemd(1363) ← systemd(1)

# Ответы на вопросы

### Чем процесс отличается от программы  
Программа — это пассивный набор инструкций (файл на диске).  
Процесс — это активное выполнение программы с выделенной памятью, PID и состоянием в ОС.  

### Что будет, если вызвать fork() без wait()  
Родитель создаст дочерний процесс, но если не вызвать `wait()`, завершившийся ребёнок станет «зомби».  
Такие процессы занимают запись в таблице процессов, пока родитель не соберёт их статус.  

### Как система хранит информацию о процессах  
Ядро хранит данные в таблице процессов (`task_struct` в Linux).  
Пользователь может видеть их через виртуальную файловую систему `/proc/<pid>/`.  

### Что делает exec() и зачем он нужен  
`exec()` заменяет образ текущего процесса новой программой, сохраняя PID.  
Это позволяет оболочке сначала форкнуть процесс, а затем загрузить в него нужную программу.  

### Почему в /proc нет «настоящих» файлов  
`/proc` — это виртуальная файловая система, данные в ней генерируются ядром «на лету».  
Она не хранится на диске, а отражает текущее состояние процессов и системы.  

### Как интерпретировать поля top: %CPU, %MEM, VIRT, RES, SHR, TIME+  
- **%CPU** — доля процессорного времени.  
- **%MEM** — доля оперативной памяти.  
- **VIRT** — виртуальная память, **RES** — реально занятая RAM, **SHR** — разделяемая память.  
- **TIME+** — суммарное процессорное время.  

### Почему сумма %CPU может быть больше 100%  
На многопроцессорных системах 100% соответствует одному ядру.  
Если процесс использует несколько ядер, сумма может превышать 100%.  

### Чем отличается мгновенное %CPU от load average. Что означает строка Cpu(s) в top (в т.ч. wa)  
`%CPU` в `top` показывает текущую загрузку процессора процессами.  
`load average` — это среднее количество процессов в очереди за 1, 5 и 15 минут.  
В строке `Cpu(s)` поле `wa` означает процент времени, когда CPU простаивает, ожидая завершения операций ввода-вывода.  

### Чем IO‑нагрузка отличается от CPU‑нагрузки и как её увидеть  
CPU‑нагрузка связана с вычислениями, IO‑нагрузка — с ожиданием диска или сети.  
Её можно смотреть через `pidstat -d`, `iotop` или `/proc/<pid>/io`.  

### Что такое nice/приоритеты процессов и как они влияют на планирование  
`nice` — это «вежливость» процесса: чем выше значение, тем ниже приоритет.  
Планировщик чаще выделяет CPU процессам с низким `nice` (высоким приоритетом).  

### Чем поток отличается от процесса и как увидеть потоки в ps/top  
Потоки разделяют память и ресурсы внутри одного процесса, но имеют собственные идентификаторы (TID).  
В `ps -eLf` или `top -H` можно увидеть отдельные потоки.  

### Что такое зомби и сироты, как они возникают и куда «деваются»  
Зомби — это завершившиеся процессы, чьи статусы не собрал родитель.  
Сироты — это процессы, у которых умер родитель; они переподчиняются `init/systemd`.  
Зомби исчезают, когда родитель вызывает `wait()`, а сироты продолжают работать под PID 1.  

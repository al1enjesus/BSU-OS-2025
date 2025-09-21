цель: Понять модель процессов Linux, принципы порождения и ожидания завершения, а также научиться извлекать информацию из /proc.
Ход работы  

1.Создание поцессов
Разработана программа на python, создающая два дочерних процесса.

```python 
import os
import sys
import time

def main():
    parent_pid = os.getpid()
    print(f"parent(PID={parent_pid})", flush=True)

    children = []
    labels = ['A', 'B']

    for i in range(2):
        pid = os.fork()
        if pid == 0:
            
            child_label = labels[i]
            my_pid = os.getpid()
            parent_pid = os.getppid()
            print(f"child_{child_label}: PID={my_pid}, PPID={parent_pid}", flush=True)

           
            time.sleep(15)

            print(f"child_{child_label}: exiting now", flush=True)
            sys.exit(i + 10)  

        else:
            
            children.append(pid)

    
    for child_pid in children:
        pid, status = os.waitpid(child_pid, 0)
        # Поддержка Python < 3.9
        if hasattr(os, 'waitstatus_to_exitcode'):
            exit_code = os.waitstatus_to_exitcode(status)
        else:
            exit_code = status >> 8  

        print(f"parent: child PID={pid} exited with code {exit_code}", flush=True)

    print("parent: all children finished", flush=True)

if __name__ == "__main__":
    main()
```
Результат вывода:

```bash
parent(PID=13122)
child_A: PID=13123, PPID=13122
child_B: PID=13124, PPID=13122
child_B: exiting now
child_A: exiting now
parent: child PID=13123 exited with code 10
parent: child PID=13124 exited with code 11
parent: all children finished
```

2. Исследование дерева процессов
во вермя выполнения програмы были выполнены команды:
ps -ef --forest | head -n 30 | cat
pstree -p | head -n 50 | cat

Результат
```bash
UID          PID    PPID  C STIME TTY          TIME CMD
root           2       0  0 18:05 ?        00:00:00 [kthreadd]
root           3       2  0 18:05 ?        00:00:00  \_ [pool_workqueue_release]
root           4       2  0 18:05 ?        00:00:00  \_ [kworker/R-rcu_gp]
root           5       2  0 18:05 ?        00:00:00  \_ [kworker/R-sync_wq]
root           6       2  0 18:05 ?        00:00:00  \_ [kworker/R-kvfree_rcu_reclaim]
root           7       2  0 18:05 ?        00:00:00  \_ [kworker/R-slub_flushwq]
root           8       2  0 18:05 ?        00:00:00  \_ [kworker/R-netns]
root          13       2  0 18:05 ?        00:00:00  \_ [kworker/R-mm_percpu_wq]
root          14       2  0 18:05 ?        00:00:00  \_ [rcu_tasks_kthread]
root          15       2  0 18:05 ?        00:00:00  \_ [rcu_tasks_rude_kthread]
root          16       2  0 18:05 ?        00:00:00  \_ [rcu_tasks_trace_kthread]
root          17       2  0 18:05 ?        00:00:00  \_ [ksoftirqd/0]
root          18       2  0 18:05 ?        00:00:33  \_ [rcu_preempt]
root          19       2  0 18:05 ?        00:00:00  \_ [rcu_exp_par_gp_kthread_worker/0]
root          20       2  0 18:05 ?        00:00:00  \_ [rcu_exp_gp_kthread_worker]
root          21       2  0 18:05 ?        00:00:00  \_ [migration/0]
root          22       2  0 18:05 ?        00:00:00  \_ [idle_inject/0]
root          23       2  0 18:05 ?        00:00:00  \_ [cpuhp/0]
root          24       2  0 18:05 ?        00:00:00  \_ [cpuhp/1]
root          25       2  0 18:05 ?        00:00:00  \_ [idle_inject/1]
root          26       2  0 18:05 ?        00:00:00  \_ [migration/1]
root          27       2  0 18:05 ?        00:00:28  \_ [ksoftirqd/1]
root          30       2  0 18:05 ?        00:00:00  \_ [cpuhp/2]
root          31       2  0 18:05 ?        00:00:00  \_ [idle_inject/2]
root          32       2  0 18:05 ?        00:00:00  \_ [migration/2]
root          23  0 18:05 ?        00:00:00  \_ [ksoftirqd/2]
root          35       2  0 18:05 ?        00:00:00  \_ [kworker/2:0H-kblockd]
root          36       2  0 18:05 ?        00:00:00  \_ [cpuhp/3]
root          37       2  0 18:05 ?        00:00:00  \_ [idle_inject/3]
```
Однако они не вывели ничего, связанного с нашими пользовательскими процессами. Вероятно, это происходит потому, что системные процессы имеют приоритет в отображении: утилиты `ps` и `pstree` по умолчанию сортируют или выводят процессы так, что в начале списка оказываются системные — с PID 1 (например, `systemd` или `init`) и их прямые дочерние процессы, чаще всего запущенные от имени root. Процессы обычного пользователя (в моём случае — sancher) появляются гораздо ниже в списке. Поскольку команда `head -n 30` обрезает вывод после первых 30 строк, мои процессы, находящиеся, скажем, на 35-й или 200-й позиции, просто не попадают в итоговый результат.

Поэтому используем альтернативные команды для наших процессов
```bash
root        1067       1  0 18:05 ?        00:00:00 /usr/bin/python3 /usr/share/unattended-upgrades/unattended-upgrade-shutdown --wait-for-signal
vboxuser   13961    3936  0 20:44 pts/1    00:00:00  |   |   \_ python3 fork_example.py
vboxuser   13962   13961  0 20:44 pts/1    00:00:00  |   |       \_ python3 fork_example.py
vboxuser   13963   13961  0 20:44 pts/1    00:00:00  |   |       \_ python3 fork_example.py
vboxuser   13969   13441  0 20:45 pts/0    00:00:00  |       \_ grep --color=auto -E (fork_example.py|python3)
```

`pstree -p 13961`

 Результат
```bash
python3(13961)─┬─python3(13962)
               └─python3(13963)
```

Вывод: Наглядно демонстрируется иерархическая (древовидная) структура процессов. Родительский процесс с PID 13961 породил двух дочерних — с PID 13962 и 13963. В выводе команды `ps --forest` они отображаются с отступами, подчёркивая подчинение, а в `pstree` — в виде наглядного дерева процессов, что делает связь «родитель—потомок» особенно очевидной.

3. Изучение файловой структуры /proc

команда: `echo $$`

13441

Вывод: PID это уникальный идентификатор текущего bash-сеанса 13441.

Команда: `cat /proc/13441/cmdline | tr '\0' ' '; echo`

Результат
` `

Вывод: Мой bash был запущен без дополнительных аргументов.

Команда:
head -n 20 /proc/13441/status

Результат:
```bash
Name:	bash
Umask:	0002
State:	S (sleeping)
Tgid:	13441
Ngid:	0
Pid:	13441
PPid:	3774
TracerPid:	0
Uid:	1000	1000	1000	1000
Gid:	1000	1000	1000	1000
FDSize:	256
Groups:	27 1000 
NStgid:	13441
NSpid:	13441
NSpgid:	13441
NSsid:	13441
Kthread:	0
VmPeak:	   19932 kB
VmSize:	   19932 kB
VmLck:	       0 kB
```

Вывод: с помощью этой команды я проанализировал поля статуса процесса.

Команда:

`ls -l /proc/13441/fd`

Результат:

```bash
total 0
lrwx------ 1 vboxuser vboxuser 64 Sep 21 21:07 0 -> /dev/pts/0
lrwx------ 1 vboxuser vboxuser 64 Sep 21 21:07 1 -> /dev/pts/0
lrwx------ 1 vboxuser vboxuser 64 Sep 21 21:07 2 -> /dev/pts/0
lrwx------ 1 vboxuser vboxuser 64 Sep 21 21:07 255 -> /dev/pts/0
```

Вывод:**Вывод:** Содержимое директории `fd`  представляет собой список символических ссылок, каждая из которых указывает на реальный ресурс, открытый процессом. В частности, видно, что весь ввод и вывод моего bash-процесса осуществляется через устройство `/dev/pts/1` — это псевдотерминал, используемый для взаимодействия с терминалом. Такое поведение подтверждает, что я нахожусь в активной терминальной сессии, и стандартные потоки (stdin, stdout, stderr) связаны именно с этим устройством.

4. Анализ нагрузки системы
Топ процессов по CPU

Команда:

`ps -eo pid,ppid,comm,state,%cpu,%mem,etime --sort=-%cpu | head -n 15 | cat`

Результат

```bash
   PID    PPID COMMAND         S %CPU %MEM     ELAPSED
  15253   13441 ps              R  200  0.1       00:00
   1952    1706 gnome-shell     S 22.7 10.5    03:06:26
   3393    1952 firefox         S 22.0 17.9    03:00:16
   4301    3685 Isolated Web Co S 15.3 11.7    02:59:59
   3731    3685 RDD Process     S  2.0  2.4    03:00:05
   4479    3685 Isolated Web Co S  1.2  5.0    02:59:58
   3774    1706 gnome-terminal- S  0.7  1.6    03:00:04
   4290    3685 Utility Process S  0.5  1.4    02:59:59
  12307       2 kworker/u16:2-e I  0.4  0.0       55:34
  14160       2 kworker/u16:0-e I  0.4  0.0       23:38
   2079    1706 ibus-daemon     S  0.3  0.3    03:06:24
   6426    3685 Isolated Web Co S  0.3  4.2    02:34:03
  13687       2 kworker/u16:3-e I  0.3  0.0       31:38
     18       2 rcu_preempt     I  0.3  0.0    03:07:00
```
Топ процессов по памяти

Команда:
`ps -eo pid,ppid,comm,state,%cpu,%mem,rss --sort=-%mem | head -n 15 | cat`

Результат:
```bash
PID    PPID COMMAND         S %CPU %MEM   RSS
   3393    1952 firefox         S 21.9 18.0 721924
   4301    3685 Isolated Web Co S 15.2 11.7 471792
   1952    1706 gnome-shell     S 22.7 10.5 421600
  13717    1706 nautilus        S  0.1  5.7 231372
   4479    3685 Isolated Web Co S  1.2  5.0 203448
   6426    3685 Isolated Web Co S  0.3  4.2 170124
   3723    3685 Privileged Cont S  0.2  3.6 144680
  12107    3685 Isolated Web Co S  0.1  3.0 123984
   3731    3685 RDD Process     S  2.0  2.4 98456
   3968    3685 WebExtensions   S  0.0  2.2 90776
   4298    3685 Isolated Servic S  0.2  2.1 86396
  12172    3685 Web Content     S  0.1  1.6 67408
  12225    3685 Web Content     S  0.1  1.6 67028
  12217    3685 Web Content     S  0.1  1.6 66956
```

Команда:
pidstat -u -r -d 1 5 | cat  

Результат:
```bash
Linux 6.14.0-27-generic (LinuxUbuntu) 	09/21/2025 	_x86_64_	(4 CPU)

09:17:00 PM   UID       PID    %usr %system  %guest   %wait    %CPU   CPU  Command
09:17:01 PM     0        18    0.00    0.97    0.00    0.00    0.97     0  rcu_preempt
09:17:01 PM   102       797    0.97    0.00    0.00    0.00    0.97     0  rsyslogd
09:17:01 PM  1000      1721    0.00    0.97    0.00    0.00    0.97     0  pipewire
09:17:01 PM  1000      1952    0.97    1.94    0.00    0.00    2.91     0  gnome-shell
09:17:01 PM  1000      3393    0.00    1.94    0.00    0.00    1.94     2  firefox
09:17:01 PM  1000      3774    0.00    0.97    0.00    0.00    0.97     2  gnome-terminal-
09:17:01 PM  1000      4301    3.88    1.94    0.00    0.00    5.83     2  Isolated Web Co
09:17:01 PM  1000      4479    0.97    0.00    0.00    0.00    0.97     3  Isolated Web Co
09:17:01 PM  1000     12107    0.97    0.00    0.00    0.00    0.97     1  Isolated Web Co
09:17:01 PM  1000     15449    0.00    1.94    0.00    0.00    1.94     0  pidstat

09:17:00 PM   UID       PID  minflt/s  majflt/s     VSZ     RSS   %MEM  Command
09:17:01 PM     0       265     33.01      0.00   67232   20664   0.52  systemd-journal
09:17:01 PM     0       765      7.77      0.00   18092    2660   0.07  cron
09:17:01 PM  1000      1952    140.78     26.21 5101712  421240  10.51  gnome-shell
09:17:01 PM  1000      4301    152.43      0.00 45099420  471112  11.75  Isolated Web Co
09:17:01 PM  1000     15449      2.91      0.00   18836    4240   0.11  pidstat

09:17:00 PM   UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command
09:17:01 PM  1000      3393      0.00     31.07      0.00       0  firefox

09:17:01 PM   UID       PID    %usr %system  %guest   %wait    %CPU   CPU  Command
09:17:03 PM  1000      1721    0.00    0.78    0.00    0.00    0.78     0  pipewire
09:17:03 PM  1000      1952    1.55    1.55    0.00    0.00    3.10     2  gnome-shell
09:17:03 PM  1000      3393    0.78    0.00    0.00    0.00    0.78     3  firefox
09:17:03 PM  1000      4301    6.20    2.33    0.00    0.00    8.53     0  Isolated Web Co
09:17:03 PM  1000     12172    0.00    0.78    0.00    0.00    0.78     3  Web Content
09:17:03 PM  1000     12225    0.00    0.78    0.00    0.00    0.78     1  Web Content
09:17:03 PM  1000     15449    0.78    0.78    0.00    0.00    1.55     0  pidstat

09:17:01 PM   UID       PID  minflt/s  majflt/s     VSZ     RSS   %MEM  Command
09:17:03 PM  1000      1721      0.00      0.00  125116   14616   0.36  pipewire
09:17:03 PM  1000      1952    117.05     31.78 5101712  421240  10.51  gnome-shell
09:17:03 PM  1000      3774     15.50      0.00  579632   65628   1.64  gnome-terminal-
09:17:03 PM  1000      4301     42.64      0.00 45099388  470080  11.72  Isolated Web Co

09:17:01 PM   UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command

09:17:03 PM   UID       PID    %usr %system  %guest   %wait    %CPU   CPU  Command
09:17:04 PM  1000      1952    2.00    3.00    0.00    0.00    5.00     3  gnome-shell
09:17:04 PM  1000      3393    1.00    2.00    0.00    0.00    3.00     3  firefox
09:17:04 PM  1000      4301    7.00    4.00    0.00    1.00   11.00     1  Isolated Web Co
09:17:04 PM     0     13687    0.00    1.00    0.00    0.00    1.00     0  kworker/u16:3-flush-8:0
09:17:04 PM  1000     15449    0.00    1.00    0.00    0.00    1.00     0  pidstat

09:17:03 PM   UID       PID  minflt/s  majflt/s     VSZ     RSS   %MEM  Command
09:17:04 PM  1000      1952    147.00     47.00 5101712  421240  10.51  gnome-shell
09:17:04 PM  1000      3774      5.00      0.00  579632   65628   1.64  gnome-terminal-
09:17:04 PM  1000      4301    199.00      0.00 45099404  470152  11.72  Isolated Web Co
09:17:04 PM  1000      4479      0.00      0.00 2639212  200260   4.99  Isolated Web Co

09:17:03 PM   UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command
09:17:04 PM  1000      3393      0.00     60.00     96.00       0  firefox

09:17:04 PM   UID       PID    %usr %system  %guest   %wait    %CPU   CPU  Command
09:17:05 PM  1000      1952    0.99    1.98    0.00    0.00    2.97     1  gnome-shell
09:17:05 PM  1000      3393    0.00    0.99    0.00    0.00    0.99     3  firefox
09:17:05 PM  1000      4301    0.99    0.00    0.00    0.00    0.99     1  Isolated Web Co
09:17:05 PM  1000     15449    0.00    1.98    0.00    0.00    1.98     0  pidstat

09:17:04 PM   UID       PID  minflt/s  majflt/s     VSZ     RSS   %MEM  Command
09:17:05 PM  1000      1952     75.25     22.77 5101712  421240  10.51  gnome-shell
09:17:05 PM  1000      4298      2.97      0.00 2510308   86236   2.15  Isolated Servic
09:17:05 PM  1000      4301      0.00      0.00 45099388  470152  11.72  Isolated Web Co

09:17:04 PM   UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command

09:17:05 PM   UID       PID    %usr %system  %guest   %wait    %CPU   CPU  Command
09:17:06 PM  1000      1952    0.79    0.79    0.00    0.00    1.57     0  gnome-shell
09:17:06 PM  1000      3774    0.00    0.79    0.00    0.00    0.79     3  gnome-terminal-
09:17:06 PM  1000      4298    0.00    0.79    0.00    0.00    0.79     2  Isolated Servic
09:17:06 PM  1000      4301    0.00    0.79    0.00    0.00    0.79     1  Isolated Web Co
09:17:06 PM  1000     15449    0.00    0.79    0.00    0.00    0.79     1  pidstat

09:17:05 PM   UID       PID  minflt/s  majflt/s     VSZ     RSS   %MEM  Command
09:17:06 PM  1000      1952     50.39     26.77 5101712  421240  10.51  gnome-shell
09:17:06 PM  1000      3774      1.57      0.00  579632   65628   1.64  gnome-terminal-
09:17:06 PM  1000      4298      6.30      0.00 2510308   86364   2.15  Isolated Servic

09:17:05 PM   UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command

Average:      UID       PID    %usr %system  %guest   %wait    %CPU   CPU  Command
Average:        0        18    0.00    0.18    0.00    0.00    0.18     -  rcu_preempt
Average:      102       797    0.18    0.00    0.00    0.00    0.18     -  rsyslogd
Average:     1000      1721    0.00    0.36    0.00    0.00    0.36     -  pipewire
Average:     1000      1952    1.25    1.79    0.00    0.00    3.04     -  gnome-shell
Average:     1000      3393    0.36    0.89    0.00    0.00    1.25     -  firefox
Average:     1000      3774    0.00    0.36    0.00    0.00    0.36     -  gnome-terminal-
Average:     1000      4298    0.00    0.18    0.00    0.00    0.18     -  Isolated Servic
Average:     1000      4301    3.57    1.79    0.00    0.18    5.36     -  Isolated Web Co
Average:     1000      4479    0.18    0.00    0.00    0.00    0.18     -  Isolated Web Co
Average:     1000     12107    0.18    0.00    0.00    0.00    0.18     -  Isolated Web Co
Average:     1000     12172    0.00    0.18    0.00    0.00    0.18     -  Web Content
Average:     1000     12225    0.00    0.18    0.00    0.00    0.18     -  Web Content
Average:        0     13687    0.00    0.18    0.00    0.00    0.18     -  kworker/u16:3-kvfree_rcu_reclaim
Average:     1000     15449    0.18    1.25    0.00    0.00    1.43     -  pidstat

Average:      UID       PID  minflt/s  majflt/s     VSZ     RSS   %MEM  Command
Average:        0       265      6.07      0.00   67232   20664   0.52  systemd-journal
Average:        0       765      1.43      0.00   18092    2660   0.07  cron
Average:     1000      1721      0.00      0.00  125130   14616   0.36  pipewire
Average:     1000      1952    104.11     30.71 5101712  421240  10.51  gnome-shell
Average:     1000      3774      4.82      0.00  579632   65628   1.64  gnome-terminal-
Average:     1000      4298      1.96      0.00 2510308   86262   2.15  Isolated Servic
Average:     1000      4301     73.39      0.00 45099398  470330  11.73  Isolated Web Co
Average:     1000      4479      0.00      0.00 2639218  200260   4.99  Isolated Web Co
Average:     1000     15449      0.54      0.00   18836    4240   0.11  pidstat

Average:      UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command
Average:     1000      3393      0.00     16.43     17.14       0  firefox
```
Вывод: С помощью собранных данных понятно, что основные процессы которые нагружают систему это gnome-shell и firefox.


5. Мина-утилита ptree
Цель: Написать программу которая выводит цепочку родителей от текущего процесса до PID 1.
```bash(2345) ← gnome-terminal-server(1900) ← systemd(1)```

```python
import os

def read_proc_field(pid, field):
    """Читает поле из /proc/<pid>/status"""
    try:
        with open(f'/proc/{pid}/status') as f:
            for line in f:
                if line.startswith(field + ':'):
                    return line.split(':', 1)[1].strip()
    except:
        return None

def get_process_info(pid):
    """Возвращает имя и PPid процесса"""
    name = read_proc_field(pid, 'Name') or f"process_{pid}"
    ppid_str = read_proc_field(pid, 'PPid')
    ppid = int(ppid_str) if ppid_str and ppid_str.isdigit() else None
    return name, ppid

def main():
    pid = os.getpid()
    chain = []
    
    
    while True:
        name, ppid = get_process_info(pid)
        chain.append(f"{name}({pid})")
        if pid == 1:
            break
        if ppid is None:
            break
        pid = ppid
    
    print(" ← ".join(reversed(chain)))

if __name__ == "__main__":
    main()
```

Вывод выглядит(python3 ptree.py):
```bash 
systemd(1) ← systemd(1706) ← gnome-terminal-(3774) ← bash(13441) ← python3(16753)
```
Ответы на вопросы:
1. Чем процесс отличается от программы?
Программа — статичный исполняемый файл на диске. Процесс — запущенный экземпляр программы в памяти, со своим PID, состоянием, стеком, кучей, правами и ресурсами. 
 

2. Что будет, если вызвать fork() без wait()?
Дочерний процесс после завершения станет зомби — будет висеть в таблице процессов, занимая PID, пока родитель не вызовет wait() или не завершится сам. 
 

3. Как система хранит информацию о процессах?
Ядро хранит данные в структуре task_struct в памяти. Пользователь может получить доступ через виртуальную ФС /proc/<pid>/, где каждый процесс представлен директорией с файлами-атрибутами. 
 

4. Что делает exec() и зачем он нужен?
exec() заменяет текущий образ процесса на новый (загружает другую программу). Нужен, чтобы после fork() запустить не копию родителя, а другую программу (например, /bin/ls). 
 

5. Почему в /proc нет «настоящих» файлов?
/proc — виртуальная файловая система ядра. Файлы не хранятся на диске — их содержимое генерируется динамически при чтении, отражая текущее состояние системы и процессов.

6. Как интерпретировать поля top: %CPU, %MEM, VIRT, RES, SHR, TIME+?
%CPU — загрузка CPU (на ядро), %MEM — % от RAM, VIRT — вся виртуальная память, RES — физическая память в RAM, SHR — разделяемая часть RES, TIME+ — суммарное CPU-время процесса. 

7. Почему сумма %CPU может быть больше 100%?
Потому что %CPU считается на ядро: на 4-ядерной системе максимум 400%, если процессы используют несколько ядер или потоков. 

8. Чем отличается мгновенное %CPU от load average?
Что означает строка Cpu(s) в top (в т.ч. wa)? — %CPU — текущая загрузка CPU, load average — среднее число процессов в состоянии runnable/uninterruptible за 1/5/15 мин; wa — % времени, когда CPU простаивал, ожидая I/O. 

9. Чем IO‑нагрузка отличается от CPU‑нагрузки и как её увидеть (pidstat -d, iotop, /proc/<pid>/io)?
CPU-нагрузка — процессы вычисляют, IO-нагрузка — ждут диска/сети; смотреть через pidstat -d, iotop или /proc/<pid>/io для детальной статистики по процессу. 

10. Что такое nice/приоритеты процессов и как они влияют на планирование?
nice от -20 (высший приоритет) до 19 (низший) — влияет на долю CPU-времени: чем ниже nice, тем больше процесс получает CPU при прочих равных. 

11. Чем поток отличается от процесса и как увидеть потоки в ps/top?
 Потоки — легковесные, делят память процесса; в ps — флаг -L или -T, в top — нажать H для переключения отображения потоков. 

12. Что такое зомби и сироты, как они возникают и куда «деваются»?
Зомби — завершённый процесс, не прочитанный через wait(); сирота — процесс, чей родитель умер; зомби исчезает после wait() или когда сироту «усыновляет» init, который вызывает wait() и чистит его.

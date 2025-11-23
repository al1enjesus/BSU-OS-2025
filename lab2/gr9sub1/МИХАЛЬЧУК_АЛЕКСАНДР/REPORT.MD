# Лабораторная работа 2: Процессы и файловая система /proc

**Студент:** Михальчук Александр  
**Группа:** 9  
**Подгруппа:** 1  

## Цель работы

Практическое изучение модели процессов в Linux, механизмов создания и управления процессами, а также исследование виртуальной файловой системы /proc для получения информации о процессах.

## Ход работы

### 1. Создание процессов

Разработана программа на Python, создающая два дочерних процесса с увеличенным временем выполнения для удобства наблюдения.
```python
import os
import time
import sys

def child_process(name, sleep_time, exit_code):
    print(f"{name}: PID={os.getpid()}, PPID={os.getppid()}", flush=True)
    print(f"{name}: работает {sleep_time} секунд...", flush=True)
    
    for i in range(sleep_time):
        print(f"{name}: шаг {i+1}/{sleep_time}", flush=True)
        time.sleep(1)
    
    print(f"{name}: завершается с кодом {exit_code}", flush=True)
    sys.exit(exit_code)

def main():
    print(f"parent: PID={os.getpid()}", flush=True)
    
    pid1 = os.fork()
    if pid1 == 0:
        child_process("child_A", 15, 10)
    
    pid2 = os.fork()
    if pid2 == 0:
        child_process("child_B", 10, 20)
    
    print(f"parent: создал процессы {pid1} и {pid2}", flush=True)
    
    for pid in [pid1, pid2]:
        waited_pid, exit_status = os.waitpid(pid, 0)
        if os.WIFEXITED(exit_status):
            code = os.WEXITSTATUS(exit_status)
            print(f"parent: процесс {waited_pid} завершен с кодом {code}", flush=True)

    print("parent: все процессы завершены", flush=True)

if __name__ == "__main__":
    main()
```

**Результат вывода:**

```bash
python3 fork.py
parent: PID=5653
child_A: PID=5654, PPID=5653
child_A: работает 15 секунд...
child_A: шаг 1/15
parent: создал процессы 5654 и 5655
child_B: PID=5655, PPID=5653
child_B: работает 10 секунд...
child_B: шаг 1/10
child_B: шаг 2/10
child_A: шаг 2/15
child_B: шаг 3/10
...
child_B: завершается с кодом 20
child_A: шаг 12/15
child_A: шаг 13/15
child_A: шаг 14/15
child_A: шаг 15/15
child_A: завершается с кодом 10
parent: процесс 5654 завершен с кодом 10
parent: процесс 5655 завершен с кодом 20
parent: все процессы завершены
```
### 2. Исследование дерева процессов

Во время выполнения программы выполнены команды:
`ps -ef --forest | head -n 30 | cat`
`pstree -p | head -n 50 | cat`

**Результат:**
```bash
sancher@Ubuntu:~$ ps -ef --forest | head -n 30 | cat
UID          PID    PPID  C STIME TTY          TIME CMD
root           2       0  0 09:01 ?        00:00:00 [kthreadd]
root           3       2  0 09:01 ?        00:00:00  \_ [pool_workqueue_release]
root           4       2  0 09:01 ?        00:00:00  \_ [kworker/R-rcu_gp]
root           5       2  0 09:01 ?        00:00:00  \_ [kworker/R-sync_wq]
root           6       2  0 09:01 ?        00:00:00  \_ [kworker/R-kvfree_rcu_reclaim]
root           7       2  0 09:01 ?        00:00:00  \_ [kworker/R-slub_flushwq]
root           8       2  0 09:01 ?        00:00:00  \_ [kworker/R-netns]
root           9       2  0 09:01 ?        00:00:03  \_ [kworker/0:0-events]
root          11       2  0 09:01 ?        00:00:00  \_ [kworker/0:0H-events_highpri]
root          12       2  0 09:01 ?        00:00:00  \_ [kworker/u16:0-ipv6_addrconf]
root          13       2  0 09:01 ?        00:00:00  \_ [kworker/R-mm_percpu_wq]
root          14       2  0 09:01 ?        00:00:00  \_ [rcu_tasks_kthread]
root          15       2  0 09:01 ?        00:00:00  \_ [rcu_tasks_rude_kthread]
root          16       2  0 09:01 ?        00:00:00  \_ [rcu_tasks_trace_kthread]
root          17       2  0 09:01 ?        00:00:02  \_ [ksoftirqd/0]
root          18       2  0 09:01 ?        00:00:04  \_ [rcu_preempt]
root          19       2  0 09:01 ?        00:00:00  \_ [rcu_exp_par_gp_kthread_worker/0]
root          20       2  0 09:01 ?        00:00:00  \_ [rcu_exp_gp_kthread_worker]
root          21       2  0 09:01 ?        00:00:00  \_ [migration/0]
root          22       2  0 09:01 ?        00:00:00  \_ [idle_inject/0]
root          23       2  0 09:01 ?        00:00:00  \_ [cpuhp/0]
root          24       2  0 09:01 ?        00:00:00  \_ [cpuhp/1]
root          25       2  0 09:01 ?        00:00:00  \_ [idle_inject/1]
root          26       2  0 09:01 ?        00:00:00  \_ [migration/1]
root          27       2  0 09:01 ?        00:00:00  \_ [ksoftirqd/1]
root          29       2  0 09:01 ?        00:00:00  \_ [kworker/1:0H-events_highpri]
root          30       2  0 09:01 ?        00:00:00  \_ [cpuhp/2]
root          31       2  0 09:01 ?        00:00:00  \_ [idle_inject/2]
root          32       2  0 09:01 ?        00:00:00  \_ [migration/2]
```
Но они не показали ничего, что связано с нашими процессами. Это скорее всего связано с тем, что приоритет вывода системных процессов: Утилиты ps и pstree по умолчанию сортируют или отображают процессы в таком порядке, что в начале списка оказываются системные процессы (с PID 1, systemd или init и их непосредственные дети, часто запущенные от пользователя root). Процессы обычного пользователя (в моем случае, sancher) показываются значительно ниже. Команда head -n 30 обрезает весь вывод после 30-й строки, и если мои процессы находятся на 35-й или 200-й позиции, они просто не попадают в усеченный вывод.
**Поэтому используем альтернативные команды для поиска наших процессов:**
`ps -ef --forest | grep -E "(fork.py|python3)" | head -n 10`

**Результат:**
```bash
sancher@Ubuntu:~$ ps -ef --forest | grep -E "(fork.py|python3)" | head -n 10
root        1154       1  0 09:02 ?        00:00:00 /usr/bin/python3 /usr/share/unattended-upgrades/unattended-upgrade-shutdown --wait-for-signal
sancher     5979    4793  0 10:19 pts/0    00:00:00  |   |   \_ python3 fork.py
sancher     5980    5979  0 10:19 pts/0    00:00:00  |   |       \_ python3 fork.py
sancher     5981    5979  0 10:19 pts/0    00:00:00  |   |       \_ python3 fork.py
sancher     5983    5527  0 10:19 pts/1    00:00:00  |       \_ grep --color=auto -E (fork.py|python3)
```

`pstree -p 5979`

**Результат:**
```bash
sancher@Ubuntu:~$ pstree -p 5979
python3(5979)─┬─python3(5980)
              └─python3(5981)
```
Вывод: Наглядно видна древовидная структура процессов. Родительский процесс (5979) создал двух дочерних процессов (5980 и 5981), которые отображаются с отступами в выводе ps --forest и в виде дерева в pstree.
### 3. Изучение файловой системы /proc
Команда: `echo $$`

**Результат:**
```bash
sancher@Ubuntu:~$ echo $$
5527
```
##### Вывод: PID моего текущего bash-сеанса — 5527. Это уникальный идентификатор, который система присвоила моему терминалу.
Команда: `cat /proc/5527/cmdline | tr '\0' ' '; echo`

**Результат:**
```bash
sancher@Ubuntu:~$ cat /proc/5527/cmdline | tr '\0' ' '; echo
bash
```
###### Вывод:
Мой текущий процесс bash был запущен просто командой bash, без дополнительных аргументов или скриптов.
Команда:

`head -n 20 /proc/5527/status`

**Результат:**
```bash
sancher@Ubuntu:~$ head -n 20 /proc/5527/status
Name:	bash - имя исполняемого файла
Umask:	0002
State:	S (sleeping) - означает что процесс спит
Tgid:	5527
Ngid:	0
Pid:	5527 - ID процесса
PPid:	4785 - ID процесса-родителя
TracerPid:	0
Uid:	1000	1000	1000	1000
Gid:	1000	1000	1000	1000
FDSize:	256
Groups:	27 1000 
NStgid:	5527
NSpid:	5527
NSpgid:	5527
NSsid:	5527
Kthread:	0
VmPeak:	   19928 kB - Пиковый размер виртуальной памяти процесса 
VmSize:	   19928 kB - Теуйщий размер виртуальной памяти процесса
VmLck:	       0 kB
```
##### Вывод: 
с помощью данной команды смогли проанализировать ключевые поля статуса процесса.

Команда: 

`ls -l /proc/5527/fd`

**Результат:**
```bash
sancher@Ubuntu:~$ ls -l /proc/5527/fd
total 0
lrwx------ 1 sancher sancher 64 Sep 21 10:38 0 -> /dev/pts/1
lrwx------ 1 sancher sancher 64 Sep 21 10:38 1 -> /dev/pts/1
lrwx------ 1 sancher sancher 64 Sep 21 10:38 2 -> /dev/pts/1
lrwx------ 1 sancher sancher 64 Sep 21 10:38 255 -> /dev/pts/1
Вывод: Показывает список файлов в директории fd (file descriptors — файловые дескрипторы). Каждый файл здесь — это симлинк, ведущий на реальный ресурс, открытый процессом. Весь ввод и вывод моего bash-процесса идёт через виртуальное устройство /dev/pts/1. Это подтверждает, что я работаю в терминальной сессии.
```
### 4. Анализ нагрузки системы
##### Топ процессов по CPU
Команда: 

`ps -eo pid,ppid,comm,state,%cpu,%mem,etime --sort=-%cpu | head -n 15`

**Результат:**
```bash
sancher@Ubuntu:~$ ps -eo pid,ppid,comm,state,%cpu,%mem,etime --sort=-%cpu | head -n 15 | cat
    PID    PPID COMMAND         S %CPU %MEM     ELAPSED
   2052    1800 gnome-shell     S 11.1 11.7    02:11:35
   3103    2052 firefox         S  7.2 15.5    02:10:46
   4020    3282 Isolated Web Co S  6.5  8.1    02:10:41
   5715    1800 gnome-text-edit S  3.3  7.1    01:10:29
   3838    3282 Isolated Web Co S  0.8  5.3    02:10:41
   4785    1800 gnome-terminal- S  0.5  1.7    02:02:15
   4303    1800 nautilus        S  0.2  8.9    02:08:20
   6665    2052 gjs             S  0.2  1.5       05:03
   2171    1800 ibus-daemon     S  0.2  0.3    02:11:34
   6399       2 kworker/u20:2-e I  0.1  0.0       27:22
   3317    3282 Privileged Cont S  0.1  3.8    02:10:44
   6503       2 kworker/u20:1-e I  0.1  0.0       18:41
   5151       2 kworker/u19:3-e I  0.1  0.0    01:44:18
     18       2 rcu_preempt     I  0.0  0.0    02:11:58
```
##### Топ процессов по памяти
Команда: 

`ps -eo pid,ppid,comm,state,%cpu,%mem,rss --sort=-%mem | head -n 15 | cat`

**Результат:**
```bash
sancher@Ubuntu:~$ ps -eo pid,ppid,comm,state,%cpu,%mem,rss --sort=-%mem | head -n 15 | cat

    PID    PPID COMMAND         S %CPU %MEM   RSS
   3103    2052 firefox         S  7.2 15.5 621800
   2052    1800 gnome-shell     S 11.1 11.7 472584
   4303    1800 nautilus        S  0.2  8.9 358068
   4020    3282 Isolated Web Co S  6.4  8.1 328680
   5715    1800 gnome-text-edit S  3.3  7.1 287732
   3838    3282 Isolated Web Co S  0.8  5.3 214488
   3317    3282 Privileged Cont S  0.1  3.8 153800
   3641    3282 WebExtensions   S  0.0  2.2 89816
   4785    1800 gnome-terminal- S  0.5  1.7 68912
   4221    3282 Web Content     S  0.0  1.6 68124
   4255    3282 Web Content     S  0.0  1.6 67576
   4153    3282 Web Content     S  0.0  1.6 67372
   6665    2052 gjs             S  0.2  1.5 60848
   2224    2012 evolution-alarm S  0.0  1.5 60212
```
Команда:

`pidstat -u -r -d 1 5`

**Резульат:**
```bash
sancher@Ubuntu:~$ pidstat -u -r -d 1 5 
Linux 6.14.0-29-generic (Ubuntu) 	09/21/2025 	_x86_64_	(4 CPU)

11:52:32 AM   UID       PID    %usr %system  %guest   %wait    %CPU   CPU  Command
11:52:35 AM  1000      2052    0.97    0.65    0.00    0.00    1.61     0  gnome-shell
11:52:35 AM  1000      3103    0.00    0.32    0.00    0.00    0.32     3  firefox
11:52:35 AM  1000      3317    0.00    0.32    0.00    0.00    0.32     2  Privileged Cont
11:52:35 AM  1000      4785    0.32    0.00    0.00    0.00    0.32     2  gnome-terminal-
11:52:35 AM     0      6184    0.00    0.32    0.00    0.00    0.32     0  kworker/0:2-events
11:52:35 AM     0      6609    0.00    0.32    0.00    0.00    0.32     0  kworker/u17:4-events_unbound
11:52:35 AM  1000      7261    0.00    0.32    0.00    0.00    0.32     1  pidstat

11:52:32 AM   UID       PID  minflt/s  majflt/s     VSZ     RSS   %MEM  Command
11:52:35 AM  1000      2052     60.65      2.26 4865256  479920  11.97  gnome-shell
11:52:35 AM  1000      4020      0.32      0.00 2951816  331096   8.26  Isolated Web Co
11:52:35 AM  1000      7261      0.32      0.00   18968    4108   0.10  pidstat

11:52:32 AM   UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command

11:52:35 AM   UID       PID    %usr %system  %guest   %wait    %CPU   CPU  Command
11:52:38 AM  1000      2052    0.88    0.88    0.00    0.44    1.77     0  gnome-shell
11:52:38 AM  1000      3103    0.44    0.00    0.00    0.00    0.44     3  firefox
11:52:38 AM  1000      4020    0.44    0.00    0.00    0.00    0.44     0  Isolated Web Co
11:52:38 AM  1000      7261    0.00    0.44    0.00    0.00    0.44     3  pidstat

11:52:35 AM   UID       PID  minflt/s  majflt/s     VSZ     RSS   %MEM  Command
11:52:38 AM  1000      2052     38.50      0.00 4865256  479920  11.97  gnome-shell
11:52:38 AM  1000      4020      0.88      0.00 2951816  331096   8.26  Isolated Web Co
11:52:38 AM  1000      4785      5.75      0.00  579220   69296   1.73  gnome-terminal-

11:52:35 AM   UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command

11:52:38 AM   UID       PID    %usr %system  %guest   %wait    %CPU   CPU  Command
11:52:39 AM  1000      2052    1.00    1.00    0.00    0.00    2.00     2  gnome-shell
11:52:39 AM  1000      7261    1.00    0.00    0.00    0.00    1.00     3  pidstat

11:52:38 AM   UID       PID  minflt/s  majflt/s     VSZ     RSS   %MEM  Command
11:52:39 AM  1000      2052     41.00      1.00 4861096  479920  11.97  gnome-shell

11:52:38 AM   UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command

11:52:39 AM   UID       PID    %usr %system  %guest   %wait    %CPU   CPU  Command
11:52:40 AM  1000      2052    1.71    2.29    0.00    0.00    4.00     1  gnome-shell
11:52:40 AM  1000      4785    0.57    0.00    0.00    0.57    0.57     1  gnome-terminal-
11:52:40 AM     0      7252    0.00    0.57    0.00    0.00    0.57     3  kworker/u20:0-events_power_efficient

11:52:39 AM   UID       PID  minflt/s  majflt/s     VSZ     RSS   %MEM  Command
11:52:40 AM  1000      2052     48.57      1.14 4861112  479920  11.97  gnome-shell
11:52:40 AM  1000      3317      1.71      0.00 2537400  156828   3.91  Privileged Cont

11:52:39 AM   UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command
11:52:40 AM  1000      3103      0.00     54.86      0.00       0  firefox

11:52:40 AM   UID       PID    %usr %system  %guest   %wait    %CPU   CPU  Command
11:52:42 AM  1000      2052    2.80    6.54    0.00    0.93    9.35     2  gnome-shell
11:52:42 AM  1000      4221    0.00    0.93    0.00    0.00    0.93     0  Web Content
11:52:42 AM  1000      4785    1.87    0.93    0.00    0.00    2.80     0  gnome-terminal-
11:52:42 AM     0      7252    0.00    0.93    0.00    0.00    0.93     3  kworker/u20:0-events_unbound
11:52:42 AM  1000      7261    0.00    0.93    0.00    0.00    0.93     3  pidstat

11:52:40 AM   UID       PID  minflt/s  majflt/s     VSZ     RSS   %MEM  Command
11:52:42 AM  1000      2052    305.61      7.48 4865272  479920  11.97  gnome-shell
11:52:42 AM  1000      4785      1.87      0.00  579220   69296   1.73  gnome-terminal-

11:52:40 AM   UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command

Average:      UID       PID    %usr %system  %guest   %wait    %CPU   CPU  Command
Average:     1000      2052    1.31    1.74    0.00    0.22    3.05     -  gnome-shell
Average:     1000      3103    0.11    0.11    0.00    0.00    0.22     -  firefox
Average:     1000      3317    0.00    0.11    0.00    0.00    0.11     -  Privileged Cont
Average:     1000      4020    0.11    0.00    0.00    0.00    0.11     -  Isolated Web Co
Average:     1000      4221    0.00    0.11    0.00    0.11    0.11     -  Web Content
Average:     1000      4785    0.44    0.11    0.00    0.11    0.54     -  gnome-terminal-
Average:        0      6184    0.00    0.11    0.00    0.00    0.11     -  kworker/0:2-events
Average:        0      6609    0.00    0.11    0.00    0.00    0.11     -  kworker/u17:4-events_unbound
Average:        0      7252    0.00    0.22    0.00    0.11    0.22     -  kworker/u20:0-events_unbound
Average:     1000      7261    0.11    0.33    0.00    0.00    0.44     -  pidstat

Average:      UID       PID  minflt/s  majflt/s     VSZ     RSS   %MEM  Command
Average:     1000      2052     79.30      1.96 4863598  479920  11.97  gnome-shell
Average:     1000      3317      0.33      0.00 2537390  156828   3.91  Privileged Cont
Average:     1000      4020      0.33      0.00 2951816  331096   8.26  Isolated Web Co
Average:     1000      4785      1.63      0.00  579220   69296   1.73  gnome-terminal-
Average:     1000      7261      0.11      0.00   18968    4108   0.10  pidstat

Average:      UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command
Average:     1000      3103      0.00     10.46      0.00       0  firefox
```
##### Вывод:
Что нагружает систему: На основании собранных данных, основными процессами, нагружающими систему, являются gnome-shell и firefox. Эти приложения требуют значительных ресурсов, что может быть связано с их функциональностью и количеством открытых вкладок.

### Ответы на вопросы:
##### 1. Чем процесс отличается от программы?

Процесс — это активная версия программы, обладающая собственным состоянием, памятью и ресурсами. Программа — это статичный набор инструкций, который хранится на диске. Один процесс может запускаться несколько раз, в то время как программа не выполняется, пока не станет процессом.
##### 2. Что произойдёт, если вызвать fork() без wait()?

Если выполнить fork() без вызова wait(), родительский процесс не будет ждать завершения дочернего. Это может привести к тому, что дочерний процесс станет зомби, так как информация о нём останется в таблице процессов, пока родитель не завершится.
##### 3. Как система хранит информацию о процессах?

Информация о процессах хранится в специальной структуре данных, известной как дескриптор процесса (например, task_struct в Linux). Она включает такие данные, как идентификатор процесса (PID), его состояние, приоритет и используемая память.
##### 4. Что делает exec() и для чего он нужен?

exec() заменяет текущий процесс на новый, загружая другую программу в его адресное пространство. Это позволяет запускать другой код без создания нового процесса.
##### 5. Почему в /proc нет «настоящих» файлов?

В /proc находятся виртуальные файлы, которые не занимают места на диске и не являются физическими файлами. Они служат интерфейсом для доступа к информации о системе и процессах, предоставляемой ядром в реальном времени.
##### 6. Как интерпретировать поля top: %CPU, %MEM, VIRT, RES, SHR, TIME+?

    %CPU: процент использования процессора данным процессом.
    %MEM: процент использования оперативной памяти.
    VIRT: объем виртуальной памяти, используемой процессом.
    RES: объем физической памяти, занимаемой процессом.
    SHR: объем общей памяти, используемой совместно с другими процессами.
    TIME+: общее время, затраченное процессом на выполнение.

##### 7. Почему сумма %CPU может превышать 100%?

Сумма %CPU может быть больше 100%, если процессор многоядерный. Каждое ядро может использовать до 100% CPU, поэтому в системе с 4 ядрами сумма может достигать 400%.
##### 8. Чем мгновенное %CPU отличается от load average? Что означает строка Cpu(s) в top (в т.ч. wa)?

    Мгновенное %CPU: показывает текущую загрузку процессора процессом.
    Load average: усредненная нагрузка системы за последние 1, 5 и 15 минут.
    Строка Cpu(s): демонстрирует состояние процессора, включая:
        us: пользовательский режим,
        sy: системный режим,
        id: простаивающий,
        wa: время ожидания ввода-вывода.

##### 9. Чем IO-нагрузка отличается от CPU-нагрузки и как увидеть это (pidstat -d, iotop, /proc/<pid>/io)?

IO-нагрузка связана с операциями чтения и записи на устройства, в то время как CPU-нагрузка — с выполнением инструкций. Для мониторинга IO-нагрузки можно использовать:

    pidstat -d: показывает статистику ввода-вывода для процессов.
    iotop: отображает активные процессы, использующие IO.
    /proc/<pid>/io: предоставляет информацию о вводе-выводе конкретного процесса.

##### 10. Что такое nice/приоритеты процессов и как они влияют на планирование?

Nice-значение — это параметр, определяющий приоритет процесса в планировщике. Низкое значение nice соответствует высокому приоритету, что влияет на распределение CPU между процессами, позволяя более важным процессам получать больше ресурсов.
##### 11. Чем поток отличается от процесса и как увидеть потоки в ps/top?

Поток — это легковесная версия процесса, которая делит память с другими потоками в рамках одного процесса. Для просмотра потоков в ps можно использовать опцию -L, а в top — нажатие H для отображения потоков.
##### 12. Что такое зомби и сироты, как они возникают и куда «исчезают»?

    Зомби: завершившие выполнение процессы, информация о которых всё еще хранится в таблице процессов, пока родитель не вызовет wait().
    Сироты: процессы, чьи родительские процессы завершились, но они продолжают существовать. Сироты становятся дочерними процессами init, который управляет ими, удаляя их из таблицы процессов.


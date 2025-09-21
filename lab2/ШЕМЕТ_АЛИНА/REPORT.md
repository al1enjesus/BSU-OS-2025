# Лабораторная работа 2: Процессы и файловая система /proc

## Цель
Понять модель процессов Linux, принципы порождения и ожидания завершения, а также научиться извлекать информацию из `/proc`.

## Шаги выполнения

### Задание 1: Создание процессов

**Код программы** (`src/fork_example.py`):
```python
import os
import sys
import time

def main():
    print(f"Parent: Starting (PID={os.getpid()})", flush=True)
    children = []
    for i in range(2):
        pid = os.fork()
        if pid == 0:
            print(f"Child {chr(65 + i)}: PID={os.getpid()}, PPID={os.getppid()}", flush=True)
            time.sleep(1)
            sys.exit(i + 1)
        else:
            children.append(pid)
    for pid in children:
        child_pid, status = os.waitpid(pid, 0)
        exit_code = status >> 8
        print(f"Parent: Child {child_pid} exited with code {exit_code}", flush=True)
    print(f"Parent: All children done (PID={os.getpid()})", flush=True)

if __name__ == "__main__":
    main()
```

**Команда**:
```bash
python3 src/fork_example.py
```

**Вывод**:
```
Parent: Starting (PID=4953)
Child A: PID=4954, PPID=4953
Child B: PID=4955, PPID=4953
Parent: Child 4954 exited with code 1
Parent: Child 4955 exited with code 2
Parent: All children done (PID=4953)
```

**Команда**:
```bash
python3 src/fork_example.py & pstree -p $!
```

**Вывод**:
```
[1] 4976
Parent: Starting (PID=4976)
Child B: PID=4978, PPID=4976
Child A: PID=4977, PPID=4976
python3(4976)─┬─python3(4977)
              └─python3(4978)
```

**Пояснение**:
 С помощью `os.fork()` программа `fork_example.py` создаёт два дочерних процесса (Child A и Child B). Каждый дочерний процесс выводит свой PID и PPID. Родительский процесс использует `os.waitpid()` для ожидания завершения дочерних процессов и выводит их коды возврата (1 для Child A, 2 для Child B). Команда `pstree -p $!` показывает дерево процессов, где родительский процесс `python3` (PID=4976) имеет двух дочерних: `python3` (PID=4977) и `python3` (PID=4978).

### Задание 2: Исследование дерева процессов

**Команда**:
```bash
ps -ef --forest | grep -E "bash|code|gnome-terminal|systemd" | grep -v grep
```

**Вывод**:
```
root         272     1  0 15:26 ?        00:00:01 /usr/lib/systemd/systemd-journald
vboxuser    2246     1  0 15:26 ?        00:00:01 /usr/lib/systemd/systemd --user
vboxuser    3470  2246  0 15:32 ?        00:00:07  \_ /usr/libexec/gnome-terminal-server
vboxuser    3478  3470  0 15:32 pts/0    00:00:00  |   \_ bash
vboxuser    4256  2246  0 15:45 ?        00:00:30  \_ /usr/share/code/code
```

**Команда**:
```bash
pstree -p 3478
```

**Вывод**:
```
bash(3478)───pstree(5136)
```

**Команда**:
```bash
pstree -p 2246 | head -n 50
```

**Вывод** (ключевые строки):
```
systemd(2246)─┬─gnome-terminal-(3470)─┬─bash(3478)───pstree(5254)
              ├─code(4256)─┬─code(4261)───code(4293)
              │            ├─code(4262)───code(4264)─┬─code(4330)
              │            │                        └─code(4546)
              │            ├─code(4302)
              │            ├─code(4342)
              │            ├─code(4343)
              │            ├─code(4363)
              │            ├─code(4561)
              │            └─code(4572)───code(4627)
```

**Пояснение**:
Мой текущий процесс — это `bash` (PID=3478), запущенный из терминала `gnome-terminal-server` (PID=3470), который является потомком пользовательского экземпляра `systemd` (PID=2246). Пользовательский `systemd` (PID=2246) порождён системным `systemd` (PID=1). Процесс `code` (PID=4256), представляющий Visual Studio Code, также является потомком `systemd --user` (PID=2246) и имеет несколько дочерних процессов (например, PID=4261, 4262). Вывод `pstree -p 3478` показывает только `bash` и процесс `pstree` (PID=5136). Вывод `pstree -p 2246` даёт более полное представление об иерархии. Команды `ps -ef --forest` и `pstree -p` вместе демонстрируют структуру процессов в системе, где `systemd` (PID=1) является корневым процессом.

### Задание 3: Изучение /proc

**Команды**:
```bash
echo $$
cat /proc/3478/cmdline | tr '\0' ' '; echo
head -n 20 /proc/3478/status
ls -l /proc/3478/fd
```

**Вывод**:
```
3478
```
```
bash
```
```
Name: bash
Umask: 0002
State: S (sleeping)
Tgid: 3478
Ngid: 0
Pid: 3478
PPid: 3470
TracerPid: 0
Uid: 1000 1000 1000 1000
Gid: 1000 1000 1000 1000
FDSize: 256
Groups: 27 1000
NStgid: 3478
NSpid: 3478
NSpgid: 3478
NSsid: 3478
Kthread: 0
VmPeak: 12524 kB
VmSize: 12524 kB
VmLck: 0 kB
```
```
total 0
lrwx------ 1 vboxuser vboxuser 64 Sep 15 17:55 0 -> /dev/pts/0
lrwx------ 1 vboxuser vboxuser 64 Sep 15 17:55 1 -> /dev/pts/0
lrwx------ 1 vboxuser vboxuser 64 Sep 15 17:55 2 -> /dev/pts/0
lrwx------ 1 vboxuser vboxuser 64 Sep 15 17:55 255 -> /dev/pts/0
```

**Пояснение**:
- `echo $$`: Показывает PID текущего процесса — `bash` (PID=3478).
- `/proc/3478/cmdline`: Содержит командную строку процесса, в данном случае `bash`, что указывает на запуск оболочки bash.
- `/proc/3478/status`: Предоставляет информацию о процессе:
  - `Name: bash` — имя процесса.
  - `State: S (sleeping)` — процесс находится в состоянии ожидания (спящий).
  - `Pid: 3478` — идентификатор процесса.
  - `PPid: 3470` — идентификатор родительского процесса (`gnome-terminal-server`).
  - `Uid: 1000` и `Gid: 1000` — идентификаторы пользователя и группы (`vboxuser`).
  - `VmPeak: 12524 kB` и `VmSize: 12524 kB` — пиковое и текущее использование виртуальной памяти процессом.
  - `Groups: 27 1000` — группы, к которым относится процесс (27 — группа `sudo`, 1000 — группа `vboxuser`).
- `/proc/3478/fd`: Показывает открытые файловые дескрипторы:
  - Дескрипторы 0, 1, 2 и 255 связаны с `/dev/pts/0` (псевдотерминал), где 0 — стандартный ввод (stdin), 1 — стандартный вывод (stdout), 2 — стандартный поток ошибок (stderr), 255 — дескриптор терминала.


### Задание 4: Анализ процессов (нагрузка CPU/память/IO)

**Команды**:
```bash
sudo apt install -y sysstat htop iotop
top -b -n 1 | head -n 20 > top_output.txt
ps -eo pid,ppid,comm,state,%cpu,%mem,etime --sort=-%cpu | head -n 15 > ps_cpu_output.txt
ps -eo pid,ppid,comm,state,%cpu,%mem,rss --sort=-%mem | head -n 15 > ps_mem_output.txt
pidstat -u -r -d 1 5 > ~/lab2/gr9sub1/ШЕМЕТ_АЛИНА/pidstat_output.txt
sudo iotop -b -n 5 | head -n 30 > ~/lab2/gr9sub1/ШЕМЕТ_АЛИНА/iotop_output.txt
htop -u "$USER"
```

**Проверка установки пакетов**:
```bash
sudo apt install -y sysstat htop iotop
```

**Проверка записи в файлы**:
```bash
ls -l top_output.txt ps_cpu_output.txt ps_mem_output.txt
```
```
-rw-rw-r-- 1 vboxuser vboxuser  840 Sep 16 14:21 ps_cpu_output.txt
-rw-rw-r-- 1 vboxuser vboxuser  758 Sep 16 14:21 ps_mem_output.txt
-rw-rw-r-- 1 vboxuser vboxuser 1489 Sep 16 14:21 top_output.txt
```

**Выводы команд**:
- `top` (`top_output.txt`):
```
top - 14:21:53 up 2:50,  2 users,  load average: 0.10, 0.26, 0.28
Tasks: 227 total,   1 running, 226 sleeping,   0 stopped,   0 zombie
%Cpu(s):  0.0 us,  1.6 sy,  0.0 ni, 93.4 id,  4.9 wa,  0.0 hi,  0.0 si,  0.0 st
MiB Mem :   3351.7 total,   318.2 free,  1755.1 used,  1598.6 buff/cache
MiB Swap:      0.0 total,     0.0 free,     0.0 used.   1596.6 avail Mem
    PID USER      PR  NI    VIRT    RES    SHR S  %CPU  %MEM     TIME+ COMMAND
   1424 root      20   0  355720   2652   2524 S   7.7   0.1   0:19.85 VBoxDRM+
   6240 vboxuser  20   0 1393.6g 149444  80024 S   7.7   4.4   0:07.42 code
      1 root      20   0   25204  15640  10776 S   0.0   0.5   0:03.01 systemd
      2 root      20   0       0      0      0 S   0.0   0.0   0:00.03 kthreadd
      3 root      20   0       0      0      0 S   0.0   0.0   0:00.00 pool_wo+
      4 root       0 -20       0      0      0 I   0.0   0.0   0:00.00 kworker+
      5 root       0 -20       0      0      0 I   0.0   0.0   0:00.00 kworker+
      6 root       0 -20       0      0      0 I   0.0   0.0   0:00.00 kworker+
      7 root       0 -20       0      0      0 I   0.0   0.0   0:00.00 kworker+
      8 root       0 -20       0      0      0 I   0.0   0.0   0:00.00 kworker+
     11 root       0 -20       0      0      0 I   0.0   0.0   0:00.00 kworker+
     13 root       0 -20       0      0      0 I   0.0   0.0   0:00.00 kworker+
     14 root      20   0       0      0      0 I   0.0   0.0   0:00.00 rcu_tas+
```

- `ps` (CPU, `ps_cpu_output.txt`):
```
    PID PPID COMMAND         S %CPU %MEM ELAPSED
   6378 5903 ps              R  300  0.1 00:00
   6165 6094 code            S 12.4  9.0 06:09
   2496 2246 gnome-shell     S  4.6 14.0 02:49:44
   6128 6091 code            S  3.1  3.9 06:10
   6087 2496 code            S  2.8  5.5 06:11
   6240 6087 code            S  2.0  4.3 06:07
   6200 6087 code            S  1.0  3.2 06:07
   5511    2 kworker/u16:5-i I  0.9  0.0 08:28
   5291    2 kworker/u16:1-e I  0.5  0.0 20:25
   5895 2246 gnome-terminal- S  0.5  1.8 07:20
   5133    2 kworker/u16:4-e I  0.4  0.0 46:04
   2915 2496 Xwayland        S  0.3  2.7 02:49:42
   6201 6087 code            S  0.3  3.0 06:07
   6340 6240 code            S  0.2  2.7 05:40
```

- `ps` (память, `ps_mem_output.txt`):
```
    PID PPID COMMAND         S %CPU %MEM    RSS
   2496 2246 gnome-shell     S  4.6 14.0 482904
   6165 6094 code            S 12.4  9.0 308912
   6087 2496 code            S  2.8  5.5 191656
   6240 6087 code            S  2.0  4.3 149444
   6128 6091 code            S  3.1  3.9 134164
   6200 6087 code            S  1.0  3.2 111164
   3043 2496 mutter-x11-fram S  0.0  3.1 109140
   6201 6087 code            S  0.3  3.0 104628
   2915 2496 Xwayland        S  0.3  2.7  94980
   6340 6240 code            S  0.2  2.7  93420
   3002 2246 gsd-xsettings   S  0.0  2.3  81024
   6139 6087 code            S  0.1  2.1  73972
   5419 2496 gjs             S  0.1  1.9  67384
   5895 2246 gnome-terminal- S  0.5  1.8  62000
```

- `pidstat` (`~/lab2/gr9sub1/ШЕМЕТ_АЛИНА/pidstat_output.txt`):
```
Linux 6.14.0-29-generic (ubuntu) 09/16/2025 _x86_64_ (4 CPU)
02:46:51 PM UID PID %usr %system %guest %wait %CPU CPU Command
02:46:52 PM 1000 2496 0.98 2.94 0.00 0.00 3.92 0 gnome-shell
02:46:52 PM 1000 3093 0.00 0.98 0.00 0.00 0.98 1 VBoxClient
02:46:52 PM 1000 6240 0.00 0.98 0.00 0.00 0.98 3 code
02:46:52 PM 1000 6544 0.00 0.98 0.00 0.00 0.98 3 pidstat
02:46:51 PM UID PID minflt/s majflt/s VSZ RSS %MEM Command
02:46:52 PM 0 2238 3.92 0.00 388984 11164 0.33 gdm-session-wor
02:46:52 PM 1000 2496 207.84 142.16 5165572 484212 14.11 gnome-shell
02:46:52 PM 1000 6087 8.82 0.00 1461520436 192800 5.62 code
02:46:52 PM 1000 6128 8.82 0.00 34113072 134896 3.93 code
02:46:52 PM 1000 6139 3.92 0.00 33859032 74464 2.17 code
02:46:52 PM 1000 6165 3.92 0.00 1478120996 324972 9.47 code
02:46:52 PM 1000 6544 0.98 0.00 10372 4280 0.12 pidstat
02:46:51 PM UID PID kB_rd/s kB_wr/s kB_ccwr/s iodelay Command
02:46:52 PM UID PID %usr %system %guest %wait %CPU CPU Command
02:46:53 PM 1000 2496 3.00 9.00 0.00 0.00 12.00 3 gnome-shell
02:46:53 PM 1000 5895 1.00 0.00 0.00 1.00 1.00 0 gnome-terminal-
02:46:53 PM 1000 6087 0.00 1.00 0.00 0.00 1.00 3 code
02:46:53 PM 0 6424 0.00 1.00 0.00 0.00 1.00 2 kworker/u16:2-events_unbound
02:46:53 PM 1000 6544 1.00 1.00 0.00 0.00 2.00 3 pidstat
02:46:52 PM UID PID minflt/s majflt/s VSZ RSS %MEM Command
02:46:53 PM 1000 2496 223.00 125.00 5165588 484212 14.11 gnome-shell
02:46:53 PM 1000 6544 1.00 0.00 10372 4280 0.12 pidstat
02:46:52 PM UID PID kB_rd/s kB_wr/s kB_ccwr/s iodelay Command
02:46:53 PM UID PID %usr %system %guest %wait %CPU CPU Command
02:46:54 PM 1000 2496 0.00 0.99 0.00 0.00 0.99 3 gnome-shell
02:46:54 PM 1000 5895 0.00 0.99 0.00 0.00 0.99 1 gnome-terminal-
02:46:53 PM UID PID minflt/s majflt/s VSZ RSS %MEM Command
02:46:54 PM 0 1 1.98 0.00 25204 15640 0.46 systemd
02:46:54 PM 1000 2496 6.93 0.00 5157908 484212 14.11 gnome-shell
02:46:54 PM 1000 6128 0.00 0.00 34113072 134768 3.93 code
02:46:54 PM 1000 6240 3.96 0.00 1461295820 156780 4.57 code
02:46:53 PM UID PID kB_rd/s kB_wr/s kB_ccwr/s iodelay Command
02:46:54 PM UID PID %usr %system %guest %wait %CPU CPU Command
02:46:55 PM 0 4997 0.00 1.00 0.00 0.00 1.00 3 kworker/3:0-mm_percpu_wq
02:46:55 PM 1000 6240 0.00 1.00 0.00 0.00 1.00 0 code
02:46:55 PM 1000 6544 0.00 1.00 0.00 0.00 1.00 3 pidstat
02:46:54 PM UID PID minflt/s majflt/s VSZ RSS %MEM Command
02:46:55 PM 1000 2496 5.00 4.00 5165588 484212 14.11 gnome-shell
02:46:55 PM 1000 6200 14.00 0.00 1461294888 112416 3.28 code
02:46:55 PM 1000 6201 2.00 0.00 1461295432 104736 3.05 code
02:46:55 PM 1000 6240 5.00 0.00 1461295820 156780 4.57 code
02:46:54 PM UID PID kB_rd/s kB_wr/s kB_ccwr/s iodelay Command
02:46:55 PM UID PID %usr %system %guest %wait %CPU CPU Command
02:46:56 PM 1000 2496 0.00 2.00 0.00 0.00 2.00 0 gnome-shell
02:46:56 PM 1000 2896 0.00 1.00 0.00 0.00 1.00 2 gvfs-gphoto2-vo
02:46:56 PM 1000 3039 0.00 1.00 0.00 0.00 1.00 2 gjs
02:46:56 PM 1000 6201 0.00 1.00 0.00 0.00 1.00 2 code
02:46:56 PM 1000 6240 1.00 0.00 0.00 0.00 1.00 0 code
02:46:56 PM 0 6424 0.00 1.00 0.00 0.00 1.00 0 kworker/u16:2-events_unbound
02:46:56 PM 1000 6505 0.00 1.00 0.00 0.00 1.00 1 gjs
02:46:56 PM 1000 6544 1.00 1.00 0.00 0.00 2.00 3 pidstat
02:46:55 PM UID PID minflt/s majflt/s VSZ RSS %MEM Command
02:46:56 PM 0 1 84.00 0.00 25204 15640 0.46 systemd
02:46:56 PM 1000 2496 10.00 0.00 5157904 484212 14.11 gnome-shell
02:46:56 PM 1000 3039 3.00 0.00 2660012 28420 0.83 gjs
02:46:56 PM 1000 6087 9.00 0.00 1461520436 192800 5.62 code
02:46:56 PM 1000 6139 4.00 0.00 33859032 74464 2.17 code
02:46:56 PM 1000 6201 1.00 0.00 1461295432 104736 3.05 code
02:46:56 PM 1000 6505 6.00 0.00 2724836 67068 1.95 gjs
02:46:55 PM UID PID kB_rd/s kB_wr/s kB_ccwr/s iodelay Command
Average: UID PID %usr %system %guest %wait %CPU CPU Command
Average: 1000 2496 0.80 2.98 0.00 0.00 3.78 - gnome-shell
Average: 1000 2896 0.00 0.20 0.00 0.00 0.20 - gvfs-gphoto2-vo
Average: 1000 3039 0.00 0.20 0.00 0.00 0.20 - gjs
Average: 1000 3093 0.00 0.20 0.00 0.00 0.20 - VBoxClient
Average: 0 4997 0.00 0.20 0.00 0.20 0.00 - kworker/3:0-mm_percpu_wq
Average: 1000 5895 0.20 0.20 0.00 0.20 0.40 - gnome-terminal-
Average: 1000 6087 0.00 0.20 0.00 0.00 0.20 - code
Average: 1000 6201 0.00 0.20 0.00 0.00 0.20 - code
Average: 1000 6240 0.20 0.40 0.00 0.00 0.60 - code
Average: 0 6424 0.00 0.40 0.00 0.00 0.40 - kworker/u16:2-events_unbound
Average: 1000 6505 0.00 0.20 0.00 0.00 0.20 - gjs
Average: 1000 6544 0.40 0.80 0.00 0.00 1.19 - pidstat
Average: UID PID minflt/s majflt/s VSZ RSS %MEM Command
Average: 0 1 17.10 0.00 25204 15640 0.46 systemd
Average: 0 2238 0.80 0.00 388984 11164 0.33 gdm-session-wor
Average: 1000 2496 90.85 54.47 5162512 484212 14.11 gnome-shell
Average: 1000 3039 0.60 0.00 2660012 28420 0.83 gjs
Average: 1000 6087 3.58 0.00 1461520436 192800 5.62 code
Average: 1000 6128 1.79 0.00 34113072 134819 3.93 code
Average: 1000 6139 1.59 0.00 33859032 74464 2.17 code
Average: 1000 6165 0.80 0.00 1478120996 324972 9.47 code
Average: 1000 6200 2.78 0.00 1461294888 112416 3.28 code
Average: 1000 6201 0.60 0.00 1461295432 104736 3.05 code
Average: 1000 6240 1.79 0.00 1461295820 156780 4.57 code
Average: 1000 6505 1.19 0.00 2724836 67068 1.95 gjs
Average: 1000 6544 0.40 0.00 10372 4280 0.12 pidstat
Average: UID PID kB_rd/s kB_wr/s kB_ccwr/s iodelay Command
```

- `iotop` (`~/lab2/gr9sub1/ШЕМЕТ_АЛИНА/iotop_output.txt`):
```
Total DISK READ: 0.00 B/s | Total DISK WRITE: 0.00 B/s
Current DISK READ: 0.00 B/s | Current DISK WRITE: 0.00 B/s
    TID PRIO USER DISK READ DISK WRITE SWAPIN IO COMMAND
      1 be/4 root 0.00 B/s 0.00 B/s ?unavailable? init splash noprompt noshell automatic-ubiquity crashkernel=2G-4G:320M,4G-32G:512M,32G-64G:1024M,64G-128G:2048M,128G-:4096M
      2 be/4 root 0.00 B/s 0.00 B/s ?unavailable? [kthreadd]
      3 be/4 root 0.00 B/s 0.00 B/s ?unavailable? [pool_workqueue_release]
      4 be/0 root 0.00 B/s 0.00 B/s ?unavailable? [kworker/R-rcu_gp]
      5 be/0 root 0.00 B/s 0.00 B/s ?unavailable? [kworker/R-sync_wq]
      6 be/0 root 0.00 B/s 0.00 B/s ?unavailable? [kworker/R-kvfree_rcu_reclaim]
      7 be/0 root 0.00 B/s 0.00 B/s ?unavailable? [kworker/R-slub_flushwq]
      8 be/0 root 0.00 B/s 0.00 B/s ?unavailable? [kworker/R-netns]
     11 be/0 root 0.00 B/s 0.00 B/s ?unavailable? [kworker/0:0H-events_highpri]
     13 be/0 root 0.00 B/s 0.00 B/s ?unavailable? [kworker/R-mm_percpu_wq]
     14 be/4 root 0.00 B/s 0.00 B/s ?unavailable? [rcu_tasks_kthread]
     15 be/4 root 0.00 B/s 0.00 B/s ?unavailable? [rcu_tasks_rude_kthread]
     16 be/4 root 0.00 B/s 0.00 B/s ?unavailable? [rcu_tasks_trace_kthread]
     17 be/4 root 0.00 B/s 0.00 B/s ?unavailable? [ksoftirqd/0]
     18 be/4 root 0.00 B/s 0.00 B/s ?unavailable? [rcu_preempt]
     19 be/4 root 0.00 B/s 0.00 B/s ?unavailable? [rcu_exp_par_gp_kthread_worker/0]
     20 be/4 root 0.00 B/s 0.00 B/s ?unavailable? [rcu_exp_gp_kthread_worker]
     21 rt/4 root 0.00 B/s 0.00 B/s ?unavailable? [migration/0]
     22 rt/4 root 0.00 B/s 0.00 B/s ?unavailable? [idle_inject/0]
     23 be/4 root 0.00 B/s 0.00 B/s ?unavailable? [cpuhp/0]
     24 be/4 root 0.00 B/s 0.00 B/s ?unavailable? [cpuhp/1]
     25 rt/4 root 0.00 B/s 0.00 B/s ?unavailable? [idle_inject/1]
     26 rt/4 root 0.00 B/s 0.00 B/s ?unavailable? [migration/1]
     27 be/4 root 0.00 B/s 0.00 B/s ?unavailable? [ksoftirqd/1]
     29 be/0 root 0.00 B/s 0.00 B/s ?unavailable? [kworker/1:0H-kblockd]
     30 be/4 root 0.00 B/s 0.00 B/s ?unavailable? [cpuhp/2]
     31 rt/4 root 0.00 B/s 0.00 B/s ?unavailable? [idle_inject/2]
```

#### TOP-5 по CPU
| PID  | COMMAND         | %CPU | ELAPSED   |
|------|-----------------|------|-----------|
| 6165 | code            | 12.4 | 06:09     |
| 2496 | gnome-shell     | 4.6  | 02:49:44  |
| 6128 | code            | 3.1  | 06:10     |
| 6087 | code            | 2.8  | 06:11     |
| 6240 | code            | 2.0  | 06:07     |

#### TOP-5 по памяти
| PID  | COMMAND         | %MEM | RSS (MiB) |
|------|-----------------|------|-----------|
| 2496 | gnome-shell     | 14.0 | 471.6     |
| 6165 | code            | 9.0  | 301.7     |
| 6087 | code            | 5.5  | 187.2     |
| 6240 | code            | 4.3  | 146.0     |
| 6128 | code            | 3.9  | 131.0     |

#### TOP-5 по IO
| PID  | COMMAND         | DISK READ (B/s) | DISK WRITE (B/s) |
|------|-----------------|-----------------|------------------|
| 2496 | gnome-shell     | 0.00            | 0.00             |
| 6165 | code            | 0.00            | 0.00             |
| 6087 | code            | 0.00            | 0.00             |
| 6240 | code            | 0.00            | 0.00             |
| 6128 | code            | 0.00            | 0.00             |

**Выводы**:
Основную нагрузку на CPU и память создают процессы `code` (VS Code, PID=6165, 6087, 6240, 6128) и `gnome-shell` (PID=2496). Высокое потребление CPU и памяти процессами `code` связано с работой Visual Studio Code с открытыми проектами и расширениями. `gnome-shell` нагружает систему из-за графического интерфейса Ubuntu. IO-нагрузка минимальна (0.00 B/s для чтения и записи), что подтверждается выводами `pidstat` и `iotop`. Это типично для виртуальной машины, где дисковые операции ограничены настройками VirtualBox и отсутствием интенсивных задач.


### Задание 5: Мини-утилита ptree

**Код программы** (`src/ptree.py`):
```python
import os
import re

def get_ppid(pid):
    try:
        with open(f"/proc/{pid}/status", "r") as f:
            for line in f:
                if line.startswith("PPid:"):
                    return int(re.search(r"\d+", line).group())
    except FileNotFoundError:
        return None

def main():
    pid = os.getpid()
    chain = []
    while pid and pid != 1:
        chain.append(f"{os.path.basename(os.readlink(f'/proc/{pid}/exe')) if os.path.exists(f'/proc/{pid}/exe') else 'unknown'}({pid})")
        pid = get_ppid(pid)
    chain.append("systemd(1)")
    print(" ← ".join(reversed(chain)))

if __name__ == "__main__":
    main()
```

**Команда**:
```bash
python3 ~/lab2/gr9sub1/ШЕМЕТ_АЛИНА/src/ptree.py
```

**Вывод**:
```
systemd(1) ← unknown(2246) ← gnome-terminal-server(5895) ← bash(5903) ← python3.13(6992)
```

**Пояснение**:
Утилита `ptree.py` строит цепочку родительских процессов от текущего процесса (`python3.13`, PID=6992) до корневого `systemd` (PID=1). Она использует `os.getpid()` для получения текущего PID, читает PPID из `/proc/<pid>/status` с помощью функции `get_ppid()`, и получает имя процесса через `os.readlink('/proc/<pid>/exe')`. Если доступ к `/proc/<pid>/exe` ограничен (например, для PID=2246, пользовательского `systemd`), выводится `unknown`.

### Ответы на вопросы

1. **Что такое PID и PPID?**
   PID — идентификатор процесса, уникальный номер, присваиваемый каждому процессу в Linux. PPID — идентификатор родительского процесса, указывающий, какой процесс породил текущий.

2. **Как работает `fork()` в Linux?**
   Функция `fork()` создаёт копию текущего процесса (дочерний процесс). Родитель получает PID дочернего процесса, а дочерний — 0.

3. **Что содержится в `/proc/<pid>/status`?**
   Файл содержит информацию о процессе: имя, состояние, PID, PPID, использование памяти, UID и т.д.

4. **Что такое `/proc/<pid>/fd`?**
   Каталог содержит файловые дескрипторы процесса, например, 0 (stdin), 1 (stdout), 2 (stderr), связанные с файлами или устройствами, такими как `/dev/pts/0`.

5. **Какой процесс имеет PID=1?**
   Это `systemd`, корневой процесс, инициализирующий систему и порождающий другие процессы.

6. **Зачем нужен `os.waitpid()` в `fork_example.py`?**
   `os.waitpid()` позволяет родительскому процессу дождаться завершения дочернего и получить его код возврата.

7. **Почему в `pstree` видны дочерние процессы `code`?**
   Visual Studio Code создаёт несколько процессов для обработки расширений, рендеринга интерфейса и других задач.

8. **Что означает состояние `S` в `/proc/<pid>/status`?**
   `S` (sleeping) — процесс спит, ожидая события, например, ввода пользователя.

9. **Почему IO-нагрузка низкая в виртуальной машине?**
   Виртуальная машина использует виртуальный диск, а IO-операции ограничены настройками VirtualBox и отсутствием интенсивных задач.

10. **Что показывает `top`?**
    `top` отображает текущую загрузку системы (CPU, память, своп) и список процессов с их характеристиками.

11. **Что такое `kworker` процессы?**
    Это системные процессы ядра Linux, выполняющие фоновые задачи, такие как обработка прерываний или IO.

12. **Как работает `iotop`?**
    `iotop` отслеживает операции чтения/записи на диск для каждого процесса, требуя прав root для доступа к данным.
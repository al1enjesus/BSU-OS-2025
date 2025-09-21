# лабораторная 2 - процессы и файловая система /proc

## окружение

```
Ubuntu 25.04 (VirtualBox)
GNU bash, версия 5.2.37(1)-release
Python 3.13.3
```

## цель работы

изучить организацию процессов в системе linux — как их создавать (fork, exec), кто ими управляет, как за ними наблюдать через /proc.

## задача 1 - порождатель процессов

написан скрипт на python, который создаёт два дочерних процесса, и удерживает родителя до нажатия enter, чтобы его можно было найти в дальнейших заданиях:

```py
import os
import sys

print(f"parent: PID={os.getpid()}, PPID={os.getppid()}")
for i in range(2):
	pid = os.fork()
	if pid == 0:
        # ...child...
		print(f"child[{i}]: PID={os.getpid()}, PPID={os.getppid()}")
		sys.exit(0)
	else:
        # ...parent...
		os.waitpid(pid, 0)	

input("press enter to exit...")
```

вывод программы (`python src/fork.py`):

```
parent: PID=5447, PPID=3141
child[0]: PID=5448, PPID=5447
child[1]: PID=5449, PPID=5447
press enter to exit...
```

## задача 2 - поиск родителя

pid процесса скрипта `fork.py` - `5547`. найдём его через утилиты `ps` и `pstree`:

`ps -ef --forest | grep -C 3 '5547' | cat`:

```
(username) (pid!)  (ppid)
someran+    3064    2764  0 10:49 pts/4    00:00:00      \_ bash
someran+    3095    2764  0 10:50 pts/5    00:00:00      \_ bash
someran+    3141    2764  0 10:55 pts/6    00:00:00      \_ bash
someran+    5447    3141  0 12:04 pts/6    00:00:00      |   \_ python fork.py
someran+    3153    2764  0 10:55 pts/7    00:00:00      \_ bash
someran+    5451    2764  3 12:04 pts/8    00:00:00      \_ bash
someran+    5462    5451 99 12:04 pts/8    00:00:00          \_ ps -ef --forest
someran+    5463    5451  0 12:04 pts/8    00:00:00          \_ grep --color=auto -C 3 5447
someran+    5464    5451  0 12:04 pts/8    00:00:00          \_ cat
rtkit       1736       1  0 10:40 ?        00:00:00 /usr/libexec/rtkit-daemon
colord      2047       1  0 10:41 ?        00:00:00 /usr/libexec/colord
```

`pstree -p | grep -C 3 '5547' | cat`:

```
(здесь обрезано, но над bash ещё идёт gnome-terminal)
           |               |                       |-bash(3025)
           |               |                       |-bash(3064)
           |               |                       |-bash(3095)
           |               |                       |-bash(3141)---python(5447)
           |               |                       |-bash(3153)
           |               |                       |-bash(5451)-+-cat(5497)
           |               |                       |            |-grep(5496)
```

## задача 3 - виртуальная папка `/proc`

узнаём pid текущего терминала через `echo $$` -> `5638`.

### `cat /proc/5638/cmdline | tr '\0' ' '; echo`

выведет то, как вызывалась программа. например, если вызывался скрипт `abc.py`, то для интерпретатора питона данная команда выведет `python abc.py`. в нашем случае:

```
bash
```

т.е. мы просто открыли новый терминал bash.

### `head -n 20 /proc/5638/status`

файл `status` содержит информацию о процессе, включая его название, состояние, pid, ppid (pid родительского процесса), и т.д. в нашем случае:

```
Name:	bash
Umask:	0002
State:	S (sleeping)
Tgid:	5638
Ngid:	0
Pid:	5638
PPid:	2764
TracerPid:	0
Uid:	1000	1000	1000	1000
Gid:	1000	1000	1000	1000
FDSize:	256
Groups:	4 24 27 30 46 100 115 1000 
NStgid:	5638
NSpid:	5638
NSpgid:	5638
NSsid:	5638
Kthread:	0
VmPeak:	   20948 kB
VmSize:	   20948 kB
VmLck:	       0 kB
```

### `ls -l /proc/5638/fd`

```
total 0
lrwx------ 1 somerandomprog somerandomprog 64 Sep 19 12:28 0 -> /dev/pts/8
lrwx------ 1 somerandomprog somerandomprog 64 Sep 19 12:28 1 -> /dev/pts/8
lrwx------ 1 somerandomprog somerandomprog 64 Sep 19 12:28 2 -> /dev/pts/8
lrwx------ 1 somerandomprog somerandomprog 64 Sep 19 12:28 255 -> /dev/pts/8
```
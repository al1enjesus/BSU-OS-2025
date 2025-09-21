#Лабораторная работа номер 2 — Процессы и файловая система /proc

##Цель лабораторной работы:
Понять модель процессов Linux, принципы порождения и ожидания завершения, а также научиться извлекать информацию из /proc.

##Ход работы:

### 1) Создание процессов
Реализована программа на Python, в которой процесс-родитель вызывает порождение дважды.
- Код программы находится в: src/fork.py 
- Терминал: python3 src/fork.py #запуск программы

- Вывод в консоли:
```bash
parent: PID=15079
child_A: PID=15080, PPID=15079
child_B: PID=15081, PPID=15079
parent: child_A (PID=15080) finished with status 0
parent: child_B (PID=15081) finished with status 0
parent: all children finished
```

###Вывод: Программа создала два дочерних процесса. child_A и child_B вывели PID, PPID. Родительский процесс подождал завершения процессов и вывел соответствующее сообщение.

###2) Исследование дерева процессов

###1.1 ps -ef --forest | head -n 30 | cat

###Вывод в консоли: 
```bash
UID          PID    PPID  C STIME TTY          TIME CMD
root           2       0  0 17:55 ?        00:00:00 [kthreadd]
root           3       2  0 17:55 ?        00:00:00  \_ [pool_workqueue_release]
root           4       2  0 17:55 ?        00:00:00  \_ [kworker/R-rcu_gp]
root           5       2  0 17:55 ?        00:00:00  \_ [kworker/R-sync_wq]
...
```

ps -ef - показывает все процессы в системе в полном формате
--forest - отображает процессы в виде дерева (показывает иерархию)
head -n 30 - показывает только первые 30 строк
cat - просто выводит результат

###1.2 pstree -p | head -n 50 | cat 

###Вывод в консоли:
```bash
systemd(1)-+-ModemManager(1196)-+-{ModemManager}(1206)
           |                    |-{ModemManager}(1207)
           |                    `-{ModemManager}(1211)
           |-NetworkManager(1155)-+-{NetworkManager}(1200)
           |                      |-{NetworkManager}(1201)
           |                      `-{NetworkManager}(1202)
           |-VBoxDRMClient(1464)-+-{VBoxDRMClient}(1481)
           |                     |-{VBoxDRMClient}(1482)
           |                     |-{VBoxDRMClient}(1484)
           |                     |-{VBoxDRMClient}(1485)
           |                     `-{VBoxDRMClient}(2544)
           |-VBoxService(1468)-+-{VBoxService}(1469)
           |                   |-{VBoxService}(1470)
           |                   |-{VBoxService}(1471)
           |                   |-{VBoxService}(1472)
           |                   |-{VBoxService}(1473)
           |                   |-{VBoxService}(1474)
           |                   |-{VBoxService}(1475)
           |                   `-{VBoxService}(1476)
...
```

pstree - показывает процессы в виде дерева (более наглядно)
-p - показывает PID процессов в скобках
head -n 50 - показывает первые 50 строк
cat - выводит результат

###Вывод: Эти команды помогают убедиться, что программа создает правильную иерархию процессов и что родительско-дочерние связи работают корректно.

##3) Изучение /proc
###Узнаем PID текущей оболочки:
echo $$
###Вывод в консоли: 4659
- Эта команда показывает Process ID текущего bash-процесса.

###Исследуем содержимое /proc для этого PID
cat /proc/4659/cmdline | tr '\0' ' '; echo
###Вывод в консоли: bash
- Показывает полную командную строку, которой был запущен процесс. Вместо пробелов используются нулевые байты (\0), поэтому tr '\0' ' ' заменяет их на пробелы для читаемости.

###Информация о статусе процесса:
```bash 
head -n 20 /proc/4659/status
Вывод в консоли:
Name:	bash
Umask:	0002
State:	S (sleeping)
Tgid:	4659
Ngid:	0
Pid:	4659
PPid:	4651
TracerPid:	0
Uid:	1000	1000	1000	1000
Gid:	1000	1000	1000	1000
FDSize:	256
Groups:	4 24 27 30 46 100 115 1000 
NStgid:	4659
NSpid:	4659
NSpgid:	4659
NSsid:	4659
Kthread:	0
VmPeak:	   11440 kB
VmSize:	   11408 kB
VmLck:	       0 kB
```

- Показывает детальную информацию о процессе. Первые 20 строк показывают самое важное:
Name: имя процесса (bash)
Umask: маска прав доступа для новых файлов
State: состояние процесса (S - sleeping, R - running)
Pid: идентификатор процесса
PPid: идентификатор родительского процесса
Uid/Gid: реальный и эффективный пользователь/группа
Groups: группы процесса
VmSize: размер виртуальной памяти

###Открытие файловые дескрипторы:
ls -l /proc/4659/fd
###Вывод в консоли:
```bash
итого 0
lrwx------ 1 alphacrux alphacrux 64 сен 21 23:05 0 -> /dev/pts/0
lrwx------ 1 alphacrux alphacrux 64 сен 21 23:05 1 -> /dev/pts/0
lrwx------ 1 alphacrux alphacrux 64 сен 21 23:05 2 -> /dev/pts/0
lrwx------ 1 alphacrux alphacrux 64 сен 21 23:05 255 -> /dev/pts/0
```

- Показывает список всех открытых файловых дескрипторов процесса:
0 → stdin (стандартный ввод)
1 → stdout (стандартный вывод)
2 → stderr (стандартная ошибка)
255 → обычно файл самого bash
Другие номера → открытые файлы или сокеты

##4) Анализ процессов (нагрузка CPU/память/IO)
###Моментальный срез нагрузки:
top -b -n 1 | head -n 20 | cat

###Вывод в консоли: 
```bash
top - 23:09:12 up  5:14,  2 users,  load average: 1,50, 1,03, 0,87
Задачи: 226 total,   1 running, 224 sleeping,   0 stopped,   1 zombie
%Cpu(s):  5,3 us,  0,0 sy,  0,0 ni, 94,7 id,  0,0 wa,  0,0 hi,  0,0 si,  0,0 st 
МиБ Mem :   3403,4 total,    131,2 free,   2423,8 used,    665,2 buff/cache     
МиБ Swap:   3915,0 total,   3182,4 free,    732,6 used.    979,6 avail Mem 

    PID USER      PR  NI    VIRT    RES    SHR S  %CPU  %MEM     TIME+
   5637 alphacr+  20   0 2857804 377100 102004 S  20,0  10,8   5:42.85
      1 root      20   0   25476  13468   9756 S   0,0   0,4   0:02.63
      2 root      20   0       0      0      0 S   0,0   0,0   0:00.00
      3 root      20   0       0      0      0 S   0,0   0,0   0:00.00
      4 root       0 -20       0      0      0 I   0,0   0,0   0:00.00
      ...
```

- Показывает первые 20 строк вывода top - самые ресурсоемкие процессы на момент запуска.
-b - пакетный режим (для вывода в консоль)
-n 1 - только один снимок (вместо непрерывного обновления)

###ps -eo pid,ppid,comm,state,%cpu,%mem,etime --sort=-%cpu | head -n 15 | cat

###Вывод в консоли:
```bash
    PID    PPID COMMAND         S %CPU %MEM     ELAPSED
   1821    1579 gnome-shell     S  7.4 11.7    05:14:19
   2619    1821 firefox         S  5.5 18.6    05:14:11
   8204    2766 Isolated Web Co S  3.5 12.0    03:21:46
```

- Показывает: 15 процессов, сортированных по загрузке CPU (%cpu).
pid,ppid - ID процесса и родителя
comm - имя команды
state - состояние процесса
%cpu,%mem - загрузка CPU и памяти
etime - время работы процессаП

###ps -eo pid,ppid,comm,state,%cpu,%mem,rss --sort=-%mem | head -n 15 | cat

###Вывод в консоли:
```bash
    PID    PPID COMMAND         S %CPU %MEM   RSS
   2619    1821 firefox         S  5.5 18.6 648744
   1821    1579 gnome-shell     S  7.4 11.8 411840
   8204    2766 Isolated Web Co S  3.5 11.0 383724
   5637    2766 Isolated Web Co S  1.9  9.5 331384
   ...
```

- Показывает: 15 процессов, сортированных по использованию памяти (%mem).
rss - Resident Set Size (физическая память в KB)

###Мониторинг с интервалами:
pidstat -u -r -d 1 5 | cat   # 5 интервалов по 1 сек

###Вывод в консоли:
```bash
Linux 6.14.0-29-generic (alphacrux-VirtualBox) 	21.09.2025 	_x86_64_	(2 CPU)

23:11:04      UID       PID    %usr %system  %guest   %wait    %CPU   CPU  Command
23:11:05     1000      1821    1,96    0,00    0,00    0,00    1,96     1  gnome-shell
23:11:05     1000      2619    0,98    0,00    0,00    0,00    0,98     0  firefox
23:11:05     1000      8204    4,90    0,00    0,00    0,00    4,90     0  Isolated Web Co
23:11:05     1000     16152    0,00    0,98    0,00    0,00    0,98     0  pidstat

23:11:04      UID       PID  minflt/s  majflt/s     VSZ     RSS   %MEM  Command
23:11:05     1000      1821    305,88     34,31 4173568  411524  11,81  gnome-shell
23:11:05     1000      2619     52,94      0,00 12477480  647376  18,58  firefox
23:11:05     1000      8204    888,24      0,00 15690292  386368  11,09  Isolated Web Co
23:11:05     1000     16152      3,92      0,00    7700    4308   0,12  pidstat

23:11:04      UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command
23:11:05     1000      2619      0,00    682,35      0,00       0  firefox

23:11:05      UID       PID    %usr %system  %guest   %wait    %CPU   CPU  Command
23:11:06     1000      1821   15,00   10,00    0,00    7,00   25,00     1  gnome-shell
23:11:06     1000      2619   13,00    5,00    0,00    3,00   18,00     1  firefox
23:11:06     1000      3330    1,00    0,00    0,00    0,00    1,00     0  Isolated Web Co
23:11:06     1000      4651    2,00    1,00    0,00    1,00    3,00     1  gnome-terminal-
23:11:06     1000      5637    1,00    0,00    0,00    0,00    1,00     0  Isolated Web Co
23:11:06     1000      9742   10,00    2,00    0,00    4,00   12,00     0  Isolated Web Co
23:11:06        0     13981    0,00    1,00    0,00    0,00    1,00     1  kworker/u8:0-events_unbound
23:11:06        0     15078    0,00    3,00    0,00    1,00    3,00     1  kworker/u8:3-events_power_efficient

23:11:05      UID       PID  minflt/s  majflt/s     VSZ     RSS   %MEM  Command
23:11:06     1000      1821   1426,00    205,00 4173568  411524  11,81  gnome-shell
23:11:06     1000      3330      2,00      0,00 2744104  308716   8,86  Isolated Web Co
23:11:06     1000      8204     10,00      0,00 15690292  386368  11,09  Isolated Web Co
23:11:06     1000     16152      1,00      0,00    7700    4308   0,12  pidstat

23:11:05      UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command

23:11:06      UID       PID    %usr %system  %guest   %wait    %CPU   CPU  Command
23:11:07      989       274    0,00    0,99    0,00    0,00    0,99     0  systemd-oomd
23:11:07     1000      1821   12,87    4,95    0,00    3,96   17,82     0  gnome-shell
23:11:07     1000      2619    7,92    4,95    0,00    1,98   12,87     1  firefox
23:11:07     1000      8204    1,98    0,00    0,00    0,99    1,98     1  Isolated Web Co
23:11:07     1000      9742    6,93    2,97    0,00    2,97    9,90     0  Isolated Web Co
23:11:07        0     13981    0,00    2,97    0,00    1,98    2,97     1  kworker/u8:0-events_unbound
23:11:07     1000     16152    0,99    1,98    0,00    0,00    2,97     0  pidstat

23:11:06      UID       PID  minflt/s  majflt/s     VSZ     RSS   %MEM  Command
23:11:07     1000      1821   1185,15    324,75 4173568  411396  11,80  gnome-shell
23:11:07     1000      8204    110,89      0,00 15690308  386752  11,10  Isolated Web Co

23:11:06      UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command
23:11:07     1000      2619      0,00      7,92      0,00       0  firefox

23:11:07      UID       PID    %usr %system  %guest   %wait    %CPU   CPU  Command
23:11:08        0      1464    0,99    0,00    0,00    0,00    0,99     0  VBoxDRMClient
23:11:08     1000      1821    0,99    0,00    0,00    0,00    0,99     1  gnome-shell
23:11:08     1000      4651    0,00    0,99    0,00    0,00    0,99     1  gnome-terminal-

23:11:07      UID       PID  minflt/s  majflt/s     VSZ     RSS   %MEM  Command
23:11:08     1000      1821      0,00      0,00 4166368  411396  11,80  gnome-shell
23:11:08     1000      4651      0,00      0,99  636904   54452   1,56  gnome-terminal-
23:11:08     1000      8204      0,00      0,00 15690292  386752  11,10  Isolated Web Co

23:11:07      UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command
23:11:08     1000      4651      3,96      0,00      0,00       0  gnome-terminal-

23:11:08      UID       PID    %usr %system  %guest   %wait    %CPU   CPU  Command
23:11:09     1000      1821    3,00    1,00    0,00    0,00    4,00     1  gnome-shell
23:11:09     1000      4651    1,00    0,00    0,00    0,00    1,00     0  gnome-terminal-
23:11:09     1000      8204    1,00    0,00    0,00    0,00    1,00     1  Isolated Web Co
23:11:09     1000     16152    0,00    1,00    0,00    0,00    1,00     0  pidstat

23:11:08      UID       PID  minflt/s  majflt/s     VSZ     RSS   %MEM  Command
23:11:09     1000      1821    380,00     60,00 4173568  411396  11,80  gnome-shell
23:11:09     1000      8204     23,00      0,00 15690292  386880  11,10  Isolated Web Co

23:11:08      UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command

Среднее:   UID       PID    %usr %system  %guest   %wait    %CPU   CPU  Command
Среднее:   989       274    0,00    0,20    0,00    0,00    0,20     -  systemd-oomd
Среднее:     0      1464    0,20    0,00    0,00    0,00    0,20     -  VBoxDRMClient
Среднее:  1000      1821    6,75    3,17    0,00    2,18    9,92     -  gnome-shell
Среднее:  1000      2619    4,37    1,98    0,00    0,99    6,35     -  firefox
Среднее:  1000      3330    0,20    0,00    0,00    0,00    0,20     -  Isolated Web Co
Среднее:  1000      4651    0,60    0,40    0,00    0,20    0,99     -  gnome-terminal-
Среднее:  1000      5637    0,20    0,00    0,00    0,00    0,20     -  Isolated Web Co
Среднее:  1000      8204    1,59    0,00    0,00    0,20    1,59     -  Isolated Web Co
Среднее:  1000      9742    3,37    0,99    0,00    1,39    4,37     -  Isolated Web Co
Среднее:     0     13981    0,00    0,79    0,00    0,40    0,79     -  kworker/u8:0-events_unbound
Среднее:     0     15078    0,00    0,60    0,00    0,20    0,60     -  kworker/u8:3-kvfree_rcu_reclaim
Среднее:  1000     16152    0,20    0,79    0,00    0,00    0,99     -  pidstat

Среднее:   UID       PID  minflt/s  majflt/s     VSZ     RSS   %MEM  Command
Среднее:  1000      1821    657,74    124,60 4172128  411447  11,81  gnome-shell
Среднее:  1000      2619     10,71      0,00 12477480  647376  18,58  firefox
Среднее:  1000      3330      0,40      0,00 2744104  308716   8,86  Isolated Web Co
Среднее:  1000      4651      0,00      0,20  636904   54452   1,56  gnome-terminal-
Среднее:  1000      8204    208,53      0,00 15690295  386624  11,09  Isolated Web Co
Среднее:  1000     16152      0,99      0,00    7700    4308   0,12  pidstat

Среднее:   UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command
Среднее:  1000      2619      0,00    139,68      0,00       0  firefox
Среднее:  1000      4651      0,79      0,00      0,00       0  gnome-terminal-
```

- Показывает cтатистику за 5 интервалов по 1 секунде:
-u - использование CPU
-r - использование памяти
-d - дисковые операции I/O
Вывод включает: PID, %CPU, KB_rd/s (чтение), KB_wr/s (запись)

###Дисковый I/O (требует root):
sudo iotop -b -n 5 | head -n 30 | cat

###Вывод в консоли:
```bash
Total DISK READ:         4.89 K/s | Total DISK WRITE:         5.96 K/s
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
     11 be/0 root        0.00 B/s    0.00 B/s ?unavailable?  [kworker/0:0H-kblockd]
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
     31 be/0 root        0.00 B/s    0.00 B/s ?unavailable?  [kworker/R-inet_frag_wq]
```

- Показывает: Топ-30 процессов по дисковым операциям за 5 снимков.
-b - пакетный режим
-n 5 - 5 итераций
- Показывает: PID, пользователь, скорость чтения/записи

###Визуальный мониторинг: htop -u "$USER"
- Показывает интерактивный монитор процессов только для текущего пользователя.

###Итог: Топ процессов по CPU: Наибольшая нагрузка: gnome-shell, firefox, Isolated Web Co...
Топ процессов по памяти: firefox, gnome-shell...

Анализ проводился в виртуальной машине, что может незначительно влиять на показатели %CPU и I/O из-за виртуализации ресурсов.

##5) Мини‑утилита ptree (*)
Программа в src/ptree.py

###Вывод в консоли: 
```bash
python3(17301) ← bash(4659) ← gnome-terminal-(4651) ← systemd(1579) ← systemd(1)
```

- Утилита ptree анализирует цепочку родительских процессов, читая информацию из виртуальной файловой системы /proc. Для каждого процесса определяется его имя и PID родителя, после чего строится наглядная цепочка до системного инициализатора systemd(1).

##6) Вопросы для отчёта:

1. Программа — это файл с инструкциями на диске, а процесс — выполняющийся экземпляр программы в памяти со своими ресурсами, контекстом и PID.
2. Если вызвать fork() без wait(), то завершившийся дочерний процесс станет зомби, пока родитель не соберёт его статус; если родитель завершится раньше, то дети станут сиротами и будут переподчинены init/systemd.
3. Система хранит информацию о процессах в ядре в специальных структурах (task_struct), где содержатся PID, PPID, состояние, приоритет, память и другие данные; доступ к ним предоставляется через /proc и утилиты вроде ps и top.
4. Вызов exec() заменяет текущий процесс новым образом программы, оставляя PID тем же самым; это нужно для запуска другой программы в уже созданном процессе, например сразу после fork().
5. В /proc нет настоящих файлов, потому что это виртуальная файловая система: данные генерируются ядром динамически при обращении и отражают текущее состояние системы и процессов.
6. В top поле %CPU показывает долю процессорного времени, %MEM — долю оперативной памяти, VIRT — общий объём виртуальной памяти процесса, RES — реально занятую физическую память, SHR — разделяемую память, TIME+ — суммарное время работы на CPU.
7. Сумма %CPU может быть больше 100%, потому что это значение считается по всем ядрам, и на многопроцессорной системе каждый процесс может использовать одно или несколько ядер параллельно.
8. Мгновенное %CPU показывает нагрузку прямо сейчас, а load average — среднее количество процессов в очереди за 1, 5 и 15 минут; строка Cpu(s) в top показывает распределение использования процессора: пользовательское время, системное, idle (простой), wa (ожидание ввода-вывода) и другие категории.
9. IO-нагрузка отличается от CPU-нагрузки тем, что процесс тратит время на операции с диском или сетью вместо вычислений на процессоре; её можно наблюдать через pidstat -d, iotop или файлы /proc/<pid>/io.
10. Nice и приоритеты процессов определяют, сколько процессорного времени получит процесс: чем ниже nice (и выше приоритет), тем больше шансов у процесса быть выбранным планировщиком для выполнения.
11. Поток — это «лёгкий» процесс, разделяющий с другими потоками того же процесса память и ресурсы; увидеть их можно в ps с ключом -L или в top с опцией H.
12. Зомби — это процессы, которые уже завершились, но их статус ещё не был собран родителем, они висят как записи в таблице процессов; сироты — это процессы, у которых умер родитель, они переподчиняются init/systemd и продолжают выполняться.

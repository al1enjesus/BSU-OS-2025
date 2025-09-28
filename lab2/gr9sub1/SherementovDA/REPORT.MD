Отчёт по лабораторной работе 2
Процессы и файловая система /proc

1) Создание процессов

Для написания программы я использовал Python 

Код: src/fork_example.py

Вывод программы: parent: PID=13660
child[1]: PID=13662, PPID=13660
child[2]: PID=13663, PPID=13660
parent: both children completed. Exit codes: 0, 0
Вывод: родитель создал двух детей и дождался их завершения.

2) Исследование дерева процессов
Вывод команды ps -ef --forest:
yoshido+   18558   13500  0 17:21 pts/1    00:00:00  |   |   \_ grep --color=auto yoshidori
Вывод команды pstree -p:
bash(13500)───pstree(18678)
Вывод: все процессы получились видны.

3) Изучение /proc
PID текущей оболочки: 13500
cat /proc/13500/cmdline | tr '\0' ' '; echo: bash. Это командная строка запуска процессора
Вывод команды head -n 20 /proc/13500/status
Name:	bash
Umask:	0002
State:	S (sleeping)
Tgid:	13500
Ngid:	0
Pid:	13500
PPid:	13493
TracerPid:	0
Uid:	1000	1000	1000	1000
Gid:	1000	1000	1000	1000
FDSize:	256
Groups:	27 1000 
NStgid:	13500
NSpid:	13500
NSpgid:	13500
NSsid:	13500
Kthread:	0
VmPeak:	   12280 kB
VmSize:	   12280 kB
VmLck:	       0 kB
Команда показывает статус и информацию о процессе.
Вывод команды ls -l /proc/13500/fd: 
итого 0
lrwx------ 1 yoshidori yoshidori 64 Sep 28 14:29 0 -> /dev/pts/1
lrwx------ 1 yoshidori yoshidori 64 Sep 28 14:29 1 -> /dev/pts/1
lrwx------ 1 yoshidori yoshidori 64 Sep 28 14:29 2 -> /dev/pts/1
lrwx------ 1 yoshidori yoshidori 64 Sep 28 14:29 255 -> /dev/pts/1
Команда показывает открытые файловые дескрипторы.

4) Анализ процессов (нагрузка CPU/память/IO)
1. Моментальный срез top: 
top - 14:31:23 up  2:09,  2 users,  load average: 1.25, 0.86, 0.54
Задачи: 235 total,   3 running, 231 sleeping,   0 stopped,   1 zombie
%Cpu(s): 10.0 us,  2.0 sy,  0.0 ni, 82.0 id,  4.0 wa,  0.0 hi,  2.0 si,  0.0 st 
МиБ Mem :   3402.7 total,    118.9 free,   2383.0 used,   1011.5 buff/cache     
МиБ Swap:      0.0 total,      0.0 free,      0.0 used.   1019.6 avail Mem 

    PID USER      PR  NI    VIRT    RES    SHR S  %CPU  %MEM     TIME+
  12561 yoshido+  20   0 2961660 428028 105752 R  54.5  12.3   6:32.78
  11303 yoshido+  20   0   11.6g 650492 275872 S   9.1  18.7   8:00.68
  15396 yoshido+  20   0   13516   5536   3616 R   9.1   0.2   0:00.02
      1 root      20   0   25292  15540  10548 S   0.0   0.4   0:04.77
      2 root      20   0       0      0      0 S   0.0   0.0   0:00.04
      3 root      20   0       0      0      0 S   0.0   0.0   0:00.00
      4 root       0 -20       0      0      0 I   0.0   0.0   0:00.00
      5 root       0 -20       0      0      0 I   0.0   0.0   0:00.00
      6 root       0 -20       0      0      0 I   0.0   0.0   0:00.00
      7 root       0 -20       0      0      0 I   0.0   0.0   0:00.00
      8 root       0 -20       0      0      0 I   0.0   0.0   0:00.00
     11 root       0 -20       0      0      0 I   0.0   0.0   0:00.14
     13 root       0 -20       0      0      0 I   0.0   0.0   0:00.00
Показывает процессы с наибольшей нагрузкой на CPU.
Вывод команды ps -eo pid,ppid,comm,state,%cpu,%mem,etime --sort=-%cpu | head -n 15 | cat:
    PID    PPID COMMAND         S %CPU %MEM     ELAPSED
  15399   13500 ps              R 50.0  0.1       00:00
   2281    2031 gnome-shell     S 12.5 15.5    02:09:10
  11303    2281 firefox         S  6.4 18.6    02:04:57
  12561   11479 Isolated Web Co S  5.3 12.1    02:03:23
  15375       2 kworker/u16:4-e I  2.6  0.0       00:52
  15162       2 kworker/u16:1-e I  1.1  0.0       10:52
  14000    2031 nautilus        S  0.9  9.0    01:05:02
  12621   11479 Isolated Web Co S  0.6  6.0    02:03:04
  14635       2 kworker/u16:2-e I  0.4  0.0       42:26
  13493    2031 gnome-terminal- S  0.4  1.8    01:30:16
  14835       2 kworker/u16:3-e I  0.4  0.0       30:31
  12628   11479 Isolated Web Co S  0.3  5.8    02:03:04
  11507   11479 Privileged Cont S  0.3  4.0    02:04:53
  14553       2 kworker/u16:0-e I  0.2  0.0       49:30
Показывает топ-15 процессов по использованию CPU.
Вывод команды ps -eo pid,ppid,comm,state,%cpu,%mem,rss --sort=-%mem | head -n 15 | cat:
    PID    PPID COMMAND         S %CPU %MEM   RSS
  11303    2281 firefox         S  6.4 18.6 650492
   2281    2031 gnome-shell     S 12.5 15.5 543216
  12561   11479 Isolated Web Co R  5.3 12.1 425044
  14000    2031 nautilus        S  0.9  9.0 314356
  12621   11479 Isolated Web Co S  0.6  6.0 210044
  12628   11479 Isolated Web Co S  0.3  5.8 204724
  11507   11479 Privileged Cont S  0.3  4.0 141476
  11721   11479 WebExtensions   S  0.0  2.5 90544
  12722   11479 Web Content     S  0.1  2.0 70076
  12674   11479 Web Content     S  0.1  2.0 70008
  13403   11479 Web Content     S  0.1  1.9 69488
  15127    2281 gjs             S  0.2  1.8 65904
  13493    2031 gnome-terminal- S  0.4  1.8 64524
   2458    2245 evolution-alarm S  0.0  1.7 62264
Показывает топ-15 процессов по использованию памяти.

топ-5 по использованию CPU: 
    PID    PPID COMMAND         S %CPU     ELAPSED
   2281    2031 gnome-shell     S  9.9    05:49:55
  11303    2281 firefox         S  4.3    05:45:42
  12561   11479 Isolated Web Co S  4.2    05:44:09
  18902       2 kworker/u16:0-e I  0.6       21:30
  19126       2 kworker/u16:2-e I  0.6       04:54
gnome-shell -  перерисовывает интерфейс
firefox - открыто несколько вкладок
топ-5 по использованию памяти:    
PID COMMAND         %MEM   RSS
11303 firefox 17.6 599MB
2281 gnome-shell 15.6 532MB
12561 Isolated Web 0MB 9.7 340636
14000 nautilus 8.2 281MB
12628 Isolated Web 0MB 5.8 202832
firefox - кэш страниц
gnome-shell - графические буферы, темы, виджеты.
топ-5 по IO:
1 be/4 root        0.00 B/s    0.00 B/s ?unavailable?  init splash noprompt noshell automatic-ubiquity crashkernel=2G-4G:320M,4G-32G:512M,32G-64G:1024M,64G-128G:2048M,128G-:4096M
      2 be/4 root        0.00 B/s    0.00 B/s ?unavailable?  [kthreadd]
      3 be/4 root        0.00 B/s    0.00 B/s ?unavailable?  [pool_workqueue_release]
      4 be/0 root        0.00 B/s    0.00 B/s ?unavailable?  [kworker/R-rcu_gp]
      5 be/0 root        0.00 B/s    0.00 B/s ?unavailable?  [kworker/R-sync_wq]
IO-нагрузка минимальна.
5). Мини-утилита ptree
Цель: написать утилиту печатающую цепочку родителей от текущего процесса до PID1.
Программа написана на Python.
Код ptree.py.
Вывод программы: 
Process tree for PID 19384:
systemd(1) -- systemd(2031) -- gnome-terminal-(13493) -- bash(13500) -- python3(19384)

Ответы на вопросы: 
1) Программа - файл на диске. Процесс - запущенная программа в памяти с PID и контекстом. 
2) Дочерний процесс станет зомби - завершится, но останется в таблице процессов, пока родитель не прочитает статус.
3) В структурах ядра (task_struct) и виртуальной файловой системе /proс. Каждый процесс имеет папку /proc/<PID>/.
4) Заменяет код текущего процесса новой программой. Нужен для запуска других программ в том же процессе.
5) /proc - виртуальная файловая система. Файлы генерируются ядром при чтении, отражая текущее состояние системы.
6) %CPU - загрузка CPU процессом
   %MEM - доля физической памяти
   VIRT - виртуальная память
   RES - резидентная память
   SHR - разделяемая память
   TIME+ - общее время CPU
7) На многоядерных системах каждое ядро даёт до 100%. 4 ядра = до 400% суммарно.
8)  %CPU - текущая загрузка
   load average - средняя очередь процессов за 1, 5, 15 минут
   wa - % времени ожидания IO
9) CPU - вычисления, IO - дисковые операции.
   Команды: pidstat -d, iotop, cat /proc/<pid>/io
10) nice влияет на проиритет процесса, чем выше nice, тем ниже приоритет. 
11) Процессы - изолированы, потоки - разделяют память.
Можно увидеть с помощью команд: ps -eLf, top -H или /proc/<pid>/task/
12) Зомби - завершённые процессы, ожидающие wait()
    Сироты - процессы, чей родитель умер (усыновляются init)
    Исчезают после wait() или перезагрузки.

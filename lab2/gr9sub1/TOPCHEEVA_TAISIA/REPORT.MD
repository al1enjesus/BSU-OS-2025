# REPORT.MD

## Цель

Изучить модель процессов в Linux, освоить механизмы порождения и ожидания процессов, а также извлечение информации из `/proc`. Реализовать программы на Python для создания процессов и анализа их иерархии.

## Шаги

### 1. Создание процессов

Реализована программа на Python (`src/fork_example.py`), где родитель порождает двух дочерних процессов с помощью `os.fork()`. Каждый дочерний процесс выводит свой PID и PPID. Родитель ожидает их завершения с помощью `os.waitpid()`.

Код: см. `src/fork_example.py`.

Команда запуска:
```
python3 src/fork_example.py
```



**Вывод**:
```
Parent starting
child[0]: PID=9802, PPID=9801
child[1]: PID=9803, PPID=9801
Child 9802 exited with code 0
Child 9803 exited with code 0
Parent finished
```

Визуализация дерева процессов:
```
parent(PID=9801)
├─ child[0](PID=9802)
└─ child[1](PID=9803)
```

### 2. Исследование дерева процессов

Команда:
```
ps -ef --forest | head -n 30
```

**Вывод**:
```
UID          PID    PPID  C STIME TTY          TIME CMD
root           2       0  0 16:17 ?        00:00:00 [kthreadd]
root           3       2  0 16:17 ?        00:00:00  \_ [pool_workqueue_release]
root           4       2  0 16:17 ?        00:00:00  \_ [kworker/R-rcu_gp]
root           5       2  0 16:17 ?        00:00:00  \_ [kworker/R-sync_wq]
root           6       2  0 16:17 ?        00:00:00  \_ [kworker/R-kvfree_rcu_reclaim]
root           7       2  0 16:17 ?        00:00:00  \_ [kworker/R-slub_flushwq]
root           8       2  0 16:17 ?        00:00:00  \_ [kworker/R-netns]
root          10       2  0 16:17 ?        00:00:00  \_ [kworker/0:0H-events_highpri]
root          13       2  0 16:17 ?        00:00:00  \_ [kworker/R-mm_percpu_wq]
root          14       2  0 16:17 ?        00:00:00  \_ [rcu_tasks_kthread]
root          15       2  0 16:17 ?        00:00:00  \_ [rcu_tasks_rude_kthread]
root          16       2  0 16:17 ?        00:00:00  \_ [rcu_tasks_trace_kthread]
root          17       2  0 16:17 ?        00:00:00  \_ [ksoftirqd/0]
root          18       2  0 16:17 ?        00:00:01  \_ [rcu_preempt]
root          19       2  0 16:17 ?        00:00:00  \_ [rcu_exp_par_gp_kthread_worker/0]
root          20       2  0 16:17 ?        00:00:00  \_ [rcu_exp_gp_kthread_worker]
root          21       2  0 16:17 ?        00:00:00  \_ [migration/0]
root          22       2  0 16:17 ?        00:00:00  \_ [idle_inject/0]
root          23       2  0 16:17 ?        00:00:00  \_ [cpuhp/0]
root          24       2  0 16:17 ?        00:00:00  \_ [cpuhp/1]
root          25       2  0 16:17 ?        00:00:00  \_ [idle_inject/1]
root          26       2  0 16:17 ?        00:00:00  \_ [migration/1]
root          27       2  0 16:17 ?        00:00:00  \_ [ksoftirqd/1]
root          29       2  0 16:17 ?        00:00:00  \_ [kworker/1:0H-events_highpri]
root          30       2  0 16:17 ?        00:00:00  \_ [kdevtmpfs]
root          31       2  0 16:17 ?        00:00:00  \_ [kworker/R-inet_frag_wq]
root          32       2  0 16:17 ?        00:00:00  \_ [kauditd]
root          33       2  0 16:17 ?        00:00:00  \_ [khungtaskd]
root          35       2  0 16:17 ?        00:00:00  \_ [oom_reaper]
```

**Анализ**: Мой процесс (`bash`, PID=3092) — потомок `gnome-terminal-` (PID=3084), который подчинён `systemd` (PID=1776), а тот — `systemd` (PID=1). Это подтверждает иерархию процессов в Linux.

Команда:
```
pstree -p | head -n 50
```

**Вывод**:
```
systemd(1)-+-ModemManager(1615)-+-{ModemManager}(1635)
           |                    |-{ModemManager}(1637)
           |                    `-{ModemManager}(1640)
           |-NetworkManager(1555)-+-{NetworkManager}(1642)
           |                      |-{NetworkManager}(1643)
           |                      `-{NetworkManager}(1644)
           |-VGAuthService(1159)
           |-accounts-daemon(1446)-+-{accounts-daemon}(1504)
           |                       |-{accounts-daemon}(1505)
           |                       `-{accounts-daemon}(1559)
           |-avahi-daemon(1300)---avahi-daemon(1522)
           |-colord(2097)-+-{colord}(2104)
           |              |-{colord}(2105)
           |              `-{colord}(2107)
           |-cron(1448)
           |-cups-browsed(6137)-+-{cups-browsed}(6146)
           |                    |-{cups-browsed}(6147)
           |                    `-{cups-browsed}(6148)
           |-cupsd(6136)
           |-dbus-daemon(1304)
           |-fwupd(2962)-+-{fwupd}(2967)
           |             |-{fwupd}(2968)
           |             `-{fwupd}(2969)
           |-gdm3(1761)-+-gdm-session-wor(1768)-+-gdm-wayland-ses(1837)-+-gnome-session-b(1846)-+-{gnome-session-b}(1921)
           |            |                       |                       |                       |-{gnome-session-b}(1922)
           |            |                       |                       |                       `-{gnome-session-b}(1923)
           |            |                       |                       |-{gdm-wayland-ses}(1840)
           |            |                       |                       |-{gdm-wayland-ses}(1841)
           |            |                       |                       `-{gdm-wayland-ses}(1842)
           |            |                       |-{gdm-session-wor}(1769)
           |            |                       |-{gdm-session-wor}(1770)
           |            |                       `-{gdm-session-wor}(1772)
           |            |-{gdm3}(1762)
           |            |-{gdm3}(1763)
           |            `-{gdm3}(1764)
           |-gnome-remote-de(1361)-+-{gnome-remote-de}(1568)
           |                       |-{gnome-remote-de}(1572)
           |                       `-{gnome-remote-de}(1581)
           |-polkitd(1381)-+-{polkitd}(1550)
           |               |-{polkitd}(1551)
           |               `-{polkitd}(1564)
           |-power-profiles-(2796)-+-{power-profiles-}(2801)
           |                       |-{power-profiles-}(2802)
           |                       `-{power-profiles-}(2803)
           |-rsyslogd(1517)-+-{rsyslogd}(1547)
           |                |-{rsyslogd}(1548)
           |                `-{rsyslogd}(1549)
           |-rtkit-daemon(1830)-+-{rtkit-daemon}(1843)
           |                    `-{rtkit-daemon}(1844)
           |-snapd(6196)-+-{snapd}(6205)
```

**Сравнение**: `ps --forest` показывает текстовую иерархию процессов, `pstree` — графическую, более наглядную. Мой процесс (`bash`, PID=3092) — потомок `gnome-terminal-`, который связан с `systemd`.

### 3. Изучение /proc

PID текущей оболочки:
```
echo $$
```
**Вывод**: 3092

- **cmdline**:
  ```
  cat /proc/3092/cmdline | tr '\0' ' '; echo
  ```
  **Вывод**:
  ```
  bash
  ```
  **Пояснение**: Показывает командную строку запуска процесса (`bash`).

- **status**:
  ```
  head -n 20 /proc/3092/status
  ```
  **Вывод**:
  ```
  Name:	bash
  Umask:	0002
  State:	S (sleeping)
  Tgid:	3092
  Ngid:	0
  Pid:	3092
  PPid:	3084
  TracerPid:	0
  Uid:	1000	1000	1000	1000
  Gid:	1000	1000	1000	1000
  FDSize:	256
  Groups:	4 24 27 30 46 100 115 1000 
  NStgid:	3092
  NSpid:	3092
  NSpgid:	3092
  NSsid:	3092
  Kthread:	0
  VmPeak:	   10844 kB
  VmSize:	   10844 kB
  VmLck:	       0 kB
  ```
  **Пояснение**: Содержит имя процесса (`bash`), состояние (`S` — sleeping), PPID (`3084` — `gnome-terminal-`), UID/GID (1000 — пользователь `tayjie`), использование памяти и другие параметры.

- **fd**:
  ```
  ls -l /proc/3092/fd
  ```
  **Вывод**:
  ```
  lrwx------ 1 tayjie tayjie 64 сен 19 13:34 0 -> /dev/pts/0
  lrwx------ 1 tayjie tayjie 64 сен 19 13:34 1 -> /dev/pts/0
  lrwx------ 1 tayjie tayjie 64 сен 19 13:34 2 -> /dev/pts/0
  lrwx------ 1 tayjie tayjie 64 сен 19 13:34 255 -> /dev/pts/0
  ```
  **Пояснение**: Показывает открытые файловые дескрипторы: 0 (stdin), 1 (stdout), 2 (stderr), 255 (дополнительный дескриптор, связанный с терминалом).

### 4. Анализ процессов (нагрузка CPU/память/IO)

Команда:
```
top -b -n 1 | head -n 20
```
**Вывод**:
```
top - 23:19:51 up  7:02,  2 users,  load average: 0,00, 0,00, 0,00
Задачи: 299 total,   1 running, 298 sleeping,   0 stopped,   0 zombie
%Cpu(s):  0,0 us,  4,8 sy,  0,0 ni, 95,2 id,  0,0 wa,  0,0 hi,  0,0 si,  0,0 st 
МиБ Mem :   3387,3 total,    718,6 free,   1252,7 used,   1627,0 buff/cache     
МиБ Swap:   3259,0 total,   3259,0 free,      0,0 used.   2134,6 avail Mem 

    PID USER      PR  NI    VIRT    RES    SHR S  %CPU  %MEM     TIME+
  10073 tayjie    20   0   10344   4752   2832 R   9,1   0,1   0:00.01
      1 root      20   0   25388  14500   9508 S   0,0   0,4   0:05.11
      2 root      20   0       0      0      0 S   0,0   0,0   0:00.01
      3 root      20   0       0      0      0 S   0,0   0,0   0:00.00
      4 root       0 -20       0      0      0 I   0,0   0,0   0:00.00
      5 root       0 -20       0      0      0 I   0,0   0,0   0:00.00
      6 root       0 -20       0      0      0 I   0,0   0,0   0:00.00
      7 root       0 -20       0      0      0 I   0,0   0,0   0:00.00
      8 root       0 -20       0      0      0 I   0,0   0,0   0:00.00
     10 root       0 -20       0      0      0 I   0,0   0,0   0:00.00
     13 root       0 -20       0      0      0 I   0,0   0,0   0:00.00
     14 root      20   0       0      0      0 I   0,0   0,0   0:00.00
     15 root      20   0       0      0      0 I   0,0   0,0   0:00.00
```

**TOP-5 по CPU**:
```
    PID    PPID COMMAND         S %CPU %MEM     ELAPSED
  10292    3092 ps              R  100  0.1       00:00
   6196       1 snapd           S  1.0  1.2    03:12:09
   2003    1776 gnome-shell     S  0.3  8.3    07:03:07
   1793    1776 pipewire        S  0.2  0.3    07:03:07
   2232    1776 vmtoolsd        S  0.1  1.1    07:03:06
```
1. **PID: 10292, COMMAND: ps, %CPU: 100, ELAPSED: 00:00** — команда `ps` выполнялась в момент замера, поэтому показала высокую загрузку CPU (кратковременная).
2. **PID: 6196, COMMAND: snapd, %CPU: 1.0, ELAPSED: 03:12:09** — демон управления пакетами Snap, работает в фоновом режиме для обновления приложений.
3. **PID: 2003, COMMAND: gnome-shell, %CPU: 0.3, ELAPSED: 07:03:07** — графическая оболочка GNOME, нагружает CPU для рендеринга интерфейса и анимаций.
4. **PID: 1793, COMMAND: pipewire, %CPU: 0.2, ELAPSED: 07:03:07** — сервер мультимедиа, обрабатывает аудио и видео в системе.
5. **PID: 2232, COMMAND: vmtoolsd, %CPU: 0.1, ELAPSED: 07:03:06** — демон VMware Tools, используется для интеграции с виртуальной машиной (например, синхронизация времени).

**TOP-5 по памяти**:
```
    PID    PPID COMMAND         S %CPU %MEM   RSS
   2003    1776 gnome-shell     S  0.3  8.3 289240
   2624    2003 mutter-x11-fram S  0.0  2.8 97124
   2485    2003 Xwayland        S  0.0  2.5 89772
   2599    1776 gsd-xsettings   S  0.0  2.3 83224
   2206    1961 evolution-alarm S  0.0  1.8 63428
```
1. **PID: 2003, COMMAND: gnome-shell, %MEM: 8.3, RSS: 289240 KiB** — GNOME Shell, основной потребитель памяти из-за графического интерфейса и множества открытых приложений.
2. **PID: 2624, COMMAND: mutter-x11-fram, %MEM: 2.8, RSS: 97124 KiB** — оконный менеджер Mutter, часть GNOME, отвечает за управление окнами.
3. **PID: 2485, COMMAND: Xwayland, %MEM: 2.5, RSS: 89772 KiB** — сервер X11 для совместимости приложений с Wayland.
4. **PID: 2599, COMMAND: gsd-xsettings, %MEM: 2.3, RSS: 83224 KiB** — демон GNOME для управления настройками дисплея и тем.
5. **PID: 2206, COMMAND: evolution-alarm, %MEM: 1.8, RSS: 63428 KiB** — уведомления календаря Evolution, хранит данные о событиях.

**pidstat**:
```
pidstat -u -r -d 1 5
```
**Вывод**:
```
Linux 6.14.0-29-generic (tayjie) 	21.09.2025 	_aarch64_	(2 CPU)

23:21:37      UID       PID    %usr %system  %guest   %wait    %CPU   CPU  Command
23:21:38     1000      2003    2,97    2,97    0,00    0,00    5,94     1  gnome-shell
23:21:38     1000      2493    0,99    0,00    0,00    0,00    0,99     0  ibus-engine-sim
23:21:38     1000      3084    1,98    0,00    0,00    0,00    1,98     0  gnome-terminal-
23:21:38     1000     10300    0,00    0,99    0,00    0,00    0,99     0  pidstat

23:21:37      UID       PID  minflt/s  majflt/s     VSZ     RSS   %MEM  Command
23:21:38     1000      2003     23,76      0,00 3856012  289028   8,33  gnome-shell
23:21:38     1000     10300      1,98      0,00    7984    4576   0,13  pidstat

23:21:37      UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command

23:21:38      UID       PID    %usr %system  %guest   %wait    %CPU   CPU  Command
23:21:39        0      1161    0,00    1,00    0,00    0,00    1,00     0  vmtoolsd
23:21:39     1000      2003    6,00    3,00    0,00    1,00    9,00     1  gnome-shell
23:21:39     1000      3084    2,00    0,00    0,00    0,00    2,00     1  gnome-terminal-
23:21:39        0      9927    0,00    1,00    0,00    0,00    1,00     1  kworker/u8:0-events_unbound
23:21:39     1000     10300    0,00    1,00    0,00    0,00    1,00     0  pidstat

23:21:38      UID       PID  minflt/s  majflt/s     VSZ     RSS   %MEM  Command
23:21:39     1000      2003     32,00      0,00 3856012  289028   8,33  gnome-shell
23:21:39     1000      3064      2,00      0,00   41576   29664   0,86  python3

23:21:38      UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command

23:21:39      UID       PID    %usr %system  %guest   %wait    %CPU   CPU  Command
23:21:40     1000      2003    3,00    2,00    0,00    1,00    5,00     1  gnome-shell
23:21:40     1000      3084    1,00    1,00    0,00    0,00    2,00     0  gnome-terminal-
23:21:40     1000     10300    1,00    1,00    0,00    0,00    2,00     0  pidstat

23:21:39      UID       PID  minflt/s  majflt/s     VSZ     RSS   %MEM  Command
23:21:40     1000      2003     32,00      0,00 3856012  289028   8,33  gnome-shell

23:21:39      UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command

23:21:40      UID       PID    %usr %system  %guest   %wait    %CPU   CPU  Command
23:21:41        0      1161    1,00    0,00    0,00    0,00    1,00     0  vmtoolsd
23:21:41     1000      2003    6,00    2,00    0,00    0,00    8,00     0  gnome-shell
23:21:41     1000      2167    1,00    0,00    0,00    0,00    1,00     0  ibus-daemon
23:21:41     1000      2232    0,00    1,00    0,00    0,00    1,00     1  vmtoolsd
23:21:41     1000      3084    4,00    0,00    0,00    0,00    4,00     1  gnome-terminal-
23:21:41        0      8945    0,00    1,00    0,00    0,00    1,00     0  kworker/u8:2-events_unbound

23:21:40      UID       PID  minflt/s  majflt/s     VSZ     RSS   %MEM  Command
23:21:41     1000      2003      2,00      0,00 3856012  289028   8,33  gnome-shell
23:21:41     1000      3084      1,00      0,00  713852   60636   1,75  gnome-terminal-

23:21:40      UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command

23:21:41      UID       PID    %usr %system  %guest   %wait    %CPU   CPU  Command
23:21:42        0       413    0,00    1,00    0,00    0,00    1,00     0  irq/59-vmwgfx
23:21:42     1000      2003    8,00    3,00    0,00    0,00   11,00     1  gnome-shell
23:21:42     1000      3084    6,00    0,00    0,00    1,00    6,00     1  gnome-terminal-
23:21:42     1000     10300    0,00    1,00    0,00    0,00    1,00     0  pidstat

23:21:41      UID       PID  minflt/s  majflt/s     VSZ     RSS   %MEM  Command
23:21:41     1000      2003      2,00      0,00 3856012  289028   8,33  gnome-shell
23:21:41     1000      3084      1,00      0,00  713852   60636   1,75  gnome-terminal-

23:21:41      UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command

Среднее:   UID       PID    %usr %system  %guest   %wait    %CPU   CPU  Command
Среднее:     0       413    0,00    0,20    0,00    0,00    0,20     -  irq/59-vmwgfx
Среднее:     0      1161    0,20    0,20    0,00    0,00    0,40     -  vmtoolsd
Среднее:  1000      2003    5,19    2,59    0,00    0,40    7,78     -  gnome-shell
Среднее:  1000      2167    0,20    0,00    0,00    0,00    0,20     -  ibus-daemon
Среднее:  1000      2232    0,00    0,20    0,00    0,00    0,20     -  vmtoolsd
Среднее:  1000      2493    0,20    0,00    0,00    0,00    0,20     -  ibus-engine-sim
Среднее:  1000      3084    2,99    0,20    0,00    0,20    3,19     -  gnome-terminal-
Среднее:     0      8945    0,00    0,20    0,00    0,00    0,20     -  kworker/u8:2-events_unbound
Среднее:     0      9927    0,00    0,20    0,00    0,00    0,20     -  kworker/u8:0-kvfree_rcu_reclaim
Среднее:  1000     10300    0,20    0,80    0,00    0,00    1,00     -  pidstat

Среднее:   UID       PID  minflt/s  majflt/s     VSZ     RSS   %MEM  Command
Среднее:  1000      2003     17,96      0,00 3856012  289028   8,33  gnome-shell
Среднее:  1000      3064      0,40      0,00   41576   29664   0,86  python3
Среднее:  1000      3084      0,20      0,00  713852   60636   1,75  gnome-terminal-
Среднее:  1000     10300      0,40      0,00    7984    4576   0,13  pidstat

Среднее:   UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command
```

**Анализ**: Нагрузка CPU в основном создаётся `gnome-shell` (графический интерфейс) и `gnome-terminal-` (терминал). Память активно используется `gnome-shell` и связанными процессами (`mutter-x11-fram`, `Xwayland`), что ожидаемо для графической среды. IO-нагрузка минимальна, так как `iotop` показывает 0 B/s.

**(Опционально) TOP-5 по IO**:
```
sudo iotop -b -n 5 | head -n 30
```
**Вывод**:
```
Total DISK READ:         0.00 B/s | Total DISK WRITE:         0.00 B/s
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
    zilch     10 be/0 root        0.00 B/s    0.00 B/s ?unavailable?  [kworker/0:0H-events_highpri]
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

**Анализ**: IO-нагрузка минимальна (0 B/s). Основные процессы (`gnome-shell`, `gnome-terminal-`) нагружают CPU и память, но не IO.

**Выводы**: Система в основном нагружена графической оболочкой (`gnome-shell`) и связанными процессами (`mutter-x11-fram`, `Xwayland`) из-за рендеринга интерфейса. Низкая IO-нагрузка указывает на отсутствие интенсивных дисковых операций в момент замера. 

### 5. Мини-утилита ptree

Реализована в `src/ptree.py`. Выводит цепочку родителей от текущего процесса до PID 1, читая PPID из `/proc/<pid>/status`.

Код: см. `src/ptree.py`.

Команда:
```
python3 src/ptree.py
```

**Вывод**:
```
systemd(1) ← systemd(1776) ← gnome-terminal-(3084) ← bash(3092) ← python3(9798)
```

**Вывод run.sh (включает ptree)**:
```
Running fork_example.py
Parent starting
child[0]: PID=9802, PPID=9801
child[1]: PID=9803, PPID=9801
Child 9802 exited with code 0
Child 9803 exited with code 0
Parent finished
Running ptree.py
systemd(1) ← systemd(1776) ← gnome-terminal-(3084) ← bash(3092) ← run.sh(9800) ← python3(9804)
```

## Выводы

Освоила создание процессов на Python с использованием `os.fork` и `os.waitpid`. Изучила `/proc` и методы анализа нагрузки системы (`ps`, `pstree`, `top`, `pidstat`, `iotop`). Утилита `ptree` корректно отображает иерархию процессов от текущего до `systemd`.Проверка `ps -ef | grep Z` показала отсутствие зомби-процессов, что подтверждает корректное завершение дочерних процессов.

## Ответы на вопросы

1. **Чем процесс отличается от программы?**  
   Программа — статический исполняемый файл на диске. Процесс — экземпляр программы, выполняющийся в памяти с уникальным PID, контекстом и ресурсами.

2. **Что будет, если вызвать fork() без wait()?**  
   Дочерний процесс может стать зомби (если завершится, но родитель не соберёт его статус) или сиротой (если родитель завершится раньше), усыновляется `init`/`systemd`.

3. **Как система хранит информацию о процессах?**  
   В структурах данных ядра (`task_struct`), доступных через виртуальную файловую систему `/proc`.

4. **Что делает exec() и зачем он нужен?**  
   Заменяет текущий образ процесса новой программой, сохраняя PID. Используется для запуска новых программ после `fork` (например, в shell).

5. **Почему в /proc нет «настоящих» файлов?**  
   `/proc` — виртуальная файловая система, генерируемая ядром в реальном времени для предоставления информации о процессах.

6. **Как интерпретировать поля top: %CPU, %MEM, VIRT, RES, SHR, TIME+?**  
   - %CPU — процент использования CPU.  
   - %MEM — процент памяти.  
   - VIRT — виртуальная память.  
   - RES — резидентная (физическая) память.  
   - SHR — разделяемая память.  
   - TIME+ — общее время CPU.

7. **Почему сумма %CPU может быть больше 100%?**  
   На многоядерных системах %CPU считается на ядро (2 ядра = до 200%).

8. **Чем отличается мгновенное %CPU от load average? Что означает строка Cpu(s) в top (в т.ч. wa)?**  
   %CPU — моментальная загрузка, load average — средняя очередь задач за 1/5/15 минут. Cpu(s): `us` (пользователь), `sy` (система), `ni` (nice), `id` (idle), `wa` (ожидание IO), `hi`/`si` (прерывания), `st` (steal).

9. **Чем IO-нагрузка отличается от CPU-нагрузки и как её увидеть?**  
   IO — ожидание операций ввода-вывода (диск/сеть), CPU — вычисления. Увидеть IO: `pidstat -d`, `iotop`, `/proc/<pid>/io`.

10. **Что такое nice/приоритеты процессов и как они влияют на планирование?**  
    Nice — приоритет от -20 (высокий) до 19 (низкий). Планировщик CFS выделяет больше CPU-времени процессам с низким nice.

11. **Чем поток отличается от процесса и как увидеть потоки в ps/top?**  
    Поток — легковесный процесс, делит память с родительским процессом (один PID, разные TID). Увидеть: `ps -eLf` (LWP), `top -H` (потоки).

12. **Что такое зомби и сироты, как они возникают и куда «деваются»?**  
    Зомби — завершённый процесс, ждущий `wait()` от родителя (состояние Z). Сирота — процесс, чей родитель завершился, усыновляется `init`. Зомби исчезают после `wait()` или усыновления.

## Проверка

Выполнено в Ubuntu . Программы работают корректно, зомби-процессы отсутствуют (`ps -ef | grep Z` показал только `grep`).

## Использование AI

AI использовался для помощи с написанием отчета и пониманием использования некоторых команд.

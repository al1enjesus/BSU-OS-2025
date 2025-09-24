# Лабораторная работа 2 — Процессы и файловая система /proc

## Цель работы
Понять модель процессов Linux, принципы порождения и ожидания завершения, а также научиться извлекать информацию из `/proc`.

## Ход работы

### 1) Создание процессов

**Код программы на C:**
```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t child1, child2;
    int status1, status2;
    
    printf("parent: PID=%d, starting...\n", getpid());
    fflush(stdout);
    
    child1 = fork();
    if (child1 == 0) {
        // Первый дочерний процесс
        printf("child_A: PID=%d, PPID=%d\n", getpid(), getppid());
        fflush(stdout);
        sleep(2);
        exit(0);
    }
    
    child2 = fork();
    if (child2 == 0) {
        // Второй дочерний процесс
        printf("child_B: PID=%d, PPID=%d\n", getpid(), getppid());
        fflush(stdout);
        sleep(1);
        exit(0);
    }
    
    // Родительский процесс ожидает завершения детей
    waitpid(child1, &status1, 0);
    waitpid(child2, &status2, 0);
    
    printf("parent: both children completed\n");
    printf("child_A exit code: %d\n", WEXITSTATUS(status1));
    printf("child_B exit code: %d\n", WEXITSTATUS(status2));
    
    return 0;
}
```

**Компиляция и запуск:**
```bash
gcc -Wall -Wextra -O2 ~/labs/lr2/example.c -o example
./example
```

**Вывод программы:**
```
parent: PID=3569, starting...
child_A: PID=3570, PPID=3569
child_B: PID=3571, PPID=3569
parent: both children completed
child_A exit code: 0
child_B exit code: 0
```

### 2) Исследование дерева процессов

**Команды для анализа дерева процессов:**

```bash
ps -ef --forest | head -n 30
```

```
UID          PID    PPID  C STIME TTY          TIME CMD
root           2       0  0 08:55 ?        00:00:00 [kthreadd]
root           3       2  0 08:55 ?        00:00:00  \_ [pool_workqueue_release]
root           4       2  0 08:55 ?        00:00:00  \_ [kworker/R-rcu_gp]
root           5       2  0 08:55 ?        00:00:00  \_ [kworker/R-sync_wq]
root           6       2  0 08:55 ?        00:00:00  \_ [kworker/R-kvfree_rcu_reclaim]
root           7       2  0 08:55 ?        00:00:00  \_ [kworker/R-slub_flushwq]
root           8       2  0 08:55 ?        00:00:00  \_ [kworker/R-netns]
root           9       2  0 08:55 ?        00:00:03  \_ [kworker/0:0-events]
root          13       2  0 08:55 ?        00:00:00  \_ [kworker/R-mm_percpu_wq]
root          14       2  0 08:55 ?        00:00:00  \_ [rcu_tasks_kthread]
root          15       2  0 08:55 ?        00:00:00  \_ [rcu_tasks_rude_kthread]
root          16       2  0 08:55 ?        00:00:00  \_ [rcu_tasks_trace_kthread]
root          17       2  0 08:55 ?        00:00:01  \_ [ksoftirqd/0]
root          18       2  0 08:55 ?        00:00:00  \_ [rcu_preempt]
root          19       2  0 08:55 ?        00:00:00  \_ [rcu_exp_par_gp_kthread_worker/0]
root          20       2  0 08:55 ?        00:00:00  \_ [rcu_exp_gp_kthread_worker]
root          21       2  0 08:55 ?        00:00:00  \_ [migration/0]
root          22       2  0 08:55 ?        00:00:00  \_ [idle_inject/0]
root          23       2  0 08:55 ?        00:00:00  \_ [cpuhp/0]
root          24       2  0 08:55 ?        00:00:00  \_ [kdevtmpfs]
root          25       2  0 08:55 ?        00:00:00  \_ [kworker/R-inet_frag_wq]
root          26       2  0 08:55 ?        00:00:00  \_ [kauditd]
root          27       2  0 08:55 ?        00:00:00  \_ [khungtaskd]
root          29       2  0 08:55 ?        00:00:00  \_ [oom_reaper]
root          30       2  0 08:55 ?        00:00:02  \_ [kworker/u4:2-kvfree_rcu_reclaim]
root          31       2  0 08:55 ?        00:00:00  \_ [kworker/R-writeback]
root          32       2  0 08:55 ?        00:00:00  \_ [kcompactd0]
root          33       2  0 08:55 ?        00:00:00  \_ [ksmd]
root          34       2  0 08:55 ?        00:00:00  \_ [khugepaged]

```

```bash
pstree -p | head -n 50
```

```
systemd(1)-+-ModemManager(772)-+-{ModemManager}(802)
           |                   |-{ModemManager}(806)
           |                   `-{ModemManager}(809)
           |-NetworkManager(707)-+-{NetworkManager}(792)
           |                     |-{NetworkManager}(793)
           |                     `-{NetworkManager}(794)
           |-VBoxDRMClient(1249)-+-{VBoxDRMClient}(1257)
           |                     |-{VBoxDRMClient}(1258)
           |                     |-{VBoxDRMClient}(1259)
           |                     |-{VBoxDRMClient}(1260)
           |                     `-{VBoxDRMClient}(2841)
           |-VBoxService(1252)-+-{VBoxService}(1261)
           |                   |-{VBoxService}(1262)
           |                   |-{VBoxService}(1263)
           |                   |-{VBoxService}(1266)
           |                   |-{VBoxService}(1267)
           |                   |-{VBoxService}(1269)
           |                   |-{VBoxService}(1272)
           |                   `-{VBoxService}(1274)
           |-accounts-daemon(620)-+-{accounts-daemon}(655)
           |                      |-{accounts-daemon}(656)
           |                      `-{accounts-daemon}(658)
           |-avahi-daemon(560)---avahi-daemon(692)
           |-colord(1471)-+-{colord}(1477)
           |              |-{colord}(1478)
           |              `-{colord}(1480)
           |-cron(625)
           |-cups-browsed(1287)-+-{cups-browsed}(1317)
           |                    |-{cups-browsed}(1318)
           |                    `-{cups-browsed}(1319)
           |-cupsd(1231)
           |-dbus-daemon(562)
           |-fwupd(3384)-+-{fwupd}(3385)
           |             |-{fwupd}(3386)
           |             |-{fwupd}(3387)
           |             |-{fwupd}(3388)
           |             `-{fwupd}(3390)
           |-gdm3(1307)-+-gdm-session-wor(1837)-+-gdm-wayland-ses(1933)-+-gnome-session-b(1958)-+-{gnome-session-b}(2025)
           |            |                       |                       |                       |-{gnome-session-b}(2026)
           |            |                       |                       |                       `-{gnome-session-b}(2027)
           |            |                       |                       |-{gdm-wayland-ses}(1948)
           |            |                       |                       |-{gdm-wayland-ses}(1950)
           |            |                       |                       `-{gdm-wayland-ses}(1956)
           |            |                       |-{gdm-session-wor}(1838)
           |            |                       |-{gdm-session-wor}(1839)
           |            |                       `-{gdm-session-wor}(1841)
           |            |-{gdm3}(1312)
           |            |-{gdm3}(1313)
           |            `-{gdm3}(1314)
           |-gnome-remote-de(581)-+-{gnome-remote-de}(779)

```

### 3) Изучение /proc

**Анализ текущей оболочки:**
```bash
echo $$
```
```
3545
```

**Исследование файлов в /proc:**
```bash
cat /proc/3545/cmdline | tr '\0' ' '; echo
```
Вывод: `bash` — показывает командную строку запуска процесса

```bash
head -n 20 /proc/3545/status
```
```
Name:	bash
Umask:	0002
State:	S (sleeping)
Tgid:	3545
Ngid:	0
Pid:	3545
PPid:	3537
TracerPid:	0
Uid:	1000	1000	1000	1000
Gid:	1000	1000	1000	1000
FDSize:	256
Groups:	4 24 27 30 46 100 114 1000 
NStgid:	3545
NSpid:	3545
NSpgid:	3545
NSsid:	3545
Kthread:	0
VmPeak:	   19932 kB
VmSize:	   19932 kB
VmLck:	       0 kB
```

```bash
ls -l /proc/3545/fd
```
```
итого 0
lrwx------ 1 daberauoy daberauoy 64 сен 24 09:15 0 -> /dev/pts/0
lrwx------ 1 daberauoy daberauoy 64 сен 24 09:15 1 -> /dev/pts/0
lrwx------ 1 daberauoy daberauoy 64 сен 24 09:15 2 -> /dev/pts/0
lrwx------ 1 daberauoy daberauoy 64 сен 24 09:15 255 -> /dev/pts/0
```

**Пояснения:**
- `/proc/<pid>/cmdline` — командная строка запуска процесса
- `/proc/<pid>/status` — подробная информация о состоянии процесса
- `/proc/<pid>/fd/` — открытые файловые дескрипторы процесса

### 4) Анализ процессов (нагрузка CPU/память/IO)

**Топ процессов по CPU:**
```bash
ps -eo pid,ppid,comm,state,%cpu,%mem,etime --sort=-%cpu | head -n 15
```
```
    PID    PPID COMMAND         S %CPU %MEM     ELAPSED
   3635    3545 ps              R 53.8  0.0       00:00
   3636    3545 head            S  7.6  0.0       00:00
   2089    1863 gnome-shell     S  5.1  8.4       19:12
   3140    1863 nautilus        S  0.7  5.5       17:18
   3537    1863 gnome-terminal- S  0.6  1.1       09:48
   3384       1 fwupd           S  0.6  0.8       15:59
   2416    2212 ibus-extension- S  0.6  0.6       18:56
   1875    1863 pipewire        S  0.5  0.3       19:19
      1       0 systemd         S  0.4  0.2       21:35
    618       1 snapd           S  0.4  0.8       20:57
     57       2 kworker/u4:5-ev I  0.3  0.0       21:33
      9       2 kworker/0:0-eve I  0.3  0.0       21:34
     30       2 kworker/u4:2-fl I  0.2  0.0       21:34
     55       2 kworker/u4:3-ex I  0.2  0.0       21:33
```

**Топ процессов по памяти:**
```bash
ps -eo pid,ppid,comm,state,%cpu,%mem,rss --sort=-%mem | head -n 15
```
```
    PID    PPID COMMAND         S %CPU %MEM   RSS
   2089    1863 gnome-shell     S  5.1  8.4 416468
   3140    1863 nautilus        S  0.7  5.5 273228
   2813    2089 mutter-x11-fram S  0.0  2.0 98520
   2680    1863 gsd-xsettings   S  0.0  1.6 81092
   2540    2089 Xwayland        S  0.0  1.3 68648
   2963    2089 gjs             S  0.1  1.3 65316
   2264    2053 evolution-alarm S  0.0  1.3 65112
   3537    1863 gnome-terminal- S  0.6  1.1 57540
   2202    1863 evolution-sourc S  0.0  0.8 43212
   3384       1 fwupd           S  0.6  0.8 42940
   2788    1863 xdg-desktop-por S  0.0  0.8 40164
    618       1 snapd           S  0.4  0.8 39368
   3003    2053 update-notifier S  0.0  0.6 32752
   2416    2212 ibus-extension- S  0.5  0.6 30916
```

**Мониторинг в реальном времени:**
```bash
pidstat -u -r -d 1 5
```
```
Linux 6.14.0-29-generic (ubuntu-vm) 	24.09.2025 	_x86_64_	(1 CPU)

Среднее:   UID       PID    %usr %system  %guest   %wait    %CPU   CPU  Command
Среднее:     0         9    0,00    0,20    0,00    0,59    0,20     -  kworker/0:0-events
Среднее:     0        17    0,00    0,20    0,00    0,39    0,20     -  ksoftirqd/0
Среднее:     0        55    0,00    1,76    0,00    2,15    1,76     -  kworker/u4:3-ext4-rsv-conversion
Среднее:     0        57    0,00    0,20    0,00    0,20    0,20     -  kworker/u4:5-events_unbound
Среднее:   101       562    0,00    0,20    0,00    0,20    0,20     -  dbus-daemon
Среднее:     0      1249    0,00    0,20    0,00    0,00    0,20     -  VBoxDRMClient
Среднее:     0      1252    0,00    0,20    0,00    0,00    0,20     -  VBoxService
Среднее:     0      1509    0,00    0,20    0,00    0,00    0,20     -  upowerd
Среднее:  1000      2089    0,59    8,22    0,00    3,91    8,81     -  gnome-shell
Среднее:  1000      2963    0,00    0,20    0,00    0,20    0,20     -  gjs
Среднее:  1000      3537    0,39    0,78    0,00    2,15    1,17     -  gnome-terminal-
Среднее:     0      3630    0,00    0,59    0,00    0,00    0,59     -  kworker/0:1H-kblockd
Среднее:  1000      3644    0,00    2,74    0,00    0,98    2,74     -  pidstat

Среднее:   UID       PID  minflt/s  majflt/s     VSZ     RSS   %MEM  Command
Среднее:  1000      2089    307,63      0,00 3547908  416468   8,47  gnome-shell
Среднее:  1000      3537      1,17      0,00  634332   57540   1,17  gnome-terminal-
Среднее:  1000      3644      0,20      0,00   18568    3856   0,08  pidstat

Среднее:   UID       PID   kB_rd/s   kB_wr/s kB_ccwr/s iodelay  Command

```

**Аналитика процессов:**

**TOP-5 по CPU:**
| PID | COMMAND | %CPU | ETIME | Причина нагрузки |
|-----|---------|------|-------|------------------|
| 3635 | ps | 53.8 | 00:00 | Временный процесс выполнения команды мониторинга |
| 3636 | head | 7.6 | 00:00 | Обработка вывода команды ps |
| 2089 | gnome-shell | 5.1 | 19:12 | Графическая оболочка GNOME |
| 3140 | nautilus | 0.7 | 17:18 | Файловый менеджер |
| 3537 | gnome-terminal- | 0.6 | 09:48 | Терминал GNOME |

**TOP-5 по памяти:**
| PID | COMMAND | %MEM | RSS (KB) | Причина использования памяти |
|-----|---------|------|----------|-------------------------------|
| 2089 | gnome-shell | 8.4 | 416,468 | Графическая оболочка GNOME с расширениями |
| 3140 | nautilus | 5.5 | 273,228 | Файловый менеджер с открытыми окнами |
| 2813 | mutter-x11-fram | 2.0 | 98,520 | Композитный менеджер окон для GNOME |
| 2680 | gsd-xsettings | 1.6 | 81,092 | Сервис настройки тем GNOME |
| 2540 | Xwayland | 1.3 | 68,648 | X-сервер для совместимости |

**Особенности системы:**
- **Однопроцессорная система** (1 CPU) - видно по максимальной загрузке 53.8% у процесса `ps`
- **Виртуальная машина** - присутствуют процессы VirtualBox (VBoxDRMClient, VBoxService)
- **Графическая среда GNOME** - основные процессы связаны с графическим интерфейсом
- **Низкая общая нагрузка** - система работает стабильно, нет процессов с аномальной нагрузкой

**Выводы:** 
Основную нагрузку создают процессы графической среды GNOME. Система работает на виртуальной машине с одним процессором. Наибольшее потребление памяти у графических компонентов (gnome-shell, nautilus), что характерно для систем с графическим интерфейсом. Временные высокие значения %CPU у утилит мониторинга (ps, head) связаны с моментом выполнения команды и не отражают постоянную нагрузку.

### 5) Мини-утилита ptree

**Код утилиты:**
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_LINE 256

void print_process_tree(pid_t pid) {
    char path[64];
    char line[MAX_LINE];
    char name[64];
    pid_t ppid;
    FILE *fp;
    
    while (pid > 1) {
        snprintf(path, sizeof(path), "/proc/%d/status", pid);
        fp = fopen(path, "r");
        if (!fp) break;
        
        ppid = 0;
        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "Name:", 5) == 0) {
                sscanf(line, "Name:\t%s", name);
            } else if (strncmp(line, "PPid:", 5) == 0) {
                sscanf(line, "PPid:\t%d", &ppid);
            }
        }
        fclose(fp);
        
        printf("%s(%d)", name, pid);
        if (ppid > 0) {
            printf(" ← ");
        }
        pid = ppid;
    }
    printf("systemd(1)\n");
}

int main(int argc, char *argv[]) {
    pid_t pid;
    
    if (argc == 2) {
        pid = atoi(argv[1]);
    } else {
        pid = getpid();
    }
    
    printf("Process tree for PID %d: ", pid);
    print_process_tree(pid);
    
    return 0;
}
```

**Запуск и вывод:**
```bash
gcc -Wall -Wextra -O2 ~labs/lr2/ptree.c -o ptree
./ptree
```
```
Process tree for PID 3706: ptree(3706) ← bash(3545) ← gnome-terminal-(3537) ← systemd(1863) ← systemd(1)
```

## Ответы на вопросы

1. **Чем процесс отличается от программы?**  
   Программа — это статичный исполняемый файл, а процесс — это выполняющийся экземпляр программы с собственным контекстом исполнения.

2. **Что будет, если вызвать `fork()` без `wait()`?**  
   Дочерний процесс станет "зомби" после завершения, пока родитель не вызовет `wait()` или не завершится сам.

3. **Как система хранит информацию о процессах?**  
   В таблице процессов ядра и виртуальной файловой системе `/proc`.

4. **Что делает `exec()` и зачем он нужен?**  
   Заменяет текущий образ процесса новым исполняемым файлом, сохраняя PID.

5. **Почему в `/proc` нет «настоящих» файлов?**  
   Это виртуальная файловая система, которая предоставляет интерфейс для доступа к данным ядра в реальном времени.

6. **Как интерпретировать поля `top`?**  
   - `%CPU` — использование процессорного времени
   - `%MEM` — использование физической памяти
   - `VIRT` — общая виртуальная память
   - `RES` — резидентная память в RAM
   - `SHR` — разделяемая память
   - `TIME+` — общее процессорное время

7. **Почему сумма `%CPU` может быть больше 100%?**  
   На многоядерных системах каждое ядро дает 100%, поэтому сумма может превышать 100%.

8. **Чем отличается мгновенное `%CPU` от `load average`?**  
   `%CPU` — мгновенная загрузка CPU процессом, `load average` — средняя загрузка системы за 1, 5 и 15 минут.

9. **Чем IO‑нагрузка отличается от CPU‑нагрузки?**  
   CPU-нагрузка — использование процессора, IO-нагрузка — операции ввода/вывода на диске и сети.

10. **Что такое `nice`/приоритеты процессов?**  
    Значение nice влияет на планировщик процессов: более высокий nice означает более низкий приоритет.

11. **Чем поток отличается от процесса?**  
    Потоки разделяют память и ресурсы процесса, а процессы изолированы друг от друга.

12. **Что такое зомби и сироты?**  
    Зомби — завершенные процессы, ожидающие вызова `wait()` родителем. Сироты — процессы, чей родитель завершился.

## Выводы
В ходе работы изучены механизмы создания и управления процессами в Linux, исследована файловая система `/proc`, проанализирована нагрузка на систему. Получены практические навыки работы с процессами через системные вызовы и утилиты мониторинга.
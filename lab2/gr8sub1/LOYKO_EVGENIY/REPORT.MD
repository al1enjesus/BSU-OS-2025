Лабораторная работа 2 - Процессы и файловая система /proc

Цель работы:
Изучить модель процессов Linux, принципы создания процессов и извлечения информации из /proc.

1. Создание процессов

Код программы:

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t child1, child2;
    int status1, status2;
    
    printf("=== Process Creation Example ===\n");
    printf("parent: starting, PID=%d, PPID=%d\n", getpid(), getppid());
    fflush(stdout);
    
    // Создаем первого потомка
    child1 = fork();
    
    if (child1 == 0) {
        // Код первого потомка
        printf("child_A: PID=%d, PPID=%d\n", getpid(), getppid());
        fflush(stdout);
        printf("child_A: sleeping for 2 seconds...\n");
        fflush(stdout);
        sleep(2);
        printf("child_A: finished\n");
        fflush(stdout);
        exit(10);
    } else if (child1 > 0) {
        // Код родителя
        printf("parent: created child_A with PID=%d\n", child1);
        fflush(stdout);
        
        // Создаем второго потомка
        child2 = fork();
        
        if (child2 == 0) {
            // Код второго потомка
            printf("child_B: PID=%d, PPID=%d\n", getpid(), getppid());
            fflush(stdout);
            printf("child_B: sleeping for 1 second...\n");
            fflush(stdout);
            sleep(1);
            printf("child_B: finished\n");
            fflush(stdout);
            exit(20);
        } else if (child2 > 0) {
            // Код родителя
            printf("parent: created child_B with PID=%d\n", child2);
            fflush(stdout);
            
            printf("parent: waiting for children to finish...\n");
            fflush(stdout);
            
            // Ожидаем завершения обоих потомков
            waitpid(child1, &status1, 0);
            printf("parent: child_A (PID=%d) finished with status %d\n", 
                   child1, WEXITSTATUS(status1));
            fflush(stdout);
            
            waitpid(child2, &status2, 0);
            printf("parent: child_B (PID=%d) finished with status %d\n", 
                   child2, WEXITSTATUS(status2));
            fflush(stdout);
            
            printf("parent: all children finished, exiting\n");
            fflush(stdout);
        } else {
            perror("fork failed for child_B");
            exit(1);
        }
    } else {
        perror("fork failed for child_A");
        exit(1);
    }
    
    return 0;
}

Компиляция и запуск:
make fork_example
./fork_example


Вывод программы:
=== Process Creation Example ===
parent: starting, PID=4861, PPID=2957
parent: created child_A with PID=4862
child_A: PID=4862, PPID=4861
child_A: sleeping for 2 seconds...
parent: created child_B with PID=4863
parent: waiting for children to finish...
child_B: PID=4863, PPID=4861
child_B: sleeping for 1 second...
child_B: finished
child_A: finished
parent: child_A (PID=4862) finished with status 10
parent: child_B (PID=4863) finished with status 20
parent: all children finished, exiting

Родительский процесс создает двух потомков через fork()
Каждый процесс выводит свой PID и PPID (Parent PID)
Родитель ждет завершения потомков через waitpid()
Процессы завершаются с разными кодами выхода (10 и 20)


2. Исследование дерева процессов
ps -ef --forest | head -n 30
UID          PID    PPID  C STIME TTY          TIME CMD
root           2       0  0 05:23 ?        00:00:00 [kthreadd]
root           3       2  0 05:23 ?        00:00:00  \_ [pool_workqueue_release]
root           4       2  0 05:23 ?        00:00:00  \_ [kworker/R-rcu_g]
root           5       2  0 05:23 ?        00:00:00  \_ [kworker/R-rcu_p]
root           6       2  0 05:23 ?        00:00:00  \_ [kworker/R-slub_]
root           7       2  0 05:23 ?        00:00:00  \_ [kworker/R-netns]
root           9       2  0 05:23 ?        00:00:00  \_ [kworker/0:0H-kblockd]
root          11       2  0 05:23 ?        00:00:01  \_ [kworker/u8:0-events_unbound]
root          12       2  0 05:23 ?        00:00:00  \_ [kworker/R-mm_pe]
root          13       2  0 05:23 ?        00:00:00  \_ [rcu_tasks_kthread]
root          14       2  0 05:23 ?        00:00:00  \_ [rcu_tasks_rude_kthread]
root          15       2  0 05:23 ?        00:00:00  \_ [rcu_tasks_trace_kthread]
root          16       2  0 05:23 ?        00:00:00  \_ [ksoftirqd/0]
root          17       2  0 05:23 ?        00:00:00  \_ [rcu_preempt]
root          18       2  0 05:23 ?        00:00:00  \_ [migration/0]
root          19       2  0 05:23 ?        00:00:00  \_ [idle_inject/0]
root          20       2  0 05:23 ?        00:00:00  \_ [cpuhp/0]
root          21       2  0 05:23 ?        00:00:00  \_ [cpuhp/1]
root          22       2  0 05:23 ?        00:00:00  \_ [idle_inject/1]
root          23       2  0 05:23 ?        00:00:00  \_ [migration/1]
root          24       2  0 05:23 ?        00:00:00  \_ [ksoftirqd/1]
root          26       2  0 05:23 ?        00:00:00  \_ [kworker/1:0H-events_highpri]
root          27       2  0 05:23 ?        00:00:00  \_ [cpuhp/2]
root          28       2  0 05:23 ?        00:00:00  \_ [idle_inject/2]
root          29       2  0 05:23 ?        00:00:00  \_ [migration/2]
root          30       2  0 05:23 ?        00:00:00  \_ [ksoftirqd/2]
root          32       2  0 05:23 ?        00:00:00  \_ [kworker/2:0H-events_highpri]
root          33       2  0 05:23 ?        00:00:00  \_ [cpuhp/3]
root          34       2  0 05:23 ?        00:00:00  \_ [idle_inject/3]
Команда pstree -p | head -n 50:
Вывод:
systemd(1)-+-ModemManager(995)-+-{ModemManager}(1005)
           |                   |-{ModemManager}(1007)
           |                   `-{ModemManager}(1010)
           |-NetworkManager(936)-+-{NetworkManager}(997)
           |                     |-{NetworkManager}(1001)
           |                     `-{NetworkManager}(1002)
           |-accounts-daemon(851)-+-{accounts-daemon}(882)
           |                      |-{accounts-daemon}(883)
           |                      `-{accounts-daemon}(885)
           |-agetty(1223)
           |-avahi-daemon(803)---avahi-daemon(918)
           |-colord(1342)-+-{colord}(1347)
           |              |-{colord}(1348)
           |              `-{colord}(1350)
           |-cron(1208)
           |-cupsd(5002)
           |-dbus-daemon(804)
           |-fwupd(5009)-+-{fwupd}(5014)
           |             |-{fwupd}(5025)
           |             |-{fwupd}(5026)
           |             |-{fwupd}(5027)
           |             `-{fwupd}(5029)
           |-gdm3(1228)-+-gdm-session-wor(1716)-+-gdm-wayland-ses(1786)-+-gnome-session-b(1792)-+-{gnome-session-b}(1843)
           |            |                       |                       |                       |-{gnome-session-b}(1844)
           |            |                       |                       |                       `-{gnome-session-b}(1845)
           |            |                       |                       |-{gdm-wayland-ses}(1789)
           |            |                       |                       |-{gdm-wayland-ses}(1790)
           |            |                       |                       `-{gdm-wayland-ses}(1791)
           |            |                       |-{gdm-session-wor}(1717)
           |            |                       |-{gdm-session-wor}(1718)
           |            |                       `-{gdm-session-wor}(1719)
           |            |-{gdm3}(1230)
           |            |-{gdm3}(1231)
           |            `-{gdm3}(1232)
           |-gnome-remote-de(808)-+-{gnome-remote-de}(996)
           |                      |-{gnome-remote-de}(999)
           |                      `-{gnome-remote-de}(1000)
           |-kerneloops(1211)
           |-kerneloops(1218)
           |-multipathd(390)-+-{multipathd}(405)
           |                 |-{multipathd}(406)
           |                 |-{multipathd}(407)
           |                 |-{multipathd}(409)
           |                 |-{multipathd}(410)
           |                 `-{multipathd}(411)
           |-polkitd(819)-+-{polkitd}(972)
           |              |-{polkitd}(973)
           |              `-{polkitd}(974)
           |-power-profiles-(827)-+-{power-profiles-}(859)
           |                      |-{power-profiles-}(860)
ps -ef --forest показывает дерево процессов в виде иерархии
pstree -p отображает процессы в древовидной форме с PID

3. Изучение файловой системы /proc
Команда: echo "PID текущей оболочки: $$"
Вывод:  PID текущей оболочки : 2957
Командная строка процесса
Команда: cat /proc/$$/cmdline | tr '\0' ' '; echo
head -n 20 /proc/$$/status
Name:	bash
Umask:	0002
State:	S (sleeping)
Tgid:	2957
Ngid:	0
Pid:	2957
PPid:	2950
TracerPid:	0
Uid:	1000	1000	1000	1000
Gid:	1000	1000	1000	1000
FDSize:	256
Groups:	4 24 27 30 46 101 1000 
NStgid:	2957
NSpid:	2957
NSpgid:	2957
NSsid:	2957
Kthread:	0
VmPeak:	8372 kB
VmSize:	8372 kB
VmLck:	0 kB

ls -la /proc/$$/fd
total 0
dr-x------ 2 datastream datastream  0 Sep 22 05:35 .
dr-xr-xr-x 9 datastream datastream  0 Sep 22 05:33 ..
lrwx------ 1 datastream datastream 64 Sep 22 05:35 0 -> /dev/pts/0
lrwx------ 1 datastream datastream 64 Sep 22 05:35 1 -> /dev/pts/0
lrwx------ 1 datastream datastream 64 Sep 22 05:35 2 -> /dev/pts/0
lrwx------ 1 datastream datastream 64 Sep 22 05:35 255 -> /dev/pts/0
PID текущей оболочки: 2957
Состояние процесса: S (sleeping) - процесс ожидает ввода
PPid: 2950 - PID родительского процесса
Открытые файловые дескрипторы: 0,1,2,255 - все связаны с терминалом (/dev/pts/0)
	•	
4. Анализ нагрузки системы
Топ процессов по CPU
top -b -n 1 | head -n 20
Вывод: top - 05:52:08 up 28 min,  1 user,  load average: 0,22, 0,13, 0,09
Tasks: 211 total,   1 running, 210 sleeping,   0 stopped,   0 zombie
%Cpu(s):  1,9 us,  5,8 sy,  0,0 ni, 92,3 id,  0,0 wa,  0,0 hi,  0,0 si,  0,0 st 
MiB Mem :   3902,5 total,    992,0 free,   1100,7 used,   2043,6 buff/cache     
MiB Swap:   3902,0 total,   3902,0 free,      0,0 used.   2801,8 avail Mem 

    PID USER      PR  NI    VIRT    RES    SHR S  %CPU  %MEM     TIME+ COMMAND
   4883 datastr+  20   0   11752   5120   3072 R  15,4   0,1   0:00.05 top
      1 root      20   0   23204  13172   8436 S   0,0   0,3   0:03.07 systemd
      2 root      20   0       0      0      0 S   0,0   0,0   0:00.00 kthreadd
      3 root      20   0       0      0      0 S   0,0   0,0   0:00.00 pool_wo+
      4 root       0 -20       0      0      0 I   0,0   0,0   0:00.00 kworker+
      5 root       0 -20       0      0      0 I   0,0   0,0   0:00.00 kworker+
      6 root       0 -20       0      0      0 I   0,0   0,0   0:00.00 kworker+
      7 root       0 -20       0      0      0 I   0,0   0,0   0:00.00 kworker+
      9 root       0 -20       0      0      0 I   0,0   0,0   0:00.04 kworker+
     11 root      20   0       0      0      0 I   0,0   0,0   0:01.13 kworker+
     12 root       0 -20       0      0      0 I   0,0   0,0   0:00.00 kworker+
     13 root      20   0       0      0      0 I   0,0   0,0   0:00.00 rcu_tas+
     14 root      20   0       0      0      0 I   0,0   0,0   0:00.00 rcu_tas+

Процессы по использованию CPU
ps -eo pid,ppid,comm,state,%cpu,%mem,etime --sort=-%cpu | head -n 15

PID    PPID COMMAND         S %CPU %MEM     ELAPSED
   4885    2957 ps              R  200  0.0       00:00
   1903    1727 gnome-shell     S  6.0 10.0       26:24
   4792    4750 unattended-upgr S  2.0  3.1       02:28
   4847    4792 http            S  1.7  0.2       02:25
   2950    1727 gnome-terminal- S  1.0  1.4       18:49
    837       1 snapd           S  0.7  0.9       28:50
   1665       1 spice-vdagentd  S  0.1  0.1       26:38
      1       0 systemd         S  0.1  0.3       28:54
   2014    1727 ibus-daemon     S  0.1  0.2       26:23
   2143    2014 ibus-extension- S  0.1  0.7       26:23
    693       1 systemd-oomd    S  0.1  0.1       28:50
   2845    1903 gjs             S  0.1  1.5       19:01
   3746       2 kworker/0:3-eve I  0.0  0.0       12:06
     11       2 kworker/u8:0-ev I  0.0  0.0       28:54

Процессы по использованию памяти

ps -eo pid,ppid,comm,state,%cpu,%mem,rss --sort=-%mem | head -n 15

PID    PPID COMMAND         S %CPU %MEM     ELAPSED
   4885    2957 ps              R  200  0.0       00:00
   1903    1727 gnome-shell     S  6.0 10.0       26:24
   4792    4750 unattended-upgr S  2.0  3.1       02:28
   4847    4792 http            S  1.7  0.2       02:25
   2950    1727 gnome-terminal- S  1.0  1.4       18:49
    837       1 snapd           S  0.7  0.9       28:50
   1665       1 spice-vdagentd  S  0.1  0.1       26:38
      1       0 systemd         S  0.1  0.3       28:54
   2014    1727 ibus-daemon     S  0.1  0.2       26:23
   2143    2014 ibus-extension- S  0.1  0.7       26:23
    693       1 systemd-oomd    S  0.1  0.1       28:50
   2845    1903 gjs             S  0.1  1.5       19:01
   3746       2 kworker/0:3-eve I  0.0  0.0       12:06
     11       2 kworker/u8:0-ev I  0.0  0.0       28:54

5. Утилита ptree

Компиляция и запуск
make ptree
./ptree
./ptree 1

Вывод для текущего процесса
Process tree for PID 4865:
ptree(4865) \u2190 bash(2957) \u2190 gnome-terminal-(2950) \u2190 systemd(1727) \u2190 systemd(1)

Вывод для init процесса (PID 1)
Process tree for PID 1:
systemd(1)


Ответы на вопросы
1. Чем процесс отличается от программы?
Ответ: Программа - это исполняемый файл на диске, а процесс - это выполняющийся экземпляр программы в памяти с собственным контекстом, PID и ресурсами.
2. Что будет, если вызвать fork() без wait()?
Ответ: Дочерний процесс станет "зомби" после завершения - он освободит память, но запись в таблице процессов останется до тех пор, пока родитель не вызовет wait().
3. Как система хранит информацию о процессах?
Ответ: В таблице процессов ядра и через виртуальную файловую систему /proc, где каждый процесс представлен директорией /proc/PID/.
4. Что делает exec() и зачем он нужен?
Ответ: exec() заменяет текущий образ процесса новым, сохраняя тот же PID. Используется после fork() для запуска другой программы.
5. Почему в /proc нет «настоящих» файлов?
Ответ: /proc - виртуальная файловая система, файлы генерируются ядром на лету при чтении и не занимают места на диске.
6. Как интерпретировать поля top: %CPU, %MEM, VIRT, RES, SHR, TIME+?
Ответ:
%CPU - использование процессора (100% = одно ядро)
%MEM - использование оперативной памяти
VIRT - виртуальная память (включая своп)
RES - резидентная память (физическая RAM)
SHR - разделяемая память (библиотеки)
TIME+ - общее время использования CPU

7. Почему сумма %CPU может быть больше 100%?
Ответ: На многоядерных системах каждое ядро дает 100%. Если 4 процесса используют по 100% одного ядра, суммарно будет 400%.
8. Чем отличается мгновенное %CPU от load average?
Ответ: %CPU - текущее использование в момент снятия снимка, load average - среднее количество процессов в очереди за 1, 5 и 15 минут.
9. Что означает строка Cpu(s) в top (в т.ч. wa)?
Ответ: wa - время ожидания ввода-вывода (I/O wait), показывает процент времени, когда CPU простаивал в ожидании операций ввода-вывода.
10. Чем IO-нагрузка отличается от CPU-нагрузки?
Ответ: CPU-нагрузка - процессы ждут процессорного времени, IO-нагрузка - процессы ждут операций с диском, сетью или другими устройствами.
11. Что такое nice/приоритеты процессов?
Ответ: Nice value от -20 (высший приоритет) до 19 (низший) влияет на планировщик процессов. Обычные пользователи могут только повышать nice value.
12. Чем поток отличается от процесса?
Ответ: Поток разделяет память с родительским процессом, а процесс имеет изолированное адресное пространство. Потоки легче и быстрее создаются.
13. Что такое зомби и сироты?
Ответ: Зомби - завершенные процессы, не "подобранные" родителем. Сироты - процессы, чей родитель завершился (усыновляются init).

Использование ИИ при выполнении лабораторной работы:
Для понимания работы команд и концепций:
	•	
Объяснение системных вызовов: Получил подробное объяснение работы fork(), wait(), waitpid() и их взаимодействия
Анализ файловой системы /proc: Уточнил назначение различных файлов в /proc (status, cmdline, fd, maps)

Для решения технических проблем:
Устранение ошибок компиляции: Помощь в диагностике и исправлении ошибок "undefined reference to main" и проблем с типами данных

Для уточнения теоретических вопросов:
Разбор вопросов к отчету: Подробные объяснения по всем 12 обязательным вопросам

	•	










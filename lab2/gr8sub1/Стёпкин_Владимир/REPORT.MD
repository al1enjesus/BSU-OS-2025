#Лабораторная работа 2. Процессы и файловая система /proc

##Цель работы: понять модель процессов Linux, принципы порождения и ожидания завершения, а также научиться извлекать информацию из /proc.

##Задание 1: Создание процессов

###Код программы на С
**Файл: `src/fork_example.c`**
```c
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

int main() {
    printf("parent(PID=%d) started\n", getpid());
    fflush(stdout);

    pid_t child_pids[2];

    for (int i = 0; i < 2; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork failed");
            exit(1);
        } else if (pid == 0) {
            // Дочерний процесс
            printf("child_%d: PID=%d, PPID=%d\n", i, getpid(), getppid());
            fflush(stdout);
            exit(0);  // дочерний завершается
        } else {
            // Родительский процесс
            child_pids[i] = pid;
        }
    }

    // Родитель ждёт дочерних процессов
    for (int i = 0; i < 2; i++) {
        int status;
        pid_t wpid = waitpid(child_pids[i], &status, 0);
        if (wpid > 0) {
            if (WIFEXITED(status)) {
                printf("child with PID %d exited with code %d\n", wpid, WEXITSTATUS(status));
            } else {
                printf("child with PID %d terminated abnormally\n", wpid);
            }
            fflush(stdout);
        }
    }

    printf("parent(PID=%d) finished\n", getpid());
    fflush(stdout);

    return 0;
}


```
**Файл: `Makefile`**

```makefile
CC=gcc
CFLAGS=-Wall -Wextra -O2

SRC=src/fork_example.c
BIN=fork_example

all: $(BIN)

$(BIN): $(SRC)
        $(CC) $(CFLAGS) $(SRC) -o $(BIN)

run: $(BIN)
        ./$(BIN)

clean:
        rm -f $(BIN)




```

**Файл: `run.sh`**

```run.sh
set -e  # выйти при ошибке

echo "===> Сборка проекта"
make clean
make

echo "===> Запуск программы"
./fork_example

```

Псоле выполнения `./run.sh` получаем вывод:

```
===> Сборка проекта
rm -f fork_example
gcc -Wall -Wextra -O2 src/fork_example.c -o fork_example
===> Запуск программы
parent(PID=14056) started
child_0: PID=14057, PPID=14056
child_1: PID=14058, PPID=14056
child with PID 14057 exited with code 0
child with PID 14058 exited with code 0
parent(PID=14056) finished

```
###Объяснение работы программы:
Родительский процесс (PID=14056) создает двух потомков с помощью fork()
Первый потомок (Child_0,PID=14057) выводит свои PID и PPID
Второй потомок (Child_B,PID=14058) выводит свои PID и PPID
Родитель ожидает завершения обоих детей с помощью waitpid()

###Выводы:
Программа успешно создает и управляет дочерними процессами.
Корректно работает механизм fork() и waitpid().
Все процессы правильно идентифицируют себя и своих родителей.

##Задание 2: Исследование дерева процессов

###Шаг 1

Запустим программу в фоновом режиме.

```bash
./fork_example &
```
В результате выполнения программы получим вывод:

```text
[1]1443
Parent process started (PID=1443)
Child_A: My PID is 1444,My Parent's PID is 1443
Child_B: My PID is 1445,My Parent's PID is 1443
Parent: Child A (PID=1444) finished with status:0
Parent: Child B (PID=1445) finished with status:0
Parent: All children finished. Exiting.
```
###Шаг 2
Выполним анализ дерева процессов:

```bash
ps -ef --forest | head -n 30 | cat
```
Часть вывода после выаолнения команды:
```
UID           PID      PRID  C STIME TTY           TIME CMD
root            2         0  0 18:54 ?         00:00:00 [kthreadd]
root            3         2  0 18:54 ?         00:00:00 \_ [pool_workqueue_release]
root            4         2  0 18:54 ?         00:00:00 [kworker/R-kvfree_rsu_reclaim]
root            5         2  0 18:54 ?         00:00:00 [kworker/R-rcu_gr]
root            6         2  0 18:54 ?         00:00:00 [kworker/R-sync_wq]
...
```
```bash
pstree -p | head -n 50 | cat
```
Часть вывода после выполнения программы:
```
systemd(1)-+-ModemManager(698)-+-{ModemManager}(704)
           |                   |-{ModemManager}(705)
           |                   `-{ModemManager}(708)
           |-NetworkManager(638)-+-{NetworkManager}(699)
           |                     |-{NetworkManager}(700)
           |                     `-{NetworkManager}(701)
           |-VBoxClient(982)---VBoxClient(983)-+-{VBoxClient}(985)
           |                                   |-{VBoxClient}(986)
           |                                   `-{VBoxClient}(987)

...
```
##Задание 3: Изучение виртуальной файловой системы /proc

###Шаг 1: Определение PID текущей оболочки
```bash
echo $$
```
Вывод:
```
13389
```

####Шаг 2:
1. файл cmdline - командная строка запуска
```bash
cat/proc/13389/cmdline | tr'\0''';echo
```
Вывод:
```
/usr/bin/zsh 

```
Команда:
```bash
head -n 20/proc/13389/status
```
Вывод:
```
Name:   zsh
Umask:  0002
State:  S (sleeping)
Tgid:   13389
Ngid:   0
Pid:    13389
PPid:   13357
TracerPid:      0
Uid:    0       0       0       0
Gid:    0       0       0       0
FDSize: 64
Groups: 0 
NStgid: 13389
NSpid:  13389
NSpgid: 13389
NSsid:  13389
Kthread:        0
VmPeak:    14324 kB
VmSize:    13836 kB
VmLck:         0 kB


```
##Задание 4: Анализ процессов (нагрузка CPU/память/IO)

###Шаг 1: Моментальный снимок системы
**1. Топ процессов по CPU**
```bash
top -b -n 1 | head -n 20
```
Фрагмент вывода:
```
top - 13:57:07 up 37 min,  2 users,  load average: 0.01, 0.14, 0.18
Tasks: 212 total,   1 running, 211 sleeping,   0 stopped,   0 zombie
%Cpu(s):  1.2 us,  1.2 sy,  0.0 ni, 95.0 id,  1.2 wa,  0.0 hi,  1.2 si,  0.0 st 
MiB Mem :   7947.3 total,   5051.0 free,   2272.8 used,    950.8 buff/cache     
MiB Swap:   1024.0 total,   1024.0 free,      0.0 used.   5674.4 avail Mem 

    PID USER      PR  NI    VIRT    RES    SHR S  %CPU  %MEM     TIME+ COMMAND
      1 root      20   0   23412  13708  10356 S   0.0   0.2   0:01.14 systemd
      2 root      20   0       0      0      0 S   0.0   0.0   0:00.00 kthreadd
      3 root      20   0       0      0      0 S   0.0   0.0   0:00.00 pool_wo+
      4 root       0 -20       0      0      0 I   0.0   0.0   0:00.00 kworker+
      5 root       0 -20       0      0      0 I   0.0   0.0   0:00.00 kworker+
      6 root       0 -20       0      0      0 I   0.0   0.0   0:00.00 kworker+
      7 root       0 -20       0      0      0 I   0.0   0.0   0:00.00 kworker+
      9 root      20   0       0      0      0 I   0.0   0.0   0:00.01 kworker+
     12 root       0 -20       0      0      0 I   0.0   0.0   0:00.00 kworker+
     13 root      20   0       0      0      0 I   0.0   0.0   0:00.00 rcu_tas+
     14 root      20   0       0      0      0 I   0.0   0.0   0:00.00 rcu_tas+
     15 root      20   0       0      0      0 I   0.0   0.0   0:00.00 rcu_tas+
     16 root      20   0       0      0      0 S   0.0   0.0   0:00.04 ksoftir+

...
```
**2.Топ процессов по CPU через ps**
```bash
ps -eo pid,ppid,comm,state,%cpu,%mem,etime --sort=-%cpu | head -n 15 | cat
```
Вывод:
```
    PID    PPID COMMAND         S %CPU %MEM     ELAPSED
   1192     922 firefox-esr     S 14.3  9.2       38:13
   1740    1192 Isolated Web Co S  4.8  4.3       38:09
    740     728 Xorg            S  3.8  2.3       38:22
   1993    1192 Isolated Web Co S  1.4  3.2       38:03
   1910    1192 Isolated Web Co S  1.0  2.5       38:03
   1065     922 xfwm4           S  0.9  1.5       38:14
   1930    1192 Isolated Web Co S  0.9  2.3       38:03
   1606    1192 WebExtensions   S  0.7  1.6       38:10
   2207    1116 x-terminal-emul S  0.6  1.2       37:49
   1962    1192 RDD Process     S  0.5  1.0       38:03
   1135    1116 panel-13-cpugra S  0.2  0.9       38:13
  13357    1116 x-terminal-emul R  0.2  1.2       18:10
   1137    1116 panel-15-genmon S  0.2  0.3       38:13
   1006    1005 VBoxClient      S  0.1  0.0       38:14
                                                          
```
**3.Топ процессоров по памяти**
```bash
ps -eo pid,ppid,comm,state,%cpu,%mem,rss --sort=-%mem | head -n 15 | cat
```
Вывод:
```
    PID    PPID COMMAND         S %CPU %MEM   RSS
   1192     922 firefox-esr     S 14.2  9.2 752912
   1740    1192 Isolated Web Co S  4.8  4.3 352284
   1993    1192 Isolated Web Co S  1.4  3.2 265768
   1910    1192 Isolated Web Co S  1.0  2.5 207004
   1930    1192 Isolated Web Co S  0.9  2.3 189256
    740     728 Xorg            S  3.7  2.3 188276
   5029    1192 Isolated Web Co S  0.1  1.8 147640
   1549    1192 Privileged Cont S  0.0  1.7 141392
   1606    1192 WebExtensions   S  0.7  1.6 137488
   1065     922 xfwm4           S  0.9  1.5 128012
  13357    1116 x-terminal-emul S  0.2  1.2 104204
   2207    1116 x-terminal-emul S  0.6  1.2 103616
   5193    1192 Isolated Web Co S  0.0  1.1 92988
   1773    1192 Isolated Servic S  0.1  1.1 91568

```

##Ответы на контрольные вопросы

###1. Чем процесс отличается от программы?
**Программа** - это исполняемый файл на диске (код + данные). **Процесс**- это запущенная программа со своим состоянием (регистры, память и тд).

###2. Что будет, если вызвать fork() без wait()?
Если вызвать fork() без wait(), дочерние процессы после завершения станут **зомби**. 

###3. Как система хванит информацию о процессах?
Система хванит информацию о процессах в структурах данных ядра. Для пользователя эта информация доаступна через виртуальную файловую систему `/proc` и 
системные вызовы.

###4. Что делает exec() и зачем он нужен?
`exec()` заменяет образ текущего процесса на новый, загружая другую программу. Код, данные и стек заменяются, но PID, открытые файлы и права остаются 
прежними. Нужен для запуска любой программы внустри существующего процесса.

###5. Почему в /proc нет "настоящих файлов"?
Файлы в `/proc` виртуальные - они не хранятся на диске. При обращении к ним ядро динамически генерирует содержимое, отражающее текущее состояние системы и 
процессов.

###6. Как интерпретировать поля top: %CPU, &MEM, VIRT, RES, SHR, TIME+?
-**%CPU** - использование процессорного времени
-**%MEM** - процент использования физической памяти
-**VIRT** - виртуальная память (вся выделенная память)
-**RES** - резидентная память
-**SHR** - разделяемая память (библиотека и др.)
-**TIME** - общее процессорное время

###7. Почему сумма %CPU может быть больше 100%?
Процесс может создавать несколько потоков, которые выполняются на разных ядрах CPU одновременно. 100% = использование одного ядра на 100%.Поэтоу процесс с 
двумя активными потоками может нагружать систему на 200%.

###8. Чем отличается мгновенное %CPU от load average?
%CPU - мгновенная нагрузка, load average - средняя очередь процессов за 1/5/15 мин. В top: us - пользователь sy -система, id - простой, wa - ожидание  I/O.
 
###9. Чем IO-нагрузка отличается от CPU-нагрузки и как её увидеть?
CPU-процессор вычисляет, IO - ждет диск\сеть. Смотреть: iotop, pidstst -d, /proc/pid/io

###10. Что такое nice/приоритеты процессоров?
Приоритеты от -20(высший) до 19(низший). Чем меньше число - тем больше CPU времени получает процесс.

###11. Чем поток отличается от процесса?
ПРоцесс - контейнер ресурсов, поток - поток инсполнения внутри процесса. Смотреть: ps -eLf

###12. Что такое зоибт и сироты?
Зомби - завершенный процесс, который ждет wait() от родителяю Сироты - процессы, чей родитель умер (усыновляются init). 

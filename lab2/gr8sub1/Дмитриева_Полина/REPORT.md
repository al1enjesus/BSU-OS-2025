#Лабораторная работа 2. Процессы и файловая система /proc
##Цель работы: понять модель процессов Linux, принципы порождения и ожидания завершения, а также научиться извлекать информацию из /proc.

##Задание 1: Создание процессов
###Код программы на С++
**Файл: `src/fork_example.cpp`**
```cpp
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>

int main(){
std::cout<<"Parent pro# cess started (PID="<<getpid()<<")"<<std::endl;

pid_t child_a = fork();

if(child_a==-1){
std::cerr<<"First fork failed!"<<std::endl;
exit(1);
}
else if(child_a==0){
std::cout<<"Child_A:My PID is "<<getpid()<<",My Parent`s PID is "<<getppid()<<std::endl;
exit(0);
}

pid_t child_b=fork();

if(child_b==-1){
std::cerr<<"Second fork failed!"<<std::endl;
exit(1);
}
else if(child_b==0){
std::cout<<"Child_B: My PID is "<<getpid()<<",My Parent`s PID is "<<getppid()<<std::endl;
exit(0);
}
int status;
waitpid(child_a,&status,0);
std::cout<<"Parent: Child A(PID="<<child_a<<") finished with status: "<<WEXITSTATUS(status)<<std::endl;

waitpid(child_b,&status,0);
std::cout<<"Parent: Child B (PID="<<child_b<<") finished with status: "<<WEXITSTATUS(status)<<std::endl;

std::cout<<"Parent: All children finished. Exiting."<<std::endl;

return 0;
}
```
**Файл: `Makefile`**
```makefile
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11 -02
TARGET = fork_example
SRCS = src/fork_example.cpp

all: $(TARGET)

$(TARGET): $(SRCS)
$(CXX) $(CXXFLAGS) -o $@ $<

run: $(TARGET)
./$(TARGET)

clean:
rm -f $(TARGET)

.PHONY: all run clean
```
###Вывод:
```text
Parent process started (PID=1334)
Child_A: My PID is 1335,My Parent's PID is 1334
Child_B: My PID is 1336,My Parent's PID is 1334
Parent: Child A (PID=1335) finished with status:0
Parent: Child B (PID=1336) finished with status:0
Parent: All children finished. Exiting.
```
###Объяснение работы программы:
Родительский процесс (PID=1334) создает двух потомков с помощью fork()
Первый потомок (Child_A,PID=1335) выводит свои PID и PPID
Второй потомок (Child_B,PID=1336) выводит свои PID и PPID
Родитель ожидает завершения обоих детей с помощью waitpid()
Оба потомка завершаются с кодом 0 (успех)

###Выводы:
Программа успешно создает и управляет дочерними процессами.
Корректно работает механизм fork() и waitpid().
Все процессы правильно идентифицируют себя и своих родителей.

##Задание 2: Исследование дерева процессов
###Выполение задания
####Шаг 1: Запуск программы в фоне

```bash
cd ~/BSU-OS-2025/lab2/gr8sub1/Дмитриева_Полина/
./fork_example &
```
```text
[1]1443
Parent process started (PID=1443)
Child_A: My PID is 1444,My Parent's PID is 1443
Child_B: My PID is 1445,My Parent's PID is 1443
Parent: Child A (PID=1444) finished with status:0
Parent: Child B (PID=1445) finished with status:0
Parent: All children finished. Exiting.
```
####Шаг 2: Анализ дерева ps
Команда:
```bash
ps -ef --forest | head -n 30 | cat
```
Фрагмент результата:
```text
UID           PID      PRID  C STIME TTY           TIME CMD
root            2         0  0 18:54 ?         00:00:00 [kthreadd]
root            3         2  0 18:54 ?         00:00:00 \_ [pool_workqueue_release]
root            4         2  0 18:54 ?         00:00:00 [kworker/R-kvfree_rsu_reclaim]
root            5         2  0 18:54 ?         00:00:00 [kworker/R-rcu_gr]
root            6         2  0 18:54 ?         00:00:00 [kworker/R-sync_wq]
...
```
Команда:
```bash
pstree -p | head -n 50 | cat
```
Фрагмент программы:
```text
systemd(1)-+-VBoxDRMClient(906)-+-{VBoxDRMClient}(914)
           |                    |-{VBoxDRMClient}(915)
           |                    |-{VBoxDRMClient}(918)
           |                    |-{VBoxDRMClient}(920)
           |-VBoxService(911)-+-{VBoxService}(916)
           |                  |{VBoxService}(917)
           |                  |{VBoxService}(919)
           |                  |{VBoxService}(921)
           |                  |{VBoxService}(922)
           |                  |{VBoxService}(924)
           |                  |{VBoxService}(925)
...
```
##Задание 3: Изучение виртуальной файловой системы /proc
###Выполнение задания:

####Шаг 1: Определение PID текущей оболочки
```bash
echo $$
```
Вывод:
```text
1019
```

####Шаг 2:
1. файл cmdline - командная строка запуска
```bash
cat/proc/1019/cmdline | tr'\0''';echo
```
Вывод:
```text
/bin/bash
```
Команда:
```bash
head -n 20/proc/1019/status
```
Вывод:
```text
Name:   bash
Umask:  0002
State:  S (sleeping)
Tgid:   1019
Ngit:   0
Pid:    1019
PPid:   817
TracerPid:       0
Uid:    1000     1000     1000     1000
Gid:    1000     1000     1000     1000
FDSize: 256
Groups: 24 25 27 29 30 44 46 100 101 103 1000
NStgid: 1019
NSpid:  1019
NSpgid: 1019
NSsid:  1019
Kthread:      0
VmPeak:   9588 kB  
VmSize:   9588 kB  
VmLck:       0 kB  
```
##Задание 4: Анализ процессов (нагрузка CPU/память/IO)
###выполнение задания
####Шаг 1: Моментальный снимок системы
**1. Топ процессов по CPU**
```bash
top -b -n 1 | head -n 20
```
Фрагмент вывода:
```text
top - 21:09:14 up  2:15, 1 user, load average: 0,00, 0,00, 0,00
Tasks: 108 total, 1 running, 107 sleeping, 0 stopped, 0 zombie
%Cpu(s): 0,0 us, 0,0 sy, 0,0 ni, 95,7 id, 4,3 ua, 0,0 hi, 0,0 si, 0,0 st 
MiB Mem :   967,2 total,  654,1 free,    275,6 used,  169,8 buff/cache
MiB Swap:   480,0 total,  480,0 free,      0,0 used,  691,6 avall Mem
...
```
**2.Топ процессов по CPU через ps**
```bash
ps -eo pid,ppid,comm,state,%cpu,%mem,etime --sort=-%cpu | head -n 15 | cat
```
Вывод:
```text
   PID   PPID COMMAND          S %CPU %MEM     ELAPSED
   860      1 espeakup         S  2.6  1.8     02:20:52
  1553      2 kworker/1:1-eve  I  0.4  0.0        00:04
  1446      2 kworker/0:1-mm_  I  0.4  0.0        26:01
  1495      2 kworker/1:2-eve  I  0.4  0.0        15:34
```
**3.Топ процессоров по памяти**
```bash
ps -eo pid,ppid,comm,state,%cpu,%mem,rss --sort=-%mem | head -n 15 | cat
```
Вывод:
```text
   PID   PPID COMMAND          S %CPU %MEM     RSS
   860      1 espeakup         S  2.5  1.8     17836
   278      1 systemd-journal  S  0.0  1.5     15164
     1      0 systemd          S  0.0  1.4     14260 
  1007      1 systemd          S  0.0  1.2     12232
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

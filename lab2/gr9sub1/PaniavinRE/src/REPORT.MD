##Лабораторная работа 2 - Процессы и файловая система /proc

PaniavinRE
gr9sub1

#Цель работы
Изучение модели процессов Linux, принципов порождения и ожидания завершения процессов, а также извлечение информации из виртуальной файловой системы /proc.

## 1. Создание процессов

# Код программы fork_example.py
python
import os
import time
import sys

def child_process(name):
    """Function for a child process"""
    print(f"{name}: PID={os.getpid()}, PPID={os.getppid()}", flush=True)
    time.sleep(2)  
    print(f"{name}: finished", flush=True)
    return 0

def main():
    print(f"Parent process: PID={os.getpid()}", flush=True)
    
    pid1 = os.fork()
    if pid1 == 0:
        sys.exit(child_process("child_A"))
    
    pid2 = os.fork()
    if pid2 == 0:
        sys.exit(child_process("child_B"))
    
    if pid1 > 0 and pid2 > 0:
        print("Parent: Waiting for child process to complete...", flush=True)
        pid1, status1 = os.waitpid(pid1, 0)
        pid2, status2 = os.waitpid(pid2, 0)
        print(f"Parent: Both child process are complete with codes: {status1}, {status2}", flush=True)

if __name__ == "__main__":
    main() 
    
    Результат выполнения:

    Parent process: PID=4389
    Parent: Waiting for child process to complete...
    child_A: PID=4390, PPID=4389
    child_B: PID=4391, PPID=4389
    child_B: finished
    child_A: finished
    Parent: Both child process are complete with codes: 0, 0
    
## 2. Исследование дерева процессов

#Команда: ps -ef --forest | head -n 30

Вывод показывает древовидную структуру процессов системы, начиная с системных процессов ядра.

#Команда: pstree -p | head -n 50\

Отображает древовидную структуру процессов с PID, начиная от systemd(1) через различные системные сервисы до пользовательских процессов.

## 3. Изучение /proc
Текущий PID оболочки: 4215

#Команда: cat /proc/$$/cmdline | tr '\0' ' '; echo

Вывод: bash

Пояснение: Показывает командную строку запуска текущего процесса (оболочки bash)

#Команда: head -n 20 /proc/$$/status

Вывод: Информация о статусе процесса:

    * Name: bash - имя процесса

    * State: S (sleeping) - состояние процесса

    * Pid: 4215 - идентификатор процесса

    * PPid: 4207 - идентификатор родительского процесса

    * Uid/Gid: информация о пользователе и группе

    * VmSize: 8380 kB - размер виртуальной памяти
    
#Команда: ls -l /proc/$$/fd

Вывод:
    * total 0
    * lrwx------ 1 riman riman 64 Sep 21 20:17 0 -> /dev/pts/0
    * lrwx------ 1 riman riman 64 Sep 21 20:17 1 -> /dev/pts/0
    * lrwx------ 1 riman riman 64 Sep 21 20:17 2 -> /dev/pts/0
    * lrwx------ 1 riman riman 64 Sep 21 20:17 255 -> /dev/pts/0
Пояснение: Отображает открытые файловые дескрипторы процесса (стандартные потоки ввода/вывода/ошибок)

## 4. Анализ процессов

#TOP-5 процессов по CPU:

    PID    PPID COMMAND         S %CPU %MEM     ELAPSED
    * 1862    1665 gnome-shell     S 15.5 10.7       14:25
    * 2544    1862 firefox         S  1.2  5.7       14:19
    * 4233    1665 nautilus        S  0.8  3.3       07:04
    * 3473    2790 Isolated Web Co S  0.4  3.2       14:16
    * 4207    1665 gnome-terminal- S  0.4  0.8       07:34

#TOP-5 процессов по памяти:

    PID    PPID COMMAND         S %CPU %MEM   RSS
    * 1862    1665 gnome-shell     S 15.5 10.7 871760
    * 2544    1862 firefox         S  1.2  5.7 467672
    * 4233    1665 nautilus        S  0.8  3.3 271388
    * 3473    2790 Isolated Web Co S  0.4  3.2 267524
    * 2810    2790 Privileged Cont S  0.1  1.8 149632

#Анализ pidstat:

Утилита pidstat показала детальную статистику по использованию CPU, памяти и IO процессов за 3 интервала по 1 секунде.

Выводы аналитики:

    Наибольшую нагрузку на CPU создает gnome-shell (в среднем 23.59%)

    Наибольший объем памяти используют графические приложения (gnome-shell, firefox)

    Система в основном простаивает (90.7% idle time)

    Дисковый ввод/вывод практически отсутствует

## 5. Утилита ptree

Код программы ptree.py

import os
import sys

def get_parent_chain(pid):
    """Get parent chain for PID"""
    chain = []
    current_pid = pid
    
    while current_pid != 1 and current_pid != 0:
        try:
            with open(f"/proc/{current_pid}/status", "r") as f:
                for line in f:
                    if line.startswith("Name:"):
                        name = line.split(":")[1].strip()
                    elif line.startswith("PPid:"):
                        ppid = int(line.split(":")[1].strip())
                        break
            chain.append((current_pid, name))
            current_pid = ppid
        except (FileNotFoundError, PermissionError):
            break
    
    chain.append((1, "systemd")) 
    return chain

def main():
    if len(sys.argv) > 1:
        target_pid = int(sys.argv[1])
    else:
        target_pid = os.getpid()  
    
    chain = get_parent_chain(target_pid)
    
    result = " ← ".join([f"{name}({pid})" for pid, name in reversed(chain)])
    print(result)

if __name__ == "__main__":
    main()
    
# Результат выполнения:
    systemd(1) ← systemd(1665) ← gnome-terminal-(4207) ← bash(4215) ← python3(4597)

# Лабораторная работа 2 - Процессы и файловая система /proc

## Цель
Разобраться с процессами в Linux: fork, exec, wait, а также научиться смотреть информацию о них через /proc.

## 1. Создание процессов
Код: [src/fork_example.c](src/fork_example.c)

Пример вывода программы:
	Parent start: PID=5504
	child[0]: PID=5505,PPID=5504
	child[1]: PID=5506,PPID=5504
	Parent: child PID=5505 exited with status 0
	Parent: child PID=5506 exited with status 0
	Parent end: PID=5504

### Вывод:
Родитель создал двух детей и дождался их завершения.

## 2. Исследование дерева процессов


Вывод команды 'ps -ef --forest':
	m6tyz       3935    3423  0 16:14 pts/0    00:00:00  |   |   \_ ./fork_example
	m6tyz       3936    3935  0 16:14 pts/0    00:00:00  |   |       \_ ./fork_example
	m6tyz       3937    3935  0 16:14 pts/0    00:00:00  |   |       \_ ./fork_example
	m6tyz       3940    3438  0 16:14 pts/1    00:00:00  |       \_ grep --color=auto fork_example

Вывод команды 'pstree -p':
	           |               |-gnome-terminal-(3413)-+-bash(3423)---fork_example(3935)-+-fork_example(3936)
                                                                                     `-fork_example(3937)
### Вывод: 
Все процессы получились и выдны в дереве процессов
Родительский процесс я fork_example имеет PID=3935
Дочерние процессы имеют PID=3936 и 3937
bash имеет PID=3423


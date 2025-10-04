# Лабораторная работа №3 - Потоки, синхронизация и гонки данных
**Варинт 1**

## Цель
Научиться работать с потоками, увидеть гонку данных и устранить ее с помощью синхронизации.

## А. Гонка данных

Файл: [[src/thread_race.c]]

Режимы:
- `unsync` - без синхронизации
- `mutex` - с `pthread_mutex_t`
- `atomic` - c`stdatomic.h`

Команды:
```bash
./thread_race 4 1000000 unsync
./thread_race 4 1000000 mutex
./thread_race 4 1000000 atomic
```
Результаты:
```bash
Mode: unsync | Threads: 4 | Iters/thread: 1000000
Expected: 4000000 | Actual: 1091804 | Time: 0.013714 s

Mode: mutex | Threads: 4 | Iters/thread: 1000000
Expected: 4000000 | Actual: 4000000 | Time: 0.108754 s

Mode: atomic | Threads: 4 | Iters/thread: 1000000
Expected: 4000000 | Actual: 4000000 | Time: 0.032549 s
```
Вывод:
Без синхронизации возникает расхождение, с mutex и atomic - результат корректен. 

## B. Producer-Consumer (mutex + condvar)

Реализован кольцевой буфер фиксированного размера. Производители добавляют элементы, потребители их извлекают. Используется pthread_mutex_t и pthread_cond_t

Команды:
```bash
./prodcons -P 2 -C 2 -N 100000 -B 64
```
Результы:
```bash
Produced sum = 1410165408 | Consumed sum = 1410165408
```
Вывод:
Количество произведенных и потребленных элементов совпадает, программа завершает все потоки корректно

## C. Где видны потоки

Команды:
```bash
ps -L -p 46731 -o pid,tid,psr,pcpu,stat,comm
cat /proc/46731/status | grep Threads
ls -l /proc/46731/task | head
```
Результат:
```bash
 PID     TID PSR %CPU STAT COMMAND
  46731   46731  15  0.0 Sl   thread_race
  46731   46732  17 89.0 Rl   thread_race
  46731   46733  13 88.8 Rl   thread_race
  46731   46734   3 89.7 Rl   thread_race
  46731   46735  12 89.0 Rl   thread_race
  46731   46736  19 90.8 Rl   thread_race
  46731   46737   2 89.8 Rl   thread_race
  46731   46738  16 89.7 Rl   thread_race
  46731   46739  11 89.1 Rl   thread_race

Threads:	9

total 0
dr-xr-xr-x 7 m6tyz m6tyz 0 Oct  4 22:42 46731
dr-xr-xr-x 7 m6tyz m6tyz 0 Oct  4 22:42 46732
dr-xr-xr-x 7 m6tyz m6tyz 0 Oct  4 22:42 46733
dr-xr-xr-x 7 m6tyz m6tyz 0 Oct  4 22:42 46734
dr-xr-xr-x 7 m6tyz m6tyz 0 Oct  4 22:42 46735
dr-xr-xr-x 7 m6tyz m6tyz 0 Oct  4 22:42 46736
dr-xr-xr-x 7 m6tyz m6tyz 0 Oct  4 22:42 46737
dr-xr-xr-x 7 m6tyz m6tyz 0 Oct  4 22:42 46738
dr-xr-xr-x 7 m6tyz m6tyz 0 Oct  4 22:42 46739
```
Вывод:
Потоки выдны в ps -L и в /proc/46731/task/. Каждый поток имеет свой TID

## Ответы на вопросы
1. Поток - это часть процесса, он использует общую память, а процессы изолированы. Потоки выдны в ps -L и в /proc/46731/task/. Каждый поток имеет свой TID
2. Race condition - одновременный доступ к общим данным, volatile не делаеь операции атомарными
3. mutex/condvar - синхронизация действий, semaphore - счетчик русурсов, atomic - простые операции без блокировок
4. Синхронизация замедляет из-за блокировок. Уменьшить contention можно разделением данных и атомиками
5. False sharing - это потоки используют разные данные в одной кэш линии. Решается выравнивает иои разносом данных
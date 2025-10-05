# Лабораторная 3 — Потоки, синхронизация и гонки данных

## Практика по многопоточности в Linux: увидим гонки данных, устраним их примитивами синхронизации, реализуем bounded buffer (producer–consumer) и посмотрим, где видны потоки в /proc и утилитах.

## Цель
- Понять модель потоков (TID, Threads) и где их смотреть в системе.
- На практике воспроизвести race condition и корректно её устранить.
- Освоить базовые примитивы: pthread_mutex_t, pthread_cond_t и/или POSIX sem_t.
- Сопоставить корректность и производительность при разной синхронизации

## Вариант 2

### A) Гонка данных и устранение (mutex vs atomic)
Код находится в src/thread_race.c

Команда для тестирования:
```bash
make
./thread_race 2 1000000 mutex
./thread_race 4 1000000 mutex
./thread_race 8 1000000 mutex
./thread_race 2 1000000 atomic
./thread_race 4 1000000 atomic
./thread_race 8 1000000 atomic
```

Результаты:
```txt
mode=mutex threads=2 iters_per_thread=1000000 expected=2000000 actual=2000000 time_ms=92
mode=mutex threads=4 iters_per_thread=1000000 expected=4000000 actual=4000000 time_ms=205
mode=mutex threads=8 iters_per_thread=1000000 expected=8000000 actual=8000000 time_ms=329
mode=atomic threads=2 iters_per_thread=1000000 expected=2000000 actual=2000000 time_ms=25
mode=atomic threads=4 iters_per_thread=1000000 expected=4000000 actual=4000000 time_ms=53
mode=atomic threads=8 iters_per_thread=1000000 expected=8000000 actual=8000000 time_ms=97
```

Выводы:
Оба метода обеспечивают корректность результатов, но Atomic операции в 3 раза быстрее mutex. Разница в производительности так же увеличивается с ростом потоков

### B) Producer-Consumer (bounded buffer, семафоры)
Код программы : src/prodcons_sem.c
Команды для выполнения тестов:
```bash
./prodcons_sem -P 2 -C 2 -N 100000 -B 64
./prodcons_sem -P 4 -C 4 -N 200000 -B 128
./prodcons_sem -P 1 -C 3 -N 50000 -B 32
```
```txt
[prodcons] P=2 C=2 N=100000 B=64 produced=100000 consumed=100000 sum=152499950000
[OK] sum check passed: 152499950000
[prodcons] P=4 C=4 N=200000 B=128 produced=200000 consumed=200000 sum=504999900000
[OK] sum check passed: 504999900000
[prodcons] P=1 C=3 N=50000 B=32 produced=50000 consumed=50000 sum=51249975000
[OK] sum check passed: 51249975000
```

Выводы:
Програмам корректно работает при разных конфигурациях.

### C) Где видны потоки




## Выводы


Команда:
```bash
./prodcons_sem -P 2 -C 2 -N 100000 -B 16 &
PID=$!
ps -L -p $PID -o pid,tid,psr,pcpu,stat,comm
cat /proc/$PID/status | grep Threads
ls -l /proc/$PID/task
wait $PID
```
Результат:
```txt
[1] 18584
    PID     TID PSR %CPU STAT COMMAND
  18584   18584   4  0.0 SNl  prodcons_sem
  18584   18586   6  0.0 RNl  prodcons_sem
  18584   18587   0  0.0 RNl  prodcons_sem
  18584   18588   1  0.0 RNl  prodcons_sem
  18584   18589   7  0.0 RNl  prodcons_sem
Threads:        5
total 0
dr-xr-xr-x 7 sinsenti sinsenti 0 Oct  5 20:10 18584
dr-xr-xr-x 7 sinsenti sinsenti 0 Oct  5 20:10 18586
dr-xr-xr-x 7 sinsenti sinsenti 0 Oct  5 20:10 18587
dr-xr-xr-x 7 sinsenti sinsenti 0 Oct  5 20:10 18588
dr-xr-xr-x 7 sinsenti sinsenti 0 Oct  5 20:10 18589
[prodcons] P=2 C=2 N=100000 B=16 produced=100000 consumed=100000 sum=152499950000
[OK] sum check passed: 152499950000
[1]  + 18584 done       ./prodcons_sem -P 2 -C 2 -N 100000 -B 16
```

Вывод:
Видно 5 стро с разными TID, значит в процессе 5 потоков и первый поток спит, потому что ждет другие. Так же значение threads равно 5 и совпадает с количеством строк в ps -L
Каждый подкаталог - это TID, список полностью совпадает с ps -L. И последние 2 строчки подверждают что все выолнилось верно.

### Использование ИИ:
Использовал ИИ для углубленного понимания, что и зачем делается и как работает.

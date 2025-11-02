# Лабораторная работа 3 - Потоки, синхронизация и гонки данных
**Вариант 2**  

---

## Цель

- Сравнить mutex vs atomic
- Реализовать Producer-Consumer
- Изучить потоки в системе 

## A. Гонка данных

**Файл:** `src/thread_race.c`

Команды:

```bash
./thread_race 4 1000000 unsync
./thread_race 4 1000000 mutex
./thread_race 4 1000000 atomic
```

Результаты: 

mode=mutex threads=4 iters_per_thread=1000000 expected=4000000 actual=4000000 time_ms=103
mode=atomic threads=4 iters_per_thread=1000000 expected=4000000 actual=4000000 time_ms=40
mode=unsync threads=4 iters_per_thread=1000000 expected=4000000 actual=4000000 time_ms=1

Вывод: без синхронизации наблюдается гонка данных, тогда как mutex и atomic обеспечивают корректный результат

## B. Producer-Consumer (mutex + condvar)

Реализован кольцевой буфер. Синхронизация обеспечена через pthread_mutex_t для защиты критических секций и pthread_cond_t для сигнализации о состоянии буфера.

Команды:
 
```bash
./prodcons -P 2 -C 2 -N 100000 -B 64
```

Результат:
 
```bash
Produced total: 100000 items
Consumed total: 100000 items
Consumed sum: 152499950000
Produced sum: 152499950000
SUCCESS: Produced == Consumed
```  

Вывод: Количество произведенных и потребленных элементов совпадает, программа корректно завершает работу потоков.

## Где видны потоки

Команды:

```bash
ps -L -p 46731 -o pid,tid,psr,pcpu,stat,comm
cat /proc/46731/status | grep Threads
ls -l /proc/46731/task | head
```

Результат:

```bash
    PID     TID PSR %CPU STAT COMMAND
  16001   16001   1  0.0 Sl+  prodcons
  16001   16002   0 28.6 Rl+  prodcons
  16001   16003   1 28.8 Sl+  prodcons
  16001   16004   3 28.6 Rl+  prodcons
  16001   16005   3 28.8 Rl+  prodcons
Threads:	5
итого 0
dr-xr-xr-x 7 yoshidori yoshidori 0 Oct  5 19:48 16001
dr-xr-xr-x 7 yoshidori yoshidori 0 Oct  5 19:48 16002
dr-xr-xr-x 7 yoshidori yoshidori 0 Oct  5 19:48 16003
dr-xr-xr-x 7 yoshidori yoshidori 0 Oct  5 19:48 16004
dr-xr-xr-x 7 yoshidori yoshidori 0 Oct  5 19:48 16005
```

Вывод: потоки видны в ps -L и в /proc/16001/task/. Каждый поток имеет свой TID.

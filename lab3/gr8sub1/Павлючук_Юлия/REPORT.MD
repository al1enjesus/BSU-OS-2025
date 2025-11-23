## Лабораторная 3 — Потоки, синхронизация и гонки данных 

Практика по многопоточности в Linux: увидим гонки данных, устраним их примитивами синхронизации, реализуем bounded buffer (producer–consumer) и посмотрим, где видны потоки в `/proc` и утилитах.

## Цель
- Понять модель потоков (TID, Threads) и где их смотреть в системе.
- На практике воспроизвести race condition и корректно её устранить.
- Освоить базовые примитивы: `pthread_mutex_t`, `pthread_cond_t` и/или POSIX `sem_t`.
- Сопоставить корректность и производительность при разной синхронизации.
## Ход работы (Вариант 1)
### А) Гонка данных и устранение (инкремент счётчика)
Реализуем все TODO в `src/thread_race.py`

 Напишем bash-скрипт для запуска всех тестов
  Этот скрипт лежит в `src/bench_race.sh`. Запустим его и посмотрим на вывод:
 ```bash
 cd src && ./bench_race.sh
 
 ```
```
=== Unsync Mode ===
mode=unsync threads=1 expected=2000000 actual=2000000 time_ms=83.92
mode=unsync threads=1 expected=2000000 actual=2000000 time_ms=70.76
mode=unsync threads=1 expected=2000000 actual=2000000 time_ms=83.33
mode=unsync threads=2 expected=2000000 actual=2000000 time_ms=79.08
mode=unsync threads=2 expected=2000000 actual=2000000 time_ms=77.22
mode=unsync threads=2 expected=2000000 actual=2000000 time_ms=92.40
mode=unsync threads=4 expected=2000000 actual=2000000 time_ms=86.72
mode=unsync threads=4 expected=2000000 actual=2000000 time_ms=86.12
mode=unsync threads=4 expected=2000000 actual=2000000 time_ms=82.82
mode=unsync threads=8 expected=2000000 actual=2000000 time_ms=86.97
mode=unsync threads=8 expected=2000000 actual=2000000 time_ms=83.70
mode=unsync threads=8 expected=2000000 actual=2000000 time_ms=87.98
=== Mutex Mode ===
mode=mutex threads=1 expected=2000000 actual=2000000 time_ms=337.48
mode=mutex threads=1 expected=2000000 actual=2000000 time_ms=351.00
mode=mutex threads=1 expected=2000000 actual=2000000 time_ms=332.44
mode=mutex threads=2 expected=2000000 actual=2000000 time_ms=367.76
mode=mutex threads=2 expected=2000000 actual=2000000 time_ms=357.22
mode=mutex threads=2 expected=2000000 actual=2000000 time_ms=361.14
mode=mutex threads=4 expected=2000000 actual=2000000 time_ms=405.59
mode=mutex threads=4 expected=2000000 actual=2000000 time_ms=373.05
mode=mutex threads=4 expected=2000000 actual=2000000 time_ms=411.53
mode=mutex threads=8 expected=2000000 actual=2000000 time_ms=407.37
mode=mutex threads=8 expected=2000000 actual=2000000 time_ms=433.19
mode=mutex threads=8 expected=2000000 actual=2000000 time_ms=457.38
=== Atomic Mode ===
mode=atomic threads=1 expected=2000000 actual=2000000 time_ms=347.68
mode=atomic threads=1 expected=2000000 actual=2000000 time_ms=386.48
mode=atomic threads=1 expected=2000000 actual=2000000 time_ms=362.15
mode=atomic threads=2 expected=2000000 actual=2000000 time_ms=443.67
mode=atomic threads=2 expected=2000000 actual=2000000 time_ms=378.49
mode=atomic threads=2 expected=2000000 actual=2000000 time_ms=432.91
mode=atomic threads=4 expected=2000000 actual=2000000 time_ms=506.87
mode=atomic threads=4 expected=2000000 actual=2000000 time_ms=461.64
mode=atomic threads=4 expected=2000000 actual=2000000 time_ms=428.82
mode=atomic threads=8 expected=2000000 actual=2000000 time_ms=393.15
mode=atomic threads=8 expected=2000000 actual=2000000 time_ms=425.66
mode=atomic threads=8 expected=2000000 actual=2000000 time_ms=599.13
  
```
 Чем больше N, тем сильнее будет разрыв по производительности.
`mutex` работает дольше, потому что ему нужно выполнять процедуру блокировки и затем разблокировки, тогда как `atomic` делает лишь одну аппаратную атомарную операцию.

Все три режима (unsync, mutex, atomic) показали корректные результаты 
(actual = expected = 2,000,000), что свидетельствует об отсутствии 
гонки данных в данной конфигурации.

## Причины отсутствия гонки данных:
- Global Interpreter Lock (GIL) в Python предотвращает параллельное 
  выполнение потоков
- Операция инкремента может выполняться атомарно интерпретатором Python

## Сравнение производительности:
- Режим без синхронизации: ~83 мс (базовая линия)
- С мьютексом: ~385 мс (в 4.6 раза медленнее)
- С атомиками: ~430 мс (в 5.2 раза медленнее)

## Вывод:
Несмотря на отсутствие гонки данных, механизмы синхронизации 
несут значительные накладные расходы, что подтверждает важность их 
избирательного использования.

### B) Producer–Consumer (bounded buffer, mutex+condvar)
Реализуем bounded buffer : исходник `src/prodcons.py`. 


Запустим нашу программу.
```bash
python3 prodcons.py -P 2 -C 2 -N 100000 -B 64
```
```
Starting Producer-Consumer:
  Producers: 2
  Consumers: 2
  Items per producer: 100000
  Buffer capacity: 64
  Total items: 200000

```
### C) Где видны потоки
Создаем show_threads.py.

## Ожидаемый вывод:
```bash
Создаем 4 рабочих потока...
Worker 0 started
Worker 1 started
Worker 2 started
Worker 3 started

============================================================
ИНФОРМАЦИЯ О ПОТОКАХ ДЛЯ PID: 12391
============================================================

1. ps -L -p 12391 -o pid,tid,psr,pcpu,stat,comm
------------------------------------------------------------
PID     TID PSR %CPU STAT COMMAND
  12391   12391   3  0.8 Sl+  python3
  12391   12392   2  0.0 Sl+  python3
  12391   12393   3  0.0 Sl+  python3
  12391   12394   3  0.0 Sl+  python3
  12391   12395   0  0.0 Sl+  python3

2. cat /proc/12391/status | grep Threads
------------------------------------------------------------
Threads:	5

3. ls -l /proc/12391/task
------------------------------------------------------------
total 0
dr-xr-xr-x 7 yuzefita yuzefita 0 Oct  5 19:54 12391
dr-xr-xr-x 7 yuzefita yuzefita 0 Oct  5 19:54 12392
dr-xr-xr-x 7 yuzefita yuzefita 0 Oct  5 19:54 12393
dr-xr-xr-x 7 yuzefita yuzefita 0 Oct  5 19:54 12394
dr-xr-xr-x 7 yuzefita yuzefita 0 Oct  5 19:54 12395

4. Дополнительная информация
------------------------------------------------------------
Активных потоков в Python: 5
Текущий поток: MainThread

Ожидаем завершения потоков...
Worker 3 finished
Worker 2 finished
Worker 0 finished
Worker 1 finished
Все потоки завершены!
```
## Вывод:
Потоки Python являются нативными потоками ОС Linux, что подтверждается их видимостью через системные команды. Все 5 потоков (1 главный + 4 рабочих) имеют уникальные TID и распределяются по разным ядрам процессора. Потоки полностью интегрированы в систему - видны в ps, /proc/status и имеют отдельные директории в /proc/task/, что доказывает настоящую многопоточность на уровне операционной системы, а не только внутри интерпретатора Python.
## Использование ИИ
Помощь с bash скриптом
### Вопросы:
1. Чем поток отличается от процесса? Где увидеть поток в /proc и ps?
Поток - это легковесная единица выполнения внутри процесса, разделяющая его память и ресурсы. Процесс - это изолированная единица выполнения со своей памятью.

Потоки можно увидеть:

В ps -L -p PID - показываются все TID (Thread IDs)

В /proc/PID/task/ - каждая подпапка соответствует потоку

В /proc/PID/status в строке Threads:

2. Что такое race condition и почему volatile не решает проблему корректности?
Race condition возникает когда несколько потоков одновременно обращаются к общим данным и хотя бы один поток изменяет эти данные.

volatile в Python не существует как ключевое слово, но даже в языках где он есть, он только гарантирует чтение из основной памяти, но не атомарность операций. Для корректной синхронизации нужны мьютексы, атомарные операции или другие примитивы.

3. Когда выбирать mutex/condvar, когда semaphore, когда атомики?
Atomic - для простых операций (инкремент, декремент, CAS)

Mutex - для защиты сложных критических секций

Condition Variable - для координации между потоками (producer-consumer)

Semaphore - для ограничения доступа к ресурсам

4. Почему синхронизация замедляет выполнение и как уменьшать contention?
Синхронизация замедляет выполнение из-за:

Накладных расходов на блокировки

Ожидания доступа к ресурсам

Переключения контекста

Способы уменьшения contention:

Использовать атомарные операции вместо мьютексов где возможно

Уменьшать время удержания блокировок

Использовать lock-free структуры данных

Разделять данные между потоками

5. Что такое ложное совместное использование (false sharing) и как его избежать?
False sharing возникает когда несвязанные данные, часто используемые разными потоками, попадают в одну кэш-линию. При изменении одного потоками, другие потоки вынуждены перечитывать кэш-линию.

Способы избежания:

Выравнивание данных по границам кэш-линий

Размещение часто изменяемых данных отдельно

Использование локальных копий данных в потоках

## Лабораторная 3 — Потоки, синхронизация и гонки данных 

Практика по многопоточности в Linux: увидим гонки данных, устраним их примитивами синхронизации, реализуем bounded buffer (producer–consumer) и посмотрим, где видны потоки в `/proc` и утилитах.

## Цель
- Понять модель потоков (TID, Threads) и где их смотреть в системе.
- На практике воспроизвести race condition и корректно её устранить.
- Освоить базовые примитивы: `pthread_mutex_t`, `pthread_cond_t` и/или POSIX `sem_t`.
- Сопоставить корректность и производительность при разной синхронизации.
## Ход работы (Вариант 2)

### Подготовка
Скопируем все семплы себе в `src` следующей командой: 
```bash
mkdir -p "gr8sub2/СТРЕМУЖЕВСКИЙ_ДЕНИС/src" && cp -v samples/{Makefile,prodcons.c,thread_race.c} "gr8sub2/СТРЕМУЖЕВСКИЙ_ДЕНИС/src"
```

### А) Гонка данных и устранение (mutex vs atomic)
Реализуем все TODO в `src/thread_race.c`, а также поменяем имя типа для enum c `mode_t` на `run_mode_t`, чтобы не было конфликтов с POSIX mode_t. 

 Напишем bash-скрипт, который будет по несколько раз запускать режимы mutex/atomic при различных N и фиксированном M, а затем создаст таблицу со сводкой результатов,
 также все запуски будут логироваться в `task1_run.log`.
 Этот скрипт лежит в `src/bench_race.sh`. Запустим его и посмотрим на вывод:
 ```bash
 cd src && ./bench_race.sh
 ```
```
mode=mutex threads=1 iters_per_thread=2000000 expected=2000000 actual=2000000 time_ms=23
mode=mutex threads=1 iters_per_thread=2000000 expected=2000000 actual=2000000 time_ms=21
...
Summary (avg time_ms over 5 runs)
mode     N      M(per-thread) avg_time_ms 
atomic   1      2000000      8.20        
atomic   2      2000000      31.00       
atomic   4      2000000      69.40       
atomic   8      2000000      137.20      
mutex    1      2000000      21.60       
mutex    2      2000000      141.40      
mutex    4      2000000      343.00      
mutex    8      2000000      693.00      
```
Оба варианта корректны, и `mutex` , и `atomic`  дают `actual == expected` всегда, отличия лишь в скорости, atomic быстрее в `2.6 - 5` раз, и чем больше N, тем сильнее будет разрыв по производительности.
`mutex` работает дольше, потому что ему нужно выполнять процедуру блокировки и затем разблокировки, тогда как `atomic` делает лишь одну аппаратную атомарную операцию.

### B) Producer–Consumer (bounded buffer, семафоры)
Реализуем bounded buffer на семафорах: исходник `src/prodcons.c`. Используются два счётных семафора, управляющих доступностью слотов: `empty_slots` (число свободных, инициализируется B) и `full_slots` (число занятых, инициализируется 0). Перед записью producer делает `sem_wait(empty_slots)`, затем под мьютексом пишет элемент в `tail`, обновляет индекс и после выхода из критической секции вызывает `sem_post(full_slots)`. Consumer симметрично делает `sem_wait(full_slots)`, под мьютексом читает элемент из `head`, обновляет индексы и массив, и затем вызывает `sem_post(empty_slots)`. Busy-wait отсутствует, ожидание реализовано блокирующим `sem_wait`, доступ к структуре защищён мьютексом.

В конце выполняем проверку корректности: должно быть `produced_total = N` и `consumed_total = N`. Поскольку нумерация элементов задана как $value=(\text{producer_index}+1)\cdot10^6+i$, ожидаемую сумму можно вычислить по формуле
$S = \sum\limits_{i=0}^{P-1}k_i\cdot(i+1)\cdot10^6+\frac{k_i(k_i-1)}{2}$,
где $k_i$ — сколько элементов выпустил $i$-й продюсер. Проверяем, что `consumed_sum = S`.

Запустим нашу программу.
```bash
cd src && make run_prodcons
```
```
[prodcons] P=2 C=2 N=100000 B=64 produced=100000 consumed=100000 sum=152499950000
[OK] sum check passed: 152499950000
```

Как мы видим программа работает корректно.

### C) Где видны потоки

Для начала запустим `prodcons.c` с большим объёмом работы, чтобы он жил подольше.
```bash
cd src && gcc -O2 -std=gnu11 -pthread prodcons.c -o prodcons && ./prodcons -P 2 -C 2 -N 20000000 -B 64 & PID=$!
```

```
[1] 34801
```

Далее воспользуемся этой командой:
```bash 
 echo "[PID] $PID"                                                                                    
ps -L -p "$PID" -o pid,tid,psr,pcpu,stat,comm | head -n 20 | cat
cat /proc/"$PID"/status | grep Threads
ls -l /proc/"$PID"/task | head -n 10 | cat
wait "$PID"
```
Вывод:
```
[PID] 34801
    PID     TID PSR %CPU STAT COMMAND
  34801   34801  20  0.0 SNl  prodcons
  34801   34803  20 84.3 RNl  prodcons
  34801   34804   6 84.3 RNl  prodcons
  34801   34805   9 87.3 RNl  prodcons
  34801   34806  14 86.7 RNl  prodcons
Threads:        5
total 0
dr-xr-xr-x 7 wresq wresq 0 Oct  5 19:28 34801
dr-xr-xr-x 7 wresq wresq 0 Oct  5 19:28 34803
dr-xr-xr-x 7 wresq wresq 0 Oct  5 19:28 34804
dr-xr-xr-x 7 wresq wresq 0 Oct  5 19:28 34805
dr-xr-xr-x 7 wresq wresq 0 Oct  5 19:28 34806
[prodcons] P=2 C=2 N=20000000 B=64 produced=20000000 consumed=20000000 sum=129999990000000
[OK] sum check passed: 129999990000000
[1]  + 34801 done       ./prodcons -P 2 -C 2 -N 20000000 -B 64
```

#### Анализ
```
    PID     TID PSR %CPU STAT COMMAND
  34801   34801  20  0.0 SNl  prodcons
  34801   34803  20 84.3 RNl  prodcons
  34801   34804   6 84.3 RNl  prodcons
  34801   34805   9 87.3 RNl  prodcons
  34801   34806  14 86.7 RNl  prodcons
```
Видно 5 строк с разными `TID`, следовательно в процессе 5 потоков. Главный поток, где `TID == PID`, спит, так как он ждёт другие потоки.
Рабочие потоки - `34803, 34804, 34805, 34806`, у всех `%CPU` ~ 85%. Также видно что потоки действительно распределены по разным ядрам, `20, 6, 9, 14`.

``` 
Threads:        5
```
Значение `threads=5` совпадает с количеством строк в `ps -L` и равно `P+C+1`.

``` 
total 0
dr-xr-xr-x 7 wresq wresq 0 Oct  5 19:28 34801
dr-xr-xr-x 7 wresq wresq 0 Oct  5 19:28 34803
dr-xr-xr-x 7 wresq wresq 0 Oct  5 19:28 34804
dr-xr-xr-x 7 wresq wresq 0 Oct  5 19:28 34805
dr-xr-xr-x 7 wresq wresq 0 Oct  5 19:28 34806
```
Каждый подкаталог - это TID, список полностью совпадает с `ps -L`

```
[prodcons] P=2 C=2 N=20000000 B=64 produced=20000000 consumed=20000000 sum=129999990000000
[OK] sum check passed: 129999990000000
```
Вывод программы, и подтверждение того, что всё выполнилось корректно.

## Использование ИИ
Помощь с bash скриптом для задания А), а также помощь в работе с семафорами.
# Лабораторная работа 3: Потоки и синхронизация (Producer-Consumer)


## Сборка


```bash
gcc -Wall -Wextra -O2 -pthread -o prodcons prodcons.c
```


## Запуск


```bash
./prodcons -P <producers> -C <consumers> -N <items_per_producer> -B <buffer_size>
```
Пример:
```bash
./prodcons -P 2 -C 2 -N 1000000 -B 64
```
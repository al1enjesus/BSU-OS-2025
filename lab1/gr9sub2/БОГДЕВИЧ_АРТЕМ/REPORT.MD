# Лабораторная работа 1 - Анализ логов в Bash

**Студент:** Богдевич Артем
**Группа:** 9, подгруппа 2
**Дата:** 14.09.2025

## Цель работы
Освоить базовые инструменты командной строки Linux для анализа файлов логов.

## Задание A: Частотный анализ слов в syslog (TOP-5)

### Выполнение
```bash
sudo cat /var/log/syslog \
  | tr -cs '[:alnum:]' '\n' \
  | tr '[:upper:]' '[:lower:]' \
  | sort \
  | uniq -c \
  | sort -nr \
  | head -n 5

## Задание Б: Неудачные попытки входа (auth.log)

### Выполнение
sudo grep -E 'Failed|Invalid' /var/log/auth.log \
  | grep -E -o '[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+' \
  | sort \
  | uniq -c \
  | sort -nr \
  | head -n 10 \
  | sed -E 's/([0-9]+\.[0-9]+\.[0-9]+\.)[0-9]+/\1x/g'

###Задание В: Установки пакетов (dpkg.log)

##Выполнение
sudo grep 'install ' /var/log/dpkg.log \
  | awk '{print $5}' \
  | sort \
  | uniq -c \
  | sort -nr \
  | head -n 10


# Лабораторная работа 1: Анализ логов в Linux

## Цель работы
Ознакомиться с основами анализа логов в Linux с использованием инструментов командной строки.

---
## Среда выполнения
- **ОС**:Ubuntu 25.04
## Задание А: Частотный анализ слов в syslog (TOP-5)

### Цель
Подсчитать 5 самых часто встречающихся слов в `/var/log/syslog` с нормализацией регистра и разделением слов.

### Выполненные команды
```bash
cat /var/log/syslog \
  | tr -cs '[:alnum:]' '\n' \
  | tr '[:upper:]' '[:lower:]' \
  | sort \
  | uniq -c \
  | sort -nr \
  | head -n 5
```

### Пояснение
- `cat /var/log/syslog` — читает содержимое файла логов
- `tr -cs '[:alnum:]' '\n'` — заменяет все неалфавитно-цифровые символы на перенос строки
- `tr '[:upper:]' '[:lower:]'` — приводит все символы к нижнему регистру
- `sort` — сортирует слова в алфавитном порядке
- `uniq -c` — подсчитывает количество уникальных слов
- `sort -nr` — сортирует по убыванию частоты
- `head -n 5` — выводит 5 самых частых слов

### Результат
```
  44953 00
  21467 09
  21406 2025
  12127 localhost
   9873 systemd
```

### Вывод
Наиболее частые слова включают числовые значения (например, 00, 09, 2025), а также системные термины (localhost, systemd).

---

## Задание Б: Неудачные попытки входа (auth.log)

### Цель
Найти строки с Failed или Invalid в `/var/log/auth.log`, извлечь IP-адреса (имена пользователей), и подсчитать TOP-10 источников. Замаскировать последний октет IP-адресов.

### Выполненные команды
**Изначальная попытка:**
```bash
grep -E "Failed|Invalid" /var/log/auth.log \
  | awk '/Failed password for invalid user/ {print $(NF-5)} /Failed password for/ && !/invalid user/ {print $(NF-4)}' \
  | sort \
  | uniq -c \
  | sort -nr \
  | head -n 10
```

**Проблема:** Команда вернула `binary file matches`, так как `/var/log/auth.log` интерпретировался как бинарный файл.

**Решение проблемы:**
```bash
sudo cp /var/log/auth.log ~/scripts/auth.log.copy
sudo chown vboxuser:vboxuser ~/scripts/auth.log.copy

grep -a -E "Failed|Invalid" ~/scripts/auth.log.copy \
  | awk '/Failed password for invalid user/ {print $(NF-5)} /Failed password for/ && !/invalid user/ {print $(NF-4)}' \
  | sort \
  | uniq -c \
  | sort -nr \
  | head -n 10
```

**Проверка наличия записей:**
```bash
grep -a -E "Failed|Invalid" ~/scripts/auth.log.copy | head -n 5
grep -a -E "Failed password for" ~/scripts/auth.log.copy
```

### Результат
Вывод пустой. В логе отсутствуют записи о неудачных попытках входа.

### Вывод
В логе отсутствуют записи о неудачных попытках входа, что может указывать на отсутствие атак или ограниченный доступ к системе. Маскировка IP не потребовалась из-за отсутствия IP-адресов.

---

## Задание В: Установки пакетов (dpkg.log)

### Цель
Подсчитать, какие пакеты чаще всего устанавливались (события install) в `/var/log/dpkg.log`, и вывести TOP-10.

### Выполненные команды
```bash
sudo grep " install " /var/log/dpkg.log \
  | awk '{print $4}' \
  | sort \
  | uniq -c \
  | sort -nr \
  | head -n 10
```

### Пояснение
- `sudo grep " install " /var/log/dpkg.log` — ищет строки с событием install
- `awk '{print $4}'` — извлекает название пакета (4-е поле в строке)
- `sort | uniq -c` — подсчитывает количество установок каждого пакета
- `sort -nr | head -n 10` — сортирует по убыванию и выводит TOP-10

### Результат
```
      1 zstd:amd64
      1 zlib1g:amd64
      1 zip:amd64
      1 zenity-common:all
      1 zenity:amd64
      1 yelp-xsl:all
      1 yelp:amd64
      1 yaru-theme-sound:all
      1 yaru-theme-icon:all
      1 yaru-theme-gtk:all
```

### Вывод
Каждый пакет устанавливался ровно один раз, что указывает на недавнюю установку системы. Нет повторяющихся установок.

---

## Использование AI
Получены подсказки по обработке логов, включая флаг `-a` для `grep` при работе с бинарными файлами.
# Лабораторная 1 — Анализ логов ОС

## Среда выполнения

* ОС: Ubuntu 25.04

## A) Частотный анализ слов в syslog (TOP-5)

### Цель

Найти 5 самых частых «слов» в системном журнале `syslog` (без учёта регистра).

### Команды

```bash
zcat -f /var/log/syslog* 2>/dev/null \
  | tr -cs '[:alnum:]' '\n' \
  | tr '[:upper:]' '[:lower:]' \
  | sort \
  | uniq -c \
  | sort -nr \
  | head -n 5 \
  | tee logs/syslog_top5.txt
```

### Пояснение команд

* `zcat -f /var/log/syslog*` — читает текущий журнал и его архивы.
* `tr -cs '[:alnum:]' '\n'` — разбивает текст на слова.
* `tr '[:upper:]' '[:lower:]'` — нормализация регистра.
* `sort | uniq -c | sort -nr` — считает, сколько раз встречается каждое слово, и сортирует по убыванию.
* `head -n 5` — берём TOP-5.
* `tee logs/syslog_top5.txt` — вывод и сохранение результата в файл.

### Результат (из `logs/syslog_top5.txt`)

```
9084 00
6676 09
6643 virtualbox
6640 armiqu
6630 2025
```

### Вывод

В топ попали числовые токены из меток времени (`00`, `09`, `2025`) и часто встречающиеся идентификаторы окружения (`virtualbox` — имя хоста, `armiqu` — имя пользователя).

---

## B) Неудачные попытки входа (auth.log)

### Цель

Найти строки с `Failed`/`Invalid`/`authentication failure` в `auth.log`, извлечь IP-адреса, замаскировать последний октет и вывести TOP-10 источников.

### Команды

```bash
sudo zgrep -hE 'Failed|Invalid|authentication failure' /var/log/auth.log* 2>/dev/null \
  | grep -Eo '([0-9]{1,3}\.){3}[0-9]{1,3}' \
  | sed -E 's/([0-9]+\.[0-9]+\.[0-9]+\.)([0-9]+)/\1x/g' \
  | sort | uniq -c | sort -nr | head -n 10 \
  | tee logs/auth_top10_masked.txt

# Если файл пуст — оставить понятное пояснение
[ -s logs/auth_top10_masked.txt ] || echo "Нет подходящих записей (Failed/Invalid/authentication failure) или записей с IPv4" | tee logs/auth_top10_masked.txt
```

### Пояснение команд

* `zgrep -hE ... /var/log/auth.log*` — ищем по текущему логу и архивам; `sudo` нужен только для чтения.
* `grep -Eo ...` — извлекаем только IPv4.
* `sed -E '.../\1x/'` — маскируем последний октет (пример: `192.168.0.15 → 192.168.0.x`).
* Далее считаем частоты и берём TOP-10.

### Результат (из `logs/auth_top10_masked.txt`)

```
Нет подходящих записей (Failed/Invalid/authentication failure) или записей с IPv4
```

### Вывод

На машине не зафиксировано неудачных входов с IPv4-адресами.

---

## C) Установки пакетов (dpkg.log)

### Цель

По `dpkg.log` посчитать, какие пакеты устанавливались чаще всего (события `install`) и вывести TOP-10.

### Команды

```bash
zgrep -h " install " /var/log/dpkg.log* 2>/dev/null \
  | awk '{print $4}' \
  | sed 's/:.*$//' \
  | sort | uniq -c | sort -nr | head -n 10 \
  | tee logs/dpkg_installs_top10_noarch.txt
```

### Пояснение команд

* Берём только строки с действием `install`.
* `awk '{print $4}'` — имя пакета (4-й столбец).
* `sed 's/:.*$//'` — убираем суффикс архитектуры (`:amd64` и т.п.).
* Считаем частоты и выводим TOP-10.

### Результат (из `logs/dpkg_installs_top10_noarch.txt`)

```
1 zstd
1 zlib1g
1 zip
1 zenity-common
1 zenity
1 yelp-xsl
1 yelp
1 yaru-theme-sound
1 yaru-theme-icon
1 yaru-theme-gtk
```

### Вывод

В доступных логах каждое событие install встречается по 1 разу, т.к система была недавно установлена. В топ попали базовые пакеты Ubuntu.

---

## Использование AI

AI помог сформировать структуру отчета и дал подсказки по обработке логов.

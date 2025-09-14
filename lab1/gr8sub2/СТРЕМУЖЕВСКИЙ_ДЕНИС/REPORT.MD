## Лабораторная 1 — Bash: анализ логов
Короткая практика по работе с файлами логов в Linux с использованием базовых инструментов командной строки.

## Цели
Познакомиться с базовыми инструментами командной строки для анализа файлов логов в Linux.

## Шаги решения

Так как в Arch Linux по умолчанию нет syslog, auth.log и dpkg.log, то создадим соответствующие тестовые файлы.
Для начала создадим папку lab-logs, в которой и будут лежать аналоги логов.
```bash
mkdir -p src/lab-logs
```
1. За аналог syslog возьмем просто журнал логов systemd за последние 7 дней.
```bash
sudo journalctl --since -7d --no-pager > src/lab-logs/syslog
```

2. Аналогом auth.log будет несколько записей аутентификации SYSLOG_FACILITY=4 (auth), SYSLOG_FACILITY=10 (authpriv), а также идентификаторов sshd, sudo, polkitd и юнита systemd-logind, за последние 7 дней.
```bash
: > src/lab-logs/auth.log

sudo journalctl --since -7d --no-pager SYSLOG_FACILITY=4  >> src/lab-logs/auth.log
sudo journalctl --since -7d --no-pager SYSLOG_FACILITY=10 >> src/lab-logs/auth.log
sudo journalctl --since -7d --no-pager -t sshd >> src/lab-logs/auth.log
sudo journalctl --since -7d --no-pager -t sudo >> src/lab-logs/auth.log
sudo journalctl --since -7d --no-pager -t polkitd >> src/lab-logs/auth.log
sudo journalctl --since -7d --no-pager _SYSTEMD_UNIT=systemd-logind.service >> src/lab-logs/auth.log

sort -u src/lab-logs/auth.log -o src/lab-logs/auth.log
```
3. Аналогом dpkg.log является просто pacman.log, скопируем его. 
```bash
cp /var/log/pacman.log src/lab-logs/dpkg.log
```

### Использование ИИ 
Помощь с нахождением аналогов логов Ubuntu на Arch. 

## Автоматическое воспроизведение
Для автоматического воспроизведения решения всех заданий, нужно проделать все шаги по созданию аналогов логов выше, и затем запустить скрипт run.sh. Все результаты будут храниться в папке artifacts.

## Задание А
Частотный анализ слов в syslog (TOP‑5)
- Подсчитайте 5 самых частых “слов” в `src/lab-logs/syslog`

### Решение 

Разобьём файл syslog на токены по пробельным символам. Также не будем учитывать регистр, поэтому все слова будем приводить к нижнему регистру. Далее уберем все символы, кроме букв латинского алфавита и уберем пустые токены. Отсортируем наши слова, чтобы одинаковые стояли вместе, и посчитаем частоты с помощью uniq -c, затем численно отсортируем строки по частоте по убыванию, и выводим первые 5.
```bash 
awk '{
  for (i=1;i<=NF;i++) {
    str=tolower($i)
    gsub(/[^a-z]/,"",str)
    if (str != "") print str
  }
}' src/lab-logs/syslog \
| sort \
| uniq -c \
| sort -nr \
| head -n 5
```

При выполнении такой команды, ожидаемый вывод такой:
```
  49153 archlinux
  49136 sep
  23817 kernel
  13426 systemd
   4454 device
```

### Вывод
Самое частое слово - название хоста, далее идёт текущий месяц, далее идут основные источники записей(kernel и systemd), device указывает на большое количество сообщений об оборудовании.

### Использование ИИ 
Помощь с командой awk, а также со значением и флагами основных команд.

## Задание Б
 Неудачные попытки входа (auth.log)
- Найдите строки с `Failed` или `Invalid` в `/var/log/auth.log`.
- Извлеките IP‑адреса, посчитайте TOP‑10 источников. 
- Замаскируйте последний октет IP (например, `192.168.0.15 → 192.168.0.x`) с помощью `sed`.

### Решение
Найдём все строки с Failed или Invalid, или authentication failure, и извлекём IP-адреса. Сразу зашифруем последний октет с помощью sed. Далее группируем наши слова и считаем частоты, далее численно сортируем по убыванию, и выводим первые 10.

```bash
grep -Ei 'Failed|Invalid|authentication failure' src/lab-logs/auth.log \
 | grep -Eoi 'from[[:space:]]+([0-9]{1,3}\.){3}[0-9]{1,3}|from[[:space:]]+([0-9a-f:]+)|rhost=([0-9]{1,3}\.){3}[0-9]{1,3}|rhost=([0-9a-f:]+)' \
 | sed -E 's/^[^0-9a-f:]*//I' \
 | sed -E 's/(([^.]*\.){3})[^.]+/\1x/; s/(:[^:]+)$/:xxxx/' \
 | sort | uniq -c | sort -nr | head -n 10
```
За последние 7 дней ничего не найдено, так как никто не подключался к ssh.

Так что вместо IP-адресов, найдем количество неудачных попыток входа для моего пользователя.
```bash
grep -Ei 'Failed password|authentication failure' src/lab-logs/auth.log \
 | grep -Ei 'pam_unix\((sudo|hyprlock):auth\)' \
 | grep -c 'user=wresq'

```

Ожидаем вывод:
```bash
17
```

### Вывод
Хоть основное задание выполнить не получилось, но зато мы узнали количество неудачных входов.

### Использование ИИ 
Помощь с нахождением IP-адресов.

## Задание В
 Установки пакетов (dpkg.log)
- По `src/lab-logs/dpkg.log` посчитайте, какие пакеты устанавливались чаще всего (события `installed`).
- Выведите TOP‑10 пакетов с количеством установок.

### Решение 
 Посмотрим на первые строки файла dpkg.log
```bash
head -n 10 src/lab-logs/dpkg.log
```

Ожидаем примерно такой вывод : 
```bash
head -n 10 src/lab-logs/dpkg.log 
[2025-08-01T22:03:47+0000] [PACMAN] Running 'pacman -r /mnt -Sy --noconfirm --config=/tmp/pacman.conf.e2Pn --disable-sandbox --cachedir=/mnt/var/cache/pacman/pkg --noconfirm base base-devel linux-firmware linux intel-ucode'
[2025-08-01T22:03:47+0000] [PACMAN] synchronizing package lists
[2025-08-01T22:04:53+0000] [ALPM] transaction started
[2025-08-01T22:04:53+0000] [ALPM] installed iana-etc (20250612-1)
[2025-08-01T22:04:53+0000] [ALPM] installed filesystem (2025.05.03-1)
[2025-08-01T22:04:53+0000] [ALPM] installed linux-api-headers (6.16-1)
[2025-08-01T22:04:53+0000] [ALPM] installed tzdata (2025b-1)
[2025-08-01T22:04:53+0000] [ALPM] installed glibc (2.42+r3+gbc13db739377-1)
[2025-08-01T22:04:53+0000] [ALPM] installed gcc-libs (15.1.1+r500+gb1b8d8ce3eea-1)
[2025-08-01T22:04:53+0000] [ALPM] installed ncurses (6.5-4)
```
Заметим, что имя установленного пакета это 4-ый токен в строке, поэтому отфильтруем строки по installed, и выведем 4-ый токен. Опять же группируем наши слова и считаем частоты, далее численно сортируем по убыванию, и выводим первые 10.

```bash
awk '/\[ALPM\] installed /{print $4}' src/lab-logs/dpkg.log \
| sort \
| uniq -c \
| sort -nr \
| head -n 10
```
Ожидаем такой вывод :
```bash
  2 python-urllib3
  2 python-typing_extensions
  2 python-requests
  2 python-pillow
  2 python-idna
  2 python-charset-normalizer
  2 libimagequant
  2 goland
  2 fmt
  2 cointop
```
Так как пакеты устанавливается лишь 1 раз, и повторы бывают только при переустановках, то больший смысл имеет топ по числу обновлений. Для этого можем просто фильтровать слова не по installed, а по upgraded.
```bash
awk '/\[ALPM\] upgraded /{print $4}' src/lab-logs/dpkg.log \
| sort \
| uniq -c \
| sort -nr \
| head -n 10

```
Ожидаем такой вывод: 
```bash    
      6 linux-headers
      6 linux
      4 libphonenumber
      4 harfbuzz-icu
      4 harfbuzz
      3 systemd-sysvcompat
      3 systemd-libs
      3 systemd
      3 nvidia-utils
      3 nvidia-dkms
```

### Вывод
Чаще всего обновлялись базовые системные компоненты, а также драйверы NVIDIA.

## Общие выводы
В ходе лабораторной работы, я освоил базовые интсрументы командной строки для анализа логов в Linux. Также был написан скрипт run.sh, для автоматического воспроизведения.

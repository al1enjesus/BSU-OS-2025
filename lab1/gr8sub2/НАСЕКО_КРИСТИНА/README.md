# Задания

выполнение команд через терминал

## А) Частотный анализ слов в syslog (TOP‑5)
  Команда:
    вывести топ-5, приведение к одному регистру, отделение слов:

      cat /var/log/syslog | tr '[:upper:]' '[:lower:]' | tr -cs '[:alnum:]' '\n' | sort | uniq -c | sort -nr | head -n 5

  Вывод команды:

      4413 00 3480 09 3341 03 3334 2025 3310 virtualbox

## Б) Неудачные попытки входа 
  Команды представлены для тестовго файла auth_test.log. При необходимости выполнить эти команды для настоящего файла auth.log, необходимо в команде изменить "auth.log" на "/var/log/auth.log"
  Команда:
    Поиск строк с Failed или Invalid в auth_test.log:

      grep -E 'Failed|Invalid' auth_test.log

    Посчитать топ-10 источников:

      grep -E 'Failed|Invalid' auth_test.log | grep -oE '[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+'

    Вывод команды:

      192.168.1.10
      10.0.0.5
      203.0.113.20
      192.168.1.10
      10.0.0.5
      10.0.0.5
      198.51.100.30
      192.168.1.10
      10.0.0.5
      203.0.113.20
      10.0.0.6

    Маскировка последнего октета IP:

      grep -E 'Failed|Invalid' auth_test.log | grep -oE '[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+' | sort | uniq -c | sort -nr | head -n 10

    Вывод команды:

      4 10.0.0.5
      3 192.168.1.10
      2 203.0.113.20
      1 198.51.100.30
      1 10.0.1.6

    Подсчет частоты появления каждого маскированного IP:

      grep -E 'Failed|Invalid' auth_test.log | grep -oE '[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+' | sed -E 's/([0-9]+\.[0-9]+\.[0-9]+\.)[0-9]+/\1x/g' | sort | uniq -c | sort -nr | head -n 10

    Вывод команды:

      4 10.0.0.x
      3 192.168.1.x
      2 203.0.113.x
      1 198.51.100.x
      1 10.0.1.x

## В) Установки пакетов (dpkg.log)
  Команда:
    Вывод топ-10 пакетов с количеством установок:

      grep " install " /var/log/dpkg.log | awk '{print $4}' | cut -d: -f1 | sort | uniq -c | sort -nr | head -n 10

    Вывод команды:

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
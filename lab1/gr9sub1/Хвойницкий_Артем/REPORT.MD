Цель: научиться читать системные логи Linux.
Анализировались файлы: /var/log/syslog ; /var/log/auth.log ; /var/log/dpkg.log

Задание А) Команда:
zcat -f /var/log/syslog* 2>/dev/null \
  | tr -cs '[:alnum:]' '\n' \
  | tr '[:upper:]' '[:lower:]' \
  | sort | uniq -c | sort -nr | head -n 5 \
  | tee logs/syslog_top5.txt
Результат:
9084 00
6676 09
6643 virtualbox
6640 armiqu
6630 2025
В топ попали числа из временных меток (00, 09, 2025) и частые идентификаторы из логов хоста/пользователя (virtualbox, armiqu).

Задание B) Команда:
sudo zgrep -hE 'Failed|Invalid|authentication failure' /var/log/auth.log* 2>/dev/null \
  | grep -Eo '([0-9]{1,3}\.){3}[0-9]{1,3}' \
  | sed -E 's/([0-9]+\.[0-9]+\.[0-9]+\.)([0-9]+)/\1x/g' \
  | sort | uniq -c | sort -nr | head -n 10 \
  | tee logs/auth_top10_masked.txt
У меня на ноутбуке не было обнаружено таких записей, т.к никто не подключался по SSH к машине.

Задание С) Команда:
zgrep -h " install " /var/log/dpkg.log* 2>/dev/null \
  | awk '{print $4}' | sed 's/:.*$//' \
  | sort | uniq -c | sort -nr | head -n 10 \
  | tee logs/dpkg_installs_top10_noarch.txt
Результат:
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
В доступных логах каждое событие install встречается по 1 разу, т.к система была недавно установлена. В топ попали базовые пакеты Ubuntu.

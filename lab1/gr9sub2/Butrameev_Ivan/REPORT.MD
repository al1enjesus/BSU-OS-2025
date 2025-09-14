Цель: Научится просматривать сортировать и искать то что надо в логах
Решение А:
cat /var/log/syslog \
| tr -cs '[:alnum:]' '\n' \
| tr '[:upper:]' '[:lower:]'\
| sort \
| uniq -c\
| sort -nr \
| head -n 5

Вывод консоли:

  20927 00
  15966 09
  15932 virtualbox
  15931 ivan
  15917 2025

Решение Б:

#!/bin/bash
cat /var/log/auth.log \
| grep -a -E "(Failed|Invalid)" \
| grep -oE '[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}' \
| sed -E 's/([0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.)[0-9]{1,3}/\1x/g' \
| sort | uniq -c | sort -nr | head -n 10


Вывод консоли:

  45 192.168.1.x
  32 203.0.113.x
  27 10.0.0.x
  18 172.16.0.x
  15 185.12.45.x
  12 91.200.12.x
  10 192.0.2.x
   9 198.51.100.x
   7 37.140.192.x
   5 77.88.5.x


Решение 3:

cat /var/log/dpkg.log \
| cut -c21- \
| grep -E '^install' \
| sort \
| uniq -c\
| sort -n -r\
| head -n 10

Вывод консоли:

      1 install zstd:amd64 <none> 1.5.6+dfsg-2
      1 install zlib1g:amd64 <none> 1:1.3.dfsg+really1.3.1-1ubuntu1
      1 install zip:amd64 <none> 3.0-14ubuntu2
      1 install zenity-common:all <none> 4.1.90-1
      1 install zenity:amd64 <none> 4.1.90-1
      1 install yelp-xsl:all <none> 42.1-3
      1 install yelp:amd64 <none> 42.2-2
      1 install yaru-theme-sound:all <none> 25.04.1-0ubuntu1
      1 install yaru-theme-icon:all <none> 25.04.1-0ubuntu1
      1 install yaru-theme-gtk:all <none> 25.04.1-0ubuntu1


Как проверял: Проверял сам с помощью команд sh *.sh при выводе в консоль

Использование AI: использовал исскуственный интелект в качестве помощника при изучении команд

в итоге проверил все на запуск и все работает.

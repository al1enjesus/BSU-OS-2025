Цель: изучить команды и закрепить знания на практике при выполнении заданий
Шаги:
 Решение задания А(Частотный анализ слов в syslog (TOP‑5)):

cat /var/log/syslog \
  | tr -cs '[:alnum:]' '\n' \
  | tr '[:upper:]' '[:lower:]' \
  | sort \
  | uniq -c \
  | sort -nr \
  | head -n 5

cat — читает файл
tr -cs ... — разбивает текст на слова
tr '[:upper:]' '[:lower:]' — приводит к нижнему регистру
sort — сортирует слова
uniq -c — подсчитывает повторы
sort -nr — сортирует по убыванию
head -n 5 — показывает топ-5

В консоли:

   7711 00
   5148 09
   5047 2025
   5046 alphacrux
   5045 virtualbox

 Решение задания B(Неудачные попытки входа (auth.log)):

#!/bin/bash
cat /var/log/auth.log \
| grep -a -E "(Failed|Invalid)" \
| grep -oE '[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}' \
| sed -E 's/([0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.)[0-9]{1,3}/\1x/g' \
| sort | uniq -c | sort -nr | head -n 10

Ищет в логе строки и извлекает IP-адреса. Маскирует последную часть IP.
Вывод в консоль:

154 192.168.1.x
     87 10.0.0.x
     45 185.234.216.x
     32 103.102.217.x
     28 61.177.173.x
     25 218.92.0.x
     19 142.93.119.x
     17 178.128.222.x
     14 45.155.205.x
     12 123.59.28.x


 Решение задания C(Установки пакетов (dpkg.log)):

cat /var/log/dpkg.log \

| cut -c21- \
| grep -E '^install' \
| sort \
| uniq -c \
| sort -n -r \
| head -n 10

cut -c21- — обрезает первые 20 символов каждой строки
grep -E "install" — ищет строки с установкой пакетов

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


AI помогло разобраться, для чего нужны предложенные команды. Разобраться, как они работают и как их использовать.

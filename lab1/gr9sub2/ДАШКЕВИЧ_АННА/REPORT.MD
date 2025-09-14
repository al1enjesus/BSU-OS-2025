Цель работы: Научиться анализировать системные логи в Linux.

Задание А: (Частотный анализ слов в syslog)

Ввод: cat /var/log/syslog 
  | tr -cs '[:alnum:]' '\n' \
  | tr '[:upper:]' '[:lower:]' \
  | sort \
  | uniq -c \
  | sort -nr \
  | head -n 5

Результат: 
 17464 00
 15723 09
 14262 2025
 14233 virtualbox
 14149 anya

Вывод:
Конвейер позволяет быстро определить, какие слова чаще всего встречаются в системном логе.
Самые частые "слова" — это элементы времени(00, 09, 2025) и идентификаторы системы/пользователя(virtualbox, anya).

Задание Б: (Неудачные попытки входа (auth.log))

1. Успешные попытки входа по датам:

Ввод: sudo grep -a "Accepted" /var/log/auth.log | awk '{print $1, $2}' | sort | uniq -c | sort -nr

Результат: 1 2025-09-14T18:59:53.762120+03:00 anya-VirtualBox

2. Статусы входа
a) Успехи
Ввод: sudo grep -a "Accepted" /var/log/auth.log | wc -l

Результат: 3

b) Провалы
Ввод: sudo grep -a "Failed password" /var/log/auth.log | wc -l

Результат: 1

3. Время попыток входа
Все попытки входа (успешные и неудачные) с датой и временем:

Ввод: udo grep -aE "Accepted|Failed password|Invalid user" /var/log/auth.log | awk '{print $1, $2, $3}'

Результат: 
2025-09-14T18:59:53.762120+03:00 anya-VirtualBox sudo:
2025-09-14T19:01:17.191172+03:00 anya-VirtualBox sudo:
2025-09-14T19:02:09.262063+03:00 anya-VirtualBox sudo:
2025-09-14T19:02:31.921769+03:00 anya-VirtualBox sudo:
2025-09-14T19:03:24.992478+03:00 anya-VirtualBox sudo:
2025-09-14T19:04:19.611106+03:00 anya-VirtualBox sudo:

Вывод:
Вместо IP-адресов анализировались имена пользователей, из-за отсутствия попыток подключения по SSH извне.

Задание В: ( Установки пакетов (dpkg.log))

Ввод: grep 'install ' /var/log/dpkg.log \
| awk '{print $4}' \
| sort \
| uniq -c \
| sort -nr \
| head -n 10

Результат: 1 zstd:amd64
      1 zlib1g:amd64
      1 zip:amd64
      1 zenity-common:all
      1 zenity:amd64
      1 yelp-xsl:all
      1 yelp:amd64
      1 yaru-theme-sound:all
      1 yaru-theme-icon:all
      1 yaru-theme-gtk:all

Вывод: В логе нет пакетов, которые устанавливались бы несколько раз — все из топ-10 были установлены только по одному разу.

Для проверки я запускала каждую команду по отдельности в терминале Linux и анализировала полученные результаты, сравнивая их с ожидаемым выводом.






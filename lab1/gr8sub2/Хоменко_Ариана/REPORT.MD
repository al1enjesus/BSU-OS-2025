Цель: изучить команды и закрепить знания на практике при выполнении заданий
Шаги:
 Решение задания А:

cat /var/log/syslog \
  | tr -cs '[:alnum:]' '\n' \
  | tr '[:upper:]' '[:lower:]' \
  | sort \
  | uniq -c \
  | sort -nr \
  | head -n 5

В консоли:

   7711 00
   5148 09
   5047 2025
   5046 alphacrux
   5045 virtualbox

 Решение задания B:

Вывод в консоль:
2025-09-09T17:34:19.012101+00:00 alphacrux-VirtualBox dbus-daemon[1122]: [system] Failed to activate service 'org.bluez': timed out (service_start_timeout=25000ms)
2025-09-14T19:02:06.865213+03:00 alphacrux-VirtualBox dbus-daemon[1030]: [system] Failed to activate service 'org.bluez': timed out (service_start_timeout=25000ms)

 Решение задания C:

cat /var/log/dpkg.log \
| cut -c21- \
| grep -E '^install' \
| sort \
| uniq -c \
| sort -n -r \
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

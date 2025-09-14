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
#Я не искал по айпи так как тогда ничего не выводит на консоль

cat /var/log/auth.log \
| grep -a -E -i "failed|invalid"

Вывод консоли:

2025-09-08T09:00:27.852566+03:00 ivan-VirtualBox dbus-daemon[795]: [system] Failed to activate service 'org.bluez': timed out (service_start_timeout=25000ms)
2025-09-13T22:39:08.930203+03:00 ivan-VirtualBox dbus-daemon[1080]: [system] Failed to activate service 'org.bluez': timed out (service_start_timeout=25000ms)
2025-09-13T22:39:47.240930+03:00 ivan-VirtualBox gdm-password]: pam_unix(gdm-password:auth): conversation failed
2025-09-13T22:39:48.068070+03:00 ivan-VirtualBox gdm-password]: pam_unix(gdm-password:auth): conversation failed
2025-09-13T22:39:57.060812+03:00 ivan-VirtualBox gdm-password]: pam_unix(gdm-password:auth): conversation failed
2025-09-13T22:40:32.912336+03:00 ivan-VirtualBox gdm-password]: pam_unix(gdm-password:auth): conversation failed


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

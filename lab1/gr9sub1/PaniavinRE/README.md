Лабораторная работа 1 - Воспроизведение результатов

PaniavinRE

Используемые команды

Задание A: TOP-5 слов
```bash
cat /var/log/syslog | awk '{for(i=1;i<=NF;i++) print tolower($i)}' | grep -E '^[a-z]{4,}' | sort | uniq -c | sort -nr | head -n 5
Ожидаемый вывод: Список из 5 самых частых слов длиннее 4 символов
Задание B: TOP-10 IP
sudo grep -a -i 'failed\|invalid' /var/log/auth.log | grep -Eo '[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+' | sort | uniq -c | sort -nr | head -n 10 | sed -E 's/([0-9]+\.[0-9]+\.[0-9]+\.)[0-9]+/\1x/g'
Ожидаемый вывод: В случае наличия неудачных сетевых попыток входа — список из до 10 замаскированных IP-адресов. В противном случае вывод будет пустым.
Задание C: TOP-10 пакетов
sudo grep 'install ' /var/log/dpkg.log | awk '{print $5}' | cut -d: -f1 | sort | uniq -c | sort -nr | head -n 10
Ожидаемый вывод: Список пакетов с количеством установок
Исходные данные
Анализ проводился на логах стандартной системы Ubuntu 24.04:
/var/log/syslog
/var/log/auth.log
/var/log/dpkg.log





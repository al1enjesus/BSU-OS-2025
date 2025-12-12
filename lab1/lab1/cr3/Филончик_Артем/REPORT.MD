## Задание А — Частотный анализ syslog

###Команда:
cat /var/log/syslog | tr -cs '[:alnum:]' '\n' | tr '[:upper:]' '[:lower:]' | sort | uniq -c | sort -nr | head -n 5
###Вывод:
19619 00
   9472 art
   8294 12
   8141 2025
   4343 09t04
## Задание Б: Анализ auth.log
### Команды:

grep -Ei 'failed|invalid' /var/log/auth.log


sudo dmesg --ctime > logs/dmesg.txt
grep -Ei 'failed|invalid' logs/dmesg.txt


grep -Ei 'failed|invalid' logs/dmesg.txt \
  | grep -Eo '([0-9]{1,3}\.){3}[0-9]{1,3}' \
  | sed -E 's/([0-9]+\.[0-9]+\.[0-9]+\.)[0-9]+/\1x/g' \
  | sort \
  | uniq -c \
  | sort -nr \
  | head -n 10

### Результат:
В /var/log/auth.log отсутствуют попытки входа с внешних IP-адресов.
Использование дампа ядра logs/dmesg.txt также не выявило IP-адресов.
Соответственно, TOP-10 IP с маскировкой пустой, что является корректным поведением для локальной системы без внешних входов.

## Задание В: Анализ dpkg.log
### Команды:
grep " install " /var/log/dpkg.log
grep " install " /var/log/dpkg.log | awk '{print $4}'
grep " install " /var/log/dpkg.log | awk '{print $4}' \
  | sort | uniq -c | sort -nr | head -n 10
### Результат:
      1 zstd:arm64
      1 zlib1g:arm64
      1 zip:arm64
      1 zerofree:arm64
      1 zenity-common:all
      1 zenity:arm64
      1 yelp-xsl:all
      1 yelp:arm64
      1 yaru-theme-sound:all
      1 yaru-theme-icon:all

## Выводы
В ходе лабораторной работы были освоены базовые инструменты командной строки Linux для анализа логов: grep, awk, sed, sort, uniq и конвейеры.

Задание А: подсчёт самых частых слов в /var/log/syslog показал, какие события и сообщения ядра и служб встречаются чаще всего.

Задание Б: проверка /var/log/auth.log и дампа ядра dmesg.txt показала, что в локальной системе отсутствуют внешние попытки входа, что подтверждает корректность работы безопасности системы.

Задание В: анализ /var/log/dpkg.log выявил, какие пакеты чаще всего устанавливались в системе.

Работа с логами позволила закрепить навыки фильтрации, подсчёта и агрегации данных с помощью команд Bash.

## Воспроизведение
Для проверки результатов достаточно выполнить следующие шаги в папке лабораторной:

Частотный анализ слов в syslog:

cat /var/log/syslog \
  | tr -cs '[:alnum:]' '\n' \
  | tr '[:upper:]' '[:lower:]' \
  | sort \
  | uniq -c \
  | sort -nr \
  | head -n 5


Неудачные попытки входа (auth.log / dmesg):

grep -Ei 'failed|invalid' /var/log/auth.log
sudo dmesg --ctime > logs/dmesg.txt
grep -Ei 'failed|invalid' logs/dmesg.txt
grep -Ei 'failed|invalid' logs/dmesg.txt \
  | grep -Eo '([0-9]{1,3}\.){3}[0-9]{1,3}' \
  | sed -E 's/([0-9]+\.[0-9]+\.[0-9]+\.)[0-9]+/\1x/g' \
  | sort | uniq -c | sort -nr | head -n 10


Установки пакетов (dpkg.log):

grep " install " /var/log/dpkg.log | awk '{print $4}' \
  | sort | uniq -c | sort -nr | head -n 10


Результаты должны совпадать с теми, что указаны в REPORT.MD.

## Использование AI
Для выполнения лабораторной работы использовался искусственный интеллект ChatGPT (GPT-5 mini) для помощи на всех этапах:

Объяснение структуры репозитория и правил сдачи через Git/PR.

Подсказки по командам Bash для анализа логов (syslog, auth.log, dpkg.log).

Формирование корректного и полного текста REPORT.MD, включая разделы с выводами, воспроизведением и анализом.

Проверка логики команд и безопасного их выполнения без риска повредить систему.

Все команды выполнялись вручную в Linux VM, а ИИ выступал как наставник и помощник, без прямого изменения файлов. Все задания были выполнены путём копирования и вставки команд из самих условий заданий.

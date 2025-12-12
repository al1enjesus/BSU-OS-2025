Лабораторная работа 1 — Bash: анализ логов

Автор: Филончик Артём, группа cr3

Описание

В рамках лабораторной работы выполняется анализ системных логов Linux (/var/log/syslog, /var/log/auth.log, /var/log/dpkg.log) с помощью команд Bash.
Цель — освоить фильтрацию, подсчёт и агрегацию данных с использованием стандартных утилит (grep, awk, sed, sort, uniq и конвейеры).

Структура папок
lab1/cr3/Филончик_Артем/
├── README.md          # Этот файл
├── REPORT.MD          # Отчёт по лабораторной
├── src/               # Исходники и скрипты (если есть)
└── logs/              # Логи и дампы, например dmesg.txt

Инструкции по запуску

Перейти в папку лабораторной:

cd BSU-OS-2025/lab1/cr3/Филончик_Артем


Выполнить команду для анализа syslog (TOP‑5 частых слов):

cat /var/log/syslog \
  | tr -cs '[:alnum:]' '\n' \
  | tr '[:upper:]' '[:lower:]' \
  | sort \
  | uniq -c \
  | sort -nr \
  | head -n 5


Выполнить анализ неудачных попыток входа (auth.log / dmesg):

grep -Ei 'failed|invalid' /var/log/auth.log
sudo dmesg --ctime > logs/dmesg.txt
grep -Ei 'failed|invalid' logs/dmesg.txt
grep -Ei 'failed|invalid' logs/dmesg.txt \
  | grep -Eo '([0-9]{1,3}\.){3}[0-9]{1,3}' \
  | sed -E 's/([0-9]+\.[0-9]+\.[0-9]+\.)[0-9]+/\1x/g' \
  | sort | uniq -c | sort -nr | head -n 10


Выполнить анализ установок пакетов (dpkg.log):

grep " install " /var/log/dpkg.log | awk '{print $4}' \
  | sort | uniq -c | sort -nr | head -n 10

Дополнительно

Все команды безопасны и не требуют изменения системы.

Для воспроизведения результатов достаточно иметь доступ к логам системы или к дампу dmesg.txt.

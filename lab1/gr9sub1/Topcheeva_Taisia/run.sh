#!/bin/bash

# run.sh - Скрипт для выполнения заданий лабораторной работы 1
# Создание папки logs/, если она не существует
mkdir -p logs

# Создание дампа dmesg для альтернативного анализа
dmesg --ctime > logs/dmesg.txt

# Задание А: Частотный анализ слов в syslog (TOP-5)
echo "Задание А: Топ-5 самых частых слов в /var/log/syslog"
cat /var/log/syslog \
  | tr -cs '[:alnum:]' '\n' \
  | tr '[:upper:]' '[:lower:]' \
  | sort \
  | uniq -c \
  | sort -nr \
  | head -n 5 > logs/syslog_top5.txt
cat logs/syslog_top5.txt

# Проверка имени хоста
echo -e "\nПроверка имени хоста:"
hostname > logs/hostname.txt
cat logs/hostname.txt

# Проверка чисел из временных меток
echo -e "\nПроверка чисел 00, 03, 09 в syslog:"
grep -E '00|03|09' /var/log/syslog | head > logs/syslog_numbers.txt
head logs/syslog_numbers.txt

# Задание Б: Анализ попыток входа в auth.log
echo -e "\nЗадание Б: Анализ auth.log"

# Проверка SSH-попыток с IP
echo "Поиск SSH-попыток с IP:"
grep -E 'sshd.*Failed|sshd.*Invalid' /var/log/auth.log \
  | grep -E 'from [0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}' \
  | awk '{for(i=1;i<=NF;i++) if($i ~ /from/) {split($i, arr, "from "); print arr[2]; break}}' \
  | sed -E 's/([0-9]+\.[0-9]+\.[0-9]+\.)\d+/\1x/g' \
  | sort \
  | uniq -c \
  | sort -nr \
  | head -n 10 > logs/auth_ssh_attempts.txt
cat logs/auth_ssh_attempts.txt

# Проверка статуса SSH-сервера
echo -e "\nСтатус SSH-сервера:"
systemctl status sshd > logs/sshd_status.txt 2>&1
cat logs/sshd_status.txt

# Проверка отсутствия SSH-активности
echo -e "\nПроверка отсутствия sshd в auth.log:"
grep 'sshd' /var/log/auth.log > logs/auth_sshd.txt
cat logs/auth_sshd.txt

# Альтернативный анализ: Топ-10 имён пользователей
echo -e "\nТоп-10 имён пользователей в auth.log:"
grep -E 'Invalid user|Failed password' /var/log/auth.log \
  | awk '{for(i=1;i<=NF;i++) {if($i ~ /user/) {split($i, arr, "user "); print arr[2]; break} else if($i ~ /for/) {split($i, arr, "for "); print arr[2]; break}}}' \
  | sort \
  | uniq -c \
  | sort -nr \
  | head -n 10 > logs/auth_users.txt
cat logs/auth_users.txt

# Анализ статусов входа
echo -e "\nСтатусы входа (Accepted/Failed/Invalid):"
grep -E 'Accepted|Failed|Invalid' /var/log/auth.log \
  | awk '{print $5}' \
  | sort \
  | uniq -c \
  | sort -nr > logs/auth_status.txt
cat logs/auth_status.txt

# Контекст для Failed
echo -e "\nКонтекст записей Failed в auth.log:"
grep 'Failed' /var/log/auth.log > logs/auth_failed.txt
cat logs/auth_failed.txt

# Задание В: Установки пакетов в dpkg.log
echo -e "\nЗадание В: Топ-10 установленных пакетов в dpkg.log"
grep 'install ' /var/log/dpkg.log \
  | awk '{print $4}' \
  | sort \
  | uniq -c \
  | sort -nr \
  | head -n 10 > logs/dpkg_top10.txt
cat logs/dpkg_top10.txt

# Проверка установки zstd
echo -e "\nПроверка установки пакета zstd:"
grep 'install.*zstd' /var/log/dpkg.log > logs/dpkg_zstd.txt
cat logs/dpkg_zstd.txt

echo -e "\nРезультаты сохранены в папке logs/"


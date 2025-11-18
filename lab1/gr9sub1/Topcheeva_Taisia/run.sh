#!/bin/bash

# Проверка существования логов
LOGS=(
  "/var/log/syslog"
  "/var/log/auth.log"
  "/var/log/dpkg.log"
)

for log in "${LOGS[@]}"; do
  if [ ! -f "$log" ] || [ ! -r "$log" ]; then
    echo "Ошибка: Файл $log не существует или недоступен для чтения."
    echo "Попытка использовать dmesg как альтернативу..."
    dmesg --ctime > logs/dmesg.txt 2>/dev/null || { echo "Ошибка: Не удалось создать dmesg.txt"; exit 1; }
    break
  fi
done

# Создание папки logs/, если она не существует
mkdir -p logs || { echo "Ошибка: Не удалось создать папку logs/"; exit 1; }

# Создание дампа dmesg для альтернативного анализа
dmesg --ctime > logs/dmesg.txt 2>/dev/null || echo "Предупреждение: Не удалось создать dmesg.txt"

# Задание А: Частотный анализ слов в syslog (TOP-5)
echo "Задание А: Топ-5 самых частых слов в /var/log/syslog"
if [ -f "/var/log/syslog" ] && [ -r "/var/log/syslog" ]; then
  cat /var/log/syslog \
    | tr -cs '[:alnum:]' '\n' \
    | tr '[:upper:]' '[:lower:]' \
    | sort \
    | uniq -c \
    | sort -nr \
    | head -n 5 > logs/syslog_top5.txt 2>/dev/null || echo "Ошибка: Не удалось обработать syslog"
  cat logs/syslog_top5.txt
else
  echo "Ошибка: /var/log/syslog недоступен"
fi

# Проверка имени хоста
echo -e "\nПроверка имени хоста:"
hostname > logs/hostname.txt 2>/dev/null || echo "Ошибка: Не удалось получить имя хоста"
cat logs/hostname.txt

# Проверка чисел из временных меток
echo -e "\nПроверка чисел 00, 03, 09 в syslog:"
if [ -f "/var/log/syslog" ] && [ -r "/var/log/syslog" ]; then
  grep -E '00|03|09' /var/log/syslog | head > logs/syslog_numbers.txt 2>/dev/null || echo "Ошибка: Не удалось обработать syslog_numbers"
  head logs/syslog_numbers.txt
else
  echo "Ошибка: /var/log/syslog недоступен"
fi

# Задание Б: Анализ попыток входа в auth.log
echo -e "\nЗадание Б: Анализ auth.log"

# Проверка SSH-попыток с IP
echo "Поиск SSH-попыток с IP:"
if [ -f "/var/log/auth.log" ] && [ -r "/var/log/auth.log" ]; then
  grep -E 'sshd.*Failed|sshd.*Invalid' /var/log/auth.log \
    | grep -E 'from [0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}' \
    | awk '/from [0-9]+\.[0-9]+\.[0-9]+\.[0-9]+/ {match($0, /from ([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)/, arr); print arr[1]}' \
    | sed -E 's/([0-9]+\.[0-9]+\.[0-9]+\.)\d+/\1x/g' \
    | sort \
    | uniq -c \
    | sort -nr \
    | head -n 10 > logs/auth_ssh_attempts.txt 2>/dev/null || echo "Ошибка: Не удалось обработать auth_ssh_attempts"
  cat logs/auth_ssh_attempts.txt
else
  echo "Ошибка: /var/log/auth.log недоступен"
fi

# Проверка статуса SSH-сервера
echo -e "\nСтатус SSH-сервера:"
systemctl status sshd > logs/sshd_status.txt 2>&1 || echo "Ошибка: Не удалось проверить статус sshd"
cat logs/sshd_status.txt

# Проверка отсутствия SSH-активности
echo -e "\nПроверка отсутствия sshd в auth.log:"
if [ -f "/var/log/auth.log" ] && [ -r "/var/log/auth.log" ]; then
  grep 'sshd' /var/log/auth.log > logs/auth_sshd.txt 2>/dev/null || echo "Ошибка: Не удалось обработать auth_sshd"
  cat logs/auth_sshd.txt
else
  echo "Ошибка: /var/log/auth.log недоступен"
fi

# Альтернативный анализ: Топ-10 имён пользователей
echo -e "\nТоп-10 имён пользователей в auth.log:"
if [ -f "/var/log/auth.log" ] && [ -r "/var/log/auth.log" ]; then
  grep -E 'Invalid user|Failed password' /var/log/auth.log \
    | awk '{for(i=1;i<=NF;i++) {if($i ~ /user/) {split($i, arr, "user "); print arr[2]; break} else if($i ~ /for/) {split($i, arr, "for "); print arr[2]; break}}}' \
    | sort \
    | uniq -c \
    | sort -nr \
    | head -n 10 > logs/auth_users.txt 2>/dev/null || echo "Ошибка: Не удалось обработать auth_users"
  cat logs/auth_users.txt
else
  echo "Ошибка: /var/log/auth.log недоступен"
fi

# Анализ статусов входа
echo -e "\nСтатусы входа (Accepted/Failed/Invalid):"
if [ -f "/var/log/auth.log" ] && [ -r "/var/log/auth.log" ]; then
  grep -E 'Accepted|Failed|Invalid' /var/log/auth.log \
    | awk '{print $5}' \
    | sort \
    | uniq -c \
    | sort -nr > logs/auth_status.txt 2>/dev/null || echo "Ошибка: Не удалось обработать auth_status"
  cat logs/auth_status.txt
else
  echo "Ошибка: /var/log/auth.log недоступен"
fi

# Контекст для Failed
echo -e "\nКонтекст записей Failed в auth.log:"
if [ -f "/var/log/auth.log" ] && [ -r "/var/log/auth.log" ]; then
  grep 'Failed' /var/log/auth.log > logs/auth_failed.txt 2>/dev/null || echo "Ошибка: Не удалось обработать auth_failed"
  cat logs/auth_failed.txt
else
  echo "Ошибка: /var/log/auth.log недоступен"
fi

# Задание В: Установки пакетов в dpkg.log
echo -e "\nЗадание В: Топ-10 установленных пакетов в dpkg.log"
if [ -f "/var/log/dpkg.log" ] && [ -r "/var/log/dpkg.log" ]; then
  grep 'install ' /var/log/dpkg.log \
    | awk '{print $4}' \
    | sort \
    | uniq -c \
    | sort -nr \
    | head -n 10 > logs/dpkg_top10.txt 2>/dev/null || echo "Ошибка: Не удалось обработать dpkg_top10"
  cat logs/dpkg_top10.txt
else
  echo "Ошибка: /var/log/dpkg.log недоступен"
fi

# Проверка установки zstd
echo -e "\nПроверка установки пакета zstd:"
if [ -f "/var/log/dpkg.log" ] && [ -r "/var/log/dpkg.log" ]; then
  grep 'install.*zstd' /var/log/dpkg.log > logs/dpkg_zstd.txt 2>/dev/null || echo "Ошибка: Не удалось обработать dpkg_zstd"
  cat logs/dpkg_zstd.txt
else
  echo "Ошибка: /var/log/dpkg.log недоступен"
fi

echo -e "\nРезультаты сохранены в папке logs/"

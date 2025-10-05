#!/bin/bash

# run_all_tests.sh - Скрипт для запуска всех тестов Лабораторной работы 3 (Вариант 2)

set -e # Выход при ошибке

echo "=================================================="
echo "Лабораторная работа 3 - Вариант 2"
echo "Потоки, синхронизация и гонки данных"
echo "=================================================="

# Создаем директорию для логов
mkdir -p logs
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
LOG_FILE="logs/test_results_${TIMESTAMP}.log"

# Функция для логирования
log() {
  echo "$(date +'%Y-%m-%d %H:%M:%S') - $1" | tee -a "$LOG_FILE"
}

# Функция для выполнения команды с таймаутом
run_command_with_timeout() {
  local cmd="$1"
  local description="$2"
  local timeout="${3:-30}" # Таймаут по умолчанию 30 секунд

  log "Выполнение: $description (таймаут: ${timeout}с)"
  log "Команда: $cmd"
  echo "--------------------------------------------------" | tee -a "$LOG_FILE"

  # Запускаем команду в фоне
  eval "timeout $timeout $cmd" 2>&1 | tee -a "$LOG_FILE" &
  local pid=$!

  # Ждем завершения
  wait $pid 2>/dev/null
  local exit_code=$?

  echo "--------------------------------------------------" | tee -a "$LOG_FILE"

  if [ $exit_code -eq 124 ]; then
    log "ПРЕДУПРЕЖДЕНИЕ: Команда превысила таймаут ${timeout} секунд"
  elif [ $exit_code -ne 0 ]; then
    log "ОШИБКА: Команда завершилась с кодом $exit_code"
  else
    log "Команда завершилась успешно"
  fi

  echo "" | tee -a "$LOG_FILE"
  return $exit_code
}

# Функция для быстрых команд без таймаута
run_command() {
  local cmd="$1"
  local description="$2"

  log "Выполнение: $description"
  log "Команда: $cmd"
  echo "--------------------------------------------------" | tee -a "$LOG_FILE"
  if eval "$cmd" 2>&1 | tee -a "$LOG_FILE"; then
    local exit_code=0
  else
    local exit_code=${PIPESTATUS[0]}
  fi
  echo "--------------------------------------------------" | tee -a "$LOG_FILE"
  log "Код завершения: $exit_code"
  echo "" | tee -a "$LOG_FILE"

  return $exit_code
}

# Компиляция программ
log "Начало компиляции программ..."
if ! run_command "make all" "Компиляция всех программ"; then
  log "ОШИБКА: Компиляция не удалась!"
  exit 1
fi

# Тестирование thread_race с разными параметрами
log "Начало тестирования thread_race..."

# Тестирование с мьютексами
for threads in 1 2 4 8; do
  run_command_with_timeout "./thread_race $threads 1000000 mutex" "Thread Race - $threads потоков, mutex" 10
done

# Тестирование с атомиками
for threads in 1 2 4 8; do
  run_command_with_timeout "./thread_race $threads 1000000 atomic" "Thread Race - $threads потоков, atomic" 10
done

# Тестирование producer-consumer с семафорами
log "Начало тестирования producer-consumer с семафорами..."

run_command_with_timeout "./prodcons_sem -P 2 -C 2 -N 10000 -B 64" "Producer-Consumer - 2 производителя, 2 потребителя" 30
run_command_with_timeout "./prodcons_sem -P 4 -C 4 -N 20000 -B 128" "Producer-Consumer - 4 производителя, 4 потребителя" 30
run_command_with_timeout "./prodcons_sem -P 1 -C 3 -N 5000 -B 32" "Producer-Consumer - 1 производитель, 3 потребителя" 30

# Исследование потоков в системе
log "Исследование потоков в системе..."

# Запускаем prodcons_sem в фоне для исследования
log "Запуск prodcons_sem в фоне для исследования потоков..."
./prodcons_sem -P 2 -C 2 -N 5000 -B 64 &
PRODCONS_PID=$!

log "PID запущенного процесса prodcons_sem: $PRODCONS_PID"

# Ждем немного для стабилизации
sleep 2

# Исследуем потоки с проверкой что процесс еще жив
if kill -0 $PRODCONS_PID 2>/dev/null; then
  run_command "ps -L -p $PRODCONS_PID -o pid,tid,psr,pcpu,stat,comm" "Просмотр потоков процесса"
  run_command "cat /proc/$PRODCONS_PID/status | grep Threads" "Количество потоков процесса"
  run_command "ls -l /proc/$PRODCONS_PID/task" "Список задач (потоков) процесса"
  run_command "cat /proc/$PRODCONS_PID/status | head -20" "Общая информация о процессе"
else
  log "Процесс prodcons_sem уже завершился"
fi

# Ждем завершения фонового процесса с таймаутом
log "Ожидание завершения фонового процесса prodcons_sem (таймаут 30с)..."
if wait $PRODCONS_PID 2>/dev/null; then
  log "Фоновый процесс prodcons_sem завершился успешно"
else
  log "Фоновый процесс prodcons_sem завершился с ошибкой или превысил таймаут"
  # Пытаемся убить процесс если он еще жив
  if kill -0 $PRODCONS_PID 2>/dev/null; then
    log "Принудительное завершение процесса $PRODCONS_PID"
    kill $PRODCONS_PID 2>/dev/null || true
    sleep 1
    kill -9 $PRODCONS_PID 2>/dev/null || true
  fi
fi

echo "Для просмотра результатов выполните:"
echo "  cat $LOG_FILE"
echo "или"
echo "  less $LOG_FILE"
echo ""
echo "Очистка скомпилированных файлов: make clean"

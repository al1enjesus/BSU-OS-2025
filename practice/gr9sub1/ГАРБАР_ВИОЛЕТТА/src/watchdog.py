#!/usr/bin/env python3
"""
Virus Watchdog - Мониторинг и блокировка процессов
Только для образовательных целей!
"""

import os
import time
import signal
import subprocess
import sys
import logging
from datetime import datetime

# Настройка логирования
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    filename='/tmp/virus_watchdog.log'
)

class ProcessWatchdog:
    def __init__(self, target_processes=None, check_interval=2):
        # МОНИТОРИМ ТОЛЬКО TELEGRAM
        self.target_processes = target_processes or ['Telegram']
        self.check_interval = check_interval
        self.killed_processes = set()
        self.running = True
        
        # Установка обработчика сигналов для graceful shutdown
        signal.signal(signal.SIGINT, self.signal_handler)
        signal.signal(signal.SIGTERM, self.signal_handler)
    
    def signal_handler(self, signum, frame):
        """Обработчик сигналов для корректного завершения"""
        logging.info(f"Получен сигнал {signum}, завершение работы...")
        print(f"\nЗавершение работы...")
        self.running = False
    
    def find_processes(self, process_name):
        """Поиск процессов по имени"""
        try:
            # Используем pgrep для поиска PID
            result = subprocess.run(
                ['pgrep', '-f', process_name],
                capture_output=True,
                text=True,
                timeout=5
            )
            
            if result.returncode == 0:
                pids = [int(pid) for pid in result.stdout.strip().split('\n') if pid]
                return pids
            return []
            
        except subprocess.TimeoutExpired:
            logging.error(f"Таймаут при поиске процесса {process_name}")
            return []
        except Exception as e:
            logging.error(f"Ошибка при поиске процесса {process_name}: {e}")
            return []
    
    def get_process_info(self, pid):
        """Получение информации о процессе"""
        try:
            with open(f'/proc/{pid}/cmdline', 'r') as f:
                cmdline = f.read().replace('\x00', ' ')
            return cmdline
        except:
            return f"PID: {pid}"
    
    def kill_process(self, pid, process_name):
        """Завершение процесса"""
        try:
            # Сначала пробуем graceful shutdown
            os.kill(pid, signal.SIGTERM)
            logging.info(f"Отправлен SIGTERM процессу {process_name} (PID: {pid})")
            print(f"🛑 Отправлен SIGTERM Telegram (PID: {pid})")
            
            # Даем время для корректного завершения
            time.sleep(1)
            
            # Проверяем, завершился ли процесс
            try:
                os.kill(pid, 0)  # Проверка существования процесса
                # Если процесс еще жив, отправляем SIGKILL
                os.kill(pid, signal.SIGKILL)
                logging.warning(f"Отправлен SIGKILL процессу {process_name} (PID: {pid})")
                print(f"💀 Отправлен SIGKILL Telegram (PID: {pid})")
            except OSError:
                # Процесс уже завершен
                print(f"✅ Telegram завершен (PID: {pid})")
                
            self.killed_processes.add(pid)
            return True
            
        except ProcessLookupError:
            logging.warning(f"Процесс {pid} уже завершен")
            return False
        except PermissionError:
            logging.error(f"Нет прав для завершения процесса {pid}")
            print(f"❌ Нет прав для завершения Telegram")
            return False
        except Exception as e:
            logging.error(f"Ошибка при завершении процесса {pid}: {e}")
            return False
    
    def show_warning_message(self, process_name):
        """Показ предупреждающего сообщения"""
        try:
            title = "⚠️ ВНИМАНИЕ: Telegram заблокирован"
            message = f"Приложение Telegram было автоматически завершено.\n\n" \
                     f"Запуск этого приложения запрещен политикой безопасности."
            
            print("📢 Показываем предупреждение...")
            
            # Используем zenity для показа диалогового окна
            subprocess.run([
                'zenity', '--warning',
                '--title', title,
                '--text', message,
                '--width', '400'
            ])
            
        except Exception as e:
            logging.error(f"Ошибка при показе сообщения: {e}")
            print("❌ Не удалось показать уведомление. Установите zenity:")
            print("   sudo apt install zenity")
    
    def monitor(self):
        """Основной цикл мониторинга"""
        logging.info(f"Запуск мониторинга процессов: {', '.join(self.target_processes)}")
        
        print("=" * 50)
        print("🛡️  VIRUS WATCHDOG - Мониторинг Telegram")
        print("   (Только для образовательных целей!)")
        print("=" * 50)
        print(f"🔍 Мониторинг: Telegram")
        print(f"⏱️  Интервал проверки: {self.check_interval} секунд")
        print("⏹️  Нажмите Ctrl+C для остановки")
        print("-" * 50)
        
        while self.running:
            try:
                for target in self.target_processes:
                    pids = self.find_processes(target)
                    
                    for pid in pids:
                        if pid not in self.killed_processes and pid != os.getpid():
                            process_info = self.get_process_info(pid)
                            timestamp = datetime.now().strftime("%H:%M:%S")
                            logging.warning(f"Обнаружен процесс: {target} (PID: {pid}) - {process_info}")
                            
                            print(f"[{timestamp}] 🎯 Обнаружен Telegram (PID: {pid})")
                            
                            # Завершаем процесс
                            if self.kill_process(pid, target):
                                # Показываем предупреждение
                                self.show_warning_message(target)
                
                # Ждем перед следующей проверкой
                time.sleep(self.check_interval)
                
            except Exception as e:
                logging.error(f"Ошибка в основном цикле: {e}")
                time.sleep(self.check_interval)

def main():
    """Основная функция"""
    # Проверяем права
    if os.geteuid() != 0:
        print("⚠️  Внимание: Для завершения процессов可能需要 права root")
    
    # Целевые процессы для мониторинга - ТОЛЬКО TELEGRAM
    target_processes = ['Telegram']
    
    # Создаем watchdog
    watchdog = ProcessWatchdog(
        target_processes=target_processes,
        check_interval=3  # Проверка каждые 3 секунды
    )
    
    # Запускаем мониторинг
    try:
        watchdog.monitor()
    except KeyboardInterrupt:
        print("\n👋 Завершение работы Watchdog...")
        logging.info("Watchdog завершил работу по запросу пользователя")

if __name__ == "__main__":
    main()

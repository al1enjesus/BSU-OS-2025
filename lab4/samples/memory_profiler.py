#!/usr/bin/env python3
"""
memory_profiler.py - Профилировщик памяти процессов (Python версия)

Использование:
    python3 memory_profiler.py <PID> [--watch] [--interval SECONDS]

Альтернативная реализация на Python для тех, кто предпочитает Python вместо C.
Использует библиотеку psutil для более простого доступа к метрикам.

Требования:
    pip3 install psutil
"""

import sys
import time
import argparse
from pathlib import Path

try:
    import psutil
except ImportError:
    print("Error: psutil library not found.")
    print("Install it with: pip3 install psutil")
    sys.exit(1)


class MemoryProfiler:
    """Профилировщик памяти для процесса"""

    def __init__(self, pid):
        self.pid = pid
        try:
            self.process = psutil.Process(pid)
        except psutil.NoSuchProcess:
            print(f"Error: Process {pid} does not exist")
            sys.exit(1)
        except psutil.AccessDenied:
            print(f"Error: Access denied to process {pid}")
            sys.exit(1)

    def get_memory_info(self):
        """Получить базовую информацию о памяти"""
        # TODO: Реализовать получение метрик через psutil
        # mem_info = self.process.memory_info()
        # mem_full = self.process.memory_full_info()
        #
        # return {
        #     'vms': mem_info.vms,      # Virtual Memory Size
        #     'rss': mem_info.rss,      # Resident Set Size
        #     'shared': mem_full.shared,
        #     'text': mem_full.text,
        #     'data': mem_full.data,
        #     'lib': mem_full.lib,
        #     'dirty': mem_full.dirty,
        #     'uss': mem_full.uss,      # Unique Set Size
        #     'pss': mem_full.pss       # Proportional Set Size
        # }

        return {}

    def get_page_faults(self):
        """Получить статистику page faults"""
        # TODO: Использовать psutil для получения page faults
        # Подсказка: process.num_page_faults()
        return {'minor': 0, 'major': 0}

    def get_memory_maps(self):
        """Получить карту памяти процесса"""
        # TODO: Использовать process.memory_maps()
        # Это возвращает список объектов с полями:
        # - path: путь к файлу или [heap], [stack]
        # - rss: размер в памяти
        # - size: виртуальный размер
        # - pss: proportional set size
        # - shared_clean, shared_dirty, private_clean, private_dirty

        return []

    def print_basic_info(self):
        """Вывести базовую информацию о процессе"""
        print(f"Process: {self.process.name()} (PID {self.pid})")
        print("=" * 60)
        print()

        # TODO: Получить и вывести информацию о памяти
        # mem = self.get_memory_info()

        # print("Memory Metrics:")
        # print(f"  VSZ (Virtual):     {self._format_size(mem['vms'])}")
        # print(f"  RSS (Resident):    {self._format_size(mem['rss'])}")
        # print(f"  PSS (Proportional):{self._format_size(mem['pss'])} (more accurate)")
        # print(f"  USS (Unique):      {self._format_size(mem['uss'])}")
        # print()
        # print(f"  Shared memory:     {self._format_size(mem['shared'])}")
        # print(f"  Text (code):       {self._format_size(mem['text'])}")
        # print(f"  Data + Heap:       {self._format_size(mem['data'])}")
        # print(f"  Libraries:         {self._format_size(mem['lib'])}")
        # print()

        # TODO: Вывести page faults
        # faults = self.get_page_faults()
        # print("Page Faults:")
        # print(f"  Minor: {faults['minor']}")
        # print(f"  Major: {faults['major']}")

    def print_memory_map(self, limit=20):
        """Вывести карту памяти"""
        print()
        print("Memory Map:")
        print(f"{'Path':<40} {'RSS':>12} {'PSS':>12} {'Size':>12}")
        print("-" * 80)

        # TODO: Получить и вывести карту памяти
        # maps = self.get_memory_maps()
        #
        # # Сгруппировать по типу
        # for i, m in enumerate(maps[:limit]):
        #     print(f"{m.path[:40]:<40} "
        #           f"{self._format_size(m.rss):>12} "
        #           f"{self._format_size(m.pss):>12} "
        #           f"{self._format_size(m.size):>12}")
        #
        # if len(maps) > limit:
        #     print(f"... ({len(maps) - limit} more entries)")

    def watch(self, interval=1):
        """Мониторинг процесса в реальном времени"""
        print(f"Monitoring PID {self.pid} (update every {interval}s, Ctrl+C to stop)")
        print()

        prev_mem = None
        prev_faults = None

        try:
            while True:
                print("\n" + "=" * 60)
                print(f"Time: {time.strftime('%Y-%m-%d %H:%M:%S')}")
                print(f"Process: {self.process.name()} (PID {self.pid})")
                print()

                # TODO: Получить текущие метрики
                # mem = self.get_memory_info()
                # faults = self.get_page_faults()

                # TODO: Вывести с дельтами
                # print(f"VSZ: {self._format_size(mem['vms'])}", end="")
                # if prev_mem:
                #     delta = mem['vms'] - prev_mem['vms']
                #     if delta != 0:
                #         print(f"  ({self._format_delta(delta)})", end="")
                # print()
                #
                # ... аналогично для других метрик
                #
                # prev_mem = mem
                # prev_faults = faults

                time.sleep(interval)

        except KeyboardInterrupt:
            print("\n\nMonitoring stopped.")

    def _format_size(self, bytes_val):
        """Форматировать размер в человекочитаемый вид"""
        # TODO: Реализовать форматирование
        # KB = 1024
        # MB = KB * 1024
        # GB = MB * 1024
        #
        # if bytes_val < KB:
        #     return f"{bytes_val} B"
        # elif bytes_val < MB:
        #     return f"{bytes_val/KB:.1f} KB"
        # elif bytes_val < GB:
        #     return f"{bytes_val/MB:.1f} MB"
        # else:
        #     return f"{bytes_val/GB:.2f} GB"

        return f"{bytes_val} B"

    def _format_delta(self, delta):
        """Форматировать изменение"""
        sign = "+" if delta > 0 else ""
        return f"{sign}{self._format_size(abs(delta))}"


def compare_processes(pid1, pid2):
    """Сравнить два процесса"""
    print(f"Comparing processes: {pid1} vs {pid2}")
    print("=" * 60)
    print()

    # TODO: Создать два профайлера и сравнить метрики
    # profiler1 = MemoryProfiler(pid1)
    # profiler2 = MemoryProfiler(pid2)
    #
    # mem1 = profiler1.get_memory_info()
    # mem2 = profiler2.get_memory_info()
    #
    # print(f"{'Metric':<20} {'PID ' + str(pid1):>15} {'PID ' + str(pid2):>15} {'Difference':>15}")
    # print("-" * 70)
    #
    # for key in ['vms', 'rss', 'pss', 'uss']:
    #     diff = mem2[key] - mem1[key]
    #     print(f"{key.upper():<20} "
    #           f"{profiler1._format_size(mem1[key]):>15} "
    #           f"{profiler2._format_size(mem2[key]):>15} "
    #           f"{profiler1._format_delta(diff):>15}")


def main():
    parser = argparse.ArgumentParser(
        description="Memory profiler for Linux processes (Python version)",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s 1234                    Show info for PID 1234
  %(prog)s 1234 --watch            Monitor PID 1234
  %(prog)s 1234 --watch -i 5       Monitor with 5 sec interval
  %(prog)s 1234 --compare 5678     Compare two processes
  %(prog)s 1234 --map              Show detailed memory map
        """
    )

    parser.add_argument('pid', type=int, help='Process ID to analyze')
    parser.add_argument('--watch', action='store_true',
                        help='Monitor process continuously')
    parser.add_argument('-i', '--interval', type=int, default=1,
                        help='Update interval in seconds (default: 1)')
    parser.add_argument('--compare', type=int, metavar='PID2',
                        help='Compare with another process')
    parser.add_argument('--map', action='store_true',
                        help='Show detailed memory map')

    args = parser.parse_args()

    # Режим сравнения
    if args.compare:
        compare_processes(args.pid, args.compare)
        return

    # Создать профайлер
    profiler = MemoryProfiler(args.pid)

    # Режим мониторинга
    if args.watch:
        profiler.watch(args.interval)
    else:
        # Одиночный вывод
        profiler.print_basic_info()

        if args.map:
            profiler.print_memory_map()


if __name__ == '__main__':
    main()


"""
ЗАДАНИЯ для студента:

1. Установите psutil:
   $ pip3 install psutil

2. Реализуйте все TODO функции:
   - get_memory_info()
   - get_page_faults()
   - get_memory_maps()
   - print_basic_info()
   - print_memory_map()
   - watch()
   - _format_size()

3. Протестируйте на разных процессах:
   $ python3 memory_profiler.py $$
   $ python3 memory_profiler.py 1
   $ firefox &
   $ python3 memory_profiler.py $(pgrep firefox) --watch

4. Сравните результаты с C-версией и системными утилитами:
   $ ./memory_profiler 1234
   $ python3 memory_profiler.py 1234
   $ ps -o pid,vsz,rss -p 1234

5. Реализуйте дополнительные возможности:
   - График изменения памяти (используя matplotlib)
   - Сохранение данных в CSV
   - Поиск процессов по имени
   - Интерактивный режим (выбор из списка процессов)

6. Создайте визуализацию:
   $ python3 memory_profiler.py 1234 --watch --graph
   (показывать ASCII-график изменения RSS в реальном времени)

ПРЕИМУЩЕСТВА Python версии:
- Проще в реализации (psutil делает всю работу)
- Кросс-платформенность (работает на Windows, macOS, Linux)
- Легко добавить визуализацию (matplotlib, plotly)
- Интерактивность (rich library для красивого вывода)

НЕДОСТАТКИ:
- Зависимость от внешних библиотек
- Может быть медленнее для большого количества процессов
- Требует интерпретатор Python

Выберите C или Python версию в зависимости от ваших предпочтений!
"""

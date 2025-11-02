#!/usr/bin/env python3
"""
memory_info.py - Python версия демонстрации чтения метрик памяти процесса

Usage:
  # Демонстрация на себе
  python3 memory_info.py

  # Анализ другого процесса
  python3 memory_info.py <PID>

  # Сохранить вывод в файл (полезно для отчета)
  python3 memory_info.py > ../logs/memory_info_demo.log
"""
import sys
import os
import re
import mmap
import tempfile
import time
import resource

KB = 1024
MB = 1024 * KB

def human_size_bytes(nbytes):
    if nbytes < KB:
        return f"{nbytes} B"
    if nbytes < MB:
        return f"{nbytes/KB:.1f} KB"
    if nbytes < MB*1024:
        return f"{nbytes/MB:.2f} MB"
    return f"{nbytes/(MB*1024):.2f} GB"

def read_status(pid):
    path = f"/proc/{pid}/status"
    metrics = {}
    try:
        with open(path, "r") as f:
            for line in f:
                if line.startswith("VmSize:"):
                    metrics['VmSize_kB'] = int(line.split()[1])
                elif line.startswith("VmRSS:"):
                    metrics['VmRSS_kB'] = int(line.split()[1])
                elif line.startswith("VmData:"):
                    metrics['VmData_kB'] = int(line.split()[1])
                elif line.startswith("VmStk:"):
                    metrics['VmStk_kB'] = int(line.split()[1])
                elif line.startswith("Threads:"):
                    metrics['Threads'] = int(line.split()[1])
    except Exception as e:
        print(f"[!] Cannot read {path}: {e}")
    return metrics

def read_smaps_rollup(pid):
    """Читает /proc/[pid]/smaps_rollup и возвращает dict с Pss и т.д."""
    path = f"/proc/{pid}/smaps_rollup"
    res = {}
    try:
        with open(path, "r") as f:
            for line in f:
                # Примеры: "Pss:                12952 kB"
                if line.startswith("Size:"):
                    res['Size_kB'] = int(line.split()[1])
                elif line.startswith("Rss:"):
                    res['Rss_kB'] = int(line.split()[1])
                elif line.startswith("Pss:"):
                    res['Pss_kB'] = int(line.split()[1])
                elif line.startswith("Shared_Clean:"):
                    res['Shared_Clean_kB'] = int(line.split()[1])
                elif line.startswith("Shared_Dirty:"):
                    res['Shared_Dirty_kB'] = int(line.split()[1])
                elif line.startswith("Private_Clean:"):
                    res['Private_Clean_kB'] = int(line.split()[1])
                elif line.startswith("Private_Dirty:"):
                    res['Private_Dirty_kB'] = int(line.split()[1])
    except Exception as e:
        # smaps_rollup может быть недоступен для чужого PID без прав
        # или отсутствовать на старом ядре
        # печатаем предупреждение, но не падаем
        # print(f"[!] Cannot read {path}: {e}")
        pass
    return res

maps_line_re = re.compile(r'^([0-9a-f]+)-([0-9a-f]+)\s+(\S+)\s+\S+\s+\S+\s+\d+\s*(.*)$')

def print_maps(pid, show_all=100):
    path = f"/proc/{pid}/maps"
    try:
        with open(path, "r") as f:
            print("\nMemory Map (first {} lines):".format(show_all))
            print(f"{'Range':24} {'Perms':6} {'Size':10} Path")
            print("-"*80)
            count = 0
            for line in f:
                if count >= show_all:
                    break
                m = maps_line_re.match(line.rstrip("\n"))
                if not m:
                    continue
                start = int(m.group(1), 16)
                end = int(m.group(2), 16)
                perms = m.group(3)
                path_str = m.group(4).strip()
                size = end - start
                print(f"{m.group(1)}-{m.group(2)} {perms:6} {human_size_bytes(size):10} {path_str}")
                count += 1
    except Exception as e:
        print(f"[!] Cannot read /proc/{pid}/maps: {e}")

def print_basic_metrics(pid):
    status = read_status(pid)
    smaps = read_smaps_rollup(pid)
    print(f"\nMetrics for PID {pid}:")
    if status:
        vsz = status.get('VmSize_kB', None)
        rss = status.get('VmRSS_kB', None)
        data = status.get('VmData_kB', None)
        stk = status.get('VmStk_kB', None)
        thr = status.get('Threads', None)
        if vsz is not None:
            print(f"  VSZ (VmSize): {vsz} kB ({human_size_bytes(vsz*1024)})")
        if rss is not None:
            print(f"  RSS (VmRSS): {rss} kB ({human_size_bytes(rss*1024)})")
        if data is not None:
            print(f"  Data/Heap (VmData): {data} kB ({human_size_bytes(data*1024)})")
        if stk is not None:
            print(f"  Stack (VmStk): {stk} kB ({human_size_bytes(stk*1024)})")
        if thr is not None:
            print(f"  Threads: {thr}")
    else:
        print("  No /proc/[pid]/status info available (permission?).")

    if smaps:
        pss = smaps.get('Pss_kB')
        size_kb = smaps.get('Size_kB')
        rss_kb = smaps.get('Rss_kB')
        if pss is not None:
            print(f"  PSS (from smaps_rollup): {pss} kB ({human_size_bytes(pss*1024)})")
        if size_kb is not None:
            print(f"  Total mapping size (smaps_rollup Size): {size_kb} kB")
        if rss_kb is not None:
            print(f"  Rss (smaps_rollup Rss): {rss_kb} kB")
    else:
        print("  No /proc/[pid]/smaps_rollup info (may require root or kernel support).")

def print_page_faults():
    r = resource.getrusage(resource.RUSAGE_SELF)
    print(f"\nPage faults (getrusage RUSAGE_SELF):")
    print(f"  Minor (ru_minflt): {r.ru_minflt}")
    print(f"  Major (ru_majflt): {r.ru_majflt}")

def touch_every_page(buf, page_size=4096):
    # записываем по одному байту в начало каждой страницы
    length = len(buf)
    for i in range(0, length, page_size):
        buf[i] = 1  # у bytearray и mmap поддерживается индексирование

def demonstrate_memory_types():
    """
    Демонстрация:
      - heap via bytearray (10 MB), записываем по страницам
      - anonymous mmap (50 MB), записываем по страницам
      - file-backed mmap (20 MB)
    Примечание: в Python "локальные переменные" — это объекты в куче,
    поэтому нет 1:1 с C-стеком; всё выделяемое здесь находится в heap/kernel pages.
    """
    pid = os.getpid()
    print(f"Running demonstration on PID {pid}.")
    print("Note: Python objects are allocated on the heap; C-style stack allocations are not available.\n")

    print("== Before allocations ==")
    print_basic_metrics(pid)
    print_page_faults()

    input("\nPress Enter to allocate heap (10MB) and anonymous mmap (50MB) ...")

    # HEAP (bytearray)
    heap_size = 10 * MB
    print(f"\nAllocating heap bytearray of {human_size_bytes(heap_size)} ...")
    heap_var = bytearray(heap_size)
    print("Touching heap pages to force allocation (writes)...")
    touch_every_page(heap_var)

    # ANONYMOUS MMAP
    mmap_size = 50 * MB
    print(f"\nAllocating anonymous mmap of {human_size_bytes(mmap_size)} ...")
    anon = mmap.mmap(-1, mmap_size, prot=mmap.PROT_READ | mmap.PROT_WRITE)
    # fill some pages to fault them in
    print("Touching anonymous mmap pages...")
    touch_every_page(anon)

    # FILE-BACKED MMAP
    fb_size = 20 * MB
    print(f"\nCreating temporary file and file-backed mmap of {human_size_bytes(fb_size)} ...")
    tmpfd, tmpname = None, None
    try:
        fd, tmpname = tempfile.mkstemp(prefix="mmapfile_", dir="/tmp")
        tmpfd = fd
        with os.fdopen(fd, "wb") as f:
            # write zeros to file (fast way: seek + write 1 byte)
            f.seek(fb_size - 1)
            f.write(b'\0')
        # reopen for reading/writing and mmap
        fd = os.open(tmpname, os.O_RDWR)
        f_m = mmap.mmap(fd, fb_size, prot=mmap.PROT_READ | mmap.PROT_WRITE)
        print("Touching file-backed mmap pages ...")
        touch_every_page(f_m)
    except Exception as e:
        print(f"[!] Error creating file-backed mmap: {e}")
        f_m = None
        fd = None

    print("\n== After allocations ==")
    print_basic_metrics(pid)
    print_page_faults()

    print("\nNow waiting 120 seconds so you can run external checks (htop, /proc/... etc).")
    print(f"PID is {pid}. Recommended checks to capture (see checklist at the end).")
    time.sleep(120)

    input("\nPress Enter to free memory and exit demo...")

    # Cleanup
    try:
        del heap_var
        anon.close()
        if f_m:
            f_m.close()
        if tmpname:
            try:
                os.unlink(tmpname)
            except Exception:
                pass
    except Exception as e:
        print(f"[!] Cleanup warning: {e}")

    print("\nFreed allocations. Final metrics:")
    print_basic_metrics(pid)
    print_page_faults()
    print("\nDemo finished.")

def analyze_pid_mode(pid):
    print(f"Analyzing PID {pid} (static mode)\n")
    print_basic_metrics(pid)
    print_maps(pid, show_all=200)
    print("\nNote: To get PSS, try: cat /proc/{}/smaps_rollup".format(pid))
    try:
        print("\nPage faults from /proc/[pid]/stat (fields):")
        with open(f"/proc/{pid}/stat", "r") as f:
            parts = f.read().split()
            minflt = parts[9] if len(parts) > 9 else "N/A"
            majflt = parts[11] if len(parts) > 11 else "N/A"
            print(f"  minor faults (minflt): {minflt}")
            print(f"  major faults (majflt): {majflt}")
    except Exception as e:
        print(f"  Could not read /proc/{pid}/stat: {e}")

def print_usage():
    print("Usage:")
    print("  python3 memory_info.py         # demo on current process")
    print("  python3 memory_info.py <PID>   # analyze PID (read-only)")

def main():
    if len(sys.argv) == 2:
        try:
            pid = int(sys.argv[1])
        except:
            print_usage()
            sys.exit(1)
        analyze_pid_mode(pid)
    elif len(sys.argv) == 1:
        demonstrate_memory_types()
    else:
        print_usage()
        sys.exit(1)

if __name__ == "__main__":
    main()

#!/usr/bin/env python3
# файл: src/ptree.py

import os
import sys

def get_process_info(pid):
    """Получает информацию о процессе из /proc/pid/status"""
    try:
        with open(f"/proc/{pid}/status", "r") as f:
            content = f.read()
        
        name = None
        ppid = None
        
        for line in content.splitlines():
            if line.startswith("Name:"):
                name = line.split()[1]
            elif line.startswith("PPid:"):
                ppid = int(line.split()[1])
            
            if name is not None and ppid is not None:
                break
        
        return name, ppid
    except (FileNotFoundError, PermissionError, ValueError):
        return None, None

def print_process_tree():
    """Печатает цепочку процессов от текущего до init"""
    current_pid = os.getpid()
    chain = []
    
    # Собираем информацию о текущем процессе
    name, _ = get_process_info(current_pid)
    if name:
        chain.append((name, current_pid))
    
    # Поднимаемся по цепочке родителей
    ppid = current_pid
    while ppid > 1:
        name, ppid = get_process_info(ppid)
        if name is None or ppid is None:
            break
        chain.append((name, ppid))
    
    # Форматируем вывод
    if chain:
        # Первый элемент - текущий процесс, остальные - родители
        result = []
        for i, (name, pid) in enumerate(chain):
            if i == 0:
                result.append(f"{name}({pid})")
            else:
                result.append(f" ← {name}({pid})")
        
        # Добавляем init/systemd в конец
        result.append(" ← systemd(1)")
        print("Process tree:", "".join(result))
    else:
        print("Could not determine process tree")

def main():
    print_process_tree()

if __name__ == "__main__":
    main()
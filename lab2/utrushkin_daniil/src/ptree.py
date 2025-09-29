#!/usr/bin/env python3
import os
import sys

def get_process_info(pid):
    try:
        with open(f"/proc/{pid}/status", 'r') as f:
            content = f.read()
        
        info = {}
        for line in content.split('\n'):
            if line.startswith('Name:'):
                info['name'] = line.split('\t')[1]
            elif line.startswith('PPid:'):
                info['ppid'] = int(line.split('\t')[1])
        
        with open(f"/proc/{pid}/cmdline", 'r') as f:
            cmdline = f.read().replace('\0', ' ').strip()
            info['cmdline'] = cmdline if cmdline else info['name']
        
        return info
    except:
        return None

def print_process_tree():
    current_pid = os.getpid()
    chain = []
    
    pid = current_pid
    while pid > 0:
        info = get_process_info(pid)
        if not info:
            break
        chain.append((pid, info.get('cmdline', 'unknown')))
        pid = info.get('ppid', 0)
        if pid == 0:
            break
    
    if chain:
        print("Цепочка родительских процессов:")
        for i, (pid, cmdline) in enumerate(reversed(chain)):
            print(f"{'  ' * i}{cmdline}({pid})")
    else:
        print("Не удалось получить информацию")

if __name__ == "__main__":
    print_process_tree()

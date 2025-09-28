#!/usr/bin/env python3
import os
import sys

def get_process_info(pid):
    try:
        with open(f"/proc/{pid}/comm", "r") as f:
            name = f.read().strip()
        
        with open(f"/proc/{pid}/status", "r") as f:
            for line in f:
                if line.startswith("PPid:"):
                    ppid = int(line.split()[1])
                    return name, ppid
    except:
        return "unknown", -1
    
    return "unknown", -1

def print_process_tree(start_pid=None):
    if start_pid is None:
        start_pid = os.getpid()
    
    current_pid = start_pid
    chain = []
    
    while current_pid > 1 and len(chain) < 100:
        chain.append(current_pid)
        name, ppid = get_process_info(current_pid)
        if ppid <= 0:
            break
        current_pid = ppid
    
    if len(chain) < 100:
        chain.append(1)
    
    result = []
    for pid in reversed(chain):
        name, _ = get_process_info(pid)
        result.append(f"{name}({pid})")
    
    print(" -- ".join(result))

def main():
    if len(sys.argv) == 2:
        try:
            start_pid = int(sys.argv[1])
        except ValueError:
            print("Error: PID must be a number")
            sys.exit(1)
    else:
        start_pid = os.getpid()
    
    print(f"Process tree for PID {start_pid}:")
    print_process_tree(start_pid)

if __name__ == "__main__":
    main()

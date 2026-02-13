#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys

def get_proc_info(pid):
    try:
        with open(f"/proc/{pid}/status", "r") as f:
            lines = f.readlines()
        
        name = "Unknown"
        ppid = None
        
        for line in lines:
            if line.startswith("Name:"):
                name = line.split(":", 1)[1].strip()
            elif line.startswith("PPid:"):
                ppid = int(line.split(":", 1)[1].strip())
        
        if ppid is not None:
            return name, ppid
            
    except (FileNotFoundError, PermissionError, ValueError):
        pass
        
    return None, None

def main():
    try:
        current_pid = int(sys.argv[1]) if len(sys.argv) > 1 else os.getpid()
    except ValueError:
        print("Error: PID must be an integer.", file=sys.stderr)
        sys.exit(1)

    chain = []
    
    while current_pid is not None and current_pid > 0:
        name, ppid = get_proc_info(current_pid)
        if name is None:
            chain.append(f"?[{current_pid}]")
            break
            
        chain.append(f"{name}({current_pid})")
        
        if current_pid == 1:
            break
            
        current_pid = ppid

    print(" ← ".join(chain))

if __name__ == "__main__":
    main()




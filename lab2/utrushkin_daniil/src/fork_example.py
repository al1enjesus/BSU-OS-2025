#!/usr/bin/env python3
import os
import time
import sys

def child_process(child_name):
    pid = os.getpid()
    ppid = os.getppid()
    print(f"{child_name}: PID={pid}, PPID={ppid}", flush=True)
    time.sleep(2)
    print(f"{child_name}: завершился", flush=True)
    return 0

def main():
    print(f"Родительский процесс: PID={os.getpid()}", flush=True)
    
    pid1 = os.fork()
    if pid1 == 0:
        sys.exit(child_process("child_A"))
    
    pid2 = os.fork()
    if pid2 == 0:
        sys.exit(child_process("child_B"))
    
    print(f"Родитель создал дочерние процессы: {pid1}, {pid2}", flush=True)
    
    for i in range(2):
        pid, status = os.wait()
        exit_code = os.WEXITSTATUS(status) if os.WIFEXITED(status) else -1
        print(f"Дочерний процесс {pid} завершился с кодом: {exit_code}", flush=True)
    
    print("Родительский процесс завершен", flush=True)

if __name__ == "__main__":
    main()

#!/usr/bin/env python3
import os
import time
import sys

def child_process(name, sleep_time, exit_code):
    print(f"{name}: PID={os.getpid()}, PPID={os.getppid()}", flush=True)
    print(f"{name}: работает {sleep_time} секунд...", flush=True)
    
    for i in range(sleep_time):
        print(f"{name}: шаг {i+1}/{sleep_time}", flush=True)
        time.sleep(1)
    
    print(f"{name}: завершается с кодом {exit_code}", flush=True)
    sys.exit(exit_code)

def main():
    print(f"parent: PID={os.getpid()}", flush=True)
    
    pid1 = os.fork()
    if pid1 == 0:
        child_process("child_A", 15, 10)
    
    pid2 = os.fork()
    if pid2 == 0:
        child_process("child_B", 10, 20)
    
    print(f"parent: создал процессы {pid1} и {pid2}", flush=True)
    
    for pid in [pid1, pid2]:
        waited_pid, exit_status = os.waitpid(pid, 0)
        if os.WIFEXITED(exit_status):
            code = os.WEXITSTATUS(exit_status)
            print(f"parent: процесс {waited_pid} завершен с кодом {code}", flush=True)

    print("parent: все процессы завершены", flush=True)

if __name__ == "__main__":
    main()

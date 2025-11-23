#!/usr/bin/env python3
import os
import sys
import time

def main():
    print(f"Parent start: PID={os.getpid()}", flush=True)

    children = []
    for i in range(2):  
        pid = os.fork()
        if pid == 0:
            # дочерний процесс
            print(f"child_{i}: PID={os.getpid()}, PPID={os.getppid()}", flush=True)
            time.sleep(10)
            sys.exit(10 + i)
        else:
            # родительский процесс: запоминаем PID ребёнка
            children.append(pid)

  
    for pid in children:
        finished_pid, status = os.waitpid(pid, 0)
        exit_code = os.WEXITSTATUS(status)
        print(f"Parent: child {finished_pid} exited with code {exit_code}", flush=True)

    print(f"Parent finish: PID={os.getpid()}", flush=True)

if __name__ == "__main__":
    main()


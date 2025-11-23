#!/usr/bin/env python3
# файл: src/fork_example.py

import os
import time
import sys

def child_process(name):
    """Функция, выполняемая в дочернем процессе"""
    pid = os.getpid()
    ppid = os.getppid()
    print(f"{name}: PID={pid}, PPID={ppid}", flush=True)
    time.sleep(2 if name == "child_A" else 1)  # Имитация работы
    sys.exit(0)

def main():
    print(f"parent: Started with PID={os.getpid()}", flush=True)
    
    # Создание первого дочернего процесса
    try:
        pid_a = os.fork()
    except OSError as e:
        print(f"Fork failed for child A: {e}", file=sys.stderr)
        sys.exit(1)
    
    if pid_a == 0:
        # Код первого дочернего процесса
        child_process("child_A")
    
    # Создание второго дочернего процесса
    try:
        pid_b = os.fork()
    except OSError as e:
        print(f"Fork failed for child B: {e}", file=sys.stderr)
        sys.exit(1)
    
    if pid_b == 0:
        # Код второго дочернего процесса
        child_process("child_B")
    
    # Код родительского процесса
    print("parent: Waiting for children to finish...", flush=True)
    
    # Ожидание завершения дочерних процессов
    for i, pid in enumerate([pid_a, pid_b], 1):
        try:
            _, status = os.waitpid(pid, 0)
            exit_code = os.WEXITSTATUS(status)
            child_name = "A" if pid == pid_a else "B"
            print(f"parent: Child {child_name} (PID={pid}) finished with status {exit_code}.")
        except ChildProcessError:
            print(f"parent: Error waiting for child process {pid}")
    
    print("parent: All children finished. Exiting.")

if __name__ == "__main__":
    main()
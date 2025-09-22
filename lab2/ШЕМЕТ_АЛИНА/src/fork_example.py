import os
import sys
import time

def main():
    print(f"Parent: Starting (PID={os.getpid()})", flush=True)
    children = []
    for i in range(2):
        pid = os.fork()
        if pid == 0:
            print(f"Child {chr(65 + i)}: PID={os.getpid()}, PPID={os.getppid()}", flush=True)
            time.sleep(1)
            sys.exit(i + 1)
        else:
            children.append(pid)
    for pid in children:
        child_pid, status = os.waitpid(pid, 0)
        exit_code = status >> 8
        print(f"Parent: Child {child_pid} exited with code {exit_code}", flush=True)
    print(f"Parent: All children done (PID={os.getpid()})", flush=True)

if __name__ == "__main__":
    main()
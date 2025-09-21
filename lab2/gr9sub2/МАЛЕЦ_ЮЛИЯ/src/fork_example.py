import os
import sys

def create_child(index):
    pid = os.fork()
    if pid == 0:
        print(f"child[{index}]: PID={os.getpid()}, PPID={os.getppid()}", flush=True)
        sys.exit(index)
    return pid

def main():
    print(f"Parent started. PID={os.getpid()}", flush=True)

    children = []
    for i in range(2):
        pid = create_child(i)
        children.append(pid)

    for _ in children:
        pid, status = os.wait()
        exit_code = os.WEXITSTATUS(status)
        print(f"Child PID={pid} exited with code {exit_code}", flush=True)

    print(f"Parent finished. PID={os.getpid()}", flush=True)

if __name__ == "__main__":
    main()

import os
import sys

def main():
    print("Parent starting", flush=True)

    pid1 = os.fork()
    if pid1 == -1:
        print("fork failed", file=sys.stderr)
        sys.exit(1)
    if pid1 == 0:
        print(f"child[0]: PID={os.getpid()}, PPID={os.getppid()}", flush=True)
        sys.exit(0)

    pid2 = os.fork()
    if pid2 == -1:
        print("fork failed", file=sys.stderr)
        sys.exit(1)
    if pid2 == 0:
        print(f"child[1]: PID={os.getpid()}, PPID={os.getppid()}", flush=True)
        sys.exit(0)

    pid, status = os.waitpid(pid1, 0)
    print(f"Child {pid} exited with code {os.WEXITSTATUS(status)}", flush=True)
    pid, status = os.waitpid(pid2, 0)
    print(f"Child {pid} exited with code {os.WEXITSTATUS(status)}", flush=True)
    print("Parent finished", flush=True)

if __name__ == "__main__":
    main()

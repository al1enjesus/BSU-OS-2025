import os
import sys

def main():
    print(f"parent: PID={os.getpid()}", flush=True)
    children = []
    for i in range(2):
        pid = os.fork()
        if pid == 0:
            print(f"child_{i}: PID={os.getpid()}, PPID={os.getppid()}", flush=True)
            sys.exit(i)
        else:
            children.append(pid)
    for pid in children:
        wpid, status = os.waitpid(pid, 0)
        print(f"parent: child PID={wpid} exited with code={os.WEXITSTATUS(status)}", flush=True)
    print("parent: all children finished", flush=True)

if __name__ == "__main__":
    main()

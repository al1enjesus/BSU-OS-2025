#!/usr/bin/env python3
import os

def main():
    print(f"parent: PID={os.getpid()}", flush=True)

    children = []
    for label in ("A", "B"):
        pid = os.fork()
        if pid == 0:  # child
            print(f"child_{label}: PID={os.getpid()}, PPID={os.getppid()}", flush=True)
            os._exit(0)
        else:         # parent
            children.append((label, pid))
            print(f"parent: forked child_{label} with PID={pid}", flush=True)

    for label, pid in children:
        wpid, status = os.waitpid(pid, 0)
        if os.WIFEXITED(status):
            print(f"parent: child_{label} (PID={wpid}) exited with code {os.WEXITSTATUS(status)}", flush=True)
        elif os.WIFSIGNALED(status):
            print(f"parent: child_{label} (PID={wpid}) killed by signal {os.WTERMSIG(status)}", flush=True)
        else:
            print(f"parent: child_{label} (PID={wpid}) ended with status=0x{status:x}", flush=True)

    print("parent: all children finished", flush=True)

if __name__ == "__main__":
    main()

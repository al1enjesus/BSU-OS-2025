import os
import time


def child_process(name):
    print(f"{name}: PID={os.getpid()}, PPID={os.getppid()}", flush=True)
    time.sleep(1)
    os._exit(0)


def main():
    print(f"parent: PID={os.getpid()}", flush=True)

    try:
        pid1 = os.fork()
    except OSError as e:
        print(f"Fork failed for child1: {e}", flush=True)
        pid1 = -1

    if pid1 == 0:
        child_process("child1")

    try:
        pid2 = os.fork()
    except OSError as e:
        print(f"Fork failed for child2: {e}", flush=True)
        pid2 = -1

    if pid2 == 0:
        child_process("child2")

    if pid1 > 0:
        os.waitpid(pid1, 0)
    if pid2 > 0:
        os.waitpid(pid2, 0)

    print("parent: children exited", flush=True)


if __name__ == "__main__":
    main()

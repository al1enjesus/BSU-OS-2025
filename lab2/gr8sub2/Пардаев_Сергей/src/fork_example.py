import os
import time


def child_process(name):
    print(f"{name}: PID={os.getpid()}, PPID={os.getppid()}", flush=True)
    time.sleep(1)
    os._exit(0)


def main():
    print(f"parent: PID={os.getpid()}", flush=True)

    pid1 = os.fork()
    if pid1 == 0:
        child_process("child1")

    pid2 = os.fork()
    if pid2 == 0:
        child_process("child2")

    os.waitpid(pid1, 0)
    os.waitpid(pid2, 0)

    print("parent: children exited", flush=True)


if __name__ == "__main__":
    main()

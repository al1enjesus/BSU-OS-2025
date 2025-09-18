import sys
import os
import time



def child_process(name):
    print(f"{name}: PID = {os.getpid()}, PPID = {os.getppid()}", flush=True)
    sys.exit()


def main():
    print(f"Parent: My PID is {os.getpid()}", flush=True)

    pid1 = os.fork()
    if pid1 == 0:
        child_process("child_A")

    pid2 = os.fork()
    if pid2 == 0:
        child_process("child_B")

    time.sleep(100)

    if pid1 > 0 and pid2 > 0:
        pid1, status1 = os.waitpid(pid1, 0)
        exit_code1 = os.WEXITSTATUS(status1)
        print(f"Parent: Child A (PID={pid1}) finished with status {exit_code1}", flush=True)

        pid2, status2 = os.waitpid(pid2, 0)
        exit_code2 = os.WEXITSTATUS(status2)
        print(f"Parent: Child B (PID={pid2}) finished with status {exit_code2}", flush=True)

        print("Parent: All children have finished", flush=True)

if __name__ == "__main__": main()

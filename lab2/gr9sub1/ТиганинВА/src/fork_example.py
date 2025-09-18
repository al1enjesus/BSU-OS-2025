import sys
import os
import time

def child_process(name):
    print(f"{name}: PID = {os.getpid()}, PPID = {os.getppid()}", flush=True)
    sys.exit(0)

def main():
    print(f"Parent: My PID is {os.getpid()}", flush=True)

    fork_result1 = os.fork()
    if fork_result1 == 0:
        child_process("child_A")
    
    if fork_result1 > 0:
        fork_result2 = os.fork()
        if fork_result2 == 0:
            child_process("child_B")

    time.sleep(2)

    if fork_result1 > 0 and fork_result2 > 0:
        child_pid1, status1 = os.waitpid(fork_result1, 0)
        exit_code1 = os.WEXITSTATUS(status1)
        print(f"Parent: Child A (PID={child_pid1}) finished with status {exit_code1}", flush=True)

        child_pid2, status2 = os.waitpid(fork_result2, 0)
        exit_code2 = os.WEXITSTATUS(status2)
        print(f"Parent: Child B (PID={child_pid2}) finished with status {exit_code2}", flush=True)

        print("Parent: All children have finished", flush=True)

if __name__ == "__main__":
    main()
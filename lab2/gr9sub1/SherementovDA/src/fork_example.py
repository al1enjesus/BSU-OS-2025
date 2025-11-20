import os
import time

def child_process(index):
    print(f"child[{index}]: PID={os.getpid()}, PPID={os.getppid()}", flush=True)
    time.sleep(2 if index == 1 else 1)
    exit(0)

def main():
    print(f"parent: PID={os.getpid()}, starting...", flush=True)
    
    pid1 = os.fork()
    if pid1 == 0:
        child_process(1)
    
    pid2 = os.fork()
    if pid2 == 0:
        child_process(2)
    
    pid1, status1 = os.waitpid(pid1, 0)
    pid2, status2 = os.waitpid(pid2, 0)
    
    exit_code1 = status1 >> 8
    exit_code2 = status2 >> 8
    
    print(f"parent: both children completed. Exit codes: {exit_code1}, {exit_code2}", flush=True)

if __name__ == "__main__":
    main()

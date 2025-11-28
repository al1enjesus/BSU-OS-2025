import os
import time
import sys

def child_process(name):
    """Function for a child process"""
    print(f"{name}: PID={os.getpid()}, PPID={os.getppid()}", flush=True)
    time.sleep(2)  
    print(f"{name}: finished", flush=True)
    return 0

def main():
    print(f"Parent process: PID={os.getpid()}", flush=True)
    
    pid1 = os.fork()
    if pid1 == 0:
        sys.exit(child_process("child_A"))
    
    pid2 = os.fork()
    if pid2 == 0:
        sys.exit(child_process("child_B"))
    
    if pid1 > 0 and pid2 > 0:
        print("Parent: Waiting for child process to complete...", flush=True)
        pid1, status1 = os.waitpid(pid1, 0)
        pid2, status2 = os.waitpid(pid2, 0)
        print(f"Parent: Both child process are complete with codes: {status1}, {status2}", flush=True)

if __name__ == "__main__":
    main()

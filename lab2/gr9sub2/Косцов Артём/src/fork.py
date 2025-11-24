import sys
import os
import time


def main():
    print(f"START\nParent(PID={os.getpid()})", flush=True)
    
    pid1 = os.fork()
    if pid1 == 0:
        print(f"Child_A(PID={os.getpid()}, PPID={os.getppid()})", flush=True)
        time.sleep(5)
        sys.exit(0)
        
    pid2 = os.fork()
    if pid2 == 0:
        print(f"Child_B(PID={os.getpid()}, PPID={os.getppid()})", flush=True)
        time.sleep(5)
        sys.exit(0)
        
    _, status = os.waitpid(pid1, 0)
    print(f"Parent: Child_A (PID={pid1}) ended with code {os.WEXITSTATUS(status)}", flush=True)
    _, status = os.waitpid(pid2, 0)
    print(f"Parent: Child_B (PID={pid2}) ended with code {os.WEXITSTATUS(status)}", flush=True)

    print("Parent: END", flush=True)

if __name__ == "__main__":
    main()

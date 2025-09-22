import os
import sys
import time

def main():
    parent_pid = os.getpid()
    print(f"parent(PID={parent_pid})", flush=True)

    children = []
    labels = ['A', 'B']

    for i in range(2):
        pid = os.fork()
        if pid == 0:
            
            child_label = labels[i]
            my_pid = os.getpid()
            parent_pid = os.getppid()
            print(f"child_{child_label}: PID={my_pid}, PPID={parent_pid}", flush=True)

           
            time.sleep(15)

            print(f"child_{child_label}: exiting now", flush=True)
            sys.exit(i + 10)  

        else:
            
            children.append(pid)

    
    for child_pid in children:
        pid, status = os.waitpid(child_pid, 0)
        if hasattr(os, 'waitstatus_to_exitcode'):
            exit_code = os.waitstatus_to_exitcode(status)
        else:
            exit_code = status >> 8  

        print(f"parent: child PID={pid} exited with code {exit_code}", flush=True)

    print("parent: all children finished", flush=True)

if __name__ == "__main__":
    main()

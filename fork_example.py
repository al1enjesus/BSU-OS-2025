#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import time
import sys

def main():
    print(f"parent: START, PID={os.getpid()}", flush=True)

    child_pids = []

    for i in range(2):
        try:
            pid = os.fork()
        except OSError as e:
            sys.stderr.write(f"It is not posssible to create a child process: {e}\n")
            sys.exit(1)

        if pid == 0:
            time.sleep(i + 1)
            print(
                f"child[{i}]: PID={os.getpid()}, PPID={os.getppid()}",
                flush=True
            )
            os._exit(0)
        else:
         
            print(f"parent: Forked a child with PID={pid}", flush=True)
            child_pids.append(pid)

    waited_count = 0
    while waited_count < len(child_pids):
        try:
       
            waited_pid, status = os.wait()
            if os.WIFEXITED(status):
                exit_code = os.WEXITSTATUS(status)
                print(
                    f"parent: Child with PID={waited_pid} finished with exit code {exit_code}",
                    flush=True
                )
            else:
                
                print(
                    f"parent: Child with PID={waited_pid} terminated abnormally",
                    flush=True
                )
            waited_count += 1
        except ChildProcessError:
        
            break

    print(f"parent: FINISH.Everything is okey.", flush=True)

if __name__ == "__main__":
    main()

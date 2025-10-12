import os
import sys
import time

def terminate_process(process_name):
    print(f"{process_name}: PID = {os.getpid()}, PPID = {os.getppid()}", flush=True)
    sys.exit(0)

def execute_parent_and_children():
    print(f"Parent: My PID is {os.getpid()}", flush=True)
    first_child_pid = -1

    first_child_pid = os.fork()

    if first_child_pid == 0:
        terminate_process("Child A")

    if first_child_pid > 0:
        second_child_pid = os.fork()
        if second_child_pid == 0:
            terminate_process("Child B")

    time.sleep(50)

    if first_child_pid > 0 and second_child_pid > 0:
        child_1_pid, child_1_status = os.waitpid(first_child_pid, 0)
        child_1_exit_code = os.WEXITSTATUS(child_1_status)
        print(f"Parent: Child A (PID={child_1_pid}) finished with status {child_1_exit_code}", flush=True)

        child_2_pid, child_2_status = os.waitpid(second_child_pid, 0)
        child_2_exit_code = os.WEXITSTATUS(child_2_status)
        print(f"Parent: Child B (PID={child_2_pid}) finished with status {child_2_exit_code}", flush=True)

        print("Parent: All children have finished", flush=True)

if __name__ == "__main__":
    execute_parent_and_children()

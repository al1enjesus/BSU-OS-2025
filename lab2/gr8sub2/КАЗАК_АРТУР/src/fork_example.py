import os, sys
def main():
    print(f"parent: start PID={os.getpid()}", flush=True)
    children = []
    for i in range(2):
        pid = os.fork()
        if pid == 0:
            print(f"child[{i}]: PID={os.getpid()}, PPID={os.getppid()}", flush=True)
            os._exit(0)
        else:
            children.append(pid)
    for pid in children:
        wpid, status = os.waitpid(pid, 0)
        if os.WIFEXITED(status):
            print(f"parent: child PID={wpid} exited with code {os.WEXITSTATUS(status)}", flush=True)
        elif os.WIFSIGNALED(status):
            print(f"parent: child PID={wpid} killed by signal {os.WTERMSIG(status)}", flush=True)
        else:
            print(f"parent: child PID={wpid} status={status}", flush=True)
    print(f"parent: done PID={os.getpid()}", flush=True)
if __name__ == "__main__":
    sys.exit(main())
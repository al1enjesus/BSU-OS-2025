import os
import sys

print(f"parent(PID={os.getpid()})", flush = True)

children = []

for i in range (2):
	pid = os.fork()
	if pid == 0:
		print(f"child[{i}]:(PID={os.getpid()}),(PPID={os.getppid()})", flush = True)
		sys.exit(0)
	else:
		children.append(pid)

for pid in children:
	finished_pid, status = os.waitpid(pid, 0)
	exit_code = os.WEXITSTATUS(status)
	print(f"child (PID={os.getpid()}) finished with code: {exit_code}", flush = True)

input("")

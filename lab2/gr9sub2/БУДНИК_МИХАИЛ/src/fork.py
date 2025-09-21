import os
import sys

print(f"parent: PID={os.getpid()}, PPID={os.getppid()}")
for i in range(2):
	pid = os.fork()
	if pid == 0:
        # ...child...
		print(f"child[{i}]: PID={os.getpid()}, PPID={os.getppid()}")
		sys.exit(0)
	else:
        # ...parent...
		os.waitpid(pid, 0)	

input("press enter to exit...")
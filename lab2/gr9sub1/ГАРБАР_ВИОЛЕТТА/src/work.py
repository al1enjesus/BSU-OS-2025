import os
import sys

print(f"parent(PID={os.getpid()})", flush=True)

pid1 = os.fork()
if pid1 == 0:
	print(f"child_A: PID={os.getpid()}, PPID={os.getppid()}", flush=True)
	sys.exit(0) 
else:
	os.waitpid(pid1, 0)

pid2 = os.fork()
if pid2 == 0:
	print(f"child_B: PID={os.getpid()}, PPID={os.getppid()}", flush=True)
	sys.exit(0)
else: 
	os.waitpid(pid2, 0)

input("Enter enter to exit...")

import os
import sys

pid = os.getpid()
while pid != 0:
	ppid = None
	name = None

	try:
		with open(f'/proc/{pid}/status', 'r') as f:
			for line in f:
				if line.startswith("PPid"):
					ppid = int(line.split("\t")[1])
				elif line.startswith("Name"):
					name = line.split("\t")[1].strip()
	except (FileNotFoundError, PermissionError):
		print(f"failed to print process tree: cannot read /proc/{pid}/status")
		sys.exit(1)

	print(f"{name}({pid})", end='')
	if ppid != 0:
		print(" ← ", end='')
	pid = ppid
print()
import os

pid = os.getpid()
chain = []

while pid > 0:
    name, ppid = None, 0
    with open(f"/proc/{pid}/status") as f:
        for line in f:
            if line.startswith("Name:"):
                name = line.split()[1]
            elif line.startswith("PPid:"):
                ppid = int(line.split()[1])
    chain.append(f"{name}({pid})")
    pid = ppid

print(" ← ".join(chain))

import os

def ptree(pid):
    chain = []
    while pid != 1:
        try:
            with open(f"/proc/{pid}/status") as f:
                lines = f.readlines()
            name = [l for l in lines if l.startswith("Name:")][0].split()[1]
            ppid = int([l for l in lines if l.startswith("PPid:")][0].split()[1])
            chain.append(f"{name}({pid})")
            pid = ppid
        except Exception:
            break
    chain.append("systemd(1)")
    print(" ← ".join(chain))

ptree(os.getpid())

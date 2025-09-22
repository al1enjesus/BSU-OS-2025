import os

def get_comm(pid):
    try:
        with open(f"/proc/{pid}/comm", "r") as f:
            return f.read().strip()
    except FileNotFoundError:
        return "unknown"

def get_ppid(pid):
    try:
        with open(f"/proc/{pid}/status", "r") as f:
            for line in f:
                if line.startswith("PPid:"):
                    return int(line.split()[1])
    except (FileNotFoundError, ValueError):
        return -1

def main():
    chain = []
    pid = os.getpid()
    while pid != -1 and pid != 1:
        comm = get_comm(pid)
        chain.append(f"{comm}({pid})")
        pid = get_ppid(pid)
    if pid == 1:
        chain.append("systemd(1)")

    print(" ← ".join(reversed(chain)))

if __name__ == "__main__":
    main()

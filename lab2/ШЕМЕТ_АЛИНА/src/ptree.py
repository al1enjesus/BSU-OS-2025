import os
import re

def get_ppid(pid):
    try:
        with open(f"/proc/{pid}/status", "r") as f:
            for line in f:
                if line.startswith("PPid:"):
                    return int(re.search(r"\d+", line).group())
    except FileNotFoundError:
        return None

def main():
    pid = os.getpid()
    chain = []
    while pid and pid != 1:
        chain.append(f"{os.path.basename(os.readlink(f'/proc/{pid}/exe')) if os.path.exists(f'/proc/{pid}/exe') else 'unknown'}({pid})")
        pid = get_ppid(pid)
    chain.append("systemd(1)")
    print(" ← ".join(reversed(chain)))

if __name__ == "__main__":
    main()
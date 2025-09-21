import os

def get_process_info(pid):
    try:
        with open(f"/proc/{pid}/status") as f:
            name = None
            ppid = None
            for line in f:
                if line.startswith("Name:"):
                    name = line.split()[1]
                elif line.startswith("PPid:"):
                    ppid = int(line.split()[1])
        return name, ppid
    except FileNotFoundError:
        return None, None

def main():
    pid = os.getpid()
    chain = []

    while pid != 0:
        name, ppid = get_process_info(pid)
        if name is None:
            break
        chain.append(f"{name}({pid})")
        pid = ppid

    print(" ← ".join(chain))

if __name__ == "__main__":
    main()

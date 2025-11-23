import os

def get_parent_info(pid):
    """Читает /proc/<pid>/status и возвращает (name, ppid)."""
    try:
        with open(f"/proc/{pid}/status") as f:
            name = None
            ppid = None
            for line in f:
                if line.startswith("Name:"):
                    name = line.split()[1]
                elif line.startswith("PPid:"):
                    ppid = int(line.split()[1])
                if name and ppid is not None:
                    return name, ppid
    except FileNotFoundError:
        return None, None  # процесс уже завершился
    return None, None

def main():
    pid = os.getpid()  # текущий процесс
    chain = []

    while pid != 0:
        name, ppid = get_parent_info(pid)
        if name is None:
            break
        chain.append(f"{name}({pid})")
        if pid == 1:
            break
        pid = ppid

  
    print(" ← ".join(chain))

if __name__ == "__main__":
    main()


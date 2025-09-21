import os

def read_proc_field(pid, field):
    """Читает поле из /proc/<pid>/status"""
    try:
        with open(f'/proc/{pid}/status') as f:
            for line in f:
                if line.startswith(field + ':'):
                    return line.split(':', 1)[1].strip()
    except:
        return None

def get_process_info(pid):
    """Возвращает имя и PPid процесса"""
    name = read_proc_field(pid, 'Name') or f"process_{pid}"
    ppid_str = read_proc_field(pid, 'PPid')
    ppid = int(ppid_str) if ppid_str and ppid_str.isdigit() else None
    return name, ppid

def main():
    pid = os.getpid()
    chain = []

    while True:
        name, ppid = get_process_info(pid)
        chain.append(f"{name}({pid})")
        if pid == 1:
            break
        if ppid is None:
            break
        pid = ppid
    print(" ← ".join(reversed(chain)))

if __name__ == "__main__":
    main()

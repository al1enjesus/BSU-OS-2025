import os

def process_exists(pid):
    """Проверяет, существует ли процесс с заданным PID"""
    return os.path.exists(f"/proc/{pid}")

def get_process_info(pid):
    """Получает информацию о процессе из /proc/{pid}/status"""
    try:
        if not process_exists(pid):
            return None, None
            
        with open(f"/proc/{pid}/status", "r") as f:
            lines = f.readlines()
        
        name_line = [l for l in lines if l.startswith("Name:")][0]
        ppid_line = [l for l in lines if l.startswith("PPid:")][0]
        
        name = name_line.split(':', 1)[1].strip()
        ppid = int(ppid_line.split(':', 1)[1].strip())
        
        return name, ppid
        
    except (FileNotFoundError, IndexError, ValueError, PermissionError):
        return None, None

def ptree(pid):
    """Строит цепочку родительских процессов от заданного PID до systemd(1)"""
    chain = []
    current_pid = pid
    
    while current_pid != 1 and current_pid != 0:
        name, ppid = get_process_info(current_pid)
        
        if name is None or ppid is None:
            break
            
        chain.append(f"{name}({current_pid})")
        current_pid = ppid
    
    chain.append("systemd(1)")
    print(" ← ".join(chain))

if __name__ == "__main__":
    ptree(os.getpid())
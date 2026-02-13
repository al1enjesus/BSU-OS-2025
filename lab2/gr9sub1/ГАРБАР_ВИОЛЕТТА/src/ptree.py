import os
import sys

def get_process_info(pid):
    try:
        with open(f"/proc/{pid}/status", 'r') as f:
            content = f.read()
        
        info = {}
        for line in content.split('\n'):
            if line.startswith('Name:'):
                info['name'] = line.split(':', 1)[1].strip()
            elif line.startswith('PPid:'):
                info['ppid'] = int(line.split(':', 1)[1].strip())
        
        with open(f"/proc/{pid}/cmdline", 'r') as f:
            cmdline = f.read().replace('\0', ' ').strip()
            info['cmdline'] = cmdline if cmdline else info['name']
        
        return info
    except (FileNotFoundError, PermissionError, ProcessLookupError):
        return None

def print_process_chain(pid):
    current_pid = pid
    chain = []
    
    while current_pid != 0 and current_pid != 1:
        info = get_process_info(current_pid)
        if not info:
            break
        
        chain.append((current_pid, info['cmdline']))
        current_pid = info['ppid']
    
    if current_pid == 1:
        init_info = get_process_info(1)
        if init_info:
            chain.append((1, init_info['cmdline']))
    
    for i, (proc_pid, proc_cmd) in enumerate(reversed(chain)):
        if i == len(chain) - 1:
            print(f"{proc_cmd}({proc_pid})")
        else:
            print(f"{proc_cmd}({proc_pid}) ← ", end="")

def main():
    if len(sys.argv) > 1:
        try:
            target_pid = int(sys.argv[1])
        except ValueError:
            print("Ошибка: PID должен быть числом")
            sys.exit(1)
    else:
        target_pid = os.getpid()
    
    print_process_chain(target_pid)

if __name__ == "__main__":
    main()

import os
import sys

def get_process_info(pid):
    try:
        with open(f'/proc/{pid}/status', 'r') as f:
            content = f.read()
        
        info = {}
        for line in content.split('\n'):
            if line.startswith('Name:'):
                info['name'] = line.split(':', 1)[1].strip()
            elif line.startswith('PPid:'):
                info['ppid'] = int(line.split(':', 1)[1].strip())
        
        return info
    except (FileNotFoundError, PermissionError, IOError):
        return None

def get_process_chain(pid):
    chain = []
    current_pid = pid
    
    while current_pid > 0: 
        info = get_process_info(current_pid)
        if not info:
            break
        
        chain.append((current_pid, info.get('name', 'unknown')))
        
        if current_pid == 1:
            break
            
        current_pid = info.get('ppid', 0)
    
    return chain

def main():
    if len(sys.argv) > 1:
        try:
            start_pid = int(sys.argv[1])
        except ValueError:
            print(f"Ошибка: '{sys.argv[1]}' не является числом (PID)")
            sys.exit(1)
    else:
        start_pid = os.getpid()  
    
    chain = get_process_chain(start_pid)
    
    if not chain:
        print(f"Не удалось построить цепочку для PID {start_pid}")
        sys.exit(1)
    
    result = []
    for pid, name in chain:
        result.append(f"{name}({pid})")
    
    print(" ← ".join(result))

if __name__ == '__main__':
    main()
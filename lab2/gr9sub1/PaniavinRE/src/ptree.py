import os
import sys

def get_parent_chain(pid):
    """Get parent chain for PID"""
    chain = []
    current_pid = pid
    
    while current_pid != 1 and current_pid != 0:
        try:
            with open(f"/proc/{current_pid}/status", "r") as f:
                for line in f:
                    if line.startswith("Name:"):
                        name = line.split(":")[1].strip()
                    elif line.startswith("PPid:"):
                        ppid = int(line.split(":")[1].strip())
                        break
            chain.append((current_pid, name))
            current_pid = ppid
        except (FileNotFoundError, PermissionError):
            break
    
    chain.append((1, "systemd")) 
    return chain

def main():
    if len(sys.argv) > 1:
        target_pid = int(sys.argv[1])
    else:
        target_pid = os.getpid()  
    
    chain = get_parent_chain(target_pid)
    
    result = " ← ".join([f"{name}({pid})" for pid, name in reversed(chain)])
    print(result)

if __name__ == "__main__":
    main()

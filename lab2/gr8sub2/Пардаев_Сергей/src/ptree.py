import os


def get_process_info(pid):
    try:
        with open(f"/proc/{pid}/status") as f:
            content = f.read()
        info = {}
        for line in content.split("\n"):
            if line.startswith("Name:"):
                info["Name"] = line.split("\t")[1]
            elif line.startswith("PPid:"):
                info["PPid"] = int(line.split("\t")[1])
        return info
    except FileNotFoundError:
        return None


def main():
    current_pid = os.getpid()
    chain = []
    pid = current_pid
    while pid != 0 and pid != 1:
        info = get_process_info(pid)
        if not info:
            break
        chain.append((info.get("Name", "unknown"), pid))
        pid = info.get("PPid", 0)
    # get name of PID 1 dynamically
    info_1 = get_process_info(1)
    name_1 = info_1.get("Name", "unknown") if info_1 else "unknown"
    chain.append((name_1, 1))
    result = " ← ".join([f"{name}({pid})" for name, pid in chain])
    print(result)


if __name__ == "__main__":
    main()

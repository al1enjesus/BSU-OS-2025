import os

pid = os.getpid()        # получаем PID текущего процесса
chain = []               # сюда будем собирать "имя(PID)" от текущего до systemd(1)

while pid > 0:           # идём вверх по дереву процессов
    try:
        with open(f"/proc/{pid}/status") as f:
            data = f.read().splitlines()
        # берём имя процесса и PPid из файла
        name = next(x.split()[1] for x in data if x.startswith("Name:"))
        ppid = int(next(x.split()[1] for x in data if x.startswith("PPid:")))
    except FileNotFoundError:
        break  # если процесс уже исчез из /proc
    chain.append(f"{name}({pid})")
    if pid == 1:         # дошли до systemd/init
        break
    pid = ppid           # поднимаемся к родительскому процессу

# печатаем всю цепочку через стрелку
print(" \u2190 ".join(chain))

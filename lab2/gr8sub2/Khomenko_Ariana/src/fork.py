import os
import sys

# Родительский процесс
print(f"parent: PID={os.getpid()}", flush=True)

# Создаем первого ребенка
pid1 = os.fork()
if pid1 == 0:
    # Код первого ребенка
    print(f"child_A: PID={os.getpid()}, PPID={os.getppid()}", flush=True)
    sys.exit(0)

# Создаем второго ребенка
pid2 = os.fork()
if pid2 == 0:
    # Код второго ребенка
    print(f"child_B: PID={os.getpid()}, PPID={os.getppid()}", flush=True)
    sys.exit(0)

# Код только для родителя
# Ждем когда дети закончат работу и получаем их коды завершения
_, status1 = os.waitpid(pid1, 0)
_, status2 = os.waitpid(pid2, 0)

# Преобразуем статус в нормальный код завершения
exit_code1 = os.WEXITSTATUS(status1)
exit_code2 = os.WEXITSTATUS(status2)

print(f"parent: child_A (PID={pid1}) finished with status {exit_code1}")
print(f"parent: child_B (PID={pid2}) finished with status {exit_code2}")
print("parent: all children finished", flush=True)

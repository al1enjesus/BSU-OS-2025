# Лабораторная 2 — процессы и файловая система `/proc`

---

## Цель

Понять модель процессов Linux (PID/PPID), принципы порождения/ожидания процессов и научиться извлекать информацию из `/proc`; выполнить аналитику по CPU/памяти/IO.

---

## Задание 1 — Создание процессов

**Требование:** родитель порождает двух детей; дети печатают PID/PPID; родитель ждёт обоих и выводит коды завершения.

**Файл `src/fork_example.py`:**

```python
#!/usr/bin/env python3
import os

def main():
    print(f"parent: PID={os.getpid()}", flush=True)

    children = []
    for label in ("A", "B"):
        pid = os.fork()
        if pid == 0:  # child
            print(f"child_{label}: PID={os.getpid()}, PPID={os.getppid()}", flush=True)
            os._exit(0)
        else:         # parent
            children.append((label, pid))
            print(f"parent: forked child_{label} with PID={pid}", flush=True)

    for label, pid in children:
        wpid, status = os.waitpid(pid, 0)
        if os.WIFEXITED(status):
            print(f"parent: child_{label} (PID={wpid}) exited with code {os.WEXITSTATUS(status)}", flush=True)
        elif os.WIFSIGNALED(status):
            print(f"parent: child_{label} (PID={wpid}) killed by signal {os.WTERMSIG(status)}", flush=True)
        else:
            print(f"parent: child_{label} (PID={wpid}) ended with status=0x{status:x}", flush=True)

    print("parent: all children finished", flush=True)

if __name__ == "__main__":
    main()
```

**Запуск:**

```bash
python3 src/fork_example.py
```

**Фактический вывод:**

```
parent: PID=4455
parent: forked child_A with PID=4456
child_A: PID=4456, PPID=4455
child_B: PID=4457, PPID=4455
parent: forked child_B with PID=4457
parent: child_A (PID=4456) exited with code 0
parent: child_B (PID=4457) exited with code 0
parent: all children finished
```

Комментарий: родитель 4455 корректно породил и дождался двоих детей (коды выхода 0).

---

## Задание 2 — Исследование дерева процессов

**Команды:**

```bash
ps -ef --forest | head -n 30 | cat
pstree -p | head -n 50 | cat
```

**Фрагменты результатов:**

* `ps --forest` (ядровые потоки):

```
UID  PID PPID  C STIME TTY   TIME CMD
root   2    0  0 14:01 ?     00:00:00 [kthreadd]
root   3    2  0 14:01 ?     00:00:00  \_ [pool_workqueue_release]
root   4    2  0 14:01 ?     00:00:00  \_ [kworker/R-rcu_gp]
...
```

* `pstree -p` (ветка от `systemd(1)`):

```
systemd(1)-+-ModemManager(1175)-+-{ModemManager}(1187)
           |                    |-{ModemManager}(1191)
           |                    `-{ModemManager}(1193)
           |-NetworkManager(1163)-+-{NetworkManager}(1180)
           |                      |-{NetworkManager}(1181)
           |                      `-{NetworkManager}(1183)
           |-VBoxDRMClient(1387)-+-{VBoxDRMClient}(1391)
           |                     |-{VBoxDRMClient}(1392)
           |                     `-{VBoxDRMClient}(2550)
           |-VBoxService(1397)-+-{VBoxService}(1398)
           |                   `-{VBoxService}(1405)
           |-accounts-daemon(1054)-+-{accounts-daemon}(1113)
           |-avahi-daemon(1026)---avahi-daemon(1101)
           |-gdm3(1539)-+-gdm-session-wor(1552)-+-gdm-wayland-ses(1631)-+-gnome-session-b(1652)
           ...
```

Вывод: `systemd(1)` — корень; видны службы окружения GNOME и процессы VirtualBox, т.к. система работает в VM.

---

## Задание 3 — Изучение `/proc` (PID текущей оболочки)

**Команды:**

```bash
PID=$$
echo "PID=$PID"

# cmdline
cat /proc/$PID/cmdline | tr '�' ' '; echo

# status (первые 20 строк)
head -n 20 /proc/$PID/status

# fd (первые 30 ссылок)
ls -l /proc/$PID/fd | head -n 30
```

**Фактические наблюдения:**

```
PID=4356
### cmdline
bash
### status (первые 20 строк)
Name:   bash
Umask:  0002
State:  S (sleeping)
Tgid:   4356
Pid:    4356
PPid:   4349
Uid:    1000  1000  1000  1000
Gid:    1000  1000  1000  1000
FDSize: 256
VmPeak: 9664 kB
VmSize: 9664 kB
...
### fd (первые 30 ссылок)
0 -> /dev/pts/0
1 -> /dev/pts/0
2 -> /dev/pts/0
255 -> /dev/pts/0
```

Пояснения: `cmdline` — командная строка; `status` — имя/состояние/UID/GID/память; `fd` — открытые дескрипторы (stdin/stdout/stderr на `/dev/pts/0`).

---

## Задание 4 — Аналитика процессов (CPU/память/IO)

**Команды:**

```bash
(top -b -n 1 || true) | head -n 20 | tee top_head.txt
ps -eo pid,ppid,comm,state,%cpu,%mem,etime --sort=-%cpu | head -n 15 | tee cpu_top.txt
ps -eo pid,ppid,comm,state,%cpu,%mem,rss --sort=-%mem | head -n 15 \
 | awk 'NR==1{print;next}{$7=sprintf("%.1f",$7/1024); print}' \
 | sed '1!s/$/ MiB/' | tee mem_top.txt
pidstat -u -r -d 1 5 | tee pidstat_1s_5i.txt
```

**top (голова):**

```
up 27 min, 2 users, load average: 0.01, 0.04, 0.08
%Cpu(s): 0.0 us, 0.0 sy, 0.0 ni, 100.0 id, 0.0 wa, 0.0 hi, 0.0 si, 0.0 st
MiB Mem : 3403.4 total, 283.4 free, 1408.6 used, 1985.4 buff/cache
```

**TOP‑5 по CPU (`ps`):**

```
PID   PPID  COMMAND         S  %CPU  %MEM  ELAPSED
1834  1572  gnome-shell     S  3.0   17.3  28:27
1048  1     snapd           S  0.5    1.2  28:32
4535  1     fwupd           S  0.3    1.2  05:59
4594  2     kworker/u8:1-ev I  0.3    0.0  00:02
4349  1572  gnome-terminal- R  0.3    1.8  12:12
```

**TOP‑5 по памяти (`ps`):**

```
PID   PPID  COMMAND            %CPU  %MEM   RSS
1834  1572  gnome-shell         2.9  17.3   589.0 MiB
2517  1834  mutter-x11-fram     0.0   3.1   106.8 MiB
2459  1572  gsd-xsettings       0.0   2.3    79.8 MiB
2583  1834  gjs                  0.0   2.2    75.7 MiB
2060  1807  evolution-alarm      0.0   1.9    64.7 MiB
```

**`pidstat 1 5` (средние):**

```
Среднее CPU: gnome-shell ~3.37%; gnome-terminal ~0.40%; pidstat ~0.99%; kworkers ~0.2%.
Среднее MEM: gnome-shell RSS ~603232 KB (≈589–603 MiB).
IO: значимой активности не наблюдалось (rKB/s, wKB/s ≈ 0).
```

**Вывод по аналитике:** основная нагрузка — графическое окружение GNOME (`gnome-shell`, компоненты mutter/gsd/gjs). Это ожидаемо для рабочей станции/VM (VirtualBox), часть накладных расходов уходит на драйверы VBox.

---

## Ответы на контрольные вопросы

**1) Процесс vs программа**
Программа — файл на диске (код). Процесс — запущенная программа: у неё есть PID, собственное адресное пространство и состояние. Одну программу можно запустить много раз — это будут разные процессы.

**2) `fork()` без `wait()`**
Ребёнок завершится, а если родитель не вызовет `wait*()`, он останется «зомби» (STAT=`Z`) до сбора статуса. Зомби почти не потребляют память, но занимают PID/запись. Если родитель умрёт, их усыновит `PID 1` и соберёт.

**3) Где хранится информация о процессах**
В ядре — в структурах (например, `task_struct`): планирование, память, дескрипторы. Пользователю отдаётся через `/proc`: `/proc/<pid>/{status,stat,cmdline,fd,...}` и системные сводки (`/proc/stat`, `/proc/meminfo`).

**4) Что делает `exec()` и зачем он нужен**
Полностью заменяет образ текущего процесса другим (PID сохраняется). Классическое применение: `fork()` в родителе → в дочернем `exec*()` для запуска новой программы.

**5) Почему в `/proc` нет «настоящих» файлов**
`/proc` — виртуальная файловая система ядра. Файлы генерируются ядром в реальном времеени, и не хранятся на диске.

**6) Поля `top`: `%CPU, %MEM, VIRT, RES, SHR, TIME+`**

* **%CPU** — доля одного логического CPU (100% ≈ 1 ядро).
* **%MEM** — доля RAM, занятой процессом.
* **VIRT** — объём виртуальной памяти.
* **RES** — резидентная (реально используемая память).
* **SHR** — часть RES, разделяемая с другими процессами.
* **TIME+** — накопленное CPU‑время (user+system) по всем потокам с запуска.

**7) Почему сумма `%CPU` может быть >100%**
Потому что измерение на ядро: на N ядрах теоретический максимум ≈ N×100%. Многопоточный процесс может показывать >100%.

**8) %CPU vs load average; что означает строка `Cpu(s)` (включая `wa`)**

* **%CPU** — моментальная загрузка конкретного процесса.
* **Load average** — среднее количество задач в очереди исполнения + в состоянии непрерываемого ожидания I/O (`D`) за 1/5/15 минут.
* `Cpu(s)` в `top`: `us` — пользовательское, `sy` — системное, `ni` — nice, `id` — idle, `wa` — I/O wait (CPU простаивает, пока задача ждёт диск/сеть), `hi/si` — аппаратные/софт‑прерывания, `st` — time stolen (в VM).

**9) IO‑нагрузка vs CPU‑нагрузка и как её увидеть**
CPU — процесс активно выполняется; IO — процесс чаще ждёт устройство (видно рост `wa`, STAT=`D`). Смотреть: `pidstat -d 1` (rKB/s, wKB/s), `sudo iotop`, и `/proc/<pid>/io` (счётчики `read_bytes`, `write_bytes`, `syscr`, `syscw`).

**10) `nice`/приоритеты и влияние**
Для обычных задач (CFS) `nice` в диапазоне −20…19: выше nice ⇒ ниже приоритет ⇒ меньше доля CPU. Изменение: `nice`, `renice`. Для realtime (`SCHED_FIFO/RR`, приоритет 1…99) — `chrt` (требуются права).

**11) Поток vs процесс; как увидеть потоки**
Потоки (LWPs) делят адресное пространство и ресурсы процесса; у каждого свой TID. Смотрим: `ps -eLf`, `ps -T -p <PID>`, в `top` — клавиша `H`, в `/proc/<pid>/task/` — по каталогу на поток.

**12) Зомби и сироты**
Зомби — дети, завершившиеся, но не собранные `wait*()` (STAT=`Z`). Сироты — дети, у которых умер родитель; их усыновляет `PID 1`. Зомби исчезают после `wait()`, сироты продолжают работать и будут корректно «собраны» при завершении PID 1.

---

## Выводы

1. Реализовано порождение двух процессов и корректное ожидание (`waitpid`) — требования выполнены.
2. Дерево подтверждает иерархию: корень `systemd(1)`, далее службы и окружение GNOME; VM‑окружение видно по процессам VirtualBox.
3. В `/proc/<pid>` изучены `cmdline`, `status`, `fd`; понятна связь дескрипторов 0/1/2 с терминалом `/dev/pts/0`.
4. По метрикам: TOP‑1 по CPU и памяти — `gnome-shell`; IO‑активность низкая в наблюдённом интервале.

---

## Примечания

* Скриншоты: `01_fork_example_output.png`, `02_ps_forest.png`, `03_pstree_tree.png`, `04_proc_shell_cmdline_status_fd.png`, `05_top_snapshot_head.png`, `06_ps_cpu_top.png`, `07_ps_mem_top.png`, `08_pidstat_cpu_mem_io.png`.

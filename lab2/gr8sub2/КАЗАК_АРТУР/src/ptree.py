#!/usr/bin/env python3
"""
ptree.py — печатает цепочку родителей от текущего процесса до PID 1.
Читает PPid из /proc/<pid>/status и имя из /proc/<pid>/comm.
"""

from __future__ import annotations
import os
from typing import List, Optional, Tuple


def read_ppid(pid: int) -> Optional[int]:
    """
    Возвращает PPid для данного pid из /proc/<pid>/status.
    В случае ошибки — None.
    """
    status_path = f"/proc/{pid}/status"
    try:
        with open(status_path, "r", encoding="utf-8", errors="replace") as f:
            for line in f:
                if line.startswith("PPid:"):
                    # line, например: "PPid:\t72110\n"
                    _, raw_value = line.split(":", 1)
                    raw_value = raw_value.strip()
                    try:
                        return int(raw_value)
                    except ValueError:
                        return None
    except OSError:
        return None
    return None


def read_comm(pid: int) -> str:
    """
    Возвращает имя команды (comm) для pid из /proc/<pid>/comm.
    В случае ошибки возвращает "pid_<pid>".
    """
    comm_path = f"/proc/{pid}/comm"
    try:
        with open(comm_path, "r", encoding="utf-8", errors="replace") as f:
            name = f.readline().strip()
            return name if name else f"pid_{pid}"
    except OSError:
        return f"pid_{pid}"


def build_parent_chain(start_pid: int, limit: int = 256) -> List[Tuple[str, int]]:
    """
    Формирует список (comm, pid), поднимаясь от start_pid к PPid до PID 1
    или пока не достигнем limit шагов/ошибки чтения.
    """
    chain: List[Tuple[str, int]] = []
    current: Optional[int] = start_pid

    steps = 0
    while current and steps < limit:
        comm = read_comm(current)
        chain.append((comm, current))

        if current == 1:
            break

        ppid = read_ppid(current)
        if not ppid:
            break

        current = ppid
        steps += 1

    return chain


def main() -> None:
    pid = os.getpid()
    chain = build_parent_chain(pid)
    pretty = " \u2190 ".join(f"{name}({p})" for name, p in chain)
    print(pretty)


if __name__ == "__main__":
    main()

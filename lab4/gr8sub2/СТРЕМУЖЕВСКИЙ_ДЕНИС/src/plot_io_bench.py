import argparse
import csv
import os
from pathlib import Path

import matplotlib.pyplot as plt


def parse_float(s: str) -> float:
    return float(s.replace(",", ".").strip())


def load_buffers_csv(path: Path):
    buffers, mbps = [], []
    with path.open(newline="") as f:
        r = csv.reader(f)
        for row in r:
            if not row:
                continue
            if row[0].strip().lower() in ("buffer", "buffer_bytes"):
                continue
            try:
                b = int(row[0].strip())
                v = parse_float(row[1])
            except Exception:
                continue
            buffers.append(b)
            mbps.append(v)
    z = sorted(zip(buffers, mbps), key=lambda t: t[0])
    return [t[0] for t in z], [t[1] for t in z]


def load_methods_csv(path: Path):
    labels, values = [], []
    with path.open(newline="") as f:
        r = csv.reader(f)
        for row in r:
            if not row:
                continue
            if row[0].strip().lower() in ("method", "name"):
                continue
            try:
                labels.append(row[0].strip())
                values.append(parse_float(row[1]))
            except Exception:
                continue
    return labels, values


def plot_buffers(buffers, mbps, out_png: Path):
    plt.figure(figsize=(10, 6))
    plt.plot(buffers, mbps, marker="o")
    plt.xscale("log", base=2)
    plt.xlabel("Buffer size (bytes)")
    plt.ylabel("Throughput (MB/s)")
    plt.grid(True)
    plt.tight_layout()
    plt.savefig(out_png, dpi=140)
    plt.close()


def plot_methods(labels, values, out_png: Path):
    plt.figure(figsize=(8, 5))
    plt.bar(labels, values)
    plt.ylabel("Throughput (MB/s)")
    plt.grid(axis="y")
    plt.tight_layout()
    plt.savefig(out_png, dpi=140)
    plt.close()


def main():
    ap = argparse.ArgumentParser(description="Plot IO benchmark CSVs.")
    ap.add_argument("--buffers", default="buffers_vs_throughput.csv",
                    help="Путь к CSV с колонками: buffer_bytes,MBps")
    ap.add_argument("--methods", default="methods_vs_throughput.csv",
                    help="Путь к CSV с колонками: method,MBps")
    ap.add_argument("--outdir", default=".",
                    help="Куда сохранять PNG (по умолчанию текущая папка)")
    args = ap.parse_args()

    outdir = Path(args.outdir)
    outdir.mkdir(parents=True, exist_ok=True)

    buf_csv = Path(args.buffers)
    if buf_csv.exists():
        buffers, mbps = load_buffers_csv(buf_csv)
        if buffers and mbps:
            plot_buffers(buffers, mbps, outdir / "buffers_vs_throughput.png")
            print(f"[OK] Saved: {outdir / 'buffers_vs_throughput.png'}")
        else:
            print("[WARN] buffers CSV пустой или в неверном формате")
    else:
        print(f"[WARN] Нет файла: {buf_csv}")

    meth_csv = Path(args.methods)
    if meth_csv.exists():
        labels, values = load_methods_csv(meth_csv)
        if labels and values:
            plot_methods(labels, values, outdir / "methods_vs_throughput.png")
            print(f"[OK] Saved: {outdir / 'methods_vs_throughput.png'}")
        else:
            print("[WARN] methods CSV пустой или в неверном формате")
    else:
        print(f"[WARN] Нет файла: {meth_csv}")


if __name__ == "__main__":
    main()

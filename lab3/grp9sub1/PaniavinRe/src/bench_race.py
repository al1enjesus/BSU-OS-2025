#!/usr/bin/env python3
import subprocess
import sys

def run_benchmark():
    M = 2000000  # iterations per thread
    R = 5        # runs per configuration
    Ns = [1, 2, 4, 8]  # number of threads
    modes = ["mutex", "atomic"]
    
    results = []
    
    for mode in modes:
        for n in Ns:
            times = []
            for run in range(R):
                cmd = ["python3", "thread_race.py", str(n), str(M), mode]
                result = subprocess.run(cmd, capture_output=True, text=True)
                if result.returncode == 0:
                    output = result.stdout.strip()
                    print(output)
                    # Парсим время из вывода
                    for part in output.split():
                        if part.startswith("time_ms="):
                            time_ms = int(part.split("=")[1])
                            times.append(time_ms)
                            break
                else:
                    print(f"Error: {result.stderr}")
            
            if times:
                avg_time = sum(times) / len(times)
                results.append({
                    'mode': mode,
                    'N': n,
                    'M': M,
                    'avg_time_ms': avg_time
                })
    
    # Выводим summary
    print("\nSummary (avg time_ms over {R} runs)")
    print(f"{'mode':<8} {'N':<6} {'M(per-thread)':<12} {'avg_time_ms':<12}")
    for res in results:
        print(f"{res['mode']:<8} {res['N']:<6} {res['M']:<12} {res['avg_time_ms']:<12.2f}")

if __name__ == "__main__":
    run_benchmark()

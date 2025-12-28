import threading
import time
import os

def worker(worker_id):
    print(f"Worker {worker_id} started")
    time.sleep(2)
    print(f"Worker {worker_id} finished")

def main():
    print("=== Threads Information ===")
    print(f"Process PID: {os.getpid()}")
    
    threads = []
    for i in range(3):
        t = threading.Thread(target=worker, args=(i,))
        threads.append(t)
        t.start()
    
    print(f"Active threads: {threading.active_count()}")
    print("\nCurrent threads:")
    for thread in threading.enumerate():
        print(f"  - {thread.name} (ID: {thread.ident})")
    
    for t in threads:
        t.join()
    
    print("\nRun these commands in another terminal:")
    print(f"ps -L -p {os.getpid()} -o pid,tid,psr,pcpu,stat,comm")
    print(f"cat /proc/{os.getpid()}/status | grep Threads")

if __name__ == "__main__":
    main()
import multiprocessing
import os
import time

def child_processing(index):
    pid = os.getpid()
    ppid = os.getppid()
    print(f"Child_{index}: PID = {pid}, PPID = {ppid}", flush = True)
    time.sleep(10)

if __name__ == "__main__":
    print("Родитель начинает работать. PID родителя =", os.getpid(), flush = True)
    processes = []
    for i in range(1, 3):
        p = multiprocessing.Process(target=child_processing, args=(i,))
        processes.append(p)
        p.start()

    for p in processes:
        p.join() # = wait()
        print(f"Дочерний процесс {p.pid} завершился с кодом: {p.exitcode}", flush = True)
    
    print(f"Ожидание родителя своих дочерних процессов завершилось.")

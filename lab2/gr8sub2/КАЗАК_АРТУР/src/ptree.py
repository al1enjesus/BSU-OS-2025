import os
def read_ppid(pid):
    try:
        with open(f"/proc/{pid}/status") as f:
            for l in f:
                if l.startswith("PPid:"): return int(l.split(":",1)[1])
    except Exception: return None
def read_comm(pid):
    try:
        with open(f"/proc/{pid}/comm") as f: return f.readline().strip()
    except Exception: return f"pid_{pid}"
def main():
    chain=[]; cur=os.getpid()
    while cur and len(chain)<256:
        chain.append((read_comm(cur), cur))
        if cur==1: break
        p=read_ppid(cur); 
        if not p: break
        cur=p
    print(" \u2190 ".join([f"{n}({p})" for n,p in chain]))
if __name__=="__main__": main()
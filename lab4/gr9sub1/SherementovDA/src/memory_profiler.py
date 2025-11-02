#!/usr/bin/env python3
import os
import sys
import time
import argparse
import re
from collections import defaultdict

class MemoryProfiler:
    def __init__(self, pid):
        self.pid = pid
        self.prev_metrics = {}
        
    def validate_pid(self):
        try:
            with open(f'/proc/{self.pid}/status', 'r') as f:
                return True
        except:
            return False
    
    def get_process_name(self):
        try:
            with open(f'/proc/{self.pid}/comm', 'r') as f:
                return f.read().strip()
        except:
            return "Unknown"
    
    def get_memory_metrics(self):
        metrics = {}
        
        try:
            with open(f'/proc/{self.pid}/status', 'r') as f:
                for line in f:
                    if line.startswith('VmSize:'):
                        metrics['vsz'] = int(line.split()[1])
                    elif line.startswith('VmRSS:'):
                        metrics['rss'] = int(line.split()[1])
        except:
            return None
        
        try:
            with open(f'/proc/{self.pid}/stat', 'r') as f:
                stat_data = f.read().split()
                metrics['minflt'] = int(stat_data[9])
                metrics['majflt'] = int(stat_data[11])
        except:
            metrics['minflt'] = 0
            metrics['majflt'] = 0
        
        return metrics
    
    def parse_memory_maps(self):
        segments = defaultdict(lambda: {'size_kb': 0, 'count': 0})
        
        try:
            with open(f'/proc/{self.pid}/maps', 'r') as f:
                for line in f:
                    parts = line.split()
                    if len(parts) < 2:
                        continue
                    
                    addr_range = parts[0]
                    start, end = addr_range.split('-')
                    size_kb = (int(end, 16) - int(start, 16)) // 1024
                    
                    if len(parts) >= 6:
                        pathname = parts[5]
                    else:
                        if '[heap]' in line:
                            pathname = '[heap]'
                        elif '[stack]' in line:
                            pathname = '[stack]'
                        else:
                            pathname = '[anonymous]'
                    
                    segment_type = self.classify_segment(pathname)
                    segments[segment_type]['size_kb'] += size_kb
                    segments[segment_type]['count'] += 1
                    
        except Exception as e:
            pass
        
        return segments
    
    def classify_segment(self, pathname):
        if pathname == '[heap]':
            return 'heap'
        elif pathname == '[stack]':
            return 'stack'
        elif pathname.startswith('/'):
            if 'lib' in pathname and '.so' in pathname:
                return 'libraries'
            else:
                return 'files'
        else:
            return 'anonymous'
    
    def get_pss_uss(self):
        pss_total = 0
        private_total = 0
        
        try:
            with open(f'/proc/{self.pid}/smaps_rollup', 'r') as f:
                for line in f:
                    if 'Pss:' in line:
                        pss_total = int(line.split()[1])
                    elif 'Private_Clean:' in line:
                        private_total += int(line.split()[1])
                    elif 'Private_Dirty:' in line:
                        private_total += int(line.split()[1])
        except:
            pass
        
        return pss_total, private_total
    
    def get_shared_libraries(self):
        libraries = defaultdict(int)
        
        try:
            with open(f'/proc/{self.pid}/maps', 'r') as f:
                for line in f:
                    if '.so' in line and '/lib' in line:
                        parts = line.split()
                        if len(parts) >= 6:
                            lib_path = parts[5]
                            addr_range = parts[0]
                            start, end = addr_range.split('-')
                            size_kb = (int(end, 16) - int(start, 16)) // 1024
                            libraries[lib_path] += size_kb
        except:
            pass
        
        return libraries
    
    def print_basic_info(self, metrics, segments, pss, uss):
        proc_name = self.get_process_name()
        
        print(f"Process: {proc_name} (PID {self.pid})")
        print("=" * 50)
        
        print(f"VSZ: {metrics.get('vsz', 0) / 1024:.1f} MB")
        print(f"RSS: {metrics.get('rss', 0) / 1024:.1f} MB")
        print(f"PSS: {pss / 1024:.1f} MB")
        print(f"USS: {uss / 1024:.1f} MB")
        print()
        
        print("Memory Segments:")
        for seg_type, data in segments.items():
            print(f"  {seg_type:15} {data['size_kb'] / 1024:6.1f} MB")
        
        print()
        print("Page Faults:")
        print(f"  Minor: {metrics.get('minflt', 0)}")
        print(f"  Major: {metrics.get('majflt', 0)}")
    
    def print_shared_libraries(self, libraries):
        if libraries:
            print("\nShared Libraries:")
            for lib, size_kb in sorted(libraries.items(), key=lambda x: x[1], reverse=True)[:5]:
                print(f"  {size_kb / 1024:6.1f} MB - {os.path.basename(lib)}")
    
    def watch_mode(self, interval=1):
        if not self.validate_pid():
            print(f"Error: Process with PID {self.pid} not found")
            return
        
        print(f"Monitoring PID {self.pid} every {interval} second(s)...")
        print("Press Ctrl+C to stop\n")
        
        try:
            while True:
                os.system('clear')
                if not self.single_run():
                    print(f"Process {self.pid} no longer exists")
                    break
                time.sleep(interval)
        except KeyboardInterrupt:
            print("\nMonitoring stopped")
    
    def single_run(self):
        if not self.validate_pid():
            return False
            
        metrics = self.get_memory_metrics()
        if metrics is None:
            return False
            
        segments = self.parse_memory_maps()
        pss, uss = self.get_pss_uss()
        libraries = self.get_shared_libraries()
        
        self.print_basic_info(metrics, segments, pss, uss)
        self.print_shared_libraries(libraries)
        
        self.prev_metrics = metrics
        return True
    
    def compare_processes(self, pid2):
        profiler2 = MemoryProfiler(pid2)
        
        if not self.validate_pid():
            print(f"❌ Process with PID {self.pid} not found")
            return
            
        if not profiler2.validate_pid():
            print(f"❌ Process with PID {pid2} not found")
            return
        
        metrics1 = self.get_memory_metrics()
        metrics2 = profiler2.get_memory_metrics()
        
        print("PROCESS COMPARISON")
        print("=" * 60)
        print(f"{'Metric':<15} {f'PID {self.pid}':<15} {f'PID {pid2}':<15} {'Difference':<15}")
        print("-" * 60)
        
        common_metrics = ['vsz', 'rss', 'minflt', 'majflt']
        for metric in common_metrics:
            val1 = metrics1.get(metric, 0)
            val2 = metrics2.get(metric, 0)
            diff = val1 - val2
            print(f"{metric:<15} {val1:<15} {val2:<15} {diff:<15}")

def list_processes():
    print("Running processes (with PID and VSZ):")
    print("PID     VSZ(MB)   Command")
    print("-" * 40)
    
    processes = []
    
    try:
        for pid in os.listdir('/proc'):
            if pid.isdigit():
                try:
                    with open(f'/proc/{pid}/comm', 'r') as f:
                        comm = f.read().strip()
                    
                    with open(f'/proc/{pid}/status', 'r') as f:
                        vsz = 0
                        for line in f:
                            if line.startswith('VmSize:'):
                                vsz = int(line.split()[1])
                                break
                    
                    processes.append((pid, vsz, comm))
                except:
                    pass
    except:
        pass
    
    for pid, vsz, comm in sorted(processes, key=lambda x: x[1], reverse=True)[:20]:
        print(f"{pid:6} {vsz/1024:8.1f}   {comm}")

def create_test_process():
    print("Creating test processes for comparison...")
    
    import subprocess
    import tempfile
    
    test_script1 = '''
import os
import time
import array

print("Test process 1 started - PID:", os.getpid())
data1 = array.array('i', [0] * 1000000)  # ~4MB
time.sleep(300)
'''
    
    test_script2 = '''
import os
import time
import array

print("Test process 2 started - PID:", os.getpid())  
data1 = array.array('i', [0] * 500000)  # ~2MB
data2 = array.array('d', [0.0] * 500000)  # ~4MB
time.sleep(300)
'''
    
    try:
        with tempfile.NamedTemporaryFile(mode='w', suffix='.py', delete=False) as f:
            f.write(test_script1)
            script1 = f.name
        
        with tempfile.NamedTemporaryFile(mode='w', suffix='.py', delete=False) as f:
            f.write(test_script2)  
            script2 = f.name
        
        proc1 = subprocess.Popen([sys.executable, script1])
        proc2 = subprocess.Popen([sys.executable, script2])
        
        print(f"Created test processes: PID {proc1.pid} and PID {proc2.pid}")
        print("They will run for 5 minutes. You can compare them now.")
        print(f"Command: python3 {sys.argv[0]} {proc1.pid} --compare {proc2.pid}")
        
        return proc1.pid, proc2.pid
        
    except Exception as e:
        print(f"Error creating test processes: {e}")
        return None, None

def main():
    parser = argparse.ArgumentParser(description='Memory Profiler')
    parser.add_argument('pid', type=int, nargs='?', help='Process ID to profile')
    parser.add_argument('--watch', action='store_true', help='Monitor mode')
    parser.add_argument('--compare', type=int, metavar='PID2', help='Compare with another PID')
    parser.add_argument('--list', action='store_true', help='List running processes')
    parser.add_argument('--test', action='store_true', help='Create test processes for comparison')
    
    args = parser.parse_args()
    
    if args.test:
        pid1, pid2 = create_test_process()
        if pid1 and pid2:
            print(f"\nNow run: python3 {sys.argv[0]} {pid1} --compare {pid2}")
        return
    
    if args.list:
        list_processes()
        return
    
    if not args.pid and not args.compare:
        print("Error: PID required")
        print("Use --list to see available processes")
        print("Use --test to create test processes for comparison")
        sys.exit(1)
    
    if args.compare:
        if not args.pid:
            print("Error: Need first PID for comparison")
            sys.exit(1)
        profiler = MemoryProfiler(args.pid)
        profiler.compare_processes(args.compare)
    elif args.watch:
        if not args.pid:
            print("Error: PID required for watch mode")
            sys.exit(1)
        profiler = MemoryProfiler(args.pid)
        profiler.watch_mode()
    else:
        profiler = MemoryProfiler(args.pid)
        if not profiler.validate_pid():
            print(f"Error: Process with PID {args.pid} not found")
            print("Use --list to see available processes")
            print("Use --test to create test processes")
        else:
            profiler.single_run()

if __name__ == "__main__":
    main()

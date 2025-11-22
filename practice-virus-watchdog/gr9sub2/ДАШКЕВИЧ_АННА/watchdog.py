import subprocess
import time

while True:
    try:
        out = subprocess.check_output(['pgrep', 'firefox']).decode('utf-8').strip()
        if not out:

            time.sleep(0.5)
            continue

        pids = [line.strip() for line in out.splitlines() if line.strip().isdigit()]
        if not pids:
            time.sleep(0.5)
            continue

        for pid in pids:
            subprocess.run(['kill', '-KILL', pid])
            subprocess.run(['zenity', '--error', '--text', 'net net!!'])

    except subprocess.CalledProcessError:
        pass
    except Exception:
        pass
        
    time.sleep(0.5)
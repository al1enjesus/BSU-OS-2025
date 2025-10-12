import subprocess
import time
import signal
import sys

def signal_handler(signum, frame):
    sys.exit(0)

signal.signal(signal.SIGINT, signal_handler)
signal.signal(signal.SIGTERM, signal_handler)

while True:
    try:
        result = subprocess.run(['pgrep', 'Telegram'], capture_output=True, text=True)
        if result.returncode == 0:
            pids = result.stdout.strip().split('\n')
            for pid in pids:
                if pid:
                    try:
                        subprocess.run(['kill', '-TERM', pid], timeout=2)
                        time.sleep(1)
                        subprocess.run(['kill', '-0', pid], check=True)
                        subprocess.run(['kill', '-KILL', pid])
                    except:
                        pass
            subprocess.run(['zenity', '--error', '--text', 'nonono'], timeout=5)
            subprocess.run(['xdg-screensaver', 'activate'], timeout=5)
    except:
        pass
    time.sleep(0.5)

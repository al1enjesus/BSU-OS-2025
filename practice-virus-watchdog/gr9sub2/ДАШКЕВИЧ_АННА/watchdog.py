import subprocess
import time 

while True:
    try:
        pid = subprocess.check_output(['pgrep', 'firefox']).decode('utf-8').replace('\n', '')
        subprocess.run(['kill', '-KILL', pid])
        subprocess.run(['zenity', '--error', '--text', 'net net!!'])
    except:
        pass
    time.sleep(0.5) 
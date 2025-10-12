import subprocess
import time

while True:
    try:
        pid = subprocess.check_output(['pgrep', 'Telegram']).decode('utf-8').replace('\n', '')
        subprocess.run(['kill', '-KILL', pid])
        subprocess.run(['zenity', '--error', '--text', 'nonono'])
        subprocess.run(['xdg-screensaver', 'activate'])
    except:
        time.sleep(0.5)

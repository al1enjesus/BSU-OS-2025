## Описание

watchdog-скрипт отслеживает запуск приложения Telegram. Отображается предупреждающее сообщение и активируется блокировка экрана.

Реализация доступна на двух языках:
- `watchdog.sh` — Bash
- `watchdog.py` — Python

## Требования

Работает на Linux (Ubuntu/Debian). Убедитесь, что установлены следующие утилиты:

- `pgrep` — для поиска процесса
- `zenity` — для GUI-уведомлений
- `xdg-screensaver` — для блокировки экрана

## Инструкция по запуску

### Bash-версия

1. Скопируйте код ниже в файл `watchdog.sh`:

```
trap "exit 0" SIGINT SIGTERM

while true; do
    pid=$(pgrep Telegram)

    if [ -n "$pid" ]; then
        kill -KILL "$pid"
        zenity --error --text="nonono"
        xdg-screensaver activate
    fi

    sleep 0.5
done
```

2. Сделайте файл исполняемым:
```
chmod +x watchdog.sh
```

3. Запустите в фоне:
```
nohup ./watchdog.sh &
```


### Python-версия

1. Скопируйте код ниже в файл `watchdog.py`:

```
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
```

2. Запустите в фоне:
```
nohup python3 watchdog.py &
```
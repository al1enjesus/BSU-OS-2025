trap "exit 0" SIGINT SIGTERM

while true; do
    pids=$(pgrep Telegram)
    
    if [ -n "$pids" ]; then
        for pid in $pids; do
            kill -TERM "$pid" 2>/dev/null
            sleep 1
            if kill -0 "$pid" 2>/dev/null; then
                kill -KILL "$pid" 2>/dev/null
            fi
        done
        timeout 5 zenity --error --text="nonono" 2>/dev/null
        timeout 5 xdg-screensaver activate 2>/dev/null
    fi

    sleep 0.5
done


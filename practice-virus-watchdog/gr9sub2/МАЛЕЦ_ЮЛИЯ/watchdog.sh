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


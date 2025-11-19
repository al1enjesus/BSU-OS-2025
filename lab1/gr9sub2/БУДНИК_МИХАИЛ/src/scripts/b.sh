if [ ! -f "/var/log/auth.log" ]; then
        echo "файл /var/log/auth.log не найден!"
        exit 1
fi

echo "unsuccessful login attempts:"
# читаем весь файл
cat /var/log/auth.log |
	# ищем строку, которая сообщает об ошибке, и извлекаем logname
	grep -aoP 'authentication failure; logname=(.*) uid' |
	# убираем ненужные куски строки
	sed "s/authentication failure; logname=//g" |
	sed "s/uid//g" |
	# лексикографически сортируем
	sort |
	# подсчитываем кол-во повторений
	uniq -c |
	# если было введено неизвестное имя пользователя,
	# то logname будет пусто, заменяем на более понятное сообщение
	sed -E "s/([0-9])  +/\1 invalid username/g" |
	# сортируем по кол-ву попыток
	sort -nr
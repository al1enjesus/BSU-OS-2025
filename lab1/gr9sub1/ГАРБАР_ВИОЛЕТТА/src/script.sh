# 1. Анализ типов аутентификации
echo -e "\n1. ТИПЫ АУТЕНТИФИКАЦИИ И СЕРВИСЫ:"
sudo grep -a '' /var/log/auth.log 2>/dev/null 
| awk '{print $5}' 
| sort 
| uniq -c 
| sort -nr 
| head -n 10

# 2. Анализ пользователей (успешные и неуспешные попытки)
echo -e "\n2. АНАЛИЗ ПОЛЬЗОВАТЕЛЕЙ:"

# 2.1. Invalid user
sudo grep -a "Invalid user" /var/log/auth.log 
2>/dev/null 
| \ awk '{print $8}' 
| \ sort 
| uniq -c 
| sort -nr 
| head -n 5

# 2.2. Пользователи с неудачными попытками входа
sudo grep -a "Failed password" /var/log/auth.log 
2>/dev/null 
| \ awk '{for(i=1;i<=NF;i++) if($i=="for") print $(i+1)}' 
| \ sort 
| uniq -c 
| sort -nr 
| head -n 5

# 2.3. Успешные входы
sudo grep -a "session opened" /var/log/auth.log 
2>/dev/null 
| \ grep "user" 
| \ awk '{print $11}' 
| \ sort 
| uniq -c 
| sort -nr 
| head -n 5

# 3. Статистика успехов/провалов
success_count=$(sudo grep -ac "session opened" /var/log/auth.log 2>/dev/null)
failed_count=$(sudo grep -ac "Failed password" /var/log/auth.log 2>/dev/null)
invalid_user_count=$(sudo grep -ac "Invalid user" /var/log/auth.log 2>/dev/null)

echo "   Успешных входов: $success_count"
echo "   Неверный пароль: $failed_count"
echo "   Неверный user: $invalid_user_count"

# 4. Проверка SSH подключений
echo -e "\n4. ВНЕШНИЕ ПОДКЛЮЧЕНИЯ:"
ssh_ips=$(sudo grep -a "sshd.*from" /var/log/auth.log 
2>/dev/null 
| \ grep -E -o '[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+' 
| \ sort 
| uniq 
| wc -l)

if [ "$ssh_ips" -gt 0 ]; then
    echo "   Обнаружены подключения с $ssh_ips уникальных IP-адресов"
else
    echo "   Внешние подключения по SSH не обнаружены (только локальные входы)"
fi

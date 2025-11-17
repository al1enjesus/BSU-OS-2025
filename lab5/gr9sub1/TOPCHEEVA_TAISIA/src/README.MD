
# Лабораторная работа 5 - Модули ядра Linux
## Группа: 9, Подгруппа: 1
# Проверка версии ядра
uname -r

# Проверка наличия заголовков ядра
ls /lib/modules/$(uname -r)/build

# Установка необходимых пакетов (Ubuntu/Debian)
sudo apt update
sudo apt install -y build-essential linux-headers-$(uname -r) kmod

## Поддерживаемые версии ядра
- 6.14.0-34-generic (протестировано)
- 6.x.x-generic (должны работать)
- 5.x.x (возможны различия в API)

## Структура проекта
gr9sub1-lab5-TopcheevaTR/
├── src/                 # Исходные коды модулей
│   ├── hello_module.c   # Задание A - Hello World модуль
│   ├── proc_module.c    # Задание B - /proc файл с записью  
│   ├── stats_module.c   # Задание C - /proc файл со статистикой
│   └── Makefile         # Для сборки всех модулей
├── REPORT.md            # Полный отчёт с анализом и ответами на вопросы
├── README.md            # Этот файл
└── .gitignore          # Исключения для git
## Задания

### Задание A: Hello World модуль
- Простой модуль с приветствием
- Вывод сообщений при загрузке/выгрузке
- Поддержка параметра `message`

### Задание B: /proc файл с записью
- Создание `/proc/my_config` 
- Поддержка операций чтения и записи
- Сохранение данных между обращениями

###Задание C: /proc файл со статистикой
- Создание `/proc/sys_stats`
- Отображение системной информации:
  - Количество процессов
  - Используемая память
  - Uptime системы

## Быстрый старт

# Клонирование репозитория
git clone https://github.com/TopcheevaTR/gr9sub1-lab5-TopcheevaTR.git
cd gr9sub1-lab5-TopcheevaTR/src

# Компиляция всех модулей
make

# Загрузка модулей
sudo insmod hello_module.ko
sudo insmod proc_module.ko  
sudo insmod stats_module.ko

# Тестирование
cat /proc/my_config                    # Чтение конфига
echo "test" | sudo tee /proc/my_config # Запись в конфиг
cat /proc/sys_stats                    # Системная статистика

# Просмотр логов
sudo dmesg | tail -10

# Выгрузка модулей
sudo rmmod stats_module proc_module hello_module

## Требования к окружению
- Linux kernel 6.x
- Пакеты: build-essential, linux-headers-$(uname -r)
- Права root для загрузки модулей
- **ВАЖНО: Выполнять в виртуальной машине!**

## Файлы модулей

### hello_module.c
- Базовая структура модуля
- Параметр: `message` (строка)
- Логирование через printk()

### proc_module.c
- Работа с /proc файловой системой
- Функции чтения/записи
- Использование copy_to/from_user()

### stats_module.c  
- Получение системной информации
- Форматированный вывод
- Использование jiffies и si_meminfo()

## Безопасность
Код выполняется в kernel-space
- Обязательно использовать виртуальную машину
- Ошибки могут вызвать kernel panic
- Перед тестированием сделать snapshot VM

## Лицензия
GPL v2 (совместимо с лицензией ядра Linux)


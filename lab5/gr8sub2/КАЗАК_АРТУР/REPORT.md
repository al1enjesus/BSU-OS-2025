# Лабораторная работа 5 — Модули ядра Linux

Студент: **Казак Артур Эдуардович**  
Группа: **8**, подгруппа: **2**  

Директория работы:

```text
lab5/gr8sub2/КАЗАК_АРТУР/
  REPORT.MD
  README.md
  Makefile
  src/
    Makefile
    hello_module.c
    proc_module.c
    char_device.c
```

## Сборка модулей

Из каталога `lab5/gr8sub2/КАЗАК_АРТУР`:

```bash
make        # собирает все модули в каталоге src/
make clean  # очистка собранных файлов
```

В результате в `src/` появятся файлы:

- `hello_module.ko`
- `proc_module.ko`
- `char_device.ko`

## Задание A — hello_module

Файл: `src/hello_module.c`

Модуль:

- выводит сообщение при загрузке и выгрузке;
- поддерживает параметр `message` (тип `charp`).

Примеры команд:

```bash
sudo insmod src/hello_module.ko
dmesg | tail -n 20

sudo rmmod hello_module
dmesg | tail -n 20

sudo insmod src/hello_module.ko message="Custom message from user"
dmesg | tail -n 20
sudo rmmod hello_module
```

## Задание B — /proc/student_info

Файл: `src/proc_module.c`

Модуль:

- создаёт файл `/proc/student_info`;
- выводит:
  - имя и группу студента;
  - `jiffies` на момент загрузки модуля;
  - счётчик обращений к файлу (реализован с помощью `atomic_t`).

Пример проверки:

```bash
sudo insmod src/proc_module.ko
ls /proc | grep student_info

cat /proc/student_info
cat /proc/student_info
cat /proc/student_info

sudo rmmod proc_module
ls /proc | grep student_info  # файл должен исчезнуть
```

## Задание C — символьное устройство /dev/mychardev

Файл: `src/char_device.c`

Модуль:

- регистрирует символьное устройство `/dev/mychardev` с буфером 1024 байта;
- реализует операции `open`, `release`, `read`, `write`;
- защищает доступ к буферу и размеру данных с помощью `mutex`.

Пример проверки (учебная виртуальная машина):

```bash
sudo insmod src/char_device.ko
dmesg | tail -n 20          # посмотреть major-номер устройства

# допустим, ядро выдало major = 240
sudo mknod /dev/mychardev c 240 0

# права доступа: только root и группа могут читать и писать
sudo chmod 660 /dev/mychardev

echo "Hello from user space" > /dev/mychardev
cat /dev/mychardev

sudo rm /dev/mychardev
sudo rmmod char_device
```

## Логи и скриншоты

Рекомендуемые файлы:

- `screenshots/dmesg_hello.png` — вывод `dmesg` при работе `hello_module`;
- `screenshots/dmesg_proc.png` — вывод `dmesg` и `cat /proc/student_info`;
- `screenshots/dmesg_chardev.png` — лог работы `/dev/mychardev`;
- `screenshots/lsmod.png` — вывод `lsmod` с модулями;
- `logs/test_hello.txt` — сценарий проверки `hello_module`;
- `logs/test_proc.txt` — сценарий проверки `/proc/student_info`;
- `logs/test_chardev.txt` — сценарий проверки `/dev/mychardev`.

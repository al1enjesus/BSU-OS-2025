# Лабораторная работа №5 Вариант 1

## Версия ядра
`6.16.8+kali-amd64`

## Задание A: Hello World модуль

### Цель
- Создать простой модуль, который:
	- При загрузке (insmod) выводит "Hello from [ВАШ_ИМЯ] module!"
	- При выгрузке (rmmod) выводит "Goodbye from [ВАШ_ИМЯ] module!"
	- Принимает параметр message (строка)
	- Если параметр задан, выводит его вместо дефолтного сообщения 
### Шаги решения
1. Создать файл модуля `hello_module.c` и подключить необходимые заголовки.
2. Объявить строковый параметр `message` через `module_param`.
3. Реализовать функции `hello_init` (printk с приветствием) и `hello_exit` (printk с прощанием).
4. Зарегистрировать функции с помощью `module_init` и `module_exit`.
5. Указать метаданные: `MODULE_LICENSE, MODULE_AUTHOR, MODULE_DESCRIPTION`.
6. Создать `Makefile` для сборки модуля.
7. Собрать модуль командой `make`.
8. Загрузить модуль `insmod`, проверить вывод через `dmesg`.
9. Загрузить модуль с параметром `message="..."`, снова проверить вывод.
10. Выгрузить модуль `rmmod` и убедиться в корректных сообщениях через `dmesg`.
### Решение

Скомпелируем файл `hello_module.c` при помощи `Makefile`:

Вываод в консоли:
```
make -C /lib/modules/6.16.8+kali-amd64/build M=/home/kali/OS/BSU-OS-2025/lab5/gr8sub1/Стёпкин_Владимир/src modules
make[1]: Entering directory '/usr/src/linux-headers-6.16.8+kali-amd64'
make[2]: Entering directory '/home/kali/OS/BSU-OS-2025/lab5/gr8sub1/Стёпкин_Владимир/src'
  MODPOST Module.symvers
make[2]: Leaving directory '/home/kali/OS/BSU-OS-2025/lab5/gr8sub1/Стёпкин_Владимир/src'
make[1]: Leaving directory '/usr/src/linux-headers-6.16.8+kali-amd64'

```
После компиляции необходимо запустить модуль ядра при помощи команды bash-скрипта:
```bash
sudo insmod hello_module.ko
```
Проверить работу можно так же при помощи bash-скрипта:
```bash
dmesg | tail -2
```
Вывод в консоли:
```
[ 1536.404830] student_info module loaded
[ 1928.508555] Hello from Vladimir module!
```
Нам необходимо проверить работу модуля с приёмом переменной. Для этого надо выгрузить модуль при помощи команды:
```bash
sudo rmmod hello_module
```
Для проверки используем команду:
```bash
dmesg | tail -2
```
Вывод в консоли:
```
[ 1928.508555] Hello from Vladimir module!
[ 2235.958332] Goodbye from Vladimir module!
```
Отлично модуль выгружен тепкрь снова его загрузим, но при этом передадим ему строку, при помощи команды:
```bash
sudo insmod hello_module.ko message="Custom greeting"
```
Для проверки используем команду:
```bash
dmesg | tail -3 
```
Вывод в консоли:
```
[ 1928.508555] Hello from Vladimir module!
[ 2235.958332] Goodbye from Vladimir module!
[ 2399.995132] Custom greeting
```
### Использование AI
В ходе выполнения лабораторной работы №5 Задание А был использован ИИ для следующих задач:
- Помощь в написании модуля ядра `hello_module.c` с поддержкой параметра `message`.
- Объяснение принципа работы `jiffies` и механизма загрузки/выгрузки модулей ядра.
### Вывод
В ходе выполнения задания мы:
1. Создали и написали модуль ядра `hello_module.c`, который при загрузке выводит приветственное сообщение, а при выгрузке — прощальное.
2. Реализовали параметр `message` через `module_param`, позволяющий изменять текст приветствия при загрузке модуля.
3. Скомпилировали модуль с помощью `Makefile` и проверили его сборку командой `make`, убедившись в появлении файла `hello_module.ko`.
4. Провели работу модуля.
5. Сделали выводы:
	- Модуль корректно выводит стандартное сообщение при обычной загрузке.
	- Параметр `message` успешно передаётся модулю и выводится вместо стандартного сообщения.
	- Выгрузка модуля корректно освобождает ресурсы и выводит прощальное сообщение.
## Задание B: /proc файл с информацией

### Цель
Создать модуль, который создаёт файл `/proc/student_info` с информацией:
   - Ваше имя
   - Группа и подгруппа
   - Текущее время загрузки модуля (в секундах с boot, используйте jiffies)
   - Счётчик обращений к файлу
### Шаги решения
1. Создать модуль `student_info_module.c` и подключить нужные заголовки.
2. Объявить глобальные переменные для времени загрузки `(load_jiffies) и счётчика чтений (read_count)`.
3. Реализовать функцию чтения через `seq_file`, которая выводит имя, группу, загрузку модуля и увеличивает счётчик.
4. Создать `/proc/student_info` через `proc_create()` с `proc_ops`.
5. Удалять файл в функции выгрузки модуля через `remove_proc_entry()`.
6. Добавить метаданные модуля и `Makefile`, собрать модуль.
7. Проверить работу: `insmod, cat /proc/student_info`, увеличение счётчика, `rmmod`.
### Решение

Скомпилируем файл `student_info_module.c`, в котором мы написали модуль ядра, при помощи `Makefile`:
```bash
make MODULE=student_info_module
```
Вывод в консоли:
```
make -C /lib/modules/6.16.8+kali-amd64/build M=/home/kali/OS/BSU-OS-2025/lab5/gr8sub1/Стёпкин_Владимир/src modules
make[1]: Entering directory '/usr/src/linux-headers-6.16.8+kali-amd64'
make[2]: Entering directory '/home/kali/OS/BSU-OS-2025/lab5/gr8sub1/Стёпкин_Владимир/src'
  CC [M]  student_info_module.o
  MODPOST Module.symvers
  LD [M]  student_info_module.ko
  BTF [M] student_info_module.ko
make[2]: Leaving directory '/home/kali/OS/BSU-OS-2025/lab5/gr8sub1/Стёпкин_Владимир/src'
make[1]: Leaving directory '/usr/src/linux-headers-6.16.8+kali-amd64'
```
Теперь загрузим модуль ядра при помощи команды:
```bash
sudo insmod ./student_info_module.ko
```
Проверим создался ли файл и корректно ли там отбражены нужные данные. Это можно сделать при помщи команды:
```bash
cat /proc/student_info
```
Вывод в консоли:
```
Name: Vladimir Stepkin
Group: 8, Subgroup: 1
Module loaded at: 4295855179 jiffies
Read count: 1
```
Проверим работу счётчика открыв файл ещё раз:
```bash
cat /proc/student_info
```
Вывод в консоли:
```
Name: Vladimir Stepkin
Group: 8, Subgroup: 1
Module loaded at: 4295855179 jiffies
Read count: 2
```
Как видим счётчик работает корректно.

### Использование AI
В ходе выполнения лабораторной работы №5 Задание B был использован ИИ для следующих задач:
- Помощь в написании модуля ядра `student_info_module.c` с созданием `/proc/student_info`.
- Объяснение работы `jiffies` и механизма подсчёта обращений к файлу через`seq_file`.
- Исправление структуры модуля для современных версий ядра `(использование proc_ops вместо устаревших file_operations)`.
- Подсказки по корректной проверке работы модуля через команды `insmod`, `rmmod` и чтение файла `/proc/student_info`.

### Вывод
В ходе выполнения задания мы:
1. Создали и написали модуль ядра `student_info_module.c`, который при загрузке создаёт файл `/proc/student_info`.
2. Реализовали глобальные переменные для хранения времени загрузки модуля `(load_jiffies)` и счётчика обращений к файлу `(read_count)`.
3. Скомпилировали модуль с помощью `Makefile` и убедились в успешной сборке.
4. Загрузили модуль в ядро и проверили наличие файла `/proc/student_info`.
5. Убедились, что при первом чтении файла выводится корректная информация о пользователе, группе, времени загрузки модуля и счётчике чтений.
6. Проверили повторное чтение файла и убедились, что счётчик обращений увеличивается корректно.
7. Сделали выводы: модуль корректно создаёт `/proc/student_info`, хранит время загрузки в `jiffies` и правильно увеличивает счётчик обращений при каждом чтении.

## Задание C: Простой character device

### Цель
Создать `character device /dev/mychardev`, который:
   - Можно открыть/закрыть
   - При записи сохраняет данные в `kernel buffer (максимум 1024 байта)`
   - При чтении возвращает сохранённые данные
   - Выводит в `dmesg` когда устройство открывается/закрывается

### Шаги решения
1. Создать файл модуля `chardev_module.c` и подключить нужные заголовки `(module.h, fs.h, cdev.h, uaccess.h)`.
2. Объявить глобальный буфер размером `1024 байта` и переменные для хранения размера данных.
3. Реализовать функции работы с устройством:
	- `chardev_open()` — при открытии выводит сообщение в `dmesg`.
	- `chardev_release()` — при закрытии выводит сообщение в `dmesg`.
	- `chardev_read()` — возвращает сохранённые данные пользователю с помощью `copy_to_user()`.
	- `chardev_write()` — принимает данные от пользователя с помощью `copy_from_user()`, сохраняет в буфер.
4. Зарегистрировать устройство:
	- выделить номер через `alloc_chrdev_region()`,
	- инициализировать cdev через `cdev_init()` и добавить в систему `cdev_add()`.
5. При выгрузке модуля освободить ресурсы: удалить cdev и освободить номер устройства.
6. Создать `Makefile` для сборки модуля и скомпилировать.
7. Проверить работу:
	- загрузка модуля `insmod`,
	- создание устройства mknod `/dev/mychardev` c <MAJOR> 0,
	- запись в устройство, 
	- чтение,
	- просмотр сообщений.

### Решение

Скомпилируем файл `chardev_module`, в котором мы написали модуль ядра, при помощи `Makefile`:

Вывод в консоли:
```
make -C /lib/modules/6.16.8+kali-amd64/build M=/home/kali/OS/BSU-OS-2025/lab5/gr8sub1/Стёпкин_Владимир/src modules
make[1]: Entering directory '/usr/src/linux-headers-6.16.8+kali-amd64'
make[2]: Entering directory '/home/kali/OS/BSU-OS-2025/lab5/gr8sub1/Стёпкин_Владимир/src'
  CC [M]  chardev_module.o
  MODPOST Module.symvers
  CC [M]  chardev_module.mod.o
  LD [M]  chardev_module.ko
  BTF [M] chardev_module.ko
make[2]: Leaving directory '/home/kali/OS/BSU-OS-2025/lab5/gr8sub1/Стёпкин_Владимир/src'
make[1]: Leaving directory '/usr/src/linux-headers-6.16.8+kali-amd64'
```
Загрузим модуль ядра при помощи команды:
```bash
sudo insmod chardev_module.ko
```
Сначала узнаём `major number` из `dmesg` после загрузки модуля:
```bash
dmesg | tail -1
```
Вывод в консоли:
```
[ 5728.276527] mychardev: module loaded, major=244
```
Создаём устройство с этим `major number`:
```bash
sudo mknod /dev/mychardev c 244 0
```
Требования к правам доступа для `/dev/mychardev`:
- Устройство должно быть символьным (character device).
- Для корректной работы записи и чтения данные права должны позволять:
	- Чтение (read) и запись (write) для владельца устройства.
	- Чтение и запись для других пользователей, если устройство используется в тестах без `root`.
- Стандартные рекомендуемые права: `rw-rw-rw-` (т.е. все пользователи могут читать и писать).
- При необходимости ограниченного доступа можно использовать `rw-------`, чтобы только `root`мог работать с устройством.

Изменяем права доступа чтобы было доступно для чтения и записи:
```bash
sudo chmod 666 /dev/mychardev
```
Запись в устройство:
```bash
echo "Hello" > /dev/mychardev
```
Чтение из устройства:
```bash
cat /dev/mychardev
```
Вывод в консоли:
```
Hello
```
Проверка сообщений в `dmesg`:
```bash
dmesg | tail -10
```

Вывод в консоли должен включать (`mychardev: device opened` и `mychardev: device closed`):
```
[ 1928.508555] Hello from Vladimir module!
[ 2235.958332] Goodbye from Vladimir module!
[ 2399.995132] Custom greeting
[ 3724.650206] student_info module unloaded
[ 3851.512068] student_info module loaded
[ 5728.276527] mychardev: module loaded, major=244
[ 6097.683171] mychardev: device opened
[ 6097.683193] mychardev: device closed
[ 6128.888305] mychardev: device opened
[ 6128.888408] mychardev: device closed
```
Выгрузка модуля:
```bash
sudo rmmod chardev_module
```
Проверка:
```bash
dmesg | tail -5
```
Вывод в консоли:
```
[ 5728.276527] mychardev: module loaded, major=244
[ 6097.683171] mychardev: device opened
[ 6097.683193] mychardev: device closed
[ 6128.888305] mychardev: device opened
[ 6128.888408] mychardev: device closed
```

### Использование AI
В ходе выполнения лабораторной работы №5 Задание C был использован ИИ для следующих задач:
- Помощь в написании модуля `chardev_module.c` с корректной регистрацией `character device`.
- Объяснение работы функций `copy_to_user()` и `copy_from_user()`.
- Подсказки по правильной регистрации устройства через `alloc_chrdev_region()`, `cdev_init()`, `cdev_add()` и созданию `/dev/mychardev`.
- Советы по проверке работы устройства через `insmod`, `mknod`, `echo`, `cat` и `dmesg`.
- По просьбе ИИ из PR в отчёте была добавлена версия ядра, на которой выполнялась работа.
- По просьбе ИИ из PR в отчёте была добавлены информация о требованиях к  правам доступа для `/dev/mychardev`.

### Вывод
В ходе выполнения задания мы:
1. Написали модуль ядра `chardev_module.c` с буфером размером `1024 байта` и обработкой операций `open`, `release`, `read`, `write`.
2. Скомпилировали модуль с помощью `Makefile` и загрузили его в ядро.
3. Создали устройство `/dev/mychardev` с корректным `major number` и проверили работу через запись и чтение:
	- После записи "Hello" чтение возвращало "Hello".
4. Проверили вывод сообщений в `dmesg` при открытии и закрытии устройства.
5. Выгрузили модуль и убедились, что устройство удалено, а ресурсы освобождены.

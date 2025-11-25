# Отчёт по лабораторной работе №5 

---

## 1. Цель работы

Познакомиться с механизмом загрузки и выгрузки модулей ядра Linux, научиться создавать простые модули, работать с /proc файловой системой и character devices.

---

## 2. Задание A: Hello World модуль

### 2.1. Цель задания
Создать простой модуль, который выводит приветствие при загрузке и прощание при выгрузке, с поддержкой параметра message.

### 2.2. Требования
-  При загрузке выводит "Hello from Dmitrieva Polina module!"
- При выгрузке выводит "Goodbye from Dmitrieva Polina module!"
- Поддерживает параметр message (строка)
- Использует printk с KERN_INFO
- Правильные метаданные (MODULE_LICENSE, MODULE_AUTHOR)
- Использует module_param для параметра

### 2.3. Реализация

**Ключевые элементы кода (src/hello_module.c):**

```c
static char *message = NULL;
module_param(message, charp, 0644);
MODULE_PARM_DESC(message, "Custom greeting message");

static int __init hello_init(void)
{
    if (message) {
        printk(KERN_INFO "hello_module: %s\n", message);
    } else {
        printk(KERN_INFO "hello_module: Hello from Dmitrieva Polina module!\n");
    }
    return 0;
}

static void __exit hello_exit(void)
{
    printk(KERN_INFO "hello_module: Goodbye from Dmitrieva Polina module!\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dmitrieva Polina");
MODULE_DESCRIPTION("Simple Hello World kernel module");
```

###2.4 Тестирование

**Проверка работы модуля:**

```bash

# Сборка
make

# Загрузка без параметра
sudo insmod hello_module.ko
dmesg | tail -1
# Вывод: hello_module: Hello from Dmitrieva Polina module!

# Загрузка с параметром
sudo insmod hello_module.ko message="Custom greeting from Polina"
dmesg | tail -1
# Вывод: hello_module: Custom greeting from Polina

# Выгрузка
sudo rmmod hello_module
dmesg | tail -1
# Вывод: hello_module: Goodbye from Dmitrieva Polina module!
```

**Проверка параметра в sysfs:**

```bash
cat /sys/module/hello_module/parameters/message
```

###Вывод:
Модуль успешно загружается, выгружается, поддерживает параметры и корректно работает при многократном использовании.


## 3. Задание B: /proc файл с информацией
### 3.1. Цель задания

Создать модуль, который создаёт файл /proc/student_info с информацией о студенте, времени загрузки и счётчиком обращений.

### 3.2. Требования
- Создаёт файл /proc/student_info
- Выводит имя, группу, подгруппу
- Показывает время загрузки модуля в jiffies
- Счётчик обращений увеличивается при каждом чтении
- Использует proc_create() и proc_remove()
- Реализует функцию чтения proc_read

### 3.3. Реализация

**Ключевые элементы кода (src/proc_module.c):**

```c
#define PROC_NAME "student_info"
static int read_count = 0;
static unsigned long load_time = 0;

static ssize_t proc_read(struct file *file, char __user *ubuf,
                         size_t count, loff_t *ppos)
{
    char buf[MAX_SIZE];
    int len;

    if (*ppos > 0) return 0;
    read_count++;

    len = snprintf(buf, sizeof(buf),
        "Name: Dmitrieva Polina\n"
        "Group: 8, Subgroup: 1\n"
        "Module loaded at: %lu jiffies\n"
        "Read count: %d\n",
        load_time, read_count);

    if (copy_to_user(ubuf, buf, len))
        return -EFAULT;

    *ppos = len;
    return len;
}

static int __init proc_module_init(void)
{
    load_time = jiffies;
    proc_file = proc_create(PROC_NAME, 0444, NULL, &proc_file_ops);
    printk(KERN_INFO "proc_module: Created /proc/%s\n", PROC_NAME);
    return 0;
}
```

### 3.4. Тестирование

**Проверка работы модуля:**

```bash
# Загрузка модуля
sudo insmod proc_module.ko

# Первое чтение
cat /proc/student_info
# Вывод:
# Name: Dmitrieva Polina
# Group: 8, Subgroup: 1
# Module loaded at: 4297642220 jiffies
# Read count: 1

# Второе чтение (счётчик увеличивается)
cat /proc/student_info
# Read count: 2

# Выгрузка
sudo rmmod proc_module
dmesg | tail -3
# Вывод показывает создание, удаление файла и общее количество чтений
```

### 3.5. Вывод

Модуль успешно создаёт /proc файл, корректно отображает информацию, счётчик обращений работает правильно.


## 4. Задание C: Простой character device
### 4.1. Цель задания

Создать character device /dev/mychardev, который может сохранять и возвращать данные через kernel buffer.

### 4.2. Требования
- Создаёт character device /dev/mychardev
- Поддерживает операции open, close, read, write
- Сохраняет данные в kernel buffer (1024 байта)
- Выводит в dmesg при открытии/закрытии устройства
- Использует alloc_chrdev_region(), cdev_init(), cdev_add()
- Использует copy_to_user() и copy_from_user()

### 4.3. Реализация

**Ключевые элементы кода (src/chardev_module.c):**
```c

#define DEVICE_NAME "mychardev"
#define BUF_SIZE 1024

static char device_buffer[BUF_SIZE];
static int buffer_size = 0;

static int dev_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "chardev: Device opened\n");
    return 0;
}

static ssize_t dev_read(struct file *file, char __user *buf,
                        size_t len, loff_t *off)
{
    int bytes_to_read = min(len, (size_t)(buffer_size - *off));
    if (copy_to_user(buf, device_buffer + *off, bytes_to_read))
        return -EFAULT;
    *off += bytes_to_read;
    return bytes_to_read;
}

static ssize_t dev_write(struct file *file, const char __user *buf,
                         size_t len, loff_t *off)
{
    int bytes_to_write = min(len, (size_t)BUF_SIZE);
    memset(device_buffer, 0, BUF_SIZE);
    if (copy_from_user(device_buffer, buf, bytes_to_write))
        return -EFAULT;
    buffer_size = bytes_to_write;
    printk(KERN_INFO "chardev: Written %d bytes\n", bytes_to_write);
    return bytes_to_write;
}
```

### 4.4. Тестирование

**Проверка работы модуля:**
```bash

# Загрузка модуля
sudo insmod chardev_module.ko

# Получение major номера
MAJOR=$(grep "mychardev" /proc/devices | awk '{print $1}')
echo "Major number: $MAJOR"

# Создание device node
sudo mknod /dev/mychardev c $MAJOR 0
sudo chmod 666 /dev/mychardev

# Запись данных
echo "Hello from Dmitrieva Polina" > /dev/mychardev

# Чтение данных
cat /dev/mychardev
# Вывод: Hello from Dmitrieva Polina

# Очистка
sudo rm /dev/mychardev
sudo rmmod chardev_module

# Проверка логов
dmesg | tail -10
# Показывает открытие, запись, чтение, закрытие устройства
```

### 4.5. Вывод

Character device успешно создаётся, данные корректно записываются и читаются, все операции логируются в dmesg.


## 5. Сборка проекта
###5.1. Makefile

**Makefile разделён на две части:**

Часть для kbuild (когда определён KERNELRELEASE):
Здесь описываются модули и из каких объектных файлов они состоят:
```makefile

ifneq ($(KERNELRELEASE),)

obj-m += hello_module.o
obj-m += proc_module.o
obj-m += chardev_module.o

hello_module-objs   := src/hello_module.o
proc_module-objs    := src/proc_module.o
chardev_module-objs := src/chardev_module.o

ccflags-y := -std=gnu99 -Wno-declaration-after-statement
```

Часть для обычного make из каталога студента:
```makefile

else

KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
.DEFAULT_GOAL := all

all:
	@echo "Building kernel modules in $(PWD)..."
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules
	@echo ""
	@echo "✓ Modules built:"
	@ls -1 *.ko 2>/dev/null || echo "  (no .ko files found)"
```

**Также реализованы цели:**

- clean — очистка временных файлов и .ko;
- load MODULE=... / unload MODULE=... — загрузка/выгрузка модуля;
- test-hello, test-proc, test-chardev — быстрые тесты модулей;
- info, check, help — вспомогательные цели.

Такое разделение убирает бесконечную рекурсию, когда ядро снова вызывает make.

## 6. Сильные стороны реализованных решений
- Нет рекурсии и зацикливания в Makefile — чёткое разделение режима kbuild и пользовательского make.
- Аккуратная работа с памятью: используются фиксированные буферы, учитывается реальный размер данных (buffer_size).
- Безопасная работа с указателями: везде, где есть обмен с user-space, применяются copy_to_user и copy_from_user.
- Информативные сообщения в dmesg: легко отследить жизненный цикл модулей и операции чтения/записи.
- Корректное освобождение ресурсов: в proc_module_exit и chardev_exit всё удаляется и освобождается.
- Использование параметров модуля в hello_module делает модуль чуть гибче и показывает работу module_param.


## 7. Ответы на вопросы для отчёта
### 7.1. Базовые понятия

1. Что такое модуль ядра и зачем он нужен?
Модуль ядра — это кусок кода (обычно файл .ko), который можно динамически загружать и выгружать в уже работающем ядре.
Нужен, чтобы добавлять функциональность (драйверы устройств, файловые системы и т.п.) без пересборки и перезагрузки ядра.
2. Чем отличается kernel-space от user-space?
user-space — обычные приложения, запущенные пользователем. Они не имеют прямого доступа к железу и памяти ядра.
kernel-space — код ядра и модулей. Здесь есть полный доступ ко всей памяти и устройствам. Ошибка здесь может «повалить» всю систему.
3. Что произойдёт, если в модуле обратиться к NULL указателю?
Скорее всего будет kernel panic или oops: ядро попробует обратиться по адресу 0, это недопустимо → система может зависнуть, перезагрузиться или хотя бы «упасть».
4. Почему нельзя использовать printf() в модуле ядра?
printf() — это функция из стандартной C-библиотеки user-space, она недоступна в ядре.
В ядре используются свои функции логирования, в том числе printk().
5. Что такое kernel panic и как его избежать?
Kernel panic — это критическая ошибка в ядре, после которой ядро не может продолжать работу.
Избежать можно, если:

- аккуратно работать с указателями и памятью;
- проверять возвращаемые значения функций;
- не обращаться к user-space указателям напрямую;
- соблюдать протоколы работы с синхронизацией и устройствами

### 7.2. Жизненный цикл модуля
1. Какие функции вызываются при insmod и rmmod?
При insmod вызывается функция, помеченная module_init(...) (у меня это hello_init, proc_module_init, chardev_init).
При rmmod вызывается функция, помеченная module_exit(...) (hello_exit, proc_module_exit, chardev_exit).
2. Что должна делать функция module_exit()?
- Освобождать все ресурсы, которые были выделены в module_init():
- удалять proc-файлы;
- удалять cdev;
- освобождать номера устройств;
- освобождать динамическую память (если выделялась).
3. Что происходит, если module_init() возвращает ошибку?
Модуль не будет загружен. Ядро отменит загрузку, вызов module_exit() не произойдёт, а insmod покажет ошибку.
4. Можно ли выгрузить модуль, если он используется?
Обычно нет: если модуль занят (например, открыт файл устройства), rmmod вернёт ошибку Device or resource busy.
Сначала нужно закрыть все дескрипторы, остановить использование, и только потом выгружать.

### 7.3. Логирование и отладка
1. Чем printk() отличается от printf()?
printk() — функция ядра для логирования, выводит сообщения в ring-buffer ядра (dmesg).
printf() — функция стандартной библиотеки C для user-space и в ядре не используется.
2. Какие уровни логирования существуют в ядре?
Примеры уровней:

KERN_EMERG — авария системы;
KERN_ALERT, KERN_CRIT, KERN_ERR — разные уровни ошибок;
KERN_WARNING — предупреждения;
KERN_NOTICE — важная, но не ошибка;
KERN_INFO — обычная информация (используется в моих модулях);
KERN_DEBUG — отладочные сообщения.
3. Как посмотреть логи модуля?
Командой:

```bash

sudo dmesg | tail
```
или с фильтром по имени модуля:

```bash

sudo dmesg | grep hello_module
```
4. Что означает "tainted kernel"?
Это статус ядра, говорящий, что загружены проприетарные или сомнительные модули, либо произошли нестандартные ситуации. Такой ядро считается «испорченным» с точки зрения поддержки.

### 7.4. Память
1. Чем kmalloc() отличается от malloc()?
malloc() — функция пользовательской библиотеки (glibc), работает в user-space.
kmalloc() — функция ядра для выделения памяти в kernel-space, с флагами GFP_....
2. Что такое флаги GFP и зачем они нужны?
GFP — Get Free Pages флаги, которые описывают, как и когда можно выделять память (GFP_KERNEL, GFP_ATOMIC и т.д.).
Например, GFP_KERNEL можно спать и ждать память, а GFP_ATOMIC использовать в прерываниях, где спать нельзя.
3. Что произойдёт, если не освободить память в module_exit()?
Память останется занятой до перезагрузки ядра. Это приведёт к утечкам памяти в kernel-space.
Поэтому всё, что было выделено в module_init(), должно быть освобождено в module_exit().
4. Почему нельзя использовать user-space указатели напрямую в ядре?
User-space указатель может указывать на:

- область, не отображённую в память;
- память, к которой нет прав доступа;
- адрес, который станет невалидным (например, после swap’а).
Прямое разыменование такого указателя может привести к kernel panic, поэтому используются copy_to_user() и copy_from_user().

### 7.5. Взаимодействие с user-space
1. Что такое /proc и для чего он используется?
/proc — виртуальная файловая система, через которую ядро предоставляет информацию о себе и процессах.
Можно добавлять свои файлы (как /proc/student_info), чтобы передавать данные из ядра в user-space.
2. Что такое /sys (sysfs) и чем отличается от procfs?
/sys — это sysfs, файловая система для представления устройств, драйверов и их свойств как объектов.
/proc больше ориентирован на процессы и общую информацию о ядре.
В современном Linux часть информации из /proc переехала или дублируется в /sys.
3. Зачем нужны функции copy_to_user() и copy_from_user()?
Они безопасно копируют данные между kernel-space и user-space, проверяя доступ к памяти.
Без них прямое разыменование указателя из user-space опасно.
4. Что такое character device и как он работает?
Character device — символьное устройство, с которым можно работать через обычные операции чтения/записи (read, write, open, close).
Оно отображается как файл в /dev (например, /dev/mychardev), но за ним стоит драйвер в ядре.

### 7.6. Параметры и метаданные
1. Как передать параметры модулю при загрузке?
Через module_param в коде и при загрузке:

```c

module_param(message, charp, 0644);
```
Вызов:

```bash

sudo insmod hello_module.ko message="Hi"
```
2. Зачем нужен MODULE_LICENSE()?
Чтобы указать тип лицензии модуля. Если указать GPL, ядро считает модуль совместимым и не помечает себя «tainted» из-за этого модуля.
3. Что произойдёт, если не указать лицензию?
Ядро может считать модуль проприетарным, выставит флаг tainted, и некоторые возможности могут быть ограничены.

### 7.7. Безопасность
1. Какие основные правила безопасного кода в ядре?
- Проверять все указатели (на NULL и т.д.);
- Проверять коды возврата функций;
- Не выходить за границы массивов и буферов;
- Не блокировать прерывания слишком надолго;
- Всегда освобождать ресурсы при ошибках и при выгрузке.
2. Можно ли использовать бесконечный цикл в модуле?
Теоретически да, но это очень опасно. Если цикл без выхода и без schedule()/таймеров — можно повесить всю систему, особенно в контексте ядра.
3. Почему в ядре нет FPU операций?
Обычно ядро старается не использовать операции с плавающей точкой, потому что это требует сохранения/восстановления контекста FPU для каждого потока, что дорого и усложняет работу. В модулях ядра их обычно избегают.
4. Что делать, если модуль вызвал kernel panic?
- Перезагрузить систему;
- Посмотреть логи (dmesg) или crash dump;
- Найти место ошибки в коде (обычно неправильно работа с памятью или указателями);
- Исправить модуль и больше так не делать :)

### 7.8. Практические вопросы
1. Как узнать, какие модули загружены в системе?

```bash

lsmod
```
или:

```bash

cat /proc/modules
```
2. Как получить информацию о модуле (версия, параметры)?

```bash

modinfo hello_module.ko
```
Показывает: автор, описание, версию, параметры (например, message), лицензию и т.п.

## 8. Как я использовала ИИ
**При выполнении работы я использовала ИИ (ChatGPT) как помощника:**

- помог исправить рекурсивный Makefile, который вызывал бесконечную сборку;
- помог разобраться, почему не работал mknod (я писала <MAJOR> буквально, а не число);
- подсказал последовательность команд для тестирования модулей и проверки dmesg.


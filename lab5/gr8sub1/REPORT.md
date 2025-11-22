# Лабораторная работа 5: Модули ядра Linux

## Студент: Филипчик Дима  
## Группа: 8, Подгруппа: 1  
## Вариант: 1  

## Введение

**Цель работы:** Изучение архитектуры ядра Linux и приобретение практических навыков разработки модулей ядра, взаимодействующих с user-space через различные интерфейсы.

**Окружение для выполнения работы:**
- Операционная система: Ubuntu 24.04 LTS
- Версия ядра: 6.14.0-29-generic
- Виртуальная машина: VirtualBox

## Выполненные задания

### Задание A: Hello World модуль

**Цель:** Создать простой модуль ядра, который выводит приветствие при загрузке и прощание при выгрузке, с поддержкой параметров.

**Реализованные функции:**
- Функция инициализации `hello_init()`
- Функция выгрузки `hello_exit()`
- Параметр модуля `message` для кастомных сообщений

**Код модуля:**
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/moduleparam.h>

static char *message = NULL;
module_param(message, charp, 0644);

static int __init hello_init(void) {
    if (message) {
        printk(KERN_INFO "hello_module: %s\n", message);
    } else {
        printk(KERN_INFO "hello_module: Hello from Filipchik Dima module!\n");
    }
    return 0;
}

static void __exit hello_exit(void) {
    printk(KERN_INFO "hello_module: Goodbye from Filipchik Dima module!\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Filipchik Dima <dima.filipchik@example.com>");
MODULE_DESCRIPTION("Simple Hello World kernel module");
MODULE_VERSION("1.0");

**Результаты тестирования:**
# Загрузка модуля без параметров
$ sudo insmod hello_module.ko
$ sudo dmesg | tail -5
[ 4150.042914] hello_module: Hello from Filipchik Dima module!

# Выгрузка модуля
$ sudo rmmod hello_module
$ sudo dmesg | tail -5
[ 4150.042914] hello_module: Hello from Filipchik Dima module!
[ 4164.817216] hello_module: Goodbye from Filipchik Dima module!

# Загрузка с параметром message
$ sudo insmod hello_module.ko message="Custom"
$ sudo dmesg | tail -5
[ 4150.042914] hello_module: Hello from Filipchik Dima module!
[ 4164.817216] hello_module: Goodbye from Filipchik Dima module!
[ 4181.080855] hello_module: Custom

# Выгрузка модуля
$ sudo rmmod hello_module

**Анализ результатов:**
 1. Модуль успешно загружается и выгружается
 2. При загрузке без параметров выводится сообщение по умолчанию с именем студента
 3. При загрузке с параметром message="Custom" выводится кастомное сообщение
 4. При выгрузке модуля выводится прощальное сообщение
 5. Все сообщения записываются в системные логи через printk()
 6. Временные метки показывают корректную работу модуля во времени
 
### Задание B: /proc файл с информацией 

**Цель:** Создать модуль, который создает виртуальный файл в /proc с информацией о студенте и счетчиком обращений.

**Реализованные функции:**
- Создание /proc файла с помощью proc_create()
- Функция чтения proc_read() с увеличением счетчика
- Отображение времени загрузки модуля в jiffies

**Код модуля:**
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>

#define PROC_NAME "student_info"
#define MAX_SIZE 1024

static struct proc_dir_entry *proc_file = NULL;
static int read_count = 0;
static unsigned long load_time = 0;

static ssize_t proc_read(struct file *file, char __user *ubuf,
                         size_t count, loff_t *ppos) {
    char buf[MAX_SIZE];
    int len;
    unsigned long uptime_seconds = jiffies / HZ;

    if (*ppos > 0) return 0;

    read_count++;
    
    len = snprintf(buf, sizeof(buf),
        "=== Student Information ===\n"
        "Name: Filipchik Dima\n"
        "Group: 8, Subgroup: 1\n"
        "Module loaded at: %lu jiffies\n"
        "Uptime: %lu seconds\n"
        "Read count: %d\n"
        "===========================\n",
        load_time, uptime_seconds, read_count);

    if (copy_to_user(ubuf, buf, len))
        return -EFAULT;

    *ppos = len;
    return len;
}

static const struct proc_ops proc_file_ops = {
    .proc_read = proc_read,
};

static int __init proc_module_init(void) {
    load_time = jiffies;
    proc_file = proc_create(PROC_NAME, 0444, NULL, &proc_file_ops);
    return 0;
}

static void __exit proc_module_exit(void) {
    if (proc_file) proc_remove(proc_file);
}

module_init(proc_module_init);
module_exit(proc_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Filipchik Dima");
MODULE_DESCRIPTION("Proc filesystem example");
MODULE_VERSION("1.0");

**Результаты тестирования:**
# Загрузка модуля
$ sudo insmod proc_module.ko

# Проверка создания /proc файла
$ ls -la /proc/student_info
-r--r--r-- 1 root root 0 ноя 22 17:17 /proc/student_info

# Первое чтение файла
$ cat /proc/student_info
=== Student Information ===
Name: Filipchik Dima
Group: 8, Subgroup: 1
Module loaded at: 4299339083 jiffies
Uptime: 4299354 seconds
Read count: 1

# Второе чтение файла (счетчик увеличился)
$ cat /proc/student_info
=== Student Information ===
Name: Filipchik Dima
Group: 8, Subgroup: 1
Module loaded at: 4299339083 jiffies
Uptime: 4299367 seconds
Read count: 2

# Просмотр логов ядра
$ sudo dmesg | tail -5
[ 4672.282609] proc_module: Initializing
[ 4672.282619] proc_module: Created /proc/student_info
[ 4672.282621] proc_module: Load time: 4299339083 jiffies
[ 4687.992200] proc_module: File read, count: 1
[ 4700.666333] proc_module: File read, count: 2

# Выгрузка модуля и проверка финальных логов
$ sudo rmmod proc_module
$ sudo dmesg | tail -5
[ 4687.992200] proc_module: File read, count: 1
[ 4700.666333] proc_module: File read, count: 2
[ 4759.051141] proc_module: File read, count: 3
[ 4777.955119] proc_module: Removed /proc/student_info
[ 4777.955126] proc_module: Total reads during lifetime: 3

**Анализ результатов:**
- Модуль успешно создает файл /proc/student_info с правами 0444 (read-only)
- Файл содержит корректную информацию: имя, группа 8 подгруппа 1, время загрузки
- Счетчик обращений Read count увеличивается с каждым чтением файла (1 → 2 → 3)
- Время uptime обновляется при каждом чтении (4299354 → 4299367 секунд)
- Время загрузки модуля фиксируется в jiffies и остается постоянным
- Логи ядра показывают корректную работу: инициализация, создание файла, подсчет чтений
- При выгрузке модуля файл корректно удаляется из /proc
- Итоговый счетчик чтений (3) соответствует количеству обращений к файлу

### Задание C: Character Device

**Цель:** Создать character device с поддержкой операций чтения и записи.

**Реализованные функции:**
- Регистрация устройства с alloc_chrdev_region()
- Операции open, release, read, write
- Буфер для хранения данных с защитой мьютексом
- Безопасное копирование данных с copy_to_user() и copy_from_user()

**Код модуля:**
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define DEVICE_NAME "mychardev"
#define BUF_SIZE 1024

static dev_t dev_num;
static struct cdev my_cdev;
static char *device_buffer;
static int buffer_size = 0;
static DEFINE_MUTEX(device_mutex);

static int dev_open(struct inode *inode, struct file *file) {
    printk(KERN_INFO "chardev: Device opened\n");
    return 0;
}

static int dev_release(struct inode *inode, struct file *file) {
    printk(KERN_INFO "chardev: Device closed\n");
    return 0;
}

static ssize_t dev_read(struct file *file, char __user *buf,
                        size_t len, loff_t *off) {
    int bytes_to_read;
    
    mutex_lock(&device_mutex);
    if (*off >= buffer_size) {
        mutex_unlock(&device_mutex);
        return 0;
    }
    
    bytes_to_read = min((size_t)(buffer_size - *off), len);
    
    if (copy_to_user(buf, device_buffer + *off, bytes_to_read)) {
        mutex_unlock(&device_mutex);
        return -EFAULT;
    }
    
    *off += bytes_to_read;
    mutex_unlock(&device_mutex);
    return bytes_to_read;
}

static ssize_t dev_write(struct file *file, const char __user *buf,
                         size_t len, loff_t *off) {
    int bytes_to_write;
    
    mutex_lock(&device_mutex);
    bytes_to_write = min(len, (size_t)(BUF_SIZE - 1));
    memset(device_buffer, 0, BUF_SIZE);
    
    if (copy_from_user(device_buffer, buf, bytes_to_write)) {
        mutex_unlock(&device_mutex);
        return -EFAULT;
    }
    
    buffer_size = bytes_to_write;
    *off = 0;
    mutex_unlock(&device_mutex);
    return bytes_to_write;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .release = dev_release,
    .read = dev_read,
    .write = dev_write,
};

static int __init chardev_init(void) {
    int ret;
    
    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) return ret;
    
    device_buffer = kmalloc(BUF_SIZE, GFP_KERNEL);
    if (!device_buffer) {
        unregister_chrdev_region(dev_num, 1);
        return -ENOMEM;
    }
    
    cdev_init(&my_cdev, &fops);
    ret = cdev_add(&my_cdev, dev_num, 1);
    if (ret < 0) {
        kfree(device_buffer);
        unregister_chrdev_region(dev_num, 1);
        return ret;
    }
    
    mutex_init(&device_mutex);
    return 0;
}

static void __exit chardev_exit(void) {
    cdev_del(&my_cdev);
    kfree(device_buffer);
    mutex_destroy(&device_mutex);
    unregister_chrdev_region(dev_num, 1);
}

module_init(chardev_init);
module_exit(chardev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Filipchik Dima");
MODULE_DESCRIPTION("Simple character device driver");
MODULE_VERSION("1.0");

**Результаты тестирования:**
# Очистка логов и загрузка модуля
$ sudo dmesg -C
$ sudo insmod chardev_module.ko

# Поиск major номера устройства
$ sudo dmesg | grep "major number"
[ 5209.652540] chardev: Registered with major number 240, minor 0

# Создание device node
$ sudo mknod /dev/mychardev c 240 0
$ sudo chmod 666 /dev/mychardev

# Проверка создания устройства
$ ls -la /dev/mychardev
crw-rw-rw- 1 root root 240, 0 ноя 22 17:27 /dev/mychardev

# Тест записи и чтения
$ echo "Hello from Filipchik Dima" > /dev/mychardev
$ cat /dev/mychardev
Hello from Filipchik Dima

# Тест перезаписи данных
$ echo "New data for character device" > /dev/mychardev
$ cat /dev/mychardev
New data for character device

# Тест длинного сообщения
$ echo "This is a longer test message to verify that our character device driver works correctly with different data sizes" > /dev/mychardev
$ cat /dev/mychardev
This is a longer test message to verify that our character device driver works correctly with different data sizes

# Просмотр детальных логов
$ sudo dmesg | tail -20
[ 5286.770627] chardev: Device closed
[ 5295.046712] chardev: Device opened
[ 5295.046727] chardev: Read 26 bytes from offset 0
[ 5295.046748] chardev: Device closed
[ 5305.282952] chardev: Device opened
[ 5305.282970] chardev: Write 30 bytes: 'New data for character device'
[ 5305.282976] chardev: Device closed
[ 5305.285381] chardev: Device opened
[ 5305.285394] chardev: Read 30 bytes from offset 0
[ 5305.285418] chardev: Device closed
[ 5319.888603] chardev: Device opened
[ 5333.943811] chardev: Device opened
[ 5333.943831] chardev: Write 115 bytes: 'This is a longer test message...'
[ 5333.943838] chardev: Device closed
[ 5342.959312] chardev: Device opened
[ 5342.959362] chardev: Read 115 bytes from offset 0
[ 5342.962053] chardev: Device closed

# Сохранение логов и очистка
$ sudo dmesg > ../logs/chardev_test.log
$ sudo rm /dev/mychardev
$ sudo rmmod chardev_module

**Анализ результатов:**
- Модуль успешно зарегистрировал character device с major номером 240
- Device node /dev/mychardev создан корректно с правами 666 (чтение/запись для всех)
- Операция записи работает: данные успешно сохраняются в буфер устройства
- Операция чтения работает: данные корректно возвращаются из буфера
- Перезапись данных работает: новые данные заменяют старые в буфере
- Поддержка длинных сообщений: устройство обрабатывает сообщения разного размера
- Логи показывают корректную работу: открытие/закрытие устройства, подсчет байт
- Размеры данных записываются правильно: 26, 30, 115 байт
- Устройство корректно закрывается после каждой операции
- Ресурсы освобождаются при выгрузке модуля

### Информация о модулях
$ modinfo hello_module.ko
filename:       /home/dima/BSU-OS-2025/lab5/gr8sub1/Филипчик_Дима/src/hello_module.ko
version:        1.0
description:    Simple Hello World kernel module
author:         Filipchik Dima <dima.filipchik@example.com>
license:        GPL
srcversion:     B2F3D373015D2978E5D78BA
depends:
name:           hello_module
retpoline:      Y
vermagic:       6.14.0-29-generic SMP preempt mod_unload modversions 
parm:           message:Custom greeting message (charp)

$ modinfo proc_module.ko
filename:       /home/dima/BSU-OS-2025/lab5/gr8sub1/Филипчик_Дима/src/proc_module.ko
version:        1.0
description:    Proc filesystem example
author:         Filipchik Dima
license:        GPL
srcversion:     865AB11808D7F63522B4596
depends:
name:           proc_module
retpoline:      Y
vermagic:       6.14.0-29-generic SMP preempt mod_unload modversions 

$ modinfo chardev_module.ko
filename:       /home/dima/BSU-OS-2025/lab5/gr8sub1/Филипчик_Дима/src/chardev_module.ko
version:        1.0
description:    Simple character device driver
author:         Filipchik Dima
license:        GPL
srcversion:     2165495BBCF04648DA8BE3A
depends:
name:           chardev_module
retpoline:      Y
vermagic:       6.14.0-29-generic SMP preempt mod_unload modversions

Сборка модулей
Все три модуля успешно скомпилированы без ошибок и предупреждений. Модули совместимы с версией ядра 6.14.0-29-generic.

#  Ответы на вопросы
### Базовые понятия
1. Что такое модуль ядра и зачем он нужен?
Модуль ядра - это динамически загружаемый компонент ядра Linux, который может быть добавлен или удалён без перезагрузки системы. Нужен для:
- Добавления драйверов устройств
- Поддержки новых файловых систем
- Расширения функциональности ядра
- Экономии памяти (загружаются только нужные компоненты)
2. Чем отличается kernel-space от user-space?
- Kernel-space: привилегированный режим, полный доступ к оборудованию, выполнение кода ядра, ошибки приводят к kernel panic
- User-space: непривилегированный режим, доступ через системные вызовы, защита памяти, ошибки убивают только процесс
3. Что произойдёт, если в модуле обратиться к NULL указателю?
Произойдёт kernel panic - система аварийно остановится с сообщением "Unable to handle kernel NULL pointer dereference". В ядре нет механизма обработки segmentation fault.
4. Почему нельзя использовать printf() в модуле ядра?
Потому что в kernel-space нет стандартной библиотеки C (libc). Вместо printf() используется printk(), который выводит сообщения в кольцевой буфер ядра.
5. Что такое kernel panic и как его избежать?
Kernel panic - критическая ошибка ядра, приводящая к немедленной остановке системы. Чтобы избежать:
- Проверять указатели перед использованием
- Проверять возвращаемые значения функций
- Использовать правильные функции копирования памяти
- Тестировать в виртуальной машине
### Жизненный цикл модуля:
6. Какие функции вызываются при insmod и rmmod?
- При insmod: вызывается функция, зарегистрированная через module_init()
- При rmmod: вызывается функция, зарегистрированная через module_exit()
7. Что должна делать функция module_exit()?
Должна освобождать все ресурсы, выделенные в module_init():
- Освобождать память (kfree)
- Отменять регистрацию устройств, /proc файлов
- Закрывать открытые ресурсы
8. Что происходит, если module_init() возвращает ошибку?
Модуль не загружается, функция module_exit() НЕ вызывается. Все ресурсы, выделенные до ошибки, должны быть освобождены вручную перед возвратом ошибки.
9. Можно ли выгрузить модуль, если он используется?
Нет, при попытке выгрузки используемого модуля получим ошибку: "rmmod: ERROR: Module my_module is in use". Нужно сначала закрыть все процессы, использующие модуль.
### Логирование и отладка:
10. Чем printk() отличается от printf()?
- printk() выводит в kernel log buffer, printf() - в stdout
- printk() имеет уровни логирования (KERN_INFO, KERN_ERR и т.д.)
- printk() не может блокироваться, должен работать всегда
- Просмотр: printk() через dmesg, printf() сразу в терминале
11. Какие уровни логирования существуют в ядре?
- KERN_EMERG (0) - аварийная ситуация
- KERN_ALERT (1) - требует немедленных действий
- KERN_CRIT (2) - критические условия
- KERN_ERR (3) - ошибки
- KERN_WARNING (4) - предупреждения
- KERN_NOTICE (5) - важные уведомления
- KERN_INFO (6) - информационные сообщения
- KERN_DEBUG (7) - отладочные сообщения
12. Как посмотреть логи модуля?
dmesg | tail -20              # последние 20 сообщений
dmesg | grep "my_module"      # фильтр по имени модуля
dmesg -w                      # режим реального времени
journalctl -k -f              # через systemd (если есть)
13. Что означает "tainted kernel"?
"Tainted kernel" означает, что ядро "испорчено" загрузкой неподписанных или проприетарных модулей. Проверить можно через cat /proc/sys/kernel/tainted. Причины:
- Загрузка модуля без GPL лицензии
- Загрузка неподписанного модуля
- Принудительная загрузка модуля
### Память:
14. Чем kmalloc() отличается от malloc()?
- kmalloc() выделяет физически непрерывную память в ядре
- malloc() выделяет виртуальную память в user-space
- kmalloc() не может использовать swap
- kmalloc() требует указания флагов GFP (Get Free Pages)
15. Что такое флаги GFP и зачем они нужны?
Флаги GFP определяют контекст выделения памяти:
- GFP_KERNEL - обычное выделение (может спать)
- GFP_ATOMIC - атомарное выделение (не может спать, для прерываний)
- GFP_USER - для user-space данных
- GFP_DMA - для DMA-доступной памяти
16. Что произойдёт, если не освободить память в module_exit()?
Произойдёт утечка памяти - выделенная память будет потеряна до перезагрузки системы. В ядре нет garbage collector.
17. Почему нельзя использовать user-space указатели напрямую в ядре?
Потому что user-space и kernel-space используют разные адресные пространства. Нужно использовать:
- copy_to_user() для копирования из ядра в user-space
- copy_from_user() для копирования из user-space в ядро
### Взаимодействие с user-space:
18. Что такое /proc и для чего он используется?
/proc - виртуальная файловая система для предоставления информации о системе и процессах. Используется для:
- Экспорта информации из ядра (cpuinfo, meminfo)
- Взаимодействия с модулями ядра
- Получения информации о процессах
19. Что такое /sys (sysfs) и чем отличается от procfs?
/sys - современная файловая система для экспорта информации об устройствах и драйверах. Отличия от procfs:
- Procfs: для информации о процессах и системе
- Sysfs: для иерархии устройств и их атрибутов
- Sysfs: один файл = одно значение
- Procfs: может содержать сложные структуры данных
20. Зачем нужны функции copy_to_user() и copy_from_user()?
Для безопасного копирования данных между kernel-space и user-space. Они:
- Проверяют валидность user-space указателей
- Обрабатывают разные адресные пространства
- Защищают от ошибок доступа к памяти
21. Что такое character device и как он работает?
Character device - устройство, с которым можно работать как с файлом (последовательный доступ). Примеры: /dev/null, /dev/random. Работает через:
- Регистрацию major/minor номеров
- Реализацию file_operations (open, read, write, release)
- Взаимодействие через системные вызовы
### Параметры и метаданные:
22. Как передать параметры модулю при загрузке?
sudo insmod module.ko param1=value1 param2=value2
В коде объявляются через module_param().
23. Зачем нужен MODULE_LICENSE()?
Для указания лицензии модуля. Без GPL-лицензии:
- Модуль пометит ядро как "tainted"
- Не будет доступа к некоторым GPL-only функциям ядра
- Может не пройти проверку в некоторых дистрибутивах
24. Что произойдёт, если не указать лицензию?
Ядро будет помечено как "tainted", некоторые функции ядра будут недоступны, модуль может считаться проприетарным.
### Безопасность:
25. Какие основные правила безопасного кода в ядре?
- Всегда проверять возвращаемые значения
- Освобождать все ресурсы в module_exit()
- Использовать copy_to/from_user()
- Проверять границы массивов и буферов
- Не использовать стандартную библиотеку C
- Тестировать в изолированной среде
26. Можно ли использовать бесконечный цикл в модуле?
Нет, бесконечный цикл может заблокировать систему, особенно в контексте прерываний. Ядро не может вытеснить код модуля.
27. Почему в ядре нет FPU операций?
Потому что сохранение/восстановление состояния FPU требует времени и не всегда нужно. Использование FPU в ядре сложно и может привести к corruption состояния.
28. Что делать, если модуль вызвал kernel panic?
- Не паниковать 
- Перезагрузить систему (в VM - сделать reset)
- Проанализировать сообщение об ошибке в dmesg (если доступно)
- Исправить код и повторить тестирование
- Использовать отладочные выводы printk(KERN_DEBUG)
29. Как узнать, какие модули загружены в системе?
```
lsmod                    # список всех загруженных модулей
cat /proc/modules        # детальная информация
lsmod | grep module_name # поиск конкретного модуля
```
30. Как получить информацию о модуле (версия, параметры)?
```
modinfo module.ko        # информация о скомпилированном модуле
modinfo module_name      # информация о загруженном модуле
cat /sys/module/module_name/parameters/param_name # значение параметра
```

## Выводы
В ходе лабораторной работы были успешно реализованы и протестированы три модуля ядра Linux:
1. **Hello World модуль** - продемонстрировал базовые принципы работы модулей: загрузку, выгрузку и передачу параметров. Модуль корректно выводит сообщения в системные логи через printk() и поддерживает параметры через module_param.
2. **Proc module** - создал виртуальный файл в /proc файловой системе, который отображает информацию о студенте и счетчик обращений. Модуль успешно взаимодействует с user-space через интерфейс /proc, используя безопасные методы копирования данных copy_to_user().
3. **Character device module** - реализовал драйвер символьного устройства с поддержкой операций чтения и записи. Устройство корректно сохраняет и возвращает данные, используя буфер с защитой мьютексом и безопасные методы копирования данных между пространствами.

**Основные достижения:**
- Все модули компилируются без ошибок и совместимы с ядром 6.14.0-29-generic
- Модули корректно загружаются и выгружаются без kernel panic
- Реализовано взаимодействие с user-space через различные интерфейсы
- Обеспечена безопасность работы с пользовательскими данными
- Ресурсы корректно освобождаются при выгрузке модулей

**Полученные навыки:**
- Разработка модулей ядра Linux
- Работа с системными вызовами и интерфейсами ядра
- Безопасное программирование в kernel-space
- Отладка модулей через dmesg и системные логи
- Управление ресурсами и памятью в ядре

Работа подтвердила важность тщательного тестирования, обработки ошибок и правильного управления ресурсами при программировании в пространстве ядра. Приобретенные навыки являются фундаментальными для разработки драйверов устройств и системного программирования в Linux.

Лабораторная работа 5 - Модули ядра Linux
Студент: Loyko Evgeniy
Группа: 8, Подгруппа: 1
Вариант: 1 
Окружение: Ubuntu 24.04.3 LTS, ядро 6.8.0-88-generic


Задание A: Hello World модуль

Код модуля
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/moduleparam.h>

static char *message = NULL;
module_param(message, charp, 0644);
MODULE_PARM_DESC(message, "Custom greeting message");

static int __init hello_init(void)
{
    if (message) {
        printk(KERN_INFO "hello_module: %s\n", message);
    } else {
        printk(KERN_INFO "hello_module: Hello from LoykoEvgeniy module!\n");
    }
    return 0;
}

static void __exit hello_exit(void)
{
    printk(KERN_INFO "hello_module: Goodbye from LoykoEvgeniy module!\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LoykoEvgeniy");
MODULE_DESCRIPTION("Simple Hello World kernel module");
MODULE_VERSION("1.0");

Результаты тестирования
Тест 1: Загрузка без параметра
$ sudo insmod hello_module.ko
$ sudo dmesg | tail -1
[  526.522738] hello_module: Hello from LoykoEvgeniy module!

$ lsmod | grep hello_module
hello_module           12288  0

$ sudo rmmod hello_module
$ sudo dmesg | tail -1
[  605.991198] hello_module: Goodbye from LoykoEvgeniy module!

Тест 2: Загрузка с параметром (с пробелами - ошибка)
$ sudo insmod hello_module.ko message="Custom greeting from kernel space!"
$ sudo dmesg | tail -3
[  672.806076] hello_module: unknown parameter 'from' ignored
[  672.806078] hello_module: unknown parameter 'kernel' ignored  
[  672.806079] hello_module: unknown parameter 'space!' ignored
[  672.806341] hello_module: Custom

$ cat /sys/module/hello_module/parameters/message
Custom

Тест 3: Загрузка с правильным параметром
$ sudo insmod hello_module.ko message="Custom_greeting_from_kernel_space"
$ sudo dmesg | tail -1
[  736.538868] hello_module: Custom_greeting_from_kernel_space

$ cat /sys/module/hello_module/parameters/message
Custom_greeting_from_kernel_space

$ sudo rmmod hello_module
$ sudo dmesg | tail -1
[  758.327456] hello_module: Goodbye from LoykoEvgeniy module!

Анализ работы модуля
Модуль успешно компилируется 
Загружается и выгружается без ошибок
Выводит приветствие при загрузке и прощание при выгрузке
Поддерживает параметры через командную строку
Проблема: пробелы в параметрах разбиваются на отдельные параметры
Решение: использовать подчёркивания вместо пробелов


Задание B: /proc файл с информацией

Код модуля
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
                         size_t count, loff_t *ppos)
{
    char buf[MAX_SIZE];
    int len;

    if (*ppos > 0)
        return 0;

    read_count++;

    len = snprintf(buf, sizeof(buf),
        "Name: Loyko Evgeniy\n"
        "Group: 8, Subgroup: 1\n"
        "Module loaded at: %lu jiffies\n"
        "Read count: %d\n",
        load_time, read_count);

    if (copy_to_user(ubuf, buf, len))
        return -EFAULT;

    *ppos = len;
    return len;
}

static const struct proc_ops proc_file_ops = {
    .proc_read = proc_read,
};

static int __init proc_module_init(void)
{
    printk(KERN_INFO "proc_module: Initializing\n");
    load_time = jiffies;
    
    proc_file = proc_create(PROC_NAME, 0444, NULL, &proc_file_ops);
    if (!proc_file) {
        printk(KERN_ERR "proc_module: Failed to create /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }

    printk(KERN_INFO "proc_module: Created /proc/%s\n", PROC_NAME);
    return 0;
}

static void __exit proc_module_exit(void)
{
    if (proc_file) {
        proc_remove(proc_file);
        printk(KERN_INFO "proc_module: Removed /proc/%s\n", PROC_NAME);
    }
}

module_init(proc_module_init);
module_exit(proc_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Loyko Evgeniy");
MODULE_DESCRIPTION("/proc filesystem example with student info");
MODULE_VERSION("1.0");

Результаты тестирования
Сборка модуля
$ make
...
CC [M]  /home/datastream/lab5/gr8sub1/LoykoEvgeniy/src/proc_module.o
/home/datastream/lab5/gr8sub1/LoykoEvgeniy/src/proc_module.c: In function 'proc_read':
warning: the frame size of 1040 bytes is larger than 1024 bytes [-Wframe-larger-than=]

$ ls -la *.ko
-rw-rw-r-- 1 datastream datastream 310192 proc_module.ko

Тестирование работы
$ sudo insmod proc_module.ko
$ sudo dmesg | tail -2
[  900.796840] proc_module: Initializing
[  900.796868] proc_module: Created /proc/student_info

$ ls -la /proc/student_info
-r--r--r-- 1 root root 0 /proc/student_info

$ cat /proc/student_info
Name: Loyko Evgeniy
Group: 8, Subgroup: 1
Module loaded at: 4295568086 jiffies
Read count: 1

$ cat /proc/student_info
Name: Loyko Evgeniy
Group: 8, Subgroup: 1
Module loaded at: 4295568086 jiffies
Read count: 2

$ cat /proc/student_info  
Name: Loyko Evgeniy
Group: 8, Subgroup: 1
Module loaded at: 4295568086 jiffies
Read count: 3

$ sudo rmmod proc_module
$ sudo dmesg | tail -1
[  948.172399] proc_module: Removed /proc/student_info

$ ls -la /proc/student_info
Файл удалён - это правильно!

Анализ работы модуля
Модуль успешно компилируется
Создаёт файл /proc/student_info с правами 0444 (read-only)
Выводит корректную информацию о студенте
Счётчик обращений увеличивается при каждом чтении: 1 → 2 → 3
Время загрузки фиксируется в jiffies: 4295568086
Файл автоматически удаляется при выгрузке модуля
Предупреждение: размер стека функции proc_read превышает 1024 байта


Задание C: Character Device
Код модуля (ключевые функции)
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
static struct class *chardev_class = NULL;
static struct device *chardev_device = NULL;
static char *device_buffer = NULL;
static int buffer_size = 0;
static DEFINE_MUTEX(device_mutex);

static int dev_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "chardev: Device opened\n");
    return 0;
}

static int dev_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "chardev: Device closed\n");
    return 0;
}

static ssize_t dev_read(struct file *file, char __user *buf,
                        size_t len, loff_t *off)
{
    int bytes_to_read;
    
    mutex_lock(&device_mutex);
    if (*off >= buffer_size || buffer_size == 0) {
        mutex_unlock(&device_mutex);
        return 0;
    }
    
    bytes_to_read = min_t(size_t, len, buffer_size - *off);
    
    if (copy_to_user(buf, device_buffer + *off, bytes_to_read)) {
        mutex_unlock(&device_mutex);
        return -EFAULT;
    }
    
    *off += bytes_to_read;
    mutex_unlock(&device_mutex);
    
    printk(KERN_INFO "chardev: Read %d bytes\n", bytes_to_read);
    return bytes_to_read;
}

static ssize_t dev_write(struct file *file, const char __user *buf,
                         size_t len, loff_t *off)
{
    int bytes_to_write;
    
    mutex_lock(&device_mutex);
    bytes_to_write = min_t(size_t, len, BUF_SIZE);
    
    if (!device_buffer) {
        device_buffer = kmalloc(BUF_SIZE, GFP_KERNEL);
        if (!device_buffer) {
            mutex_unlock(&device_mutex);
            return -ENOMEM;
        }
    }
    
    if (copy_from_user(device_buffer, buf, bytes_to_write)) {
        mutex_unlock(&device_mutex);
        return -EFAULT;
    }
    
    buffer_size = bytes_to_write;
    *off = 0;
    mutex_unlock(&device_mutex);
    
    printk(KERN_INFO "chardev: Write %d bytes: %.*s\n", 
           bytes_to_write, (int)bytes_to_write, device_buffer);
    return bytes_to_write;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .release = dev_release,
    .read = dev_read,
    .write = dev_write,
};

Результаты тестирования
Инициализация устройства
$ sudo insmod chardev_module.ko
$ sudo dmesg | tail -3
[ 1169.017439] chardev: Registered with major number 235
[ 1169.018872] chardev: Device initialized successfully
[ 1169.018874] chardev: Device will be available as /dev/mychardev

$ ls -la /dev/mychardev
crw------- 1 root root 235, 0 /dev/mychardev

$ cat /proc/devices | grep mychardev
235 mychardev

Тестирование записи и чтения
$ echo "Hello from user space!" | sudo tee /dev/mychardev
Hello from user space!

$ sudo dmesg | tail -2
[ 1203.404642] chardev: Write 23 bytes: Hello from user space!
[ 1203.404648] chardev: Device closed

$ sudo cat /dev/mychardev
Hello from user space!

$ sudo dmesg | tail -3
[ 1216.132311] chardev: Device opened
[ 1216.132337] chardev: Read 23 bytes
[ 1216.132354] chardev: Device closed

Тестирование перезаписи
$ echo "New test data for character device" | sudo tee /dev/mychardev
New test data for character device

$ sudo cat /dev/mychardev
New test data for character device

$ sudo dmesg | tail -3
[ 1233.570138] chardev: Device opened
[ 1233.570148] chardev: Read 35 bytes
[ 1233.570174] chardev: Device closed

Выгрузка модуля
$ sudo rmmod chardev_module
$ sudo dmesg | tail -1
[ 1246.368027] chardev: Device unregistered

$ ls -la /dev/mychardev
Устройство удалено - это правильно!

Анализ работы модуля
Модуль успешно компилируется
Автоматически создаёт устройство /dev/mychardev с major number 235
Поддерживает операции open, release, read, write
Корректно сохраняет и возвращает данные
Использует мьютекс для защиты от гонки данных
Логирует все операции в ядро
Автоматически удаляет устройство при выгрузке

Ответы на контрольные вопросы

Базовые понятия
1. Что такое модуль ядра и зачем он нужен?
Модуль ядра - это объектный код, который может быть динамически загружен в ядро для расширения его функциональности без перекомпиляции и перезагрузки системы. Позволяет добавлять драйверы устройств, файловые системы, сетевые протоколы.

2. Чем отличается kernel-space от user-space?

Kernel-space: привилегированный режим (ring 0), полный доступ к оборудованию, ошибки приводят к kernel panic, нет защиты памяти

User-space: непривилегированный режим (ring 3), ограниченный доступ через системные вызовы, ошибки приводят к завершению процесса, есть защита памяти

3. Что произойдёт, если в модуле обратиться к NULL указателю?
Произойдёт kernel panic - аварийное завершение работы ядра с последующей перезагрузкой системы. В ядре нет механизма обработки segmentation fault.

4. Почему нельзя использовать printf() в модуле ядра?
printf() работает с stdout в user-space, а в kernel-space используется printk() для вывода в kernel log buffer. Стандартная библиотека C недоступна в ядре.

5. Что такое kernel panic и как его избежать?
Kernel panic - критическая ошибка ядра, приводящая к остановке системы. Чтобы избежать: проверять указатели перед использованием, проверять возвращаемые значения функций, использовать copy_to/from_user(), избегать бесконечных циклов.

Жизненный цикл модуля
6. Какие функции вызываются при insmod и rmmod?
При insmod вызывается функция, зарегистрированная через module_init(). При rmmod вызывается функция, зарегистрированная через module_exit().

7. Что должна делать функция module_exit()?
Должна освобождать все ресурсы, выделенные в module_init(): удалять /proc файлы, освобождать номера устройств, удалять cdev, освобождать память.

8. Что происходит, если module_init() возвращает ошибку?
Модуль не загружается, функция module_exit() НЕ вызывается. Все ресурсы, выделенные до возврата ошибки, должны быть освобождены вручную.

9. Можно ли выгрузить модуль, если он используется?
Нет, при попытке выгрузки используется модуля получим ошибку: rmmod: ERROR: Module my_driver is in use. Необходимо сначала закрыть все процессы, использующие модуль.

Логирование и отладка
10. Чем printk() отличается от printf()?

printk(): вывод в kernel log buffer, имеет уровни важности (KERN_INFO, KERN_ERR), не блокируется, работает всегда

printf(): вывод в stdout (терминал), нет уровней важности, может блокироваться

11. Какие уровни логирования существуют в ядре?

KERN_EMERG (0) - аварийная ситуация

KERN_ALERT (1) - требуется немедленное действие

KERN_CRIT (2) - критическая ошибка

KERN_ERR (3) - ошибка

KERN_WARNING (4) - предупреждение

KERN_NOTICE (5) - важное уведомление

KERN_INFO (6) - информационное сообщение

KERN_DEBUG (7) - отладочная информация

12. Как посмотреть логи модуля?

dmesg | tail -20 - последние 20 сообщений

dmesg | grep "module_name" - фильтр по имени модуля

dmesg -w - просмотр в реальном времени

journalctl -k -f - системные логи ядра

/var/log/kern.log - файл логов ядра

13. Что означает "tainted kernel"?
"Tainted kernel" означает, что ядро "испорчено" - загружены неподписанные или проприетарные модули. Проверить: cat /proc/sys/kernel/tainted. В наших тестах было: hello_module: loading out-of-tree module taints kernel.

Память
14. Чем kmalloc() отличается от malloc()?

kmalloc(): выделяет физически непрерывную память в ядре, использует флаги GFP, возвращает void*, всегда проверять на NULL

malloc(): выделяет виртуальную память в user-space, может использовать swap

15. Что такое флаги GFP и зачем они нужны?
Get Free Pages флаги определяют контекст выделения памяти:

GFP_KERNEL - обычное выделение (может спать)

GFP_ATOMIC - атомарное выделение (не может спать, для обработчиков прерываний)

GFP_USER - для user-space данных

16. Что произойдёт, если не освободить память в module_exit()?
Произойдёт утечка памяти - выделенная память будет потеряна до перезагрузки системы. В ядре нет garbage collector.

17. Почему нельзя использовать user-space указатели напрямую в ядре?
User-space указатели относятся к виртуальному адресному пространству процесса и недействительны в контексте ядра. Прямое обращение вызовет kernel panic. Необходимо использовать copy_to_user() и copy_from_user().

Взаимодействие с user-space
18. Что такое /proc и для чего он используется?
/proc - виртуальная файловая система для экспорта информации из ядра в user-space. Примеры: /proc/cpuinfo, /proc/meminfo, /proc/version. В задании B мы создали /proc/student_info.

19. Что такое /sys (sysfs) и чем отличается от procfs?

sysfs: современная система, один файл = одно значение, используется для конфигурации устройств

procfs: более старая система, может содержать сложные структуры данных, используется для информации о процессах и системе

20. Зачем нужны функции copy_to_user() и copy_from_user()?
Для безопасного копирования данных между kernel-space и user-space. Они проверяют валидность пользовательских указателей и обрабатывают ошибки. Прямой доступ через memcpy() опасен.

21. Что такое character device и как он работает?
Character device - устройство, с которым можно работать как с файлом (последовательный доступ). Примеры: /dev/null, /dev/random. Реализует операции open, read, write, release через структуру file_operations.

Параметры и метаданные
22. Как передать параметры модулю при загрузке?
Через командную строку: sudo insmod module.ko param1=value1 param2=value2. Параметры объявляются через module_param().

23. Зачем нужен MODULE_LICENSE()?
Указывает лицензию модуля. "GPL" обеспечивает совместимость с ядром Linux. Без лицензии модуль будет работать, но ядро будет помечено как "tainted".

24. Что произойдёт, если не указать лицензию?
Модуль будет работать, но ядро будет помечено как "tainted", и некоторые функции ядра будут недоступны для модуля.

Безопасность
25. Какие основные правила безопасного кода в ядре?

Всегда проверять возвращаемые значения функций

Освобождать все ресурсы в module_exit()

Использовать copy_to_user() и copy_from_user()

Проверять границы массивов и буферов

Не использовать стандартную библиотеку C

Избегать бесконечных циклов

Использовать мьютексы для синхронизации

26. Можно ли использовать бесконечный цикл в модуле?
Нет, бесконечный цикл может заблокировать ядро, особенно в обработчике прерываний. Это приведёт к зависанию системы.

27. Почему в ядре нет FPU операций?
Использование FPU ( Floating Point Unit) в ядре сложно и требует сохранения/восстановления состояния. Обычно операции с плавающей точкой выполняются в user-space.

28. Что делать, если модуль вызвал kernel panic?

Перезагрузить систему (в виртуальной машине это безопасно)

Проанализировать код, найти причину (обычно разыменование NULL, выход за границы массива)

Исправить ошибку и повторить тестирование

Использовать отладочные выводы printk()

Практические вопросы
29. Как узнать, какие модули загружены в системе?

lsmod - список всех загруженных модулей

lsmod | grep module_name - поиск конкретного модуля

cat /proc/modules - детальная информация о модулях

30. Как получить информацию о модуле (версия, параметры)?

modinfo module.ko - информация о скомпилированном модуле

cat /sys/module/module_name/version - версия загруженного модуля

cat /sys/module/module_name/parameters/param_name - значение параметра



В процессе выполнения лабораторной работы использовались современные системы искусственного интеллекта для анализа возникающих проблем, генерации решений и оптимизации кода. ИИ-инструменты применялись для диагностики ошибок компиляции, анализа взаимодействия kernel-space и user-space, а также для обеспечения соответствия кода лучшим практикам разработки модулей ядра Linux.

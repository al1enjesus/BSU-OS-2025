# Лабораторная работа 5 — Модули ядра Linux
## Вариант 1

## Тестовая среда:
	Ubuntu 24.04.3 LTS
	Ядро: 6.14.0-36-generic
	gcc 13.3.0

# Цель работы
Изучить:
- построение модулей ядра Linux
- работу с `/proc`
- реализацию character device
- обмен данными между user-space и kernel-space
- механизмы выделения памяти, логирования и жизненного цикла модулей

# Задание A — Hello World модуль
## Код модуля `hello_module.c`
```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/moduleparam.h>

static char *message = "Hello from Matvey Gorbach module!";
module_param(message, charp, 0444);
MODULE_PARM_DESC(message, "Greeting message");

static int __init hello_init(void) {
	printk(KERN_INFO "hello_module: %s\n", message);
	return 0;
}

static void __exit hello_exit(void) {
	printk(KERN_INFO "hello_module: Goodbye from module\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Matvey Gorbach");
MODULE_DESCRIPTION("Simple Hello World kernerl module for lab 5");
MODULE_VERSION("0.1");
```
## Результаты выполнения:
    1. Загрузка
```
sudo insmod hello_module.ko
sudo dmesg | tail -5
[  399.968827] proc_module: loading out-of-tree module taints kernel.
[  399.968832] proc_module: module verification failed: signature and/or required key missing - tainting kernel
[  399.969519] student_info module loaded
[  845.794297] student_info module unloaded
[  868.982708] hello_module: Hello from Matvey Gorbach module!
```
    2. Передача параметра
```
sudo insmod hello_module.ko message='"Custom greeting"'
sudo dmesg | tail -5
[  399.969519] student_info module loaded
[  845.794297] student_info moduvle unloaded
[  868.982708] hello_module: Hello from Matvey Gorbach module!
[ 1059.668695] hello_module: Goodbye from module
[ 1079.929362] hello_module: Custom greeting
```
    3. Выгрузка
```
sudo rmmod hello_module
sudo dmesg | tail -5
[  845.794297] student_info module unloaded
[  868.982708] hello_module: Hello from Matvey Gorbach module!
[ 1059.668695] hello_module: Goodbye from module
[ 1079.929362] hello_module: Custom greeting
[ 1110.917765] hello_module: Goodbye from module
```


# Задание B: /proc файл с информацией

## Код модуля `proc_module.c`
```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>

#define PROC_FILENAME "student_info"

static struct proc_dir_entry *proc_file;
static unsigned long load_time; 
static int read_count = 0; 

static ssize_t student_read(struct file *file, char __user *ubuf,
size_t count, loff_t *ppos)
{
	char buffer[256];
	int len = 0;
	
	if (*ppos > 0)
		return 0;

	read_count++;

	len = snprintf(buffer, sizeof(buffer),
		"Name: Gorbach Matvey\n"
		"Group: 9\n"
		"Module loaded at: %lu jiffies\n"
		"Read count: %d\n",
	load_time, read_count);

	if (copy_to_user(ubuf, buffer, len))
		return -EFAULT;
	
	*ppos = len;
	return len;
}

static const struct proc_ops student_ops = {
	.proc_read = student_read,
};

static int __init proc_module_init(void)
{
	load_time = jiffies;

	proc_file = proc_create(PROC_FILENAME, 0444, NULL, &student_ops);
	if (!proc_file) {
		printk(KERN_ERR "student_info: failed to create proc file\n");
		return -ENOMEM;
	}

	printk(KERN_INFO "student_info module loaded\n");
	return 0;
}

static void __exit proc_module_exit(void)
{
	proc_remove(proc_file);
	printk(KERN_INFO "student_info module unloaded\n");
}

module_init(proc_module_init);
module_exit(proc_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Gorbach Matvey");
MODULE_DESCRIPTION("Procfs student info module");
MODULE_VERSION("0.1");
```

## Результаты выполнения:

```
sudo insmod proc_module.ko  

sudo dmesg | tail -1
[ 3179.456867] student_info module loaded

cat /proc/student_info
Name: Gorbach Matvey
Group: 9
Module loaded at: 4297846708 jiffies
Read count: 1

cat /proc/student_info
Name: Gorbach Matvey
Group: 9
Module loaded at: 4297846708 jiffies
Read count: 2

sudo rmmod proc_module
sudo dmesg | tail -2
[ 3179.456867] student_info module loaded
[ 3333.346509] student_info module unloaded
```


#Задание C: Простой character device

## Код модуля `chardev_module.c`
```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define DEVICE_NAME "mychardev"
#define BUFFER_SIZE 1024

static dev_t dev_num;
static struct cdev my_cdev;

static char *kernel_buffer; 
static int buffer_size = 0;

static int dev_open(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "mychardev: device opened\n");
	return 0;
}

static int dev_release(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "mychardev: device closed\n");
	return 0;
}

static ssize_t dev_read(struct file *file, char __user *buf,
size_t len, loff_t *ppos)
{
	if (*ppos >= buffer_size)
		return 0;

	if (len > buffer_size - *ppos)
		len = buffer_size - *ppos;

	if (copy_to_user(buf, kernel_buffer + *ppos, len))
		return -EFAULT;

	*ppos += len;
	return len;
}

static ssize_t dev_write(struct file *file, const char __user *buf,
size_t len, loff_t *ppos)
{
	if (len > BUFFER_SIZE)
		len = BUFFER_SIZE;

	if (copy_from_user(kernel_buffer, buf, len))
		return -EFAULT;

	buffer_size = len;
	printk(KERN_INFO "mychardev: received %d bytes\n", buffer_size);

	return len;
}

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = dev_open,
	.release = dev_release,
	.read = dev_read,
	.write = dev_write,
};

static int __init chardev_init(void)
{
	if (alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME) < 0) {
		printk(KERN_ERR "mychardev: alloc_chrdev_region failed\n");
		return -1;
	}

	cdev_init(&my_cdev, &fops);

	if (cdev_add(&my_cdev, dev_num, 1) < 0) {
		unregister_chrdev_region(dev_num, 1);
		printk(KERN_ERR "mychardev: cdev_add failed\n");
		return -1;
	}

	kernel_buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
	if (!kernel_buffer) {
		cdev_del(&my_cdev);
		unregister_chrdev_region(dev_num, 1);
		printk(KERN_ERR "mychardev: kmalloc failed\n");
		return -ENOMEM;
	}

	printk(KERN_INFO "mychardev: module loaded. Major=%d Minor=0\n", MAJOR(dev_num));
	return 0;
}

static void __exit chardev_exit(void)
{
	kfree(kernel_buffer);
	cdev_del(&my_cdev);
	unregister_chrdev_region(dev_num, 1);

	printk(KERN_INFO "mychardev: module unloaded\n");
}

module_init(chardev_init);
module_exit(chardev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Gorbach_Matvey");
MODULE_DESCRIPTION("Simple character device module");
MODULE_VERSION("0.1");

```
## Проверка работы:
```c
sudo insmod chardev_module.ko

sudo dmesg | tail -10
[ 2026.500909] student_info module loaded
[ 2295.074949] student_info module unloaded
[ 2381.939429] student_info module loaded
[ 2711.346931] student_info module unloaded
[ 2832.607028] audit: type=1400 audit(1763974844.581:157): apparmor="DENIED" operation="open" class="file" profile="snap.firmware-updater.firmware-notifier" name="/proc/sys/vm/max_map_count" pid=8072 comm="firmware-notifi" requested_mask="r" denied_mask="r" fsuid=1000 ouid=0
[ 3065.827746] student_info module loaded
[ 3167.514924] student_info module unloaded
[ 3179.456867] student_info module loaded
[ 3333.346509] student_info module unloaded
[ 3841.292725] mychardev: module loaded. Major=240 Minor=0

sudo mknod /dev/mychardev c 240 0

sudo chmod 666 /dev/mychardev

echo "Hello World!" > /dev/mychardev

cat /dev/mychardev
Hello World!

sudo dmesg | tail -10
[ 3065.827746] student_info module loaded
[ 3167.514924] student_info module unloaded
[ 3179.456867] student_info module loaded
[ 3333.346509] student_info module unloaded
[ 3841.292725] mychardev: module loaded. Major=240 Minor=0
[ 4105.704925] mychardev: device opened
[ 4105.704945] mychardev: received 13 bytes
[ 4105.704949] mychardev: device closed
[ 4116.689502] mychardev: device opened
[ 4116.689530] mychardev: device closed
```

Ответы на теоретические вопросы
### 1. Что такое модуль ядра и зачем он нужен?
Модуль ядра — это загружаемый во время работы системы компонент ядра Linux.
Позволяет добавлять драйверы и функциональность без пересборки и перезагрузки ядра.

### 2. Чем отличается kernel-space от user-space?
- **kernel-space** — привилегированный режим, полный доступ к железу.
- **user-space** — ограниченный режим, защита памяти, системные вызовы для работы с ядром.
### 3. Что произойдёт, если в модуле обратиться к NULL указателю?
Возникнет **kernel panic** или oops, т.к. ядро не защищено от сегфолтов, как user-space.

### 4. Почему нельзя использовать printf() в модуле ядра?
Потому что printf принадлежит user-space.
В ядре используется **printk()**, который пишет в кольцевой буфер ядра.

### 5. Что такое kernel panic и как его избежать?
Состояние, когда ядро не может продолжать работу.
Избегать:
- проверять указатели
- не выходить за границы массива
- освобождать память
- корректно обрабатывать ошибки

### 6. Какие функции вызываются при insmod и rmmod?
- `module_init()` → вызывается при загрузке (`insmod`)
- `module_exit()` → вызывается при выгрузке (`rmmod`)

### 7. Что должна делать функция module_exit()?
Освободить ресурсы:
- память (kfree)
- unregister_chrdev_region
- cdev_del
- proc_remove
- остановить таймеры, очистить sysfs и т.д.

### 8. Что происходит, если module_init() возвращает ошибку?
Модуль **не будет загружен**, а ядро автоматически вызовет очистку ресурсов.

### 9. Можно ли выгрузить модуль, если он используется?
Нет, если:
- открыт файл устройства
- модуль используется другими модулями
- у него ненулевой refcount

### 10. Чем printk() отличается от printf()?
- работает в ядре
- использует уровни логирования (`KERN_INFO`, `KERN_ERR`)
- пишет в `/dev/kmsg` и вывод `dmesg`

### 11. Какие уровни логирования существуют в ядре?
Основные:
- KERN_EMERG
- KERN_ALERT
- KERN_CRIT
- KERN_ERR
- KERN_WARNING
- KERN_NOTICE
- KERN_INFO
- KERN_DEBUG

### 12. Как посмотреть логи модуля?
Команды:
dmesg
dmesg | grep mymodule
dmesg -w

### 13. Что означает “tainted kernel”?
Ядро "запятнано":
- загружены проприетарные модули
- загружены неподписанные модули
- произошёл критический сбой
Это предупреждение разработчикам.

### 14. Чем kmalloc() отличается от malloc()?
- `kmalloc()` выделяет память в ядре (физически contiguous)
- `malloc()` — в user-space, через виртуальную память

### 15. Что такое флаги GFP и зачем они нужны?
Флаги управления выделением памяти в ядре.
Пример: `GFP_KERNEL` — обычное выделение, можно ждать; `GFP_ATOMIC` — без блокировок.

### 16. Что произойдёт, если не освободить память в module_exit()?
Будет утечка памяти в ядре.
Kernel-space не подчищается автоматически → деградация системы.

### 17. Почему нельзя использовать user-space указатели напрямую в ядре?
Потому что они относятся к адресному пространству процесса.
Правильно — использовать copy_to_user(), copy_from_user().

### 18. Что такое /proc и для чего он используется?
Виртуальная файловая система для предоставления информации ядра в user-space.
Используется для:
- статистики
- параметров ядра
- диагностических интерфейсов модулей

### 19. Что такое /sys (sysfs) и чем отличается от procfs?
- `/proc` — про процессы и параметры ядра
- `/sys` — про устройства, драйверы, ядро как объектную модель

### 20. Зачем нужны copy_to_user() и copy_from_user()?
Они проверяют корректность адресов и обеспечивают безопасный обмен данными:
- kernel → user: `copy_to_user()`
- user → kernel: `copy_from_user()`

### 21. Что такое character device и как он работает?
Тип устройства, обеспечивающий последовательный доступ к данным.
Работает через:
- alloc_chrdev_region
- cdev_init / cdev_add
- file_operations: open, read, write

### 22. Как передать параметры модулю при загрузке?
Через `module_param()`:
sudo insmod module.ko param=value

### 23. Зачем нужен MODULE_LICENSE()?
Сообщает ядру, является ли модуль GPL-совместимым.

### 24. Что произойдёт, если не указать лицензию?
Ядро пометит модуль как **proprietary**, включит tainted kernel,
часть функциональности может быть недоступна.

### 25. Какие основные правила безопасного кода в ядре?
- проверять указатели
- обрабатывать ошибки
- освобождать память
- избегать гонок
- не блокировать индексы массивов
- не использовать бесконечные циклы

### 26. Можно ли использовать бесконечный цикл в модуле?
Нет — ядро зависнет.
Разрешено только если есть `schedule()` или таймеры.

### 27. Почему в ядре нет FPU операций?
FPU регистры принадлежат процессу.
Использование FPU в ядре:
- медленно
- требует сохранения контекста
- опасно
Поэтому запрещено.

### 28. Что делать, если модуль вызвал kernel panic?
1. Перезагрузить систему
2. Проверить код
3. Удалить проблемный модуль
4. Смотреть `/var/crash` или журнал `dmesg`

### 29. Как узнать, какие модули загружены в системе?
lsmod
cat /proc/modules

### 30. Как получить информацию о модуле (версия, параметры)?
modinfo module.ko


## Дополнительно
1.	В данной лабораторной использован простой вывод через proc_ops, поскольку
объём данных небольшой. Однако для больших объёмов и потоковой выдачи
целесообразно использовать интерфейс seq_file, который упрощает
формирование длинных выводов и оптимизирует работу с буферами.

2.  В kernel-space нет защиты памяти между процессами, все модули работают в одном адресном пространстве.

3.  Это вызовет kernel oops, который может привести к панике системы если произошло в атомарном контексте.

4. User-space указатели работают в виртуальном адресном пространстве процесса, которое недоступно ядру напрямую.

5.  Я использовал `chmod 666` для упрощения тестирования. В production следует использовать более строгие права, например `chmod 600` и управлять доступом через группы.

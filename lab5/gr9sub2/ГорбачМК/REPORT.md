# Лабораторная работа 5 — Модули ядра Linux
## Вариант 1

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

## Ответы на теоретические вопросы
### 1. Модуль ядра — загружаемый компонент ядра. Нужен для расширения функциональности без пересборки ядра.
### 2. Kernel-space — привилегированный режим; user-space — ограниченный.
### 3. Обращение к NULL в модуле вызывает kernel panic.
### 4. printf нельзя использовать, потому что работает только в user-space. В ядре — printk().
### 5. Kernel panic — критический сбой ядра. Избежать: проверка указателей, границ, обработка ошибок.
### 6. insmod вызывает module_init(), rmmod — module_exit().
### 7. module_exit() освобождает ресурсы модуля.
### 8. Если module_init() возвращает ошибку — модуль не загрузится, ресурсы очистятся.
### 9. Нельзя выгрузить используемый модуль (refcount > 0).
### 10. printk() пишет в системный лог, а printf — в терминал.
### 11. Уровни логирования: EMERG, ALERT, CRIT, ERR, WARNING, NOTICE, INFO, DEBUG.
### 12. Логи смотрят через dmesg.
### 13. Tainted kernel — ядро "запятнано" неподписанными модулями или ошибками.
### 14. kmalloc выделяет память в kernel-space, malloc — в user-space.
### 15. GFP — флаги выделения памяти. GFP_KERNEL, GFP_ATOMIC и др.
### 16. Если не освободить память — утечка в ядре.
### 17. User-space указатели нельзя использовать напрямую — только через copy_to_user()/copy_from_user().
### 18. /proc — виртуальная ФС для состояния ядра.
### 19. /sys — объектная модель устройств. Отличается от procfs структурой и назначением.
### 20. copy_to_user(), copy_from_user() обеспечивают безопасный обмен данными.
### 21. Character device — устройство с последовательным доступом (read/write).
### 22. Параметры передаются через module_param().
### 23. MODULE_LICENSE сообщает тип лицензии (GPL).
### 24. Если не указать — модуль будет "proprietary", ядро станет tainted.
### 25. Правила: проверка указателей, обработка ошибок, отсутствие гонок, освобождение ресурсов.
### 26. Бесконечные циклы нельзя — ядро зависнет.
### 27. В ядре нет FPU, т.к. это дорого и опасно.
### 28. При panic: перезагрузка, анализ dmesg, исправление кода.
### 29. Список модулей — lsmod, /proc/modules.
### 30. Информация о модуле — modinfo module.ko.

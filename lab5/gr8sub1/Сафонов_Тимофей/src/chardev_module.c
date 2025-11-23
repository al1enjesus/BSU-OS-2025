/*
 * chardev_module.c - Простой character device
 *
 * Создаёт устройство /dev/mychardev, которое:
 * - Принимает данные при записи (до 1024 байт)
 * - Возвращает сохранённые данные при чтении
 *
 * Компиляция: make
 * Использование:
 *   sudo insmod chardev_module.ko
 *   sudo mknod /dev/mychardev c <MAJOR> 0
 *   echo "Hello" > /dev/mychardev
 *   cat /dev/mychardev
 *   sudo rmmod chardev_module
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "mychardev"
#define BUF_SIZE 1024

static dev_t dev_num;
static struct cdev my_cdev;
static char device_buffer[BUF_SIZE];
static int buffer_size = 0;

static int dev_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "chardev: Device opened\n");
    return 0;  // 0 = успех
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

    if (*off < 0)
        return -EINVAL;

    if(*off >= buffer_size){
        return 0;
    }
    bytes_to_read = min_t(size_t, len, buffer_size - *off);
    if (copy_to_user(buf, device_buffer + *off, bytes_to_read))
        return -EFAULT;
    *off += bytes_to_read;
    printk(KERN_INFO "chardev: Read request, Read %d bytes from offset %lld \n",bytes_to_read, *off - bytes_to_read);
    return bytes_to_read;
}

static ssize_t dev_write(struct file *file, const char __user *buf,
                         size_t len, loff_t *off)
{
    int bytes_to_write;
    bytes_to_write = min_t(size_t, len, BUF_SIZE);

    memset(device_buffer, 0, BUF_SIZE);

    if (copy_from_user(device_buffer, buf, bytes_to_write))
        return -EFAULT;

    buffer_size = bytes_to_write;

    printk(KERN_INFO "chardev: Write request, Write %d bytes\n",bytes_to_write);
    return bytes_to_write; 
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
    int ret;

    printk(KERN_INFO "chardev: Initializing\n");

    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ERR "chardev: Failed to allocate major number\n");
        return ret;
    }
    
    printk(KERN_INFO "chardev: Registered with major number %d\n", MAJOR(dev_num));

    cdev_init(&my_cdev, &fops);
    my_cdev.owner = THIS_MODULE;

    ret = cdev_add(&my_cdev, dev_num, 1);
    if (ret < 0) {
        unregister_chrdev_region(dev_num, 1);
        printk(KERN_ERR "chardev: Failed to add cdev\n");
        return ret;
    }

    printk(KERN_INFO "chardev: Device registered successfully\n");
    printk(KERN_INFO "chardev: Create device with: mknod /dev/%s c %d 0\n", 
           DEVICE_NAME, MAJOR(dev_num));

    return 0;
}

static void __exit chardev_exit(void)
{
    cdev_del(&my_cdev);

    unregister_chrdev_region(dev_num, 1);

    printk(KERN_INFO "chardev: Device unregistered successfully\n");
}

module_init(chardev_init);
module_exit(chardev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SafonovTV");
MODULE_DESCRIPTION("Simple character device driver");
MODULE_VERSION("1.0");

/*
 * ЗАДАНИЯ для студента:
 *
 * 1. Реализуйте все TODO в коде выше
 *
 * 2. В dev_read():
 *    - Проверьте границы (не читаем за пределы буфера)
 *    - Используйте copy_to_user для копирования данных
 *    - Правильно обрабатывайте *off (позицию чтения)
 *
 * 3. В dev_write():
 *    - Ограничьте запись размером BUF_SIZE
 *    - Используйте copy_from_user для копирования данных
 *    - Сохраните реальный размер данных
 *
 * 4. В chardev_init():
 *    - Выделите major/minor с помощью alloc_chrdev_region
 *    - Инициализируйте cdev
 *    - Добавьте cdev в систему
 *    - Обрабатывайте ошибки (если что-то пошло не так, откатите действия)
 *
 * 5. В chardev_exit():
 *    - Удалите cdev
 *    - Освободите номера устройств
 *
 * 6. Протестируйте:
 *    $ make
 *    $ sudo insmod chardev_module.ko
 *    $ dmesg | tail -5
 *    # Найдите MAJOR number в выводе
 *    $ sudo mknod /dev/mychardev c <MAJOR> 0
 *    $ sudo chmod 666 /dev/mychardev
 *    $ echo "Hello, kernel!" > /dev/mychardev
 *    $ cat /dev/mychardev
 *    $ dmesg | tail -10
 *    $ sudo rm /dev/mychardev
 *    $ sudo rmmod chardev_module
 *
 * ОЖИДАЕМЫЙ РЕЗУЛЬТАТ:
 *
 * $ echo "Test" > /dev/mychardev
 * $ cat /dev/mychardev
 * Test
 *
 * $ echo "New data" > /dev/mychardev
 * $ cat /dev/mychardev
 * New data
 *
 * $ dmesg | tail -10
 * [12345.678] chardev: Device opened
 * [12345.679] chardev: Write request
 * [12345.680] chardev: Device closed
 * [12346.789] chardev: Device opened
 * [12346.790] chardev: Read request
 * [12346.791] chardev: Device closed
 *
 * ЧАСТЫЕ ОШИБКИ:
 *
 * 1. "No such device" при cat:
 *    - Проверьте, что mknod выполнен с правильным MAJOR
 *    - Проверьте ls -l /dev/mychardev
 *
 * 2. "Permission denied":
 *    - Выполните: sudo chmod 666 /dev/mychardev
 *
 * 3. Пустой вывод при cat:
 *    - Проверьте, что dev_write правильно сохранил данные
 *    - Проверьте buffer_size
 *
 * 4. Kernel panic при чтении/записи:
 *    - Проверьте границы массивов
 *    - Убедитесь, что используете copy_to_user/copy_from_user
 *
 * ДОПОЛНИТЕЛЬНО (*):
 *
 * - Добавьте поддержку lseek (изменение позиции чтения)
 * - Добавьте ioctl для управления устройством
 * - Сделайте буфер динамическим (kmalloc вместо статического массива)
 * - Добавьте mutex для защиты от одновременного доступа
 */

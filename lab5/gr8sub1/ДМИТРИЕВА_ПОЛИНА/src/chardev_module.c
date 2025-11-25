#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/string.h>

#define DEVICE_NAME "mychardev"
#define BUF_SIZE 1024

static dev_t dev_num;                  // major + minor
static struct cdev my_cdev;            // структура символьного устройства
static char device_buffer[BUF_SIZE];   // буфер данных
static int buffer_size = 0;            // сколько байт реально записано


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

    if (*off >= buffer_size)
        return 0;

    if (buffer_size - *off < 0)
        return 0;

    bytes_to_read = min((size_t)len,
                        (size_t)(buffer_size - *off));

    if (bytes_to_read <= 0)
        return 0;

    if (copy_to_user(buf, device_buffer + *off, bytes_to_read))
        return -EFAULT;

    *off += bytes_to_read;

    printk(KERN_INFO "chardev: Read %d bytes, new offset=%lld\n",
           bytes_to_read, *off);

    return bytes_to_read;
}


static ssize_t dev_write(struct file *file, const char __user *buf,
                         size_t len, loff_t *off)
{
    int bytes_to_write;

    if (len == 0)
        return 0;

    bytes_to_write = min(len, (size_t)BUF_SIZE);

    memset(device_buffer, 0, BUF_SIZE);

    if (copy_from_user(device_buffer, buf, bytes_to_write))
        return -EFAULT;

    buffer_size = bytes_to_write;
    *off = 0;

    printk(KERN_INFO "chardev: Write %d bytes (buffer_size=%d)\n",
           bytes_to_write, buffer_size);

    return bytes_to_write;
}

// Таблица операций устройства
static struct file_operations fops = {
    .owner   = THIS_MODULE,
    .open    = dev_open,
    .release = dev_release,
    .read    = dev_read,
    .write   = dev_write,
};

//Инициализация модуля

static int __init chardev_init(void)
{
    int ret;

    printk(KERN_INFO "chardev: Initializing\n");

    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ERR "chardev: Failed to allocate major number\n");
        return ret;
    }

    printk(KERN_INFO "chardev: Registered with major number %d\n",
           MAJOR(dev_num));

    cdev_init(&my_cdev, &fops);
    my_cdev.owner = THIS_MODULE;

    ret = cdev_add(&my_cdev, dev_num, 1);
    if (ret < 0) {
        printk(KERN_ERR "chardev: Failed to add cdev\n");
        unregister_chrdev_region(dev_num, 1);
        return ret;
    }

    printk(KERN_INFO "chardev: Device registered successfully\n");
    printk(KERN_INFO "chardev: Create device with: "
                     "mknod /dev/%s c %d 0\n",
           DEVICE_NAME, MAJOR(dev_num));

    return 0;
}

//Выгрузка модуля
static void __exit chardev_exit(void)
{
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev_num, 1);

    printk(KERN_INFO "chardev: Device unregistered\n");
}

module_init(chardev_init);
module_exit(chardev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dmitrieva Polina");
MODULE_DESCRIPTION("Simple character device driver");
MODULE_VERSION("1.0");

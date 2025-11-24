#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define DEVICE_NAME "mychardev"
#define BUF_SIZE 1024
#define CLASS_NAME "chardev_class"

static dev_t dev_num;
static struct cdev my_cdev;
static struct class *chardev_class = NULL;
static struct device *chardev_device = NULL;

static char *device_buffer = NULL;
static int buffer_size = 0;
static DEFINE_MUTEX(device_mutex);

// Открытие
static int dev_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "chardev: Device opened\n");
    return 0;
}

// Закрытие
static int dev_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "chardev: Device closed\n");
    return 0;
}

// Чтение
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

// Запись
static ssize_t dev_write(struct file *file, const char __user *buf,
                         size_t len, loff_t *off)
{
    int bytes_to_write;

    mutex_lock(&device_mutex);

    bytes_to_write = min_t(size_t, len, BUF_SIZE);

   
    if (device_buffer) {
        kfree(device_buffer);
        device_buffer = NULL;
    }


    device_buffer = kmalloc(bytes_to_write, GFP_KERNEL);
    if (!device_buffer) {
        mutex_unlock(&device_mutex);
        return -ENOMEM;
    }

    if (copy_from_user(device_buffer, buf, bytes_to_write)) {
        mutex_unlock(&device_mutex);
        return -EFAULT;
    }

    buffer_size = bytes_to_write;
    *off = 0;

    mutex_unlock(&device_mutex);

    printk(KERN_INFO "chardev: Write %d bytes\n", bytes_to_write);
    return bytes_to_write;
}

// Таблица операций
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .release = dev_release,
    .read = dev_read,
    .write = dev_write,
};

// Инициализация
static int __init chardev_init(void)
{
    int ret;

    printk(KERN_INFO "chardev: Initializing\n");

    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ERR "chardev: Cannot allocate device number\n");
        return ret;
    }

    cdev_init(&my_cdev, &fops);
    my_cdev.owner = THIS_MODULE;

    ret = cdev_add(&my_cdev, dev_num, 1);
    if (ret < 0) {
        unregister_chrdev_region(dev_num, 1);
        printk(KERN_ERR "chardev: Failed to add cdev\n");
        return ret;
    }

    chardev_class = class_create(CLASS_NAME);
    if (IS_ERR(chardev_class)) {
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev_num, 1);
        printk(KERN_ERR "chardev: Failed to create class\n");
        return PTR_ERR(chardev_class);
    }

    chardev_device = device_create(chardev_class, NULL, dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(chardev_device)) {
        class_destroy(chardev_class);
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev_num, 1);
        printk(KERN_ERR "chardev: Failed to create device\n");
        return PTR_ERR(chardev_device);
    }

    buffer_size = 0;

    printk(KERN_INFO "chardev: Device initialized\n");
    return 0;
}
static void __exit chardev_exit(void)
{
    if (device_buffer)
        kfree(device_buffer);

    device_destroy(chardev_class, dev_num);
    class_destroy(chardev_class);

    cdev_del(&my_cdev);
    unregister_chrdev_region(dev_num, 1);

    printk(KERN_INFO "chardev: Device unregistered\n");
}

module_init(chardev_init);
module_exit(chardev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pauliuchuk Julia");  
MODULE_DESCRIPTION("Safe character device driver");
MODULE_VERSION("1.1");


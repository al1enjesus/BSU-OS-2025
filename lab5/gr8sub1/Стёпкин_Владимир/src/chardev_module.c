#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "mychardev"
#define BUF_SIZE 1024

static char kernel_buffer[BUF_SIZE];
static size_t buffer_size = 0;

static dev_t dev_num;
static struct cdev my_cdev;

/* Функция открытия устройства */
static int chardev_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "mychardev: device opened\n");
    return 0;
}

/* Функция закрытия устройства */
static int chardev_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "mychardev: device closed\n");
    return 0;
}

/* Функция чтения */
static ssize_t chardev_read(struct file *file, char __user *user_buf, size_t count, loff_t *offset)
{
    size_t to_copy = buffer_size - *offset;
    if (to_copy > count)
        to_copy = count;

    if (to_copy == 0)
        return 0;

    if (copy_to_user(user_buf, kernel_buffer + *offset, to_copy))
        return -EFAULT;

    *offset += to_copy;
    return to_copy;
}

/* Функция записи */
static ssize_t chardev_write(struct file *file, const char __user *user_buf, size_t count, loff_t *offset)
{
    size_t to_copy = count;

    if (to_copy > BUF_SIZE)
        to_copy = BUF_SIZE;

    if (copy_from_user(kernel_buffer, user_buf, to_copy))
        return -EFAULT;

    buffer_size = to_copy;
    return to_copy;
}

/* Операции с устройством */
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = chardev_open,
    .release = chardev_release,
    .read = chardev_read,
    .write = chardev_write,
};

/* Инициализация модуля */
static int __init chardev_init(void)
{
    int ret;

    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ERR "mychardev: failed to allocate major number\n");
        return ret;
    }

    cdev_init(&my_cdev, &fops);
    my_cdev.owner = THIS_MODULE;

    ret = cdev_add(&my_cdev, dev_num, 1);
    if (ret < 0) {
        printk(KERN_ERR "mychardev: failed to add cdev\n");
        unregister_chrdev_region(dev_num, 1);
        return ret;
    }

    printk(KERN_INFO "mychardev: module loaded, major=%d\n", MAJOR(dev_num));
    return 0;
}

/* Выгрузка модуля */
static void __exit chardev_exit(void)
{
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev_num, 1);
    printk(KERN_INFO "mychardev: module unloaded\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vladimir Stepkin");
MODULE_DESCRIPTION("Simple character device /dev/mychardev");

module_init(chardev_init);
module_exit(chardev_exit);

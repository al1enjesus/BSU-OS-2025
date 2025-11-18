#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ШеметАА");
MODULE_DESCRIPTION("Simple character device with 1024 byte buffer");
MODULE_VERSION("1.0");

#define DEVICE_NAME "mychardev"
#define BUFFER_SIZE 1024

static dev_t dev_num;
static struct cdev my_cdev;
static char *buffer;
static atomic_t open_count = ATOMIC_INIT(0);

static int dev_open(struct inode *inode, struct file *file)
{
    atomic_inc(&open_count);
    printk(KERN_INFO "mychardev: Device opened, open count: %d\n", atomic_read(&open_count));
    return 0;
}

static int dev_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "mychardev: Device closed, open count: %d\n", atomic_read(&open_count));
    atomic_dec(&open_count);
    return 0;
}

static ssize_t dev_read(struct file *file, char __user *ubuf, size_t len, loff_t *off)
{
    if (*off > 0) {
        return 0;  // Чтение завершено
    }

    if (len > BUFFER_SIZE) {
        len = BUFFER_SIZE;
    }

    if (copy_to_user(ubuf, buffer, len)) {
        printk(KERN_ERR "mychardev: copy_to_user failed\n");
        return -EFAULT;
    }

    *off = len;
    printk(KERN_INFO "mychardev: Read %zu bytes\n", len);
    return len;
}

static ssize_t dev_write(struct file *file, const char __user *ubuf, size_t len, loff_t *off)
{
    if (len > BUFFER_SIZE) {
        len = BUFFER_SIZE;
    }

    if (copy_from_user(buffer, ubuf, len)) {
        printk(KERN_ERR "mychardev: copy_from_user failed\n");
        return -EFAULT;
    }

    printk(KERN_INFO "mychardev: Written %zu bytes\n", len);
    return len;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .release = dev_release,
    .read = dev_read,
    .write = dev_write,
};

static int __init char_init(void)
{
    int ret;

    buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
    if (!buffer) {
        printk(KERN_ERR "mychardev: kmalloc failed\n");
        return -ENOMEM;
    }

    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        kfree(buffer);
        printk(KERN_ERR "mychardev: alloc_chrdev_region failed\n");
        return ret;
    }

    cdev_init(&my_cdev, &fops);
    ret = cdev_add(&my_cdev, dev_num, 1);
    if (ret < 0) {
        unregister_chrdev_region(dev_num, 1);
        kfree(buffer);
        printk(KERN_ERR "mychardev: cdev_add failed\n");
        return ret;
    }

    printk(KERN_INFO "mychardev: Registered with major %d\n", MAJOR(dev_num));
    return 0;
}

static void __exit char_exit(void)
{
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev_num, 1);
    kfree(buffer);
    printk(KERN_INFO "mychardev: Unregistered\n");
}

module_init(char_init);
module_exit(char_exit);

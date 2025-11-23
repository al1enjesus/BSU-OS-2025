#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/string.h>

#define DEVICE_NAME "mychardev"
#define BUF_SIZE 1024

static dev_t dev_num;
static struct cdev my_cdev;
static char device_buffer[BUF_SIZE];
static int buffer_size = 0;

static int dev_open(struct inode *inode, struct file *file)
{
    if (!inode || !file) {
        printk(KERN_ERR "chardev: Invalid inode or file pointer in open\n");
        return -EINVAL;
    }

    printk(KERN_INFO "chardev: Device opened\n");
    return 0;
}

static int dev_release(struct inode *inode, struct file *file)
{
    if (!inode || !file) {
        printk(KERN_ERR "chardev: Invalid inode or file pointer in release\n");
        return -EINVAL;
    }

    printk(KERN_INFO "chardev: Device closed\n");
    return 0;
}

static ssize_t dev_read(struct file *file, char __user *buf, size_t len, loff_t *off)
{
    int bytes_to_read;

    if (!buf || !file || !off) {
        printk(KERN_ERR "chardev: Invalid arguments in read\n");
        return -EINVAL;
    }

    if (*off >= buffer_size)
        return 0;

    bytes_to_read = min_t(int, (int)len, buffer_size - (int)(*off));
    if (bytes_to_read <= 0)
        return 0;

    if (copy_to_user(buf, device_buffer + *off, bytes_to_read)) {
        printk(KERN_ERR "chardev: copy_to_user failed in read\n");
        return -EFAULT;
    }

    *off += bytes_to_read;
    printk(KERN_INFO "chardev: Read %d bytes (offset now %lld)\n", bytes_to_read, *off);
    return bytes_to_read;
}

static ssize_t dev_write(struct file *file, const char __user *buf, size_t len, loff_t *off)
{
    int bytes_to_write;

    if (!buf || !file || !off) {
        printk(KERN_ERR "chardev: Invalid arguments in write\n");
        return -EINVAL;
    }

    bytes_to_write = min_t(int, (int)len, BUF_SIZE);
    if (bytes_to_write <= 0) {
        printk(KERN_WARNING "chardev: Nothing to write\n");
        return 0;
    }

    memset(device_buffer, 0, BUF_SIZE);

    if (copy_from_user(device_buffer, buf, bytes_to_write)) {
        printk(KERN_ERR "chardev: copy_from_user failed in write\n");
        return -EFAULT;
    }

    buffer_size = bytes_to_write;
    *off = 0;

    if ((int)len > BUF_SIZE) {
        printk(KERN_WARNING "chardev: input truncated from %zu to %d bytes\n", len, bytes_to_write);
    }

    printk(KERN_INFO "chardev: Written %d bytes\n", bytes_to_write);
    return bytes_to_write;
}

static const struct file_operations fops = {
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
    memset(device_buffer, 0, BUF_SIZE);
    buffer_size = 0;

    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ERR "chardev: Failed to allocate chrdev region (ret=%d)\n", ret);
        return ret;
    }

    cdev_init(&my_cdev, &fops);
    my_cdev.owner = THIS_MODULE;

    ret = cdev_add(&my_cdev, dev_num, 1);
    if (ret < 0) {
        printk(KERN_ERR "chardev: Failed to add cdev (ret=%d)\n", ret);
        unregister_chrdev_region(dev_num, 1);
        return ret;
    }

    printk(KERN_INFO "chardev: Registered with major %d minor %d\n", MAJOR(dev_num), MINOR(dev_num));
    printk(KERN_INFO "chardev: Registered device '%s' with major %d minor %d\n",
           DEVICE_NAME, MAJOR(dev_num), MINOR(dev_num));

    return 0;
}

static void __exit chardev_exit(void)
{
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev_num, 1);
    printk(KERN_INFO "chardev: Device unregistered\n");
}

module_init(chardev_init);
module_exit(chardev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Malets Julia <jullia.minsk@gmale.com>");
MODULE_DESCRIPTION("Character device with buffer and error handling");
MODULE_VERSION("1.0");

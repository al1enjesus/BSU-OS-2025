#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>

#define DEVICE_NAME "mariya_dev"
#define BUF_SIZE 128

static dev_t dev_num;
static struct cdev mariya_cdev;
static char device_buffer[BUF_SIZE] = "Hello from char device!";

static ssize_t device_read(struct file *file, char __user *ubuf,
                           size_t count, loff_t *ppos)
{
    int len = strlen(device_buffer);
    printk(KERN_INFO "char_device: read %d bytes\n", len);
    return simple_read_from_buffer(ubuf, count, ppos, device_buffer, len);
}

static ssize_t device_write(struct file *file, const char __user *ubuf,
                            size_t count, loff_t *ppos)
{
    if (count >= BUF_SIZE)
        return -EINVAL;

    if (copy_from_user(device_buffer, ubuf, count))
        return -EFAULT;

    device_buffer[count] = '\0';

    printk(KERN_INFO "char_device: new value = %s\n", device_buffer);
    return count;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = device_read,
    .write = device_write,
};

static int __init device_init(void)
{
    // 1) выделяем major/minor номер
    if (alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME) < 0) {
        printk(KERN_ERR "char_device: failed to allocate region\n");
        return -1;
    }

    // 2) создаём cdev
    cdev_init(&mariya_cdev, &fops);

    if (cdev_add(&mariya_cdev, dev_num, 1) < 0) {
        unregister_chrdev_region(dev_num, 1);
        printk(KERN_ERR "char_device: unable to add cdev\n");
        return -1;
    }

    printk(KERN_INFO "char_device: loaded, major=%d minor=%d\n",
           MAJOR(dev_num), MINOR(dev_num));
    return 0;
}

static void __exit device_exit(void)
{
    cdev_del(&mariya_cdev);
    unregister_chrdev_region(dev_num, 1);
    printk(KERN_INFO "char_device: unloaded\n");
}

module_init(device_init);
module_exit(device_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mariia");
MODULE_DESCRIPTION("Simple char device");

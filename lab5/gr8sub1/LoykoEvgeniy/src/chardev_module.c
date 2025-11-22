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
    
    if (!buf)
        return -EINVAL;
    
    mutex_lock(&device_mutex);
    
    if (*off >= buffer_size || buffer_size == 0) {
        mutex_unlock(&device_mutex);
        return 0;
    }
    
    bytes_to_read = min_t(size_t, len, buffer_size - *off);
    
    // Проверка границ
    if (*off + bytes_to_read > BUF_SIZE) {
        mutex_unlock(&device_mutex);
        return -EINVAL;
    }
    
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
    
    if (!buf)
        return -EINVAL;
    
    mutex_lock(&device_mutex);
    
    // Проверяем что буфер существует (выделен в init)
    if (!device_buffer) {
        mutex_unlock(&device_mutex);
        return -ENOMEM;
    }
    
    // Безопасное вычисление размера с проверкой границ
    if (len > BUF_SIZE) {
        bytes_to_write = BUF_SIZE;
        printk(KERN_WARNING "chardev: Truncating write from %zu to %d bytes\n", 
               len, BUF_SIZE);
    } else {
        bytes_to_write = len;
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
        printk(KERN_ERR "chardev: Failed to allocate device numbers\n");
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
    
    chardev_class = class_create(DEVICE_NAME);
    if (IS_ERR(chardev_class)) {
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev_num, 1);
        printk(KERN_ERR "chardev: Failed to create device class\n");
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
    
    // Выделяем буфер только один раз в init
    device_buffer = kmalloc(BUF_SIZE, GFP_KERNEL);
    if (!device_buffer) {
        device_destroy(chardev_class, dev_num);
        class_destroy(chardev_class);
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev_num, 1);
        printk(KERN_ERR "chardev: Failed to allocate buffer\n");
        return -ENOMEM;
    }
    
    buffer_size = 0;
    
    printk(KERN_INFO "chardev: Device initialized successfully\n");
    return 0;
}

static void __exit chardev_exit(void)
{
    if (chardev_device)
        device_destroy(chardev_class, dev_num);
    
    if (chardev_class)
        class_destroy(chardev_class);
    
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev_num, 1);
    
    if (device_buffer)
        kfree(device_buffer);
    
    printk(KERN_INFO "chardev: Device unregistered\n");
}

module_init(chardev_init);
module_exit(chardev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Loyko Evgeniy");
MODULE_DESCRIPTION("Simple character device driver");
MODULE_VERSION("1.0");

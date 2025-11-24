#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Bogdevich Artem");
MODULE_DESCRIPTION("Simple character device");
MODULE_VERSION("1.0");

#define DEVICE_NAME "mychardev"
#define BUFFER_SIZE 1024

static dev_t dev_num;
static struct cdev my_cdev;
static char *device_buffer;
static int buffer_used = 0;

static int device_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "=== CHAR_DEVICE: Device opened by process %d\n", current->pid);
    return 0;
}

static int device_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "=== CHAR_DEVICE: Device closed by process %d\n", current->pid);
    return 0;
}

static ssize_t device_read(struct file *file, char __user *user_buf, 
                          size_t count, loff_t *ppos)
{
    ssize_t bytes_to_read;
    
    if (*ppos >= buffer_used) {
        return 0;
    }
    
    bytes_to_read = min((size_t)(buffer_used - *ppos), count);
    
    if (copy_to_user(user_buf, device_buffer + *ppos, bytes_to_read)) {
        return -EFAULT;
    }
    
    *ppos += bytes_to_read;
    
    printk(KERN_INFO "=== CHAR_DEVICE: Read %zd bytes from device\n", bytes_to_read);
    return bytes_to_read;
}

static ssize_t device_write(struct file *file, const char __user *user_buf,
                           size_t count, loff_t *ppos)
{
    ssize_t bytes_to_write;
    
    if (count > BUFFER_SIZE) {
        count = BUFFER_SIZE;
    }
    
    bytes_to_write = min((size_t)(BUFFER_SIZE - *ppos), count);
    
    if (copy_from_user(device_buffer, user_buf, bytes_to_write)) {
        return -EFAULT;
    }
    
    buffer_used = bytes_to_write;
    *ppos = bytes_to_write;
    
    printk(KERN_INFO "=== CHAR_DEVICE: Written %zd bytes to device\n", bytes_to_write);
    return bytes_to_write;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = device_open,
    .release = device_release,
    .read = device_read,
    .write = device_write,
};

static int __init chardev_init(void)
{
    int ret;
    
    // Allocate device numbers
    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ERR "=== CHAR_DEVICE: Failed to allocate device number\n");
        return ret;
    }
    
    // Allocate buffer
    device_buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
    if (!device_buffer) {
        unregister_chrdev_region(dev_num, 1);
        printk(KERN_ERR "=== CHAR_DEVICE: Failed to allocate buffer\n");
        return -ENOMEM;
    }
    
    memset(device_buffer, 0, BUFFER_SIZE);
    
    // Initialize and add character device
    cdev_init(&my_cdev, &fops);
    my_cdev.owner = THIS_MODULE;
    
    ret = cdev_add(&my_cdev, dev_num, 1);
    if (ret < 0) {
        kfree(device_buffer);
        unregister_chrdev_region(dev_num, 1);
        printk(KERN_ERR "=== CHAR_DEVICE: Failed to add character device\n");
        return ret;
    }
    
    printk(KERN_INFO "=== CHAR_DEVICE: Character device registered with major: %d\n", 
           MAJOR(dev_num));
    printk(KERN_INFO "=== CHAR_DEVICE: Create device node with: sudo mknod /dev/%s c %d 0\n", 
           DEVICE_NAME, MAJOR(dev_num));
    
    return 0;
}

static void __exit chardev_exit(void)
{
    cdev_del(&my_cdev);
    kfree(device_buffer);
    unregister_chrdev_region(dev_num, 1);
    
    printk(KERN_INFO "=== CHAR_DEVICE: Character device unregistered\n");
}

module_init(chardev_init);
module_exit(chardev_exit);

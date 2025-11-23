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
static char device_buffer[BUF_SIZE];
static int buffer_size = 0;

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
    int ret = 0;

    if (*off < 0 || *off >= BUF_SIZE)
        return -EINVAL;

    if (*off >= buffer_size) {
        return 0;
    }
    
    bytes_to_read = min_t(size_t, len, buffer_size - *off);
    bytes_to_read = min_t(size_t, bytes_to_read, BUF_SIZE - *off);
    
    if (bytes_to_read <= 0)
        return 0;
        
    if (copy_to_user(buf, device_buffer + *off, bytes_to_read)) {
        ret = -EFAULT;
    } else {
        *off += bytes_to_read;
        ret = bytes_to_read;
        printk(KERN_INFO "chardev: Read %d bytes from offset %lld\n", 
               bytes_to_read, *off - bytes_to_read);
    }
    
    return ret;
}

static ssize_t dev_write(struct file *file, const char __user *buf,
                         size_t len, loff_t *off)
{
    int bytes_to_write;
    int ret = 0;
    
    if (*off < 0 || *off >= BUF_SIZE)
        return -EINVAL;
    
    bytes_to_write = min_t(size_t, len, BUF_SIZE - *off);
    
    if (bytes_to_write <= 0)
        return -ENOSPC;

    // БЕЗОПАСНАЯ обработка расширения буфера
    if (*off + bytes_to_write > buffer_size) {
        // Проверяем, что не выходим за границы буфера
        if (*off > buffer_size && *off <= BUF_SIZE) {
            size_t clear_size = *off - buffer_size;
            if (buffer_size + clear_size <= BUF_SIZE) {
                memset(device_buffer + buffer_size, 0, clear_size);
            } else {
                return -EINVAL;
            }
        } else if (*off > BUF_SIZE) {
            return -EINVAL;
        }
        
        // Проверяем новый размер буфера
        if (*off + bytes_to_write > BUF_SIZE) {
            return -ENOSPC;
        }
        buffer_size = *off + bytes_to_write;
    }

    if (copy_from_user(device_buffer + *off, buf, bytes_to_write)) {
        ret = -EFAULT;
    } else {
        *off += bytes_to_write;
        ret = bytes_to_write;
        printk(KERN_INFO "chardev: Write %d bytes at offset %lld\n", 
               bytes_to_write, *off - bytes_to_write);
    }
    
    return ret;
}

static loff_t dev_llseek(struct file *file, loff_t offset, int whence)
{
    loff_t new_pos;
    
    switch (whence) {
        case SEEK_SET:
            new_pos = offset;
            break;
        case SEEK_CUR:
            new_pos = file->f_pos + offset;
            break;
        case SEEK_END:
            new_pos = buffer_size + offset;
            break;
        default:
            return -EINVAL;
    }
    
    if (new_pos < 0 || new_pos > BUF_SIZE)
        return -EINVAL;
        
    file->f_pos = new_pos;
    return new_pos;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .release = dev_release,
    .read = dev_read,
    .write = dev_write,
    .llseek = dev_llseek,
};

static int __init chardev_init(void)
{
    int ret;

    printk(KERN_INFO "chardev: Initializing\n");

    // Инициализация буфера при загрузке модуля
    memset(device_buffer, 0, BUF_SIZE);
    buffer_size = 0;

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
    
    // Очистка буфера при выгрузке
    memset(device_buffer, 0, BUF_SIZE);
    buffer_size = 0;
    
    printk(KERN_INFO "chardev: Device unregistered successfully\n");
}

module_init(chardev_init);
module_exit(chardev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SafonovTV");
MODULE_DESCRIPTION("Simple character device driver");
MODULE_VERSION("1.0");
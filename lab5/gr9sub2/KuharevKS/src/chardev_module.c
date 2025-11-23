/*
 * chardev_module.c
 *
 * Создаёт устройство /dev/mychardev, которое:
 * - Принимает данные при записи (до 1024 байт)
 * - Возвращает сохранённые данные при чтении
 * - ЗАЩИТА: Проверка границ буфера, NULL-терминация, логирование без утечек
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
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/string.h>

#define DEVICE_NAME "mychardev"
#define BUF_SIZE 1024
#define MAX_LOG_SIZE 64  

static dev_t dev_num;
static struct cdev my_cdev;
static char *device_buffer;
static int buffer_size = 0;
static struct mutex device_mutex;

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
    char log_buf[MAX_LOG_SIZE];

    mutex_lock(&device_mutex);

    if (*off >= buffer_size) {
        mutex_unlock(&device_mutex);
        return 0;
    }

    bytes_to_read = min((size_t)(buffer_size - *off), len);

    if (bytes_to_read < 0 || bytes_to_read > BUF_SIZE) {
        printk(KERN_ERR "chardev: Invalid bytes_to_read: %d\n", bytes_to_read);
        mutex_unlock(&device_mutex);
        return -EINVAL;
    }

    
    if (copy_to_user(buf, device_buffer + *off, bytes_to_read)) {
        mutex_unlock(&device_mutex);
        return -EFAULT;
    }

    *off += bytes_to_read;

    snprintf(log_buf, sizeof(log_buf), "Read %d bytes from offset %lld",
             bytes_to_read, *off - bytes_to_read);
    
    mutex_unlock(&device_mutex);

    printk(KERN_INFO "chardev: %s\n", log_buf);

    return bytes_to_read;
}

static ssize_t dev_write(struct file *file, const char __user *buf,
                         size_t len, loff_t *off)
{
    int bytes_to_write;
    char log_buf[MAX_LOG_SIZE];

    mutex_lock(&device_mutex);

    if (len > BUF_SIZE) {
        bytes_to_write = BUF_SIZE;
        printk(KERN_WARNING "chardev: Write size %zu exceeds buffer, truncating to %d\n",
               len, BUF_SIZE);
    } else {
        bytes_to_write = len;
    }

    if (bytes_to_write < 0) {
        mutex_unlock(&device_mutex);
        return -EINVAL;
    }

    if (bytes_to_write > 0) {
        memset(device_buffer, 0, BUF_SIZE);

        if (copy_from_user(device_buffer, buf, bytes_to_write)) {
            mutex_unlock(&device_mutex);
            return -EFAULT;
        }

        if (bytes_to_write < BUF_SIZE) {
            device_buffer[bytes_to_write] = '\0';
        } else {
            device_buffer[BUF_SIZE - 1] = '\0';
            bytes_to_write = BUF_SIZE - 1;
        }

        buffer_size = bytes_to_write;

        snprintf(log_buf, sizeof(log_buf), "Write %d bytes", bytes_to_write);
    } else {
        buffer_size = 0;
        snprintf(log_buf, sizeof(log_buf), "Write 0 bytes (empty)");
    }

    mutex_unlock(&device_mutex);

    printk(KERN_INFO "chardev: %s\n", log_buf);

    return bytes_to_write;
}

static loff_t dev_lseek(struct file *file, loff_t offset, int whence)
{
    loff_t new_pos;

    mutex_lock(&device_mutex);

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
        mutex_unlock(&device_mutex);
        return -EINVAL;
    }

    if (new_pos < 0) {
        mutex_unlock(&device_mutex);
        return -EINVAL;
    }

    if (new_pos > BUF_SIZE) {
        new_pos = BUF_SIZE;
    }

    file->f_pos = new_pos;
    mutex_unlock(&device_mutex);

    printk(KERN_DEBUG "chardev: Seek to position %lld\n", new_pos);

    return new_pos;
}

static long dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret = 0;

    
    if (!arg) {
        return -EINVAL;
    }

    switch (cmd) {
    case 0x10:  
        mutex_lock(&device_mutex);
        if (copy_to_user((int __user *)arg, &buffer_size, sizeof(int))) {
            ret = -EFAULT;
        } else {
            printk(KERN_INFO "chardev: IOCTL GET_BUFFER_SIZE: %d\n", buffer_size);
        }
        mutex_unlock(&device_mutex);
        break;

    case 0x11: 
        mutex_lock(&device_mutex);
        memset(device_buffer, 0, BUF_SIZE);
        buffer_size = 0;
        mutex_unlock(&device_mutex);
        printk(KERN_INFO "chardev: IOCTL CLEAR_BUFFER\n");
        break;

    default:
        printk(KERN_WARNING "chardev: Unknown IOCTL command: 0x%x\n", cmd);
        return -EINVAL;
    }

    return ret;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .release = dev_release,
    .read = dev_read,
    .write = dev_write,
    .llseek = dev_lseek,
    .unlocked_ioctl = dev_ioctl,
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

    device_buffer = kmalloc(BUF_SIZE, GFP_KERNEL);
    if (!device_buffer) {
        printk(KERN_ERR "chardev: Failed to allocate device buffer\n");
        ret = -ENOMEM;
        goto fail_alloc;
    }

    memset(device_buffer, 0, BUF_SIZE);
    buffer_size = 0;

    mutex_init(&device_mutex);

    cdev_init(&my_cdev, &fops);
    my_cdev.owner = THIS_MODULE;

    ret = cdev_add(&my_cdev, dev_num, 1);
    if (ret < 0) {
        printk(KERN_ERR "chardev: Failed to add cdev\n");
        goto fail_cdev;
    }

    printk(KERN_INFO "chardev: Device registered successfully\n");
    printk(KERN_INFO "chardev: Create device with: sudo mknod /dev/%s c %d 0\n",
           DEVICE_NAME, MAJOR(dev_num));

    return 0;

fail_cdev:
    kfree(device_buffer);
    device_buffer = NULL;
fail_alloc:
    unregister_chrdev_region(dev_num, 1);
    return ret;
}

static void __exit chardev_exit(void)
{
    if (&my_cdev) {
        cdev_del(&my_cdev);
    }

    if (device_buffer) {
        kfree(device_buffer);
        device_buffer = NULL;  
    }

    unregister_chrdev_region(dev_num, 1);

    printk(KERN_INFO "chardev: Device unregistered safely\n");
}

module_init(chardev_init);
module_exit(chardev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kuharev Kirill");
MODULE_DESCRIPTION("Character device driver with security improvements");
MODULE_VERSION("1.1");

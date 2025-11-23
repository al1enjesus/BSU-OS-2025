#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define DEVICENAME "mychardev"
#define BUFSIZE 1024

static dev_t devnum;
static struct cdev mycdev;
static char device_buffer[BUFSIZE];
static int buffersize = 0;

static int dev_open(struct inode *inode, struct file *file) {
  printk(KERN_INFO "chardev: Device opened\n");
  return 0;
}

static int dev_release(struct inode *inode, struct file *file) {
  printk(KERN_INFO "chardev: Device closed\n");
  return 0;
}

static ssize_t dev_read(struct file *file, char __user *buf, size_t len,
                        loff_t *off) {
  if (*off >= buffersize)
    return 0;
  if (len > buffersize - *off)
    len = buffersize - *off;
  if (copy_to_user(buf, device_buffer + *off, len))
    return -EFAULT;
  *off += len;
  printk(KERN_INFO "chardev: Read %zu bytes\n", len);
  return len;
}

static ssize_t dev_write(struct file *file, const char __user *buf, size_t len,
                         loff_t *off) {
  if (len > BUFSIZE)
    len = BUFSIZE;
  if (copy_from_user(device_buffer, buf, len))
    return -EFAULT;
  buffersize = len;
  printk(KERN_INFO "chardev: Written %zu bytes\n", len);
  return len;
}

static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .release = dev_release,
    .read = dev_read,
    .write = dev_write,
};

static int __init chardev_init(void) {
  int ret;
  ret = alloc_chrdev_region(&devnum, 0, 1, DEVICENAME);
  if (ret) {
    printk(KERN_ERR "chardev: Failed to allocate dev number\n");
    return ret;
  }
  cdev_init(&mycdev, &fops);
  mycdev.owner = THIS_MODULE;
  ret = cdev_add(&mycdev, devnum, 1);
  if (ret) {
    unregister_chrdev_region(devnum, 1);
    printk(KERN_ERR "chardev: Failed to register device\n");
    return ret;
  }
  printk(KERN_INFO "chardev: Registered device with major %d\n", MAJOR(devnum));
  return 0;
}

static void __exit chardev_exit(void) {
  cdev_del(&mycdev);
  unregister_chrdev_region(devnum, 1);
  printk(KERN_INFO "chardev: Module unloaded\n");
}

module_init(chardev_init);
module_exit(chardev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pardaev Sergey");
MODULE_DESCRIPTION("Simple char device driver");
MODULE_VERSION("1.0");

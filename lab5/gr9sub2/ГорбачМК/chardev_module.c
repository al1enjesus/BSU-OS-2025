#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define DEVICE_NAME "mychardev"
#define BUFFER_SIZE 1024

static dev_t dev_num;
static struct cdev my_cdev;

static char *kernel_buffer; 
static int buffer_size = 0;

static int dev_open(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "mychardev: device opened\n");
	return 0;
}

static int dev_release(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "mychardev: device closed\n");
	return 0;
}

static ssize_t dev_read(struct file *file, char __user *buf,
size_t len, loff_t *ppos)
{
	if (*ppos >= buffer_size)
		return 0;

	if (len > buffer_size - *ppos)
		len = buffer_size - *ppos;

	if (copy_to_user(buf, kernel_buffer + *ppos, len))
		return -EFAULT;

	*ppos += len;
	return len;
}

static ssize_t dev_write(struct file *file, const char __user *buf,
size_t len, loff_t *ppos)
{
	if (len > BUFFER_SIZE)
		len = BUFFER_SIZE;

	if (copy_from_user(kernel_buffer, buf, len))
		return -EFAULT;

	buffer_size = len;
	printk(KERN_INFO "mychardev: received %d bytes\n", buffer_size);

	return len;
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
	if (alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME) < 0) {
		printk(KERN_ERR "mychardev: alloc_chrdev_region failed\n");
		return -1;
	}

	cdev_init(&my_cdev, &fops);

	if (cdev_add(&my_cdev, dev_num, 1) < 0) {
		unregister_chrdev_region(dev_num, 1);
		printk(KERN_ERR "mychardev: cdev_add failed\n");
		return -1;
	}

	kernel_buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
	if (!kernel_buffer) {
		cdev_del(&my_cdev);
		unregister_chrdev_region(dev_num, 1);
		printk(KERN_ERR "mychardev: kmalloc failed\n");
		return -ENOMEM;
	}

	printk(KERN_INFO "mychardev: module loaded. Major=%d Minor=0\n", MAJOR(dev_num));
	return 0;
}

static void __exit chardev_exit(void)
{
	kfree(kernel_buffer);
	cdev_del(&my_cdev);
	unregister_chrdev_region(dev_num, 1);

	printk(KERN_INFO "mychardev: module unloaded\n");
}

module_init(chardev_init);
module_exit(chardev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Gorbach_Matvey");
MODULE_DESCRIPTION("Simple character device module");
MODULE_VERSION("0.1");


// char_device.c — символьное устройство /dev/mychardev
// Вариант 1 (нечётные номера): задание C

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define MYCHDEV_NAME      "mychardev"
#define MYCHDEV_MAX_SIZE  1024

static dev_t dev_number;
static struct cdev my_cdev;

static char *device_buffer;
static size_t data_size;

static int my_open(struct inode *inode, struct file *file)
{
	pr_info("char_device: /dev/%s opened\n", MYCHDEV_NAME);
	return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
	pr_info("char_device: /dev/%s closed\n", MYCHDEV_NAME);
	return 0;
}

static ssize_t my_read(struct file *file, char __user *buf,
		       size_t len, loff_t *ppos)
{
	size_t remaining;
	size_t to_copy;

	if (*ppos >= data_size)
		return 0;

	remaining = data_size - *ppos;
	to_copy = (len > remaining) ? remaining : len;

	if (to_copy == 0)
		return 0;

	if (copy_to_user(buf, device_buffer + *ppos, to_copy))
		return -EFAULT;

	*ppos += to_copy;

	pr_info("char_device: read %zu bytes (pos=%lld, size=%zu)\n",
		to_copy, *ppos, data_size);

	return to_copy;
}

static ssize_t my_write(struct file *file, const char __user *buf,
			size_t len, loff_t *ppos)
{
	size_t space_left;
	size_t to_copy;

	if (*ppos >= MYCHDEV_MAX_SIZE)
		return -ENOSPC;

	space_left = MYCHDEV_MAX_SIZE - *ppos;
	to_copy = (len > space_left) ? space_left : len;

	if (to_copy == 0)
		return -ENOSPC;

	if (copy_from_user(device_buffer + *ppos, buf, to_copy))
		return -EFAULT;

	*ppos += to_copy;

	if (*ppos > data_size)
		data_size = *ppos;

	pr_info("char_device: written %zu bytes (pos=%lld, size=%zu)\n",
		to_copy, *ppos, data_size);

	return to_copy;
}

static const struct file_operations my_fops = {
	.owner   = THIS_MODULE,
	.open    = my_open,
	.release = my_release,
	.read    = my_read,
	.write   = my_write,
};

static int __init mychardev_init(void)
{
	int ret;

	data_size = 0;

	ret = alloc_chrdev_region(&dev_number, 0, 1, MYCHDEV_NAME);
	if (ret) {
		pr_err("char_device: alloc_chrdev_region failed, err=%d\n", ret);
		return ret;
	}

	cdev_init(&my_cdev, &my_fops);
	my_cdev.owner = THIS_MODULE;

	ret = cdev_add(&my_cdev, dev_number, 1);
	if (ret) {
		pr_err("char_device: cdev_add failed, err=%d\n", ret);
		unregister_chrdev_region(dev_number, 1);
		return ret;
	}

	device_buffer = kmalloc(MYCHDEV_MAX_SIZE, GFP_KERNEL);
	if (!device_buffer) {
		pr_err("char_device: failed to allocate buffer\n");
		cdev_del(&my_cdev);
		unregister_chrdev_region(dev_number, 1);
		return -ENOMEM;
	}

	memset(device_buffer, 0, MYCHDEV_MAX_SIZE);

	pr_info("char_device: loaded, /dev/%s (major=%d, minor=%d), buffer=%d bytes\n",
		MYCHDEV_NAME, MAJOR(dev_number), MINOR(dev_number), MYCHDEV_MAX_SIZE);
	pr_info("char_device: create node with 'mknod /dev/%s c %d 0'\n",
		MYCHDEV_NAME, MAJOR(dev_number));

	return 0;
}

static void __exit mychardev_exit(void)
{
	if (device_buffer) {
		kfree(device_buffer);
		device_buffer = NULL;
	}

	cdev_del(&my_cdev);
	unregister_chrdev_region(dev_number, 1);

	pr_info("char_device: unloaded, /dev/%s removed (node must be deleted manually)\n",
		MYCHDEV_NAME);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kazak Artur");
MODULE_DESCRIPTION("Simple character device /dev/mychardev example");
MODULE_VERSION("1.0");

module_init(mychardev_init);
module_exit(mychardev_exit);

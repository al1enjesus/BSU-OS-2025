#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>
#include <linux/atomic.h>

#define PROC_FILENAME "student_info"

static atomic_t read_count = ATOMIC_INIT(0);
static struct proc_dir_entry *proc_file;
static unsigned long load_time; 
static int read_count = 0; 

static ssize_t student_read(struct file *file, char __user *ubuf,
size_t count, loff_t *ppos)
{
	char buffer[256];
	int len = 0;
	
	if (*ppos > 0)
		return 0;

	current_count = atomic_inc_return(&read_count);

	len = snprintf(buffer, sizeof(buffer),
		"Name: Gorbach Matvey\n"
		"Group: 9\n"
		"Module loaded at: %lu jiffies\n"
		"Read count: %d\n",
	load_time, read_count);

	if (copy_to_user(ubuf, buffer, len))
		return -EFAULT;
	
	*ppos = len;
	return len;
}

static const struct proc_ops student_ops = {
	.proc_read = student_read,
};

static int __init proc_module_init(void)
{
	load_time = jiffies;

	proc_file = proc_create(PROC_FILENAME, 0444, NULL, &student_ops);
	if (!proc_file) {
		printk(KERN_ERR "student_info: failed to create proc file\n");
		return -ENOMEM;
	}

	printk(KERN_INFO "student_info module loaded\n");
	return 0;
}

static void __exit proc_module_exit(void)
{
	proc_remove(proc_file);
	printk(KERN_INFO "student_info module unloaded\n");
}

module_init(proc_module_init);
module_exit(proc_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Gorbach Matvey");
MODULE_DESCRIPTION("Procfs student info module");
MODULE_VERSION("0.1");

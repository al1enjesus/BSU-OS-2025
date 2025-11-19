// proc_module.c — модуль, создающий /proc/student_info
// Вариант 1 (нечётные номера): задание B

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/jiffies.h>
#include <linux/atomic.h>

static struct proc_dir_entry *proc_entry;
static unsigned long loaded_jiffies;
static atomic_t read_count = ATOMIC_INIT(0);

static int student_info_show(struct seq_file *m, void *v)
{
	int count = atomic_inc_return(&read_count);

	seq_printf(m, "Name: Kazak Artur\n");
	seq_printf(m, "Group: 8, Subgroup: 2\n");
	seq_printf(m, "Module loaded at: %lu jiffies\n", loaded_jiffies);
	seq_printf(m, "Read count: %d\n", count);

	return 0;
}

static int student_info_open(struct inode *inode, struct file *file)
{
	return single_open(file, student_info_show, NULL);
}

static const struct proc_ops student_info_fops = {
	.proc_open    = student_info_open,
	.proc_read    = seq_read,
	.proc_lseek   = seq_lseek,
	.proc_release = single_release,
};

static int __init student_info_init(void)
{
	loaded_jiffies = jiffies;
	atomic_set(&read_count, 0);

	proc_entry = proc_create("student_info", 0444, NULL, &student_info_fops);
	if (!proc_entry) {
		pr_err("proc_module: failed to create /proc/student_info\n");
		return -ENOMEM;
	}

	pr_info("proc_module: loaded, /proc/student_info created\n");
	return 0;
}

static void __exit student_info_exit(void)
{
	if (proc_entry) {
		proc_remove(proc_entry);
		proc_entry = NULL;
	}

	pr_info("proc_module: unloaded, /proc/student_info removed\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kazak Artur");
MODULE_DESCRIPTION("/proc_student_info example module (atomic read counter)");
MODULE_VERSION("1.1");

module_init(student_info_init);
module_exit(student_info_exit);

// proc_module.c — модуль, создающий /proc/student_info
// Вариант 1 (нечётные номера): задание B

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/jiffies.h>

static struct proc_dir_entry *proc_entry;
static unsigned long loaded_jiffies;
static unsigned long read_count;

static int student_info_show(struct seq_file *m, void *v)
{
	read_count++;

	seq_printf(m, "Name: Kazak Artur\n");
	seq_printf(m, "Group: 8, Subgroup: 2\n");
	seq_printf(m, "Module loaded at: %lu jiffies\n", loaded_jiffies);
	seq_printf(m, "Read count: %lu\n", read_count);

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
	read_count = 0;

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
MODULE_DESCRIPTION("/proc/student_info example module");
MODULE_VERSION("1.0");

module_init(student_info_init);
module_exit(student_info_exit);

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>

#define PROC_NAME "student_info"
#define MAX_SIZE 1024

static struct proc_dir_entry *proc_file = NULL;
static int read_count = 0;
static unsigned long load_time = 0;

static ssize_t proc_read(struct file *file, char __user *ubuf,
                         size_t count, loff_t *ppos)
{
    char buf[MAX_SIZE];
    int len;
    unsigned long uptime_seconds;

    if (*ppos > 0)
        return 0;

    read_count++;

    uptime_seconds = jiffies / HZ;

    len = snprintf(buf, sizeof(buf),
        "=== Student Information ===\n"
        "Name: Tereshko Daniil\n"
        "Group: 8, Subgroup: 2\n"
        "Module loaded at: %lu jiffies\n"
        "Uptime: %lu seconds\n"
        "Read count: %d\n"
        "===========================\n",
        load_time, uptime_seconds, read_count);

    if (copy_to_user(ubuf, buf, len)) {
        printk(KERN_ERR "proc_module: copy_to_user failed\n");
        return -EFAULT;
    }

    *ppos = len;

    printk(KERN_INFO "proc_module: File read, count: %d\n", read_count);
    return len;
}

static const struct proc_ops proc_file_ops = {
    .proc_read = proc_read,
};

static int __init proc_module_init(void)
{
    printk(KERN_INFO "proc_module: Initializing\n");

    load_time = jiffies;

    proc_file = proc_create(PROC_NAME, 0444, NULL, &proc_file_ops);
    if (!proc_file) {
        printk(KERN_ERR "proc_module: Failed to create /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }

    printk(KERN_INFO "proc_module: Created /proc/%s\n", PROC_NAME);
    printk(KERN_INFO "proc_module: Load time: %lu jiffies\n", load_time);
    return 0;
}

static void __exit proc_module_exit(void)
{
    if (proc_file) {
        proc_remove(proc_file);
        printk(KERN_INFO "proc_module: Removed /proc/%s\n", PROC_NAME);
    }
    
    printk(KERN_INFO "proc_module: Total reads during lifetime: %d\n", read_count);
}

module_init(proc_module_init);
module_exit(proc_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tereshko Daniil");
MODULE_DESCRIPTION("Proc filesystem example");
MODULE_VERSION("1.0");
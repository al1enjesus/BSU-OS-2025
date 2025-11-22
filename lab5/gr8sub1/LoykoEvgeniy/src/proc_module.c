#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>
#include <linux/slab.h>

#define PROC_NAME "student_info"
#define MAX_SIZE 256  // Уменьшенный размер

static struct proc_dir_entry *proc_file = NULL;
static int read_count = 0;
static unsigned long load_time = 0;

static ssize_t proc_read(struct file *file, char __user *ubuf,
                         size_t count, loff_t *ppos)
{
    char *buf = NULL;
    int len;
    int ret = 0;

    if (*ppos > 0)
        return 0;

    // Динамическое выделение вместо стека
    buf = kmalloc(MAX_SIZE, GFP_KERNEL);
    if (!buf) {
        printk(KERN_ERR "proc_module: Failed to allocate buffer\n");
        return -ENOMEM;
    }

    read_count++;

    len = snprintf(buf, MAX_SIZE,
        "Name: Loyko Evgeniy\n"
        "Group: 8, Subgroup: 1\n"
        "Module loaded at: %lu jiffies\n"
        "Read count: %d\n",
        load_time, read_count);

    // Проверка на усечение
    if (len >= MAX_SIZE) {
        printk(KERN_WARNING "proc_module: Buffer truncated\n");
        len = MAX_SIZE - 1;
    }

    if (copy_to_user(ubuf, buf, len)) {
        ret = -EFAULT;
        goto out;
    }

    *ppos = len;
    ret = len;

out:
    kfree(buf);
    return ret;
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
    return 0;
}

static void __exit proc_module_exit(void)
{
    if (proc_file) {
        proc_remove(proc_file);
        printk(KERN_INFO "proc_module: Removed /proc/%s\n", PROC_NAME);
    }
}

module_init(proc_module_init);
module_exit(proc_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Loyko Evgeniy");
MODULE_DESCRIPTION("/proc filesystem example with student info");
MODULE_VERSION("1.0");

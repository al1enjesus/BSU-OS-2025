#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>
#include <linux/limits.h> 

#define PROC_NAME "student_info"
#define MAX_SIZE 512

static struct proc_dir_entry *proc_file = NULL;
static int read_count = 0;
static unsigned long load_time = 0;

static ssize_t proc_read(struct file *file, char __user *ubuf,
                         size_t count, loff_t *ppos)
{
    char buf[MAX_SIZE];
    int len;

    if (*ppos > 0)
        return 0;


    if (read_count < INT_MAX)
        read_count++;
    else
        read_count = 1;

  
    len = snprintf(buf, MAX_SIZE,
        "Name: %s\n"
        "Group: %d, Subgroup: %d\n"
        "Module loaded at: %lu jiffies\n"
        "Read count: %d\n",
        "Julia Pauliuchuk", 8, 1, load_time, read_count);


    if (len >= MAX_SIZE)
        return -EINVAL;

    if (copy_to_user(ubuf, buf, len))
        return -EFAULT;

    *ppos = len;
    return len;
}

static const struct proc_ops proc_file_ops = {
    .proc_read = proc_read,
};

static int __init student_info_init(void)
{
    printk(KERN_INFO "student_info_module: Initializing\n");

    load_time = jiffies;

    proc_file = proc_create(PROC_NAME, 0444, NULL, &proc_file_ops);
    if (!proc_file) {
        printk(KERN_ERR "student_info_module: Failed to create /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }

    printk(KERN_INFO "student_info_module: Created /proc/%s \n", PROC_NAME);
    return 0;
}

static void __exit student_info_exit(void)
{
    if (proc_file) {
        proc_remove(proc_file);
        printk(KERN_INFO "student_info_module: Removed /proc/%s\n", PROC_NAME);
    }

    printk(KERN_INFO "student_info_module: Exiting \n");
}

module_init(student_info_init);
module_exit(student_info_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pauliuchuk Julia");
MODULE_DESCRIPTION("/proc student info module");
MODULE_VERSION("1.0");


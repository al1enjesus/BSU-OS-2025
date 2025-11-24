#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>

#define PROC_NAME "my_config"
#define MAX_SIZE 256

static struct proc_dir_entry *proc_file = NULL;
static char config_data[MAX_SIZE] = "default\n";

static ssize_t proc_read(struct file *file, char __user *ubuf,
                         size_t count, loff_t *ppos)
{
    int len = strlen(config_data);

    if (*ppos > 0)
        return 0;

    if (copy_to_user(ubuf, config_data, len))
        return -EFAULT;

    *ppos = len;
    return len;
}

static ssize_t proc_write(struct file *file, const char __user *ubuf,
                          size_t count, loff_t *ppos)
{
    size_t len = count;

    if (len >= MAX_SIZE)
        len = MAX_SIZE - 1;

    if (copy_from_user(config_data, ubuf, len))
        return -EFAULT;

    if (len > 0 && config_data[len - 1] == '\n')
        len--;

    config_data[len] = '\0';

    printk(KERN_INFO "proc_config: New value written: %s\n", config_data);

    return count;
}

static const struct proc_ops proc_file_ops = {
    .proc_read = proc_read,
    .proc_write = proc_write,
};

static int __init proc_config_init(void)
{
    printk(KERN_INFO "proc_config: Initializing\n");

    proc_file = proc_create(PROC_NAME, 0666, NULL, &proc_file_ops);
    if (!proc_file) {
        printk(KERN_ERR "proc_config: Failed to create /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }

    printk(KERN_INFO "proc_config: Created /proc/%s\n", PROC_NAME);
    return 0;
}

static void __exit proc_config_exit(void)
{
    if (proc_file) {
        proc_remove(proc_file);
        printk(KERN_INFO "proc_config: Removed /proc/%s\n", PROC_NAME);
    }
}

module_init(proc_config_init);
module_exit(proc_config_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ivan Lobanok");
MODULE_DESCRIPTION("Proc filesystem with read/write support");
MODULE_VERSION("1.0");


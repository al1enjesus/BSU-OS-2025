#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/string.h>

#define MAX_LENGTH 256
#define PROC_NAME "my_config"

static struct proc_dir_entry *proc_file;
static char config_data[MAX_LENGTH] = "default";
static size_t config_length = 7; // длина "default"

static ssize_t proc_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
    if (*ppos > 0)
        return 0;

    if (copy_to_user(ubuf, config_data, config_length)) {
        return -EFAULT;
    }

    *ppos = config_length;
    return config_length;
}

static ssize_t proc_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos)
{
    size_t len = count;
    
    if (len > MAX_LENGTH - 1) {
        len = MAX_LENGTH - 1;
    }

    if (copy_from_user(config_data, ubuf, len)) {
        return -EFAULT;
    }

    config_data[len] = '\0';
    config_length = len;
    
    printk(KERN_INFO "my_config: new value set: %s\n", config_data);
    
    return count;
}

static const struct proc_ops proc_ops = {
    .proc_read = proc_read,
    .proc_write = proc_write,
};

static int __init proc_init(void)
{
    proc_file = proc_create(PROC_NAME, 0666, NULL, &proc_ops);
    if (!proc_file) {
        printk(KERN_ERR "Failed to create /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }

    printk(KERN_INFO "/proc/%s created successfully\n", PROC_NAME);
    return 0;
}

static void __exit proc_exit(void)
{
    proc_remove(proc_file);
    printk(KERN_INFO "/proc/%s removed\n", PROC_NAME);
}

module_init(proc_init);
module_exit(proc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Daniil");
MODULE_DESCRIPTION("/proc file with read/write capability");
MODULE_VERSION("1.0");

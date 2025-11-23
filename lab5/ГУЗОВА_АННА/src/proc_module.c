#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>

#define PROC_NAME "my_config"
#define MAX_SIZE 1024
#define MAX_CONFIG_LEN  256

static struct proc_dir_entry *proc_file;
static char config_value[MAX_CONFIG_LEN] = "default";
static size_t config_len = 7;

static ssize_t proc_read(struct file *file, char __user *ubuf,
                         size_t count, loff_t *ppos)
{
    char buf[MAX_SIZE];
    int len;

    if (*ppos > 0) return 0;

    len = scnprintf(buf, sizeof(buf), "%s\n", config_value);

    if (len > count) len = count;

    if (copy_to_user(ubuf, buf, len)) return -EFAULT;

    *ppos += len;
    return len;
}

static ssize_t proc_write(struct file *file, const char __user *ubuf,
                          size_t count, loff_t *ppos)
{
    char buf[MAX_CONFIG_LEN];
    size_t to_copy;

    if (count == 0)
        return 0;

    to_copy = min(count, (size_t)(MAX_CONFIG_LEN - 1));

    if (copy_from_user(buf, ubuf, to_copy))
        return -EFAULT;

    buf[to_copy] = '\0';

    if (to_copy > 0 && buf[to_copy - 1] == '\n')buf[to_copy - 1] = '\0';

    strncpy(config_value, buf, MAX_CONFIG_LEN);
    config_len = strlen(config_value);

    return count;
}

static const struct proc_ops proc_file_ops = {
    .proc_read  = proc_read,
    .proc_write = proc_write,
};

static int __init proc_module_init(void)
{
    proc_file = proc_create(PROC_NAME, 0666, NULL, &proc_file_ops);
    if (!proc_file) {
        printk(KERN_ERR "proc_module: Failed to create /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }

    printk(KERN_INFO "proc_module: /proc/%s created, default=\"%s\"\n",
           PROC_NAME, config_value);
    return 0;
}

static void __exit proc_module_exit(void)
{
    if (proc_file) {
        proc_remove(proc_file);
        printk(KERN_INFO "proc_module: /proc/%s removed\n", PROC_NAME);
    }
}

module_init(proc_module_init);
module_exit(proc_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("hannahuzova");
MODULE_DESCRIPTION("/proc/my_config module (variant 2)");
MODULE_VERSION("1.0");

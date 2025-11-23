#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define PROC_NAME "my_config"
#define MAX_LEN   256

static struct proc_dir_entry *proc_file;

static char  config_buf[MAX_LEN] = "default";
static size_t config_len = 7;   

static ssize_t proc_read(struct file *file, char __user *ubuf,
                         size_t count, loff_t *ppos)
{
    size_t remaining;
    size_t to_copy;
    int ret;

    if (!ubuf)
        return -EFAULT;

    if (*ppos < 0)
        return -EINVAL;

    if (*ppos >= config_len)
        return 0;   

    remaining = config_len - *ppos;
    to_copy   = min(count, remaining);

    ret = copy_to_user(ubuf, config_buf + *ppos, to_copy);
    if (ret != 0)
        return -EFAULT;

    *ppos += to_copy;
    return to_copy;
}

static ssize_t proc_write(struct file *file, const char __user *ubuf,
                          size_t count, loff_t *ppos)
{
    size_t to_copy;

    if (!ubuf)
        return -EFAULT;

    if (count == 0)
        return 0;

    to_copy = min(count, (size_t)(MAX_LEN - 1));

    memset(config_buf, 0, MAX_LEN);

    if (copy_from_user(config_buf, ubuf, to_copy))
        return -EFAULT;

        if (to_copy > 0 && config_buf[to_copy - 1] == '\n') {
        config_buf[to_copy - 1] = '\0';
        config_len = to_copy - 1;
    } else {
        config_buf[to_copy] = '\0';
        config_len = to_copy;
    }

    printk(KERN_INFO "proc_config: new value set: '%s'\n", config_buf);

    return count;
}

static const struct proc_ops proc_file_ops = {
    .proc_read  = proc_read,
    .proc_write = proc_write,
};

static int __init proc_config_init(void)
{
    proc_file = proc_create(PROC_NAME, 0644, NULL, &proc_file_ops);
    if (!proc_file) {
        printk(KERN_ERR "proc_config: failed to create /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }

    printk(KERN_INFO "proc_config: /proc/%s created, default='%s'\n",
           PROC_NAME, config_buf);
    return 0;
}

static void __exit proc_config_exit(void)
{
    if (proc_file) {
        proc_remove(proc_file);
        proc_file = NULL;
        printk(KERN_INFO "proc_config: /proc/%s removed\n", PROC_NAME);
    }
}

module_init(proc_config_init);
module_exit(proc_config_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Anya");
MODULE_DESCRIPTION("/proc/my_config with read/write (partial read supported)");
MODULE_VERSION("1.1");
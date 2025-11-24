// proc_config.c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define PROC_NAME "my_config"
#define MAXLEN 256

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ярослав");
MODULE_DESCRIPTION("/proc file that stores a small configuration string");

static char *config_buf;
static struct proc_dir_entry *proc_entry;

static ssize_t proc_read_config(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
    ssize_t len;

    if (!config_buf)
        return 0;

    len = strlen(config_buf);
    if (*ppos >= len)
        return 0; // EOF

    if (count > (len - *ppos))
        count = len - *ppos;

    if (copy_to_user(ubuf, config_buf + *ppos, count))
        return -EFAULT;

    *ppos += count;
    return count;
}

static ssize_t proc_write_config(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos)
{
    ssize_t to_copy;

    if (!config_buf)
        return -ENOMEM;

    if (count == 0)
        return 0;

    if (count > MAXLEN - 1)
        to_copy = MAXLEN - 1;
    else
        to_copy = count;

    /* copy_from_user and null-terminate */
    if (copy_from_user(config_buf, ubuf, to_copy))
        return -EFAULT;

    config_buf[to_copy] = '\0';

    /* trim trailing newline if present */
    if (to_copy > 0 && config_buf[to_copy - 1] == '\n')
        config_buf[to_copy - 1] = '\0';

    return count;
}

/* proc_ops for modern kernels */
static const struct proc_ops proc_file_ops = {
    .proc_read  = proc_read_config,
    .proc_write = proc_write_config,
};

static int __init proc_config_init(void)
{
    config_buf = kzalloc(MAXLEN, GFP_KERNEL);
    if (!config_buf)
        return -ENOMEM;

    strscpy(config_buf, "default", MAXLEN);

    proc_entry = proc_create(PROC_NAME, 0666, NULL, &proc_file_ops);
    if (!proc_entry) {
        kfree(config_buf);
        return -ENOMEM;
    }

    printk(KERN_INFO "proc_config: /proc/%s created\n", PROC_NAME);
    return 0;
}

static void __exit proc_config_exit(void)
{
    proc_remove(proc_entry);
    kfree(config_buf);
    printk(KERN_INFO "proc_config: /proc/%s removed\n", PROC_NAME);
}

module_init(proc_config_init);
module_exit(proc_config_exit);

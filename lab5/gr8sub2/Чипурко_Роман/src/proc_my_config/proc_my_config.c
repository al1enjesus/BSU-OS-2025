#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>

#define PROC_NAME "my_config"
#define MAX_LEN 256

static char config_buf[MAX_LEN + 1] = "default";
static struct proc_dir_entry *proc_file;
static DEFINE_MUTEX(config_lock);

static ssize_t my_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
    char tmp[MAX_LEN + 1];
    ssize_t len;

    if (*ppos > 0)
        return 0;

    mutex_lock(&config_lock);
    len = snprintf(tmp, sizeof(tmp), "%s\n", config_buf);
    mutex_unlock(&config_lock);

    if (len > count)
        len = count;

    if (copy_to_user(ubuf, tmp, len))
        return -EFAULT;

    *ppos = len;
    return len;
}

static ssize_t my_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos)
{
    size_t to_copy;

    if (count == 0)
        return 0;

    to_copy = (count > MAX_LEN) ? MAX_LEN : count;

    mutex_lock(&config_lock);

    if (copy_from_user(config_buf, ubuf, to_copy)) {
        mutex_unlock(&config_lock);
        return -EFAULT;
    }
    config_buf[to_copy] = '\0';

    if (to_copy > 0 && config_buf[to_copy - 1] == '\n')
        config_buf[to_copy - 1] = '\0';

    mutex_unlock(&config_lock);

    return count;
}

static const struct proc_ops my_proc_ops = {
    .proc_read  = my_read,
    .proc_write = my_write,
};

static int __init my_init(void)
{
    proc_file = proc_create(PROC_NAME, 0666, NULL, &my_proc_ops);
    if (!proc_file) {
        pr_err("proc_create failed\n");
        return -ENOMEM;
    }

    pr_info("/proc/%s created with default=\"%s\"\n", PROC_NAME, config_buf);
    return 0;
}

static void __exit my_exit(void)
{
    proc_remove(proc_file);
    pr_info("/proc/%s removed\n", PROC_NAME);
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Чипурко Роман");
MODULE_DESCRIPTION("/proc/my_config writable example (max 256 bytes)");
MODULE_VERSION("1.0");

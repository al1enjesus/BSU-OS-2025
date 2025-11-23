#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/mutex.h>

#define PROC_NAME       "my_config"
#define MAX_CONFIG_LEN  256

static struct proc_dir_entry *proc_file;
static char config_buf[MAX_CONFIG_LEN] = "default";
static size_t config_len = 7;

static DEFINE_MUTEX(my_config_mutex);

static ssize_t my_config_read(struct file *file,
                              char __user *buf,
                              size_t count,
                              loff_t *ppos) {
    ssize_t ret;

    if (!buf || !ppos)
        return -EINVAL;

    mutex_lock(&my_config_mutex);

    if (config_len > MAX_CONFIG_LEN)
        config_len = MAX_CONFIG_LEN;

    ret = simple_read_from_buffer(buf, count, ppos,
                                  config_buf, config_len);

    mutex_unlock(&my_config_mutex);

    return ret;
}


static ssize_t my_config_write(struct file *file,
                               const char __user *buf,
                               size_t count,
                               loff_t *ppos) {
    size_t len;

    if (!buf)
        return -EINVAL;

    if (count == 0)
        return 0;

    len = min(count, (size_t)(MAX_CONFIG_LEN - 1));

    mutex_lock(&my_config_mutex);

    if (copy_from_user(config_buf, buf, len)) {
        mutex_unlock(&my_config_mutex);
        return -EFAULT;
    }

    if (len > 0 && config_buf[len - 1] == '\n') {
        config_buf[len - 1] = '\0';
        config_len = len - 1;
    } else {
        config_buf[len] = '\0';
        config_len = len;
    }

    if (config_len > MAX_CONFIG_LEN)
        config_len = MAX_CONFIG_LEN;

    mutex_unlock(&my_config_mutex);

    return count;
}


static const struct proc_ops my_config_ops = {
    .proc_read = my_config_read,
    .proc_write = my_config_write,
};


static int __init my_config_init(void) {
    proc_file = proc_create(PROC_NAME, 0666, NULL, &my_config_ops);
    if (!proc_file) {
        pr_err("my_config: failed to create /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }

    pr_info("my_config: module loaded, initial value: '%s'\n", config_buf);
    return 0;
}

static void __exit my_config_exit(void) {
    if (proc_file)
        proc_remove(proc_file);

    pr_info("my_config: module unloaded\n");
}

module_init(my_config_init);

module_exit(my_config_exit);

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Denis");

MODULE_DESCRIPTION("/proc/my_config read-write example");

MODULE_VERSION("1.0");

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#define PROC_NAME "my_config"
#define BUF_LEN   256

static struct proc_dir_entry* proc_entry;
static char config_value[BUF_LEN] = "default";

static ssize_t my_config_read(struct file* file, char __user* buffer,
    size_t length, loff_t* offset)
{
    int len;

    if (*offset > 0)
        return 0;

    len = strlen(config_value);

    if (copy_to_user(buffer, config_value, len))
        return -EFAULT;

    *offset = len;
    return len;
}

static ssize_t my_config_write(struct file* file, const char __user* buffer,
    size_t length, loff_t* offset)
{
    size_t to_copy = min(length, (size_t)(BUF_LEN - 1));

    if (copy_from_user(config_value, buffer, to_copy))
        return -EFAULT;

    config_value[to_copy] = '\0';

    if (to_copy > 0 && config_value[to_copy - 1] == '\n')
        config_value[to_copy - 1] = '\0';

    printk(KERN_INFO "my_config_module: new value = '%s'\n", config_value);

    return length;
}

static const struct proc_ops my_config_fops = {
    .proc_read = my_config_read,
    .proc_write = my_config_write,
};

static int __init my_config_init(void)
{
    proc_entry = proc_create(PROC_NAME, 0666, NULL, &my_config_fops);
    if (!proc_entry) {
        printk(KERN_ERR "my_config_module: failed to create /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }

    printk(KERN_INFO "my_config_module: /proc/%s created, default='%s'\n",
        PROC_NAME, config_value);
    return 0;
}

static void __exit my_config_exit(void)
{
    proc_remove(proc_entry);
    printk(KERN_INFO "my_config_module: /proc/%s removed\n", PROC_NAME);
}

module_init(my_config_init);
module_exit(my_config_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Artsem Khvainitski");
MODULE_DESCRIPTION("/proc/my_config with read/write support");

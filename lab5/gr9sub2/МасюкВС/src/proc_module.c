#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/string.h>

#define PROCFS_NAME "my_config"
#define PROC_BUFFER_SIZE 256
#define PROC_READ_BUFFER_SIZE (PROC_BUFFER_SIZE + 2)

static struct proc_dir_entry *proc_file;
static char procfs_buffer[PROC_BUFFER_SIZE];

static ssize_t proc_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
    char buffer[PROC_READ_BUFFER_SIZE];
    int len;

    if (*ppos > 0)
        return 0;

    len = snprintf(buffer, sizeof(buffer), "%s\n", procfs_buffer);
    return simple_read_from_buffer(ubuf, count, ppos, buffer, len);
}

static ssize_t proc_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos)
{
    ssize_t written;

    if (count >= PROC_BUFFER_SIZE)
        return -EFBIG;

    written = simple_write_to_buffer(procfs_buffer, PROC_BUFFER_SIZE, ppos, ubuf, count);

    if (written > 0) {
        if (written < PROC_BUFFER_SIZE)
            procfs_buffer[written] = '\0';
        else
            procfs_buffer[PROC_BUFFER_SIZE - 1] = '\0';

        if (procfs_buffer[written - 1] == '\n')
            procfs_buffer[written - 1] = '\0';
    }

    return written;
}

static const struct proc_ops proc_fops = {
    .proc_read = proc_read,
    .proc_write = proc_write,
};

static int __init proc_init(void)
{
    memset(procfs_buffer, 0, PROC_BUFFER_SIZE);
    strncpy(procfs_buffer, "default", PROC_BUFFER_SIZE - 1);

    proc_file = proc_create(PROCFS_NAME, 0644, NULL, &proc_fops);
    if (!proc_file) {
        printk(KERN_ERR "Failed to create /proc/%s\n", PROCFS_NAME);
        return -ENOMEM;
    }

    printk(KERN_INFO "/proc/%s created\n", PROCFS_NAME);
    return 0;
}

static void __exit proc_exit(void)
{
    proc_remove(proc_file);
    printk(KERN_INFO "/proc/%s removed\n", PROCFS_NAME);
}

module_init(proc_init);
module_exit(proc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vladimir");
MODULE_DESCRIPTION("/proc file with read/write support");
MODULE_VERSION("1.0");

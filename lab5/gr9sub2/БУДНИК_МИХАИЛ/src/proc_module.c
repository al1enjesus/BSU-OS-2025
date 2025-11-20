#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>

#define PROC_NAME "my_config"
#define MAX_SIZE 256

static struct proc_dir_entry *proc_file = NULL;
static char procfs_buffer[MAX_SIZE];
static unsigned long procfs_buffer_size = 0;

static ssize_t proc_read(struct file *file, char __user *buf,
                         size_t count, loff_t *ppos)
{
    if (*ppos > 0 || count < procfs_buffer_size) return 0;
    if (copy_to_user(buf, procfs_buffer, procfs_buffer_size)) return -EFAULT;

    *ppos = procfs_buffer_size;
    printk(KERN_INFO "proc_module: read %lu bytes\n", procfs_buffer_size);
    return procfs_buffer_size;
}

static ssize_t proc_write(struct file *file, 
        const char __user *buf, 
        size_t count,
        loff_t *ppos) {
    procfs_buffer_size = count > MAX_SIZE ? MAX_SIZE: count;
    if (copy_from_user(procfs_buffer, buf, procfs_buffer_size)) return -EFAULT;

    procfs_buffer[procfs_buffer_size < MAX_SIZE ? procfs_buffer_size: MAX_SIZE - 1] = '\0';
    *ppos = procfs_buffer_size;
    printk(KERN_INFO "proc_module: wrote %lu bytes\n", procfs_buffer_size);
    return procfs_buffer_size;
}

static const struct proc_ops proc_file_ops = {
    .proc_read = proc_read,
    .proc_write = proc_write,
    .proc_lseek = noop_llseek,
};

static int __init proc_module_init(void)
{
    printk(KERN_INFO "proc_module: initializing\n");

    const char* default_string = "default\n";
    memset(procfs_buffer, 0, MAX_SIZE);
    strncpy(procfs_buffer, default_string, MAX_SIZE - 1);
    procfs_buffer_size = strlen(default_string);

    proc_file = proc_create(PROC_NAME, 0666, NULL, &proc_file_ops);
    if (!proc_file) {
         printk(KERN_ERR "proc_module: failed to create /proc/%s\n", PROC_NAME);
         return -ENOMEM;
    }

    printk(KERN_INFO "proc_module: created /proc/%s\n", PROC_NAME);
    return 0;
}

static void __exit proc_module_exit(void)
{
    if (proc_file) {
        proc_remove(proc_file);
        printk(KERN_INFO "proc_module: Removed /proc/%s\n", PROC_NAME);
    }
     
    printk(KERN_INFO "proc_module: exiting\n");
}

module_init(proc_module_init);
module_exit(proc_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Michael <srp981680@gmail.com>");
MODULE_DESCRIPTION("/proc/my_config");
MODULE_VERSION("1.0");
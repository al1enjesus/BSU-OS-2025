#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("tayjie");
MODULE_DESCRIPTION("Proc module with read/write for lab5");

#define MAX_BUF_SIZE 256

static struct proc_dir_entry *proc_file;
static char proc_data[MAX_BUF_SIZE] = "default";
static size_t data_size = 7; // strlen("default")


static ssize_t proc_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos) {
    if (*ppos > 0)
        return 0;

    if (copy_to_user(ubuf, proc_data, data_size)) {
        return -EFAULT;
    }

    *ppos = data_size;
    return data_size;
}


static ssize_t proc_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos) {
    if (count >= MAX_BUF_SIZE) {
        return -EINVAL;
    }

    if (copy_from_user(proc_data, ubuf, count)) {
        return -EFAULT;
    }

    data_size = count;
    proc_data[data_size] = '\0'; // Ensure null termination

    printk(KERN_INFO "proc_module: wrote '%s'\n", proc_data);
    return count;
}

static const struct proc_ops proc_fops = {
    .proc_read = proc_read,
    .proc_write = proc_write,
};

static int __init proc_init(void) {
    proc_file = proc_create("my_config", 0666, NULL, &proc_fops);
    if (!proc_file) {
        printk(KERN_ERR "proc_module: Failed to create /proc/my_config\n");
        return -ENOMEM;
    }
    
    printk(KERN_INFO "proc_module: loaded, /proc/my_config created\n");
    return 0;
}

static void __exit proc_exit(void) {
    proc_remove(proc_file);
    printk(KERN_INFO "proc_module: unloaded\n");
}

module_init(proc_init);
module_exit(proc_exit);



#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/ctype.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("tayjie");
MODULE_DESCRIPTION("Proc module with read/write for lab5");

#define MAX_BUF_SIZE 256
#define MIN_BUF_SIZE 1

static struct proc_dir_entry *proc_file;
static char *proc_data = NULL;
static size_t data_size = 0;

static bool is_safe_string(const char *str, size_t len) {
    size_t i;
    
    for (i = 0; i < len; i++) {
        if (!isprint(str[i]) && !isspace(str[i])) {
            return false;
        }
    }
    return true;
}


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
    char *new_data;
    char kernel_buf[MAX_BUF_SIZE];
    
    if (count < MIN_BUF_SIZE) {
        printk(KERN_WARNING "proc_module: write too small (%zu bytes)\n", count);
        return -EINVAL;
    }
    
    if (count >= MAX_BUF_SIZE) {
        printk(KERN_WARNING "proc_module: write too large (%zu bytes)\n", count);
        return -EFBIG;
    }

    if (copy_from_user(kernel_buf, ubuf, count)) {
        return -EFAULT;
    }
    kernel_buf[count] = '\0';

    if (!is_safe_string(kernel_buf, count)) {
        printk(KERN_WARNING "proc_module: unsafe characters in input\n");
        return -EINVAL;
    }

    new_data = kmalloc(count + 1, GFP_KERNEL);
    if (!new_data) {
        printk(KERN_ERR "proc_module: kmalloc failed\n");
        return -ENOMEM;
    }

    memcpy(new_data, kernel_buf, count);
    new_data[count] = '\0';

    if (proc_data) {
        kfree(proc_data);
    }
    proc_data = new_data;
    data_size = count;

    printk(KERN_INFO "proc_module: wrote safe data (%zu bytes)\n", data_size);
    return count;
}

static const struct proc_ops proc_fops = {
    .proc_read = proc_read,
    .proc_write = proc_write,
};

static int __init proc_init(void) {
    proc_data = kmalloc(strlen("default") + 1, GFP_KERNEL);
    if (!proc_data) {
        return -ENOMEM;
    }
    strcpy(proc_data, "default");
    data_size = strlen("default");
    
    proc_file = proc_create("my_config", 0644, NULL, &proc_fops);
    if (!proc_file) {
        kfree(proc_data);
        printk(KERN_ERR "proc_module: Failed to create /proc/my_config\n");
        return -ENOMEM;
    }
    
    printk(KERN_INFO "proc_module: loaded, /proc/my_config created\n");
    return 0;
}

static void __exit proc_exit(void) {
    proc_remove(proc_file);
    if (proc_data) {
        kfree(proc_data);
    }
    printk(KERN_INFO "proc_module: unloaded\n");
}

module_init(proc_init);
module_exit(proc_exit);


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Bogdevich Artem");
MODULE_DESCRIPTION("/proc student info module");
MODULE_VERSION("1.0");

#define PROC_NAME "student_info"

static unsigned long load_time;
static unsigned int read_count = 0;
static struct proc_dir_entry *proc_entry;

static ssize_t proc_read(struct file *file, char __user *user_buf, 
                        size_t count, loff_t *ppos)
{
    char *buffer;
    ssize_t len;
    ssize_t ret;
    
    if (*ppos > 0) {
        return 0;
    }
    
    read_count++;
    
    buffer = kmalloc(1024, GFP_KERNEL);
    if (!buffer) {
        return -ENOMEM;
    }
    
    len = snprintf(buffer, 1024,
                  "Name: Bogdevich Artem\n"
                  "Group: 9, Subgroup: 2\n"
                  "Module loaded at: %lu jiffies\n"
                  "Current jiffies: %lu\n"
                  "Read count: %u\n",
                  load_time, jiffies, read_count);
    
    if (len <= 0) {
        kfree(buffer);
        return -EIO;
    }
    
    if (copy_to_user(user_buf, buffer, len)) {
        kfree(buffer);
        return -EFAULT;
    }
    
    *ppos = len;
    kfree(buffer);
    
    printk(KERN_INFO "=== PROC_MODULE: /proc/%s read, count: %u\n", PROC_NAME, read_count);
    return len;
}

static const struct proc_ops proc_fops = {
    .proc_read = proc_read,
};

static int __init proc_init(void)
{
    load_time = jiffies;
    
    proc_entry = proc_create(PROC_NAME, 0444, NULL, &proc_fops);
    if (!proc_entry) {
        printk(KERN_ERR "=== PROC_MODULE: Failed to create /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }
    
    printk(KERN_INFO "=== PROC_MODULE: Created /proc/%s, load time: %lu jiffies\n", 
           PROC_NAME, load_time);
    return 0;
}

static void __exit proc_exit(void)
{
    if (proc_entry) {
        proc_remove(proc_entry);
    }
    
    printk(KERN_INFO "=== PROC_MODULE: Removed /proc/%s, total reads: %u\n", 
           PROC_NAME, read_count);
}

module_init(proc_init);
module_exit(proc_exit);

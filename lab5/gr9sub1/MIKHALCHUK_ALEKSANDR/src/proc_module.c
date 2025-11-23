#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/mutex.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sasha");
MODULE_DESCRIPTION("/proc file with read/write capability");

#define PROC_NAME "my_config"
#define MAX_SIZE 256

static struct proc_dir_entry *proc_file;
static char *config_data;
static size_t config_size;
static DEFINE_MUTEX(config_mutex);

static ssize_t proc_read(struct file *file, char __user *user_buf,
                        size_t count, loff_t *ppos)
{
    ssize_t len;
    mutex_lock(&config_mutex);
    
    if (*ppos > 0) {
        mutex_unlock(&config_mutex);
        return 0;
    }
        
    if (!config_data) {
        mutex_unlock(&config_mutex);
        return 0;
    }
        
    len = strlen(config_data);
    
    if (count < len) {
        mutex_unlock(&config_mutex);
        return -EINVAL;
    }
   
    if (copy_to_user(user_buf, config_data, len)) {
        mutex_unlock(&config_mutex);
        return -EFAULT; 
    }
    
    *ppos = len; 
    mutex_unlock(&config_mutex);
    return len;
}

static ssize_t proc_write(struct file *file, const char __user *user_buf,
                         size_t count, loff_t *ppos)
{
    mutex_lock(&config_mutex);
    
    if (count > MAX_SIZE) {
        printk(KERN_ERR "Write too large: %zu > %d\n", count, MAX_SIZE);
        mutex_unlock(&config_mutex);
        return -EFBIG;
    }
    
    kfree(config_data);
    
    config_data = kmalloc(count + 1, GFP_KERNEL);
    if (!config_data) {
        printk(KERN_ERR "kmalloc failed\n");
        mutex_unlock(&config_mutex);
        return -ENOMEM;
    }
    
    if (copy_from_user(config_data, user_buf, count)) {
        kfree(config_data);
        config_data = NULL;
        mutex_unlock(&config_mutex);
        return -EFAULT;
    }
    
    config_data[count] = '\0';
    config_size = count;
    
    printk(KERN_DEBUG "my_config updated\n");
    mutex_unlock(&config_mutex);
    return count; 
}

static const struct proc_ops proc_fops = {
    .proc_read = proc_read,
    .proc_write = proc_write,
};

static int __init proc_init(void)
{
    config_data = kmalloc(MAX_SIZE, GFP_KERNEL);
    if (!config_data) {
        return -ENOMEM;
    }
    
    strcpy(config_data, "default");
    config_size = strlen(config_data);
    
    proc_file = proc_create(PROC_NAME, 0600, NULL, &proc_fops);
    if (!proc_file) {
        kfree(config_data);
        printk(KERN_ERR "Failed to create /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }
    
    printk(KERN_INFO "/proc/%s created\n", PROC_NAME);
    return 0;
}

static void __exit proc_exit(void)
{
    proc_remove(proc_file);
    kfree(config_data);
    printk(KERN_INFO "/proc/%s removed\n", PROC_NAME);
}

module_init(proc_init);
module_exit(proc_exit);

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/mutex.h>

#define MAX_LEN 256

static char config_data[MAX_LEN] = "default\n";
static size_t config_len = 8;  // длина "default\n"
static DEFINE_MUTEX(config_mutex);

static struct proc_dir_entry *proc_entry;

// Функция чтения из /proc/my_config
static ssize_t proc_module_read(struct file *file, char __user *ubuf,
                            size_t count, loff_t *pos)
{
    mutex_lock(&config_mutex);

    if (*pos >= config_len) {
        mutex_unlock(&config_mutex);
        return 0;  // всё прочитано
    }

    size_t bytes_to_read = min(count, (size_t)(config_len - *pos));
    
    if (copy_to_user(ubuf, config_data + *pos, bytes_to_read)) {
        mutex_unlock(&config_mutex);
        return -EFAULT;
    }

    *pos += bytes_to_read;
    mutex_unlock(&config_mutex);
    return bytes_to_read;
}

// Функция записи в /proc/my_config
static ssize_t proc_module_write(struct file *file, const char __user *ubuf,
                             size_t count, loff_t *pos)
{
    size_t len;
    
    if (count == 0)
        return 0;

    mutex_lock(&config_mutex);

    if (count >= MAX_LEN) {
        mutex_unlock(&config_mutex);
        return -EINVAL;
    }

    if (copy_from_user(config_data, ubuf, count)) {
        mutex_unlock(&config_mutex);
        return -EFAULT;
    }

    len = count;
    
    if (len > 0 && config_data[len-1] == '\n') {
        len--;
    }

    config_data[len] = '\0';
    config_len = len;

    mutex_unlock(&config_mutex);
    
    return count;
}

// Операции для proc-файла
static const struct proc_ops my_proc_fops = {
    .proc_read  = proc_module_read,
    .proc_write = proc_module_write,
};

static int __init proc_module_init(void)
{
    proc_entry = proc_create("my_config", 0666, NULL, &my_proc_fops);
    if (!proc_entry) {
        printk(KERN_ERR "Failed to create /proc/my_config\n");
        return -ENOMEM;
    }

    printk(KERN_INFO "my_config: /proc/my_config created successfully\n");
    return 0;
}

static void __exit proc_module_exit(void)
{
    proc_remove(proc_entry);
    printk(KERN_INFO "my_config: /proc/my_config removed\n"); 
}

module_init(proc_module_init);
module_exit(proc_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vladislav");
MODULE_DESCRIPTION("Proc filesystem example");
MODULE_VERSION("1.0");
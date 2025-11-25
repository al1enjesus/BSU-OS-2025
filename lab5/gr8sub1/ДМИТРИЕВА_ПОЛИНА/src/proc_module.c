#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>

#define PROC_NAME "student_info"
#define MAX_SIZE 1024

// Глобальные переменные
static struct proc_dir_entry *proc_file = NULL;
static int read_count = 0;
static unsigned long load_time = 0;

//Функция чтения из /proc

static ssize_t proc_read(struct file *file, char __user *ubuf,
                         size_t count, loff_t *ppos)
{
    char buf[MAX_SIZE];
    int len;

    if (*ppos > 0)
        return 0;

    read_count++;

    len = snprintf(buf, sizeof(buf),
        "Name: Polina Dmitrieva\n"
        "Group: 8, Subgroup: 1\n"
        "Module loaded at: %lu jiffies\n"
        "Read count: %d\n",
        load_time, read_count);

    if (copy_to_user(ubuf, buf, len))
        return -EFAULT;

    *ppos = len;

    return len;
}

// Структура операций для /proc файла
static const struct proc_ops proc_file_ops = {
    .proc_read = proc_read,
};

//Инициализация модуля

static int __init proc_module_init(void)
{
    printk(KERN_INFO "proc_module: Initializing\n");

    load_time = jiffies;
    read_count = 0;

    proc_file = proc_create(PROC_NAME, 0444, NULL, &proc_file_ops);
    if (!proc_file) {
        printk(KERN_ERR "proc_module: Failed to create /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }

    printk(KERN_INFO "proc_module: Created /proc/%s\n", PROC_NAME);
    return 0;
}

//Выгрузка модуля

static void __exit proc_module_exit(void)
{
    if (proc_file) {
        proc_remove(proc_file);
        proc_file = NULL;
        printk(KERN_INFO "proc_module: Removed /proc/%s\n", PROC_NAME);
    }

    printk(KERN_INFO "proc_module: Exiting\n");
}

module_init(proc_module_init);
module_exit(proc_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dmitrieva Polina");
MODULE_DESCRIPTION("Proc filesystem example");
MODULE_VERSION("1.0");

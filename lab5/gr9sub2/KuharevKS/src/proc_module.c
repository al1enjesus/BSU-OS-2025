/*
 * proc_module.c - Модуль с /proc файлом
 *
 * Создаёт файл /proc/student_info с информацией о студенте
 * и счётчиком обращений.
 *
 * Компиляция: make
 * Использование:
 *   sudo insmod proc_module.ko
 *   cat /proc/student_info
 *   sudo rmmod proc_module
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>
#include <linux/time.h>

#define PROC_NAME "student_info"
#define MAX_SIZE 1024

static struct proc_dir_entry *proc_file = NULL;
static int read_count = 0;
static unsigned long load_time = 0;

static char *student_name = "Kuharev Kirill";
static int group = 9;
static int subgroup = 2;

module_param(student_name, charp, 0644);
MODULE_PARM_DESC(student_name, "Student name");

module_param(group, int, 0644);
MODULE_PARM_DESC(group, "Group number");

module_param(subgroup, int, 0644);
MODULE_PARM_DESC(subgroup, "Subgroup number");

static ssize_t proc_read(struct file *file, char __user *ubuf,
                         size_t count, loff_t *ppos)
{
    char buf[MAX_SIZE];
    int len;
    unsigned long uptime_jiffies;
    unsigned long uptime_seconds;

    if (*ppos > 0)
        return 0;

    read_count++;

    uptime_jiffies = jiffies - load_time;
    uptime_seconds = uptime_jiffies / HZ;

    len = snprintf(buf, sizeof(buf),
        "╔══════════════════════════════════════════════════╗\n"
        "║         Student Information                      ║\n"
        "╠══════════════════════════════════════════════════╣\n"
        "  Name:              %s\n"
        "  Group:             %d\n"
        "  Subgroup:          %d\n"
        "  Module loaded at:  %lu jiffies\n"
        "  Module uptime:     %lu seconds\n"
        "  Read count:        %d\n"
        "  Current jiffies:   %lu\n"
        "╚══════════════════════════════════════════════════╝\n",
        student_name, group, subgroup, load_time, 
        uptime_seconds, read_count, jiffies);

    if (copy_to_user(ubuf, buf, len)) {
        printk(KERN_ERR "proc_module: Failed to copy data to user space\n");
        return -EFAULT;
    }

    *ppos = len;

    printk(KERN_INFO "proc_module: /proc/%s read (count: %d)\n", 
           PROC_NAME, read_count);

    return len;
}

static const struct proc_ops proc_file_ops = {
    .proc_read = proc_read,
};


static int __init proc_module_init(void)
{
    printk(KERN_INFO "proc_module: Initializing\n");

    load_time = jiffies;

    proc_file = proc_create(PROC_NAME, 0444, NULL, &proc_file_ops);

    if (!proc_file) {
        printk(KERN_ERR "proc_module: Failed to create /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }

    printk(KERN_INFO "proc_module: Successfully created /proc/%s\n", PROC_NAME);
    printk(KERN_INFO "proc_module: Student: %s, Group: %d, Subgroup: %d\n",
           student_name, group, subgroup);
    printk(KERN_INFO "proc_module: Load time: %lu jiffies\n", load_time);

    return 0;
}

static void __exit proc_module_exit(void)
{

    if (proc_file) {
        proc_remove(proc_file);
        printk(KERN_INFO "proc_module: Removed /proc/%s\n", PROC_NAME);
    }


    printk(KERN_INFO "proc_module: Module unloaded. Total reads: %d\n", read_count);
}

module_init(proc_module_init);
module_exit(proc_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kirill Kuharev");
MODULE_DESCRIPTION("Proc filesystem");
MODULE_VERSION("1.0");


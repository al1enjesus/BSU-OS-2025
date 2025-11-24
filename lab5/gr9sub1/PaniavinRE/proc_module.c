#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>
#include <linux/sched/signal.h>
#include <linux/sched.h>
#include <linux/mm.h>

#define PROC_NAME "student_info"
#define MAX_SIZE 1024

static struct proc_dir_entry *proc_file;
static int read_count = 0;
static unsigned long load_time;

static ssize_t proc_read(struct file *file, char __user *ubuf,
                         size_t count, loff_t *ppos)
{
    char buf[MAX_SIZE];
    int len = 0;
    struct sysinfo si;
    int proc_count = 0;
    struct task_struct *task;

    if (*ppos > 0)
        return 0;

    read_count++;

    // Считаем процессы
    for_each_process(task) {
        proc_count++;
    }

    // Память
    si_meminfo(&si);
    unsigned long total_mb = si.totalram * si.mem_unit / 1024 / 1024;
    unsigned long free_mb  = si.freeram  * si.mem_unit / 1024 / 1024;
    unsigned long used_mb  = total_mb - free_mb;

    // Uptime
    unsigned long uptime_sec = jiffies_to_msecs(jiffies - INITIAL_JIFFIES) / 1000;

    len = snprintf(buf, MAX_SIZE,
        "Name: Paniavin Raman Yaugenavich\n"
        "Group: 9, Subgroup: 1\n"
        "Variant: 1 (нечётный)\n"
        "Module loaded at: %lu jiffies\n"
        "Read count: %d\n"
        "\n"
        "=== System Statistics ===\n"
        "Running processes: %d\n"
        "Total RAM: %lu MB\n"
        "Used RAM: %lu MB\n"
        "Free RAM: %lu MB\n"
        "System uptime: %lu seconds\n",
        load_time, read_count,
        proc_count,
        total_mb, used_mb, free_mb,
        uptime_sec);

    if (len >= MAX_SIZE) {
            len = MAX_SIZE - 1; 
  }

    if (copy_to_user(ubuf, buf, len))
        return -EFAULT;

    *ppos = len;
    return len;
}

static const struct proc_ops proc_fops = {
    .proc_read = proc_read,
};

static int __init proc_module_init(void)
{
    load_time = jiffies;

    proc_file = proc_create(PROC_NAME, 0444, NULL, &proc_fops);
    if (!proc_file) {
        printk(KERN_ERR "proc_module: Failed to create /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }

    printk(KERN_INFO "proc_module: /proc/student_info created (Variant 1)\n");
    return 0;
}

static void __exit proc_module_exit(void)
{
    if (proc_file)
        proc_remove(proc_file);
    printk(KERN_INFO "proc_module: Module unloaded\n");
}

module_init(proc_module_init);
module_exit(proc_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Paniavin Raman");
MODULE_DESCRIPTION("Proc module with system stats - Variant 1");
MODULE_VERSION("1.0");

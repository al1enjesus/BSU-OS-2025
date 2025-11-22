#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/fs.h>
#include <linux/limits.h>

#define PROCFS_NAME "sys_stats"
#define SYS_STATS_BUFFER_SIZE 512
#define MB_DIVISOR (1024 * 1024)
#define SECONDS_IN_MS 1000
#define MAX_MEMORY_MB (ULONG_MAX / MB_DIVISOR)

static struct proc_dir_entry *proc_file;

static int count_processes(void)
{
    struct task_struct *task;
    int count = 0;
    rcu_read_lock();
    for_each_process(task)
    {
        count++;
    }
    rcu_read_unlock();
    return count;
}

static ssize_t proc_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
    char buffer[SYS_STATS_BUFFER_SIZE];
    int len;
    struct sysinfo si;
    unsigned long uptime_seconds;
    int process_count;
    unsigned long memory_used_mb;
    unsigned long pages_used;
    
    if (*ppos > 0)
        return 0;
    
    si_meminfo(&si);
    uptime_seconds = jiffies_to_msecs(get_jiffies_64()) / SECONDS_IN_MS;
    process_count = count_processes();
    if (si.totalram < si.freeram)
    {
        memory_used_mb = 0;
    }
    else
    {
        pages_used = si.totalram - si.freeram;
        if (pages_used > ULONG_MAX / si.mem_unit)
        {
            memory_used_mb = ULONG_MAX / MB_DIVISOR;
        }
        else
        {
            memory_used_mb = (pages_used * si.mem_unit) / MB_DIVISOR;
        }
    }
    len = snprintf(buffer, sizeof(buffer),
        "Processes: %d\n"
        "Memory Used: %lu MB\n"
        "System Uptime: %lu seconds\n",
        process_count,
        memory_used_mb,
        uptime_seconds);
    
    if (copy_to_user(ubuf, buffer, len))
        return -EFAULT;
    
    *ppos = len;
    return len;
}

static const struct proc_ops proc_fops = {
    .proc_read = proc_read,
};

static int __init stats_init(void)
{
    proc_file = proc_create(PROCFS_NAME, 0444, NULL, &proc_fops);
    if (!proc_file)
    {
        printk(KERN_ERR "Failed to create /proc/%s\n", PROCFS_NAME);
        return -ENOMEM;
    }
    printk(KERN_INFO "/proc/%s created\n", PROCFS_NAME);
    return 0;
}

static void __exit stats_exit(void)
{
    proc_remove(proc_file);
    printk(KERN_INFO "/proc/%s removed\n", PROCFS_NAME);
}

module_init(stats_init);
module_exit(stats_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Artem");
MODULE_DESCRIPTION("System statistics in /proc");
MODULE_VERSION("1.0");

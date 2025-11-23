#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/ktime.h>
#include <linux/sysinfo.h>
#include <linux/mm.h>
#include <linux/sched/signal.h>

#define PROC_NAME "sys_stats"
#define MAX_SIZE 1024

unsigned int count_processes(void) {
    struct task_struct *task;
    unsigned int count = 0;
    
    rcu_read_lock();
    for_each_process(task) {
        count++;
    }
    rcu_read_unlock();
    return count;
}

unsigned long get_memory_usage(void) {
    struct sysinfo si;
    long total_mem_mib, free_mem_mib, used_mem_mib;
    
    si_meminfo(&si);
    // totalram & freeram are expressed in "memory units",
    // which can be converted to bytes via mem_unit and then
    // to megabytes by diving by 1024 * 1024 (bytes in 1 megabyte)
    total_mem_mib = si.totalram * si.mem_unit / (1024 * 1024);
    free_mem_mib = si.freeram * si.mem_unit / (1024 * 1024);
    // used = total - free
    used_mem_mib = total_mem_mib - free_mem_mib; 

    return used_mem_mib;
}

long get_system_uptime(void) {
    struct timespec64 uptime;
    ktime_get_boottime_ts64(&uptime);
    return uptime.tv_sec;
}

static struct proc_dir_entry *proc_file = NULL;
static ssize_t proc_read(struct file *file, char __user *ubuf,
                         size_t count, loff_t *ppos)
{
    char buf[MAX_SIZE];
    int len;

    if (*ppos > 0)
        return 0;

    unsigned int processes = count_processes();
    unsigned long used_mem = get_memory_usage();
    long uptime_seconds = get_system_uptime();

    len = snprintf(buf, sizeof(buf),
        "Processes: %u\n"
        "Memory used: %lu MB\n"
        "System uptime: %ld seconds\n", processes, used_mem, uptime_seconds);

    if (copy_to_user(ubuf, buf, len))
        return -EFAULT;
    *ppos = len;

    return len;
}

static const struct proc_ops proc_file_ops = {
    .proc_read = proc_read,
};

static int __init proc_stats_module_init(void)
{
    printk(KERN_INFO "proc_stats_module: initializing\n");

    proc_file = proc_create(PROC_NAME, 0444, NULL, &proc_file_ops);
    if (!proc_file) {
        printk(KERN_ERR "proc_stats_module: failed to create /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }

    printk(KERN_INFO "proc_stats_module: created /proc/%s\n", PROC_NAME);
    return 0;
}

static void __exit proc_stats_module_exit(void)
{
    if (proc_file) {
        proc_remove(proc_file);
        printk(KERN_INFO "proc_stats_module: removed /proc/%s\n", PROC_NAME);
    }

    printk(KERN_INFO "proc_stats_module: exiting\n");
}

module_init(proc_stats_module_init);
module_exit(proc_stats_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Michael <srp981680@gmail.com>");
MODULE_DESCRIPTION("/proc/sys_stats");
MODULE_VERSION("1.0");
// sys_stats.c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sched/signal.h>
#include <linux/jiffies.h>
#include <linux/mm.h> 
#include <linux/mmzone.h>
#include <linux/uaccess.h>

#define PROC_NAME "sys_stats"

static int sys_stats_show(struct seq_file *m, void *v)
{
    struct task_struct *task;
    unsigned long proc_count = 0;

    rcu_read_lock();
    for_each_process(task)
        proc_count++;
    rcu_read_unlock();

    struct sysinfo si;
    si_meminfo(&si);

    unsigned long used_pages = (si.totalram - si.freeram - si.bufferram);
    unsigned long used_bytes = used_pages * (unsigned long)PAGE_SIZE;
    unsigned long used_mb = used_bytes / (1024 * 1024);

    unsigned long uptime_ms = jiffies_to_msecs(jiffies);
    unsigned long uptime_s = uptime_ms / 1000;

    seq_printf(m,
               "Processes: %lu\n"
               "Memory Used: %lu MB\n"
               "System Uptime: %lu seconds\n",
               proc_count, used_mb, uptime_s);

    return 0;
}

static int sys_stats_open(struct inode *inode, struct file *file)
{
    return single_open(file, sys_stats_show, NULL);
}

static const struct proc_ops sys_stats_fops = {
    .proc_open    = sys_stats_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

static int __init sys_stats_init(void)
{
    if (!proc_create(PROC_NAME, 0444, NULL, &sys_stats_fops)) {
        pr_err("Failed to create /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }
    pr_info("/proc/%s created\n", PROC_NAME);
    return 0;
}

static void __exit sys_stats_exit(void)
{
    proc_remove(proc_create(PROC_NAME, 0444, NULL, &sys_stats_fops));
    remove_proc_entry(PROC_NAME, NULL);
    pr_info("/proc/%s removed\n", PROC_NAME);
}

module_init(sys_stats_init);
module_exit(sys_stats_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Чипурко Роман");
MODULE_DESCRIPTION("/proc/sys_stats: processes, memory used, uptime");
MODULE_VERSION("1.0");

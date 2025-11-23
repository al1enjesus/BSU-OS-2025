#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/sysinfo.h>
#include <linux/sched/signal.h>
#include <linux/rcupdate.h>

#define PROC_NAME "sys_stats"
#define BUF_SIZE  256

static struct proc_dir_entry *proc_file;

static unsigned int count_processes(void)
{
    struct task_struct *task;
    unsigned int count = 0;

    rcu_read_lock();
    for_each_process(task)
        count++;
    rcu_read_unlock();

    return count;
}

static void get_meminfo_mb(unsigned long *used_mb, unsigned long *total_mb)
{
    struct sysinfo si;
    unsigned long long total_bytes, free_bytes;

    si_meminfo(&si);

    total_bytes = (unsigned long long)si.totalram * si.mem_unit;
    free_bytes  = (unsigned long long)si.freeram * si.mem_unit;

    *total_mb = (unsigned long)(total_bytes >> 20);
    *used_mb  = (unsigned long)((total_bytes - free_bytes) >> 20);
}

static unsigned long get_uptime_seconds(void)
{
    u64 j = get_jiffies_64();
    return jiffies_to_msecs(j) / 1000;
}

static ssize_t stats_read(struct file *file, char __user *ubuf,
                          size_t count, loff_t *ppos)
{
    char buf[BUF_SIZE];
    int len;
    unsigned int proc_count;
    unsigned long used_mb, total_mb;
    unsigned long uptime;

    if (*ppos > 0)
        return 0;

    proc_count = count_processes();
    get_meminfo_mb(&used_mb, &total_mb);
    uptime = get_uptime_seconds();

    len = scnprintf(buf, sizeof(buf),
                    "Processes: %u\n"
                    "Memory Used: %lu MB / %lu MB\n"
                    "System Uptime: %lu seconds\n",
                    proc_count, used_mb, total_mb, uptime);

    if (len > count)
        len = count;

    if (copy_to_user(ubuf, buf, len))
        return -EFAULT;

    *ppos += len;
    return len;
}

static const struct proc_ops stats_ops = {
    .proc_read = stats_read,
};

static int __init sys_stats_init(void)
{
    proc_file = proc_create(PROC_NAME, 0444, NULL, &stats_ops);
    if (!proc_file) {
        printk(KERN_ERR "sys_stats: Failed to create /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }

    printk(KERN_INFO "sys_stats: /proc/%s created\n", PROC_NAME);
    return 0;
}

static void __exit sys_stats_exit(void)
{
    if (proc_file) {
        proc_remove(proc_file);
        printk(KERN_INFO "sys_stats: /proc/%s removed\n", PROC_NAME);
    }
}

module_init(sys_stats_init);
module_exit(sys_stats_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("hannahuzova");
MODULE_DESCRIPTION("/proc/sys_stats module");
MODULE_VERSION("1.0");

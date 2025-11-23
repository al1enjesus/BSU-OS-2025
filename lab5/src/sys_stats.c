#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/sched/signal.h>
#include <linux/mm.h>
#include <linux/jiffies.h>
#include <linux/sysinfo.h>

#define PROC_NAME "sys_stats"
#define BUF_SIZE 512

static struct proc_dir_entry *proc_entry;

static ssize_t sys_stats_read(struct file *file, char __user *ubuf,
                              size_t count, loff_t *ppos)
{
    char kbuf[BUF_SIZE];
    int len = 0;
    struct task_struct *task;
    unsigned long proc_count = 0;

    struct sysinfo info;
    unsigned long used_mem_mb;
    unsigned long uptime_sec;

    if (*ppos > 0)
        return 0;

    for_each_process(task) {
        proc_count++;
    }

    si_meminfo(&info);
    used_mem_mb = (info.totalram - info.freeram);
    used_mem_mb = used_mem_mb * info.mem_unit / (1024 * 1024);

    uptime_sec = jiffies_to_msecs(jiffies) / 1000;

    len = scnprintf(kbuf, sizeof(kbuf),
                    "Processes: %lu\n"
                    "Memory Used: %lu MB\n"
                    "System Uptime: %lu seconds\n",
                    proc_count,
                    used_mem_mb,
                    uptime_sec);

    if (len > count)
        len = count;

    if (copy_to_user(ubuf, kbuf, len))
        return -EFAULT;

    *ppos = len;
    return len;
}

static const struct proc_ops sys_stats_ops = {
    .proc_read = sys_stats_read,
};

static int __init sys_stats_init(void)
{
    proc_entry = proc_create(PROC_NAME, 0444, NULL, &sys_stats_ops);
    if (!proc_entry) {
        printk(KERN_ERR "sys_stats: failed to create /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }

    printk(KERN_INFO "sys_stats: module loaded\n");
    return 0;
}

static void __exit sys_stats_exit(void)
{
    if (proc_entry)
        proc_remove(proc_entry);

    printk(KERN_INFO "sys_stats: module unloaded\n");
}

module_init(sys_stats_init);
module_exit(sys_stats_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Koscov Artem");
MODULE_DESCRIPTION("Lab5: /proc/sys_stats with system information");
MODULE_VERSION("1.0");

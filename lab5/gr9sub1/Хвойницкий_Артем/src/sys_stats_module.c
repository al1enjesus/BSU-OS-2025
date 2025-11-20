#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/mm.h>
#include <linux/jiffies.h>
#include <linux/sched/signal.h>

#define PROC_NAME_STATS "sys_stats"

static struct proc_dir_entry* proc_stats_entry;

static int sys_stats_show(struct seq_file* m, void* v)
{
    struct sysinfo info;
    unsigned long used_mem_mb;
    unsigned long uptime_sec;
    int proc_count = 0;
    struct task_struct* task;

    for_each_process(task) {
        proc_count++;
    }

    si_meminfo(&info);
    used_mem_mb = (info.totalram - info.freeram) * info.mem_unit / (1024 * 1024);

    uptime_sec = jiffies_to_msecs(jiffies) / 1000;

    seq_printf(m, "Processes: %d\n", proc_count);
    seq_printf(m, "Memory Used: %lu MB\n", used_mem_mb);
    seq_printf(m, "System Uptime: %lu seconds\n", uptime_sec);

    return 0;
}

static int sys_stats_open(struct inode* inode, struct file* file)
{
    return single_open(file, sys_stats_show, NULL);
}

static const struct proc_ops sys_stats_fops = {
    .proc_open = sys_stats_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

static int __init sys_stats_init(void)
{
    proc_stats_entry = proc_create(PROC_NAME_STATS, 0444, NULL, &sys_stats_fops);
    if (!proc_stats_entry) {
        printk(KERN_ERR "sys_stats_module: failed to create /proc/%s\n", PROC_NAME_STATS);
        return -ENOMEM;
    }

    printk(KERN_INFO "sys_stats_module: /proc/%s created\n", PROC_NAME_STATS);
    return 0;
}

static void __exit sys_stats_exit(void)
{
    proc_remove(proc_stats_entry);
    printk(KERN_INFO "sys_stats_module: /proc/%s removed\n", PROC_NAME_STATS);
}

module_init(sys_stats_init);
module_exit(sys_stats_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Artsem Khvainitski");
MODULE_DESCRIPTION("/proc/sys_stats system statistics module");

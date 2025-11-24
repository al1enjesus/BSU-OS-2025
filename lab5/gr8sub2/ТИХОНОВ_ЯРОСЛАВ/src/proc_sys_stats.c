// proc_sys_stats.c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sched/signal.h> /* for_each_process */
#include <linux/mm.h>           /* si_meminfo */
#include <linux/jiffies.h>

#define PROC_NAME "sys_stats"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ярослав");
MODULE_DESCRIPTION("/proc file providing simple system statistics");

static int sys_stats_show(struct seq_file *m, void *v)
{
    struct task_struct *task;
    unsigned long proc_count = 0;
    struct sysinfo i;
    unsigned long used_mb;
    unsigned long uptime_ms;
    unsigned long uptime_s;

    /* count processes */
    rcu_read_lock();
    for_each_process(task) {
        proc_count++;
    }
    rcu_read_unlock();

    /* memory info */
    si_meminfo(&i);
    /* totalram and freeram are in pages */
    used_mb = ((i.totalram - i.freeram) * (unsigned long)PAGE_SIZE) / (1024 * 1024);

    /* uptime in ms from jiffies */
    uptime_ms = jiffies_to_msecs(jiffies);
    uptime_s = uptime_ms / 1000;

    seq_printf(m, "Processes: %lu\n", proc_count);
    seq_printf(m, "Memory Used: %lu MB\n", used_mb);
    seq_printf(m, "System Uptime: %lu seconds\n", uptime_s);

    return 0;
}

static int sys_stats_open(struct inode *inode, struct file *file)
{
    return single_open(file, sys_stats_show, NULL);
}

static const struct proc_ops sys_stats_fops = {
    .proc_open  = sys_stats_open,
    .proc_read  = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

static struct proc_dir_entry *proc_entry;

static int __init proc_sys_stats_init(void)
{
    proc_entry = proc_create(PROC_NAME, 0444, NULL, &sys_stats_fops);
    if (!proc_entry) {
        printk(KERN_ERR "proc_sys_stats: failed to create /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }

    printk(KERN_INFO "proc_sys_stats: /proc/%s created\n", PROC_NAME);
    return 0;
}

static void __exit proc_sys_stats_exit(void)
{
    proc_remove(proc_entry);
    printk(KERN_INFO "proc_sys_stats: /proc/%s removed\n", PROC_NAME);
}

module_init(proc_sys_stats_init);
module_exit(proc_sys_stats_exit);

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>
#include <linux/sysinfo.h>

#define BUFFER_SIZE 512

static struct proc_dir_entry *proc_entry;
static char stats_buffer[BUFFER_SIZE];
static size_t stats_len;

static void update_stats(void)
{
    struct sysinfo si;
    unsigned long uptime_secs;
    long mem_used_mb;
    int proc_count = 0;
    struct task_struct *p;

    for_each_process(p) {
        proc_count++;
    }

    si_meminfo(&si);
    mem_used_mb = (si.totalram - si.freeram - si.bufferram - si.sharedram);
    mem_used_mb *= si.mem_unit;
    mem_used_mb /= (1024 * 1024);   

    uptime_secs = jiffies_to_msecs(get_jiffies_64()) / 1000;

    stats_len = scnprintf(stats_buffer, BUFFER_SIZE,
                          "Processes:      %d\n"
                          "Memory Used:    %ld MB\n"
                          "System Uptime:  %lu seconds\n",
                          proc_count, mem_used_mb, uptime_secs);
}

static ssize_t sys_stats_read(struct file *file, char __user *user_buf,
                              size_t count, loff_t *ppos)
{
    update_stats(); 
    return simple_read_from_buffer(user_buf, count, ppos, stats_buffer, stats_len);
}

static const struct proc_ops stats_fops = {
    .proc_read = sys_stats_read,
};

static int __init sys_stats_init(void)
{
    proc_entry = proc_create("sys_stats", 0444, NULL, &stats_fops);
    if (!proc_entry) {
        printk(KERN_ERR "sys_stats: failed to create /proc/sys_stats\n");
        return -ENOMEM;
    }

    printk(KERN_INFO "sys_stats: /proc/sys_stats created (simple version)\n");
    return 0;
}

static void __exit sys_stats_exit(void)
{
    proc_remove(proc_entry);
    printk(KERN_INFO "sys_stats: module unloaded\n");
}

module_init(sys_stats_init);
module_exit(sys_stats_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vladislav");
MODULE_DESCRIPTION("System stats via /proc/sys_stats using simple_read_from_buffer");
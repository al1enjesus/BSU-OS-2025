
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("tayjie");
MODULE_DESCRIPTION("System stats module for lab5");

static struct proc_dir_entry *proc_stats;
static int count_processes(void) {
    struct task_struct *task;
    int count = 0;
    
    rcu_read_lock();
    for_each_process(task) {
        count++;
    }
    rcu_read_unlock();
    
    return count;
}

static ssize_t stats_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos) {
    char buf[512];
    int len;
    struct sysinfo si;
    
    if (*ppos > 0)
        return 0;

    si_meminfo(&si);
    

    len = snprintf(buf, sizeof(buf),
        "Processes: %d\n"
        "Memory Used: %lu MB\n"
        "System Uptime: %u seconds\n", 
        count_processes(),
        (si.totalram - si.freeram) * si.mem_unit / (1024 * 1024),
        (unsigned int)(jiffies_to_msecs(get_jiffies_64()) / 1000)  
    );

    if (copy_to_user(ubuf, buf, len)) {
        return -EFAULT;
    }

    *ppos = len;
    return len;
}

static const struct proc_ops stats_fops = {
    .proc_read = stats_read,
};

static int __init stats_init(void) {
    proc_stats = proc_create("sys_stats", 0444, NULL, &stats_fops);
    if (!proc_stats) {
        printk(KERN_ERR "stats_module: Failed to create /proc/sys_stats\n");
        return -ENOMEM;
    }
    
    printk(KERN_INFO "stats_module: loaded, /proc/sys_stats created\n");
    return 0;
}

static void __exit stats_exit(void) {
    proc_remove(proc_stats);
    printk(KERN_INFO "stats_module: unloaded\n");
}

module_init(stats_init);
module_exit(stats_exit);


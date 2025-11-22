#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/jiffies.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sasha");
MODULE_DESCRIPTION("System statistics in /proc");

#define PROC_NAME "sys_stats"

static struct proc_dir_entry *proc_file;


static int count_processes(void)
{
    struct task_struct *task;
    int count = 0;
    
  
    rcu_read_lock();
    for_each_process(task) {
        count++;
    }
    rcu_read_unlock();
    
    return count;
}


static ssize_t stats_read(struct file *file, char __user *user_buf,
                         size_t count, loff_t *ppos)
{
    char buffer[512];
    int len;
    int process_count;
    struct sysinfo mem_info;
    unsigned long uptime_seconds;
    
    if (*ppos > 0)
        return 0;
    
 
    si_meminfo(&mem_info);
    process_count = count_processes();
    uptime_seconds = jiffies_to_msecs(jiffies) / 1000;
    
 
    len = snprintf(buffer, sizeof(buffer),
                  "System Statistics:\n"
                  "Processes: %d\n"
                  "Memory Total: %lu MB\n"
                  "Memory Free: %lu MB\n"
                  "Memory Used: %lu MB\n"
                  "System Uptime: %lu seconds\n",
                  process_count,
                  (mem_info.totalram * mem_info.mem_unit) / (1024 * 1024),
                  (mem_info.freeram * mem_info.mem_unit) / (1024 * 1024),
                  ((mem_info.totalram - mem_info.freeram) * mem_info.mem_unit) / (1024 * 1024),
                  uptime_seconds);
    

    if (copy_to_user(user_buf, buffer, len)) {
        return -EFAULT;
    }
    
    *ppos = len;
    return len;
}

static const struct proc_ops stats_fops = {
    .proc_read = stats_read,
};

static int __init stats_init(void)
{
    proc_file = proc_create(PROC_NAME, 0444, NULL, &stats_fops);
    if (!proc_file) {
        printk(KERN_ERR "Failed to create /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }
    
    printk(KERN_INFO "/proc/%s created\n", PROC_NAME);
    return 0;
}

static void __exit stats_exit(void)
{
    proc_remove(proc_file);
    printk(KERN_INFO "/proc/%s removed\n", PROC_NAME);
}

module_init(stats_init);
module_exit(stats_exit);

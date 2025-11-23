#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/sched.h>
#include <linux/mm.h>

#define PROC_NAME "sys_stats"
#define BUFFER_SIZE 1024

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

static unsigned long get_memory_used(void)
{
    struct sysinfo meminfo;
    si_meminfo(&meminfo);
    
    // Приблизительный расчет используемой памяти в МБ
    unsigned long used_mem = (meminfo.totalram - meminfo.freeram);
    used_mem *= meminfo.mem_unit;
    used_mem /= 1024 * 1024; // Convert to MB
    
    return used_mem;
}

static unsigned long get_uptime_seconds(void)
{
    return jiffies_to_msecs(get_jiffies_64()) / 1000;
}

static ssize_t proc_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
    char buffer[BUFFER_SIZE];
    int len;
    int processes;
    unsigned long memory_used;
    unsigned long uptime_seconds;

    if (*ppos > 0)
        return 0;

    // Получение статистики
    processes = count_processes();
    memory_used = get_memory_used();
    uptime_seconds = get_uptime_seconds();

    // Форматирование вывода
    len = snprintf(buffer, BUFFER_SIZE,
                  "Processes: %d\n"
                  "Memory Used: %lu MB\n"
                  "System Uptime: %lu seconds\n",
                  processes, memory_used, uptime_seconds);

    if (copy_to_user(ubuf, buffer, len)) {
        return -EFAULT;
    }

    *ppos = len;
    return len;
}

static const struct proc_ops proc_ops = {
    .proc_read = proc_read,
};

static int __init sys_stats_init(void)
{
    proc_file = proc_create(PROC_NAME, 0444, NULL, &proc_ops);
    if (!proc_file) {
        printk(KERN_ERR "Failed to create /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }

    printk(KERN_INFO "/proc/%s created successfully\n", PROC_NAME);
    return 0;
}

static void __exit sys_stats_exit(void)
{
    proc_remove(proc_file);
    printk(KERN_INFO "/proc/%s removed\n", PROC_NAME);
}

module_init(sys_stats_init);
module_exit(sys_stats_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Daniil");
MODULE_DESCRIPTION("System statistics in /proc");
MODULE_VERSION("1.0");

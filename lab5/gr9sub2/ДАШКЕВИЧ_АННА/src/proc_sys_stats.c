#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>
#include <linux/sched/signal.h>  // for_each_process
#include <linux/mm.h>            // si_meminfo
#include <linux/slab.h>

#define PROC_NAME "sys_stats"

static struct proc_dir_entry *proc_file;

/*
 * Функция чтения: cat /proc/sys_stats
 */
static ssize_t proc_read(struct file *file, char __user *ubuf,
                         size_t count, loff_t *ppos)
{
    char *kbuf;
    int len = 0;
    struct sysinfo i;
    unsigned long uptime_sec;
    unsigned long total_procs = 0;
    struct task_struct *task;

    /* Чтобы не печатать снова и снова при одном cat */
    if (*ppos > 0)
        return 0;

    /* Считаем количество процессов */
    for_each_process(task) {
        total_procs++;
    }

    /* Получаем инфу о памяти */
    si_meminfo(&i);
    /*
     * i.totalram, i.freeram, i.bufferram и т.д. — в страницах.
     * Переведём в МБ примерно.
     */
    unsigned long total_ram_mb = (i.totalram * 4) / 1024;   // при PAGE_SIZE=4096
    unsigned long free_ram_mb  = (i.freeram * 4) / 1024;
    unsigned long used_ram_mb  = total_ram_mb - free_ram_mb;

    /* Uptime через jiffies */
    uptime_sec = jiffies_to_msecs(get_jiffies_64()) / 1000;

    /* Выделяем временный буфер в kernel-space */
    kbuf = kmalloc(512, GFP_KERNEL);
    if (!kbuf)
        return -ENOMEM;

    len = snprintf(kbuf, 512,
                   "Processes: %lu\n"
                   "Memory Used: %lu MB\n"
                   "System Uptime: %lu seconds\n",
                   total_procs,
                   used_ram_mb,
                   uptime_sec);

    if (len < 0) {
        kfree(kbuf);
        return -EFAULT;
    }

    if (len > count) {
        kfree(kbuf);
        return -EINVAL; // буфер юзера слишком маленький
    }

    if (copy_to_user(ubuf, kbuf, len)) {
        kfree(kbuf);
        return -EFAULT;
    }

    kfree(kbuf);
    *ppos = len;
    return len;
}

static const struct proc_ops proc_file_ops = {
    .proc_read = proc_read,
};

static int __init proc_sys_stats_init(void)
{
    proc_file = proc_create(PROC_NAME, 0444, NULL, &proc_file_ops);
    if (!proc_file) {
        printk(KERN_ERR "proc_sys_stats: failed to create /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }

    printk(KERN_INFO "proc_sys_stats: /proc/%s created\n", PROC_NAME);
    return 0;
}

static void __exit proc_sys_stats_exit(void)
{
    if (proc_file) {
        proc_remove(proc_file);
        printk(KERN_INFO "proc_sys_stats: /proc/%s removed\n", PROC_NAME);
    }
}

module_init(proc_sys_stats_init);
module_exit(proc_sys_stats_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Anya");
MODULE_DESCRIPTION("/proc/sys_stats: simple system statistics");
MODULE_VERSION("1.0");
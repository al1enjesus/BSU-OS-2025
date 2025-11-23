#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>
#include <linux/sched/signal.h>
#include <linux/mm.h>
#include <linux/slab.h>

#define PROC_NAME  "sys_stats"
#define KBUF_SIZE  512

static struct proc_dir_entry *proc_file;

static char *kbuf;
static int   kbuf_len;

static ssize_t proc_read(struct file *file, char __user *ubuf,
                         size_t count, loff_t *ppos)
{
    size_t to_copy;
    struct sysinfo i;
    unsigned long uptime_sec;
    unsigned long total_procs = 0;
    struct task_struct *task;

    if (!ubuf)
        return -EFAULT;

    if (*ppos < 0)
        return -EINVAL;

    if (*ppos == 0) {
        rcu_read_lock();
        for_each_process(task) {
            total_procs++;
        }
        rcu_read_unlock();

        si_meminfo(&i);
        {
            unsigned long total_ram_mb = (i.totalram * i.mem_unit) >> 20;
            unsigned long free_ram_mb  = (i.freeram  * i.mem_unit) >> 20;
            unsigned long used_ram_mb  = total_ram_mb - free_ram_mb;

            uptime_sec = jiffies_to_msecs(get_jiffies_64()) / 1000;

            if (!kbuf) {
                pr_err("proc_sys_stats: kbuf is NULL in proc_read\n");
                return -ENOMEM;
            }

            kbuf_len = snprintf(kbuf, KBUF_SIZE,
                                "Processes: %lu\n"
                                "Memory Used: %lu MB\n"
                                "System Uptime: %lu seconds\n",
                                total_procs,
                                used_ram_mb,
                                uptime_sec);

            if (kbuf_len < 0) {
                pr_err("proc_sys_stats: snprintf failed\n");
                kbuf_len = 0;
                return -EFAULT;
            }

        }
    }

    if (!kbuf || kbuf_len <= 0)
        return 0;

    if (*ppos >= kbuf_len)
        return 0;   

    to_copy = min_t(size_t, count, kbuf_len - *ppos);

    if (copy_to_user(ubuf, kbuf + *ppos, to_copy))
        return -EFAULT;

    *ppos += to_copy;
    return to_copy;
}

static const struct proc_ops proc_file_ops = {
    .proc_read = proc_read,
};

static int __init proc_sys_stats_init(void)
{
    kbuf     = NULL;
    kbuf_len = 0;

    kbuf = kmalloc(KBUF_SIZE, GFP_KERNEL);
    if (!kbuf) {
        pr_err("proc_sys_stats: failed to allocate kbuf\n");
        return -ENOMEM;
    }

    proc_file = proc_create(PROC_NAME, 0444, NULL, &proc_file_ops);
    if (!proc_file) {
        pr_err("proc_sys_stats: failed to create /proc/%s\n", PROC_NAME);
        kfree(kbuf);
        kbuf = NULL;
        return -ENOMEM;
    }

    pr_info("proc_sys_stats: /proc/%s created\n", PROC_NAME);
    return 0;
}

static void __exit proc_sys_stats_exit(void)
{
    if (proc_file) {
        proc_remove(proc_file);
        proc_file = NULL;
        pr_info("proc_sys_stats: /proc/%s removed\n", PROC_NAME);
    }

    if (kbuf) {
        kfree(kbuf);
        kbuf = NULL;
        kbuf_len = 0;
    }
}

module_init(proc_sys_stats_init);
module_exit(proc_sys_stats_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Anya");
MODULE_DESCRIPTION("/proc/sys_stats: simple system statistics (partial read supported)");
MODULE_VERSION("1.3");
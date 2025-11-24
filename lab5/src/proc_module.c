#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>
#include <linux/mm.h>
#include <linux/sched/signal.h>
#include <linux/seq_file.h>

#define MAX_LEN 256

static char my_config_buf[MAX_LEN] = "default";

static struct proc_dir_entry *proc_my_config;
static struct proc_dir_entry *proc_sys_stats;

// Чтение из /proc/my_config
static ssize_t my_config_read(struct file *file, char __user *buf, size_t count, loff_t *ppos) {
    int len = strlen(my_config_buf);

    if (*ppos >= len)
        return 0;

    if (count > len - *ppos)
        count = len - *ppos;

    if (copy_to_user(buf, my_config_buf + *ppos, count))
        return -EFAULT;

    *ppos += count;
    return count;
}

// Запись в /proc/my_config
static ssize_t my_config_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos) {
    if (count > MAX_LEN - 1)
        return -EINVAL;

    if (copy_from_user(my_config_buf, buf, count))
        return -EFAULT;

    my_config_buf[count] = '\0';

    // Удаляем возможный \n в конце
    if (count > 0 && my_config_buf[count - 1] == '\n')
        my_config_buf[count - 1] = '\0';

    return count;
}

static const struct proc_ops my_config_fops = {
    .proc_read = my_config_read,
    .proc_write = my_config_write,
};

// Чтение из /proc/sys_stats
static int sys_stats_show(struct seq_file *m, void *v) {
    struct sysinfo info;
    unsigned long uptime_secs;
    int process_count = 0;
    struct task_struct *task;

    // Кол-во процессов
    for_each_process(task) {
        process_count++;
    }

    // Инфо о памяти
    si_meminfo(&info);

    // Uptime (jiffies to seconds)
    uptime_secs = jiffies_to_msecs(get_jiffies_64()) / 1000;

    seq_printf(m, "Processes: %d\n", process_count);
    seq_printf(m, "Memory Used: %lu MB\n", (info.totalram - info.freeram) * info.mem_unit / (1024 * 1024));
    seq_printf(m, "System Uptime: %lu seconds\n", uptime_secs);

    return 0;
}

static int sys_stats_open(struct inode *inode, struct file *file) {
    return single_open(file, sys_stats_show, NULL);
}

static const struct proc_ops sys_stats_fops = {
    .proc_open = sys_stats_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

static int __init proc_module_init(void) {
    proc_my_config = proc_create("my_config", 0666, NULL, &my_config_fops);
    if (!proc_my_config) {
        printk(KERN_ERR "Failed to create /proc/my_config\n");
        return -ENOMEM;
    }

    proc_sys_stats = proc_create("sys_stats", 0444, NULL, &sys_stats_fops);
    if (!proc_sys_stats) {
        proc_remove(proc_my_config);
        printk(KERN_ERR "Failed to create /proc/sys_stats\n");
        return -ENOMEM;
    }

    printk(KERN_INFO "proc_module loaded\n");
    return 0;
}

static void __exit proc_module_exit(void) {
    proc_remove(proc_my_config);
    proc_remove(proc_sys_stats);
    printk(KERN_INFO "proc_module unloaded\n");
}

module_init(proc_module_init);
module_exit(proc_module_exit);

MODULE_LICENSE("GPL");
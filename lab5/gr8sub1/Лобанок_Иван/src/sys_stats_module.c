#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>
#include <linux/mm.h>
#include <linux/sched/signal.h>

#define PROC_NAME "sys_stats"
#define MAX_SIZE 1024

static struct proc_dir_entry *proc_file = NULL;

// Функция чтения (cat /proc/sys_stats)
static ssize_t proc_read(struct file *file, char __user *ubuf,
                         size_t count, loff_t *ppos)
{
    char buf[MAX_SIZE];
    int len;
    int process_count = 0;
    struct task_struct *task;
    struct sysinfo si;
    unsigned long uptime_sec;

    // Если уже читали, возвращаем EOF
    if (*ppos > 0)
        return 0;

    // Подсчёт процессов
    for_each_process(task) {
        process_count++;
    }

    // Получение информации о памяти
    si_meminfo(&si);
    
    // Вычисление uptime в секундах
    uptime_sec = jiffies_to_msecs(jiffies) / 1000;

    // Форматирование вывода
    len = snprintf(buf, sizeof(buf),
        "Processes: %d\n"
        "Memory Used: %lu MB\n"
        "System Uptime: %lu seconds\n",
        process_count,
        (si.totalram - si.freeram) * si.mem_unit / (1024 * 1024),
        uptime_sec
    );

    // Копируем данные в user space
    if (copy_to_user(ubuf, buf, len))
        return -EFAULT;

    *ppos = len;
    return len;
}

// Структура операций для proc файла
static const struct proc_ops proc_file_ops = {
    .proc_read = proc_read,
};

// Инициализация модуля
static int __init sys_stats_init(void)
{
    printk(KERN_INFO "sys_stats: Initializing\n");

    // Создаём proc файл только для чтения (0444)
    proc_file = proc_create(PROC_NAME, 0444, NULL, &proc_file_ops);
    if (!proc_file) {
        printk(KERN_ERR "sys_stats: Failed to create /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }

    printk(KERN_INFO "sys_stats: Created /proc/%s\n", PROC_NAME);
    return 0;
}

// Выгрузка модуля
static void __exit sys_stats_exit(void)
{
    if (proc_file) {
        proc_remove(proc_file);
        printk(KERN_INFO "sys_stats: Removed /proc/%s\n", PROC_NAME);
    }
}

module_init(sys_stats_init);
module_exit(sys_stats_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ivan Lobanok");
MODULE_DESCRIPTION("System statistics via /proc/sys_stats");
MODULE_VERSION("1.0");

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/mm.h>
#include <linux/version.h>

#define PROC_NAME "sys_stats"
#define MAX_SIZE 1024

static struct proc_dir_entry *proc_file = NULL;

// Функция для подсчёта количества процессов
static int count_processes(void)
{
    struct task_struct *task;
    int count = 0;
    
    // Итерация по всем процессам в системе
    rcu_read_lock();
    for_each_process(task) {
        count++;
    }
    rcu_read_unlock();
    
    return count;
}

// Функция для получения информации о памяти
static void get_memory_info(unsigned long *total_mem, unsigned long *used_mem, unsigned long *free_mem)
{
    struct sysinfo mem_info;
    
    // Получаем информацию о системе
    si_meminfo(&mem_info);
    
    // ПРАВИЛЬНЫЙ расчет с использованием PAGE_SIZE
    *total_mem = (mem_info.totalram * PAGE_SIZE) / (1024 * 1024);
    *free_mem = (mem_info.freeram * PAGE_SIZE) / (1024 * 1024);
    *used_mem = *total_mem - *free_mem;
    
    // Отладочная информация
    printk(KERN_INFO "Memory stats: total=%luMB, used=%luMB, free=%luMB\n",
           *total_mem, *used_mem, *free_mem);
}

static ssize_t proc_read(struct file *file, char __user *ubuf,
                         size_t count, loff_t *ppos)
{
    char buf[MAX_SIZE];
    int len = 0;
    
    if (*ppos > 0)
        return 0;

    // Получаем системную статистику
    int process_count = count_processes();
    
    unsigned long total_mem, used_mem, free_mem;
    get_memory_info(&total_mem, &used_mem, &free_mem);
    
    // Получаем uptime системы
    unsigned long long uptime_seconds = jiffies_to_msecs(get_jiffies_64()) / 1000;
    
    // Рассчитываем проценты использования памяти
    int memory_percent = total_mem > 0 ? (used_mem * 100) / total_mem : 0;
    
    // Форматируем вывод
    len = snprintf(buf, sizeof(buf),
        "=== System Statistics ===\n"
        "Processes: %d\n"
        "Memory: %lu MB used / %lu MB total (%d%%)\n"
        "Free Memory: %lu MB\n"
        "System Uptime: %llu seconds\n"
        "========================\n",
        process_count, used_mem, total_mem, memory_percent, free_mem, uptime_seconds);

    if (len >= sizeof(buf))
        len = sizeof(buf) - 1;

    if (copy_to_user(ubuf, buf, len))
        return -EFAULT;

    *ppos = len;
    return len;
}

// Структура операций для proc файла (только чтение)
static const struct proc_ops proc_file_ops = {
    .proc_read = proc_read,
};

// Функция инициализации модуля
static int __init sys_stats_module_init(void)
{
    printk(KERN_INFO "sys_stats_module: Initializing\n");

    // Создаём proc файл только для чтения
    proc_file = proc_create(PROC_NAME, 0444, NULL, &proc_file_ops);
    if (!proc_file) {
        printk(KERN_ERR "sys_stats_module: Failed to create /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }

    printk(KERN_INFO "sys_stats_module: Created /proc/%s\n", PROC_NAME);
    printk(KERN_INFO "sys_stats_module: System statistics available\n");

    return 0;
}

// Функция выгрузки модуля
static void __exit sys_stats_module_exit(void)
{
    // Удаляем proc файл
    if (proc_file) {
        proc_remove(proc_file);
        printk(KERN_INFO "sys_stats_module: Removed /proc/%s\n", PROC_NAME);
    }

    printk(KERN_INFO "sys_stats_module: Exiting\n");
}

module_init(sys_stats_module_init);
module_exit(sys_stats_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("killerser");
MODULE_DESCRIPTION("System statistics proc module");
MODULE_VERSION("1.0");
/*
 * stats_module.c - Модуль статистики для Варианта 2 (Задание C)
 *
 * Создаёт файл /proc/sys_stats, который выводит:
 * 1. Количество процессов
 * 2. Использование памяти
 * 3. Время работы системы (uptime)
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>     // Для удобного вывода текста в proc
#include <linux/mm.h>           // Для получения информации о памяти (si_meminfo)
#include <linux/sched/signal.h> // Для перебора процессов (for_each_process)
#include <linux/jiffies.h>      // Для работы со временем

#define PROC_NAME "sys_stats"

// Функция, которая собирает и форматирует данные
static int stats_show(struct seq_file *m, void *v)
{
    struct sysinfo i;
    struct task_struct *task;
    long num_procs = 0;
    unsigned long uptime_sec;
    unsigned long total_ram_mb, free_ram_mb;

    // 1. Получаем информацию о памяти
    si_meminfo(&i);
    // Конвертируем страницы памяти в Мегабайты
    // Формула: (страницы * размер_страницы) / 1024 / 1024
    total_ram_mb = (i.totalram << (PAGE_SHIFT - 10)) / 1024;
    free_ram_mb = (i.freeram << (PAGE_SHIFT - 10)) / 1024;

    // 2. Считаем количество процессов
    // rcu_read_lock нужен для безопасного чтения списка задач
    rcu_read_lock();
    for_each_process(task) {
        num_procs++;
    }
    rcu_read_unlock();

    // 3. Получаем аптайм в секундах
    uptime_sec = jiffies_to_msecs(jiffies) / 1000;

    // Выводим данные в буфер seq_file (аналог printf)
    seq_printf(m, "=== System Statistics (Variant 2) ===\n");
    seq_printf(m, "Processes count: %ld\n", num_procs);
    seq_printf(m, "Total RAM:       %lu MB\n", total_ram_mb);
    seq_printf(m, "Free RAM:        %lu MB\n", free_ram_mb);
    seq_printf(m, "System Uptime:   %lu seconds\n", uptime_sec);
    seq_printf(m, "=====================================\n");

    return 0;
}

// Эта функция вызывается, когда пользователь открывает файл (cat /proc/sys_stats)
static int stats_open(struct inode *inode, struct file *file)
{
    // single_open связывает файл с нашей функцией stats_show
    return single_open(file, stats_show, NULL);
}

// Структура операций с файлом
static const struct proc_ops stats_fops = {
    .proc_open = stats_open,
    .proc_read = seq_read,      // Стандартная функция чтения для seq_file
    .proc_lseek = seq_lseek,    // Стандартная функция перемотки
    .proc_release = single_release,
};

// Глобальная переменная для файла
static struct proc_dir_entry *proc_file;

// Инициализация модуля
static int __init stats_init(void)
{
    // Создаем файл в /proc с правами 0444 (только чтение)
    proc_file = proc_create(PROC_NAME, 0444, NULL, &stats_fops);
    if (!proc_file) {
        printk(KERN_ERR "stats_module: Failed to create /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }
    printk(KERN_INFO "stats_module: Loaded successfully. Check /proc/%s\n", PROC_NAME);
    return 0;
}

// Выгрузка модуля
static void __exit stats_exit(void)
{
    if (proc_file) {
        proc_remove(proc_file);
        printk(KERN_INFO "stats_module: Removed /proc/%s\n", PROC_NAME);
    }
}

module_init(stats_init);
module_exit(stats_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chris");
MODULE_DESCRIPTION("Lab 5 Variant 2 Task C: System Statistics");

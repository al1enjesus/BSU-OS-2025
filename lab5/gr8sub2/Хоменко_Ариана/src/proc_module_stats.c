#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/kernel_stat.h>
#include <linux/jiffies.h>

#define PROC_NAME "sys_stats"

/* ========================================
 * ФУНКЦИЯ ПОЛУЧЕНИЯ КОЛИЧЕСТВА ПРОЦЕССОВ
 * ======================================== */
static int count_processes(void)
{
    struct task_struct *p;
    int count = 0;

    /* Итерируем по всем процессам в системе */
    for_each_process(p) {
        count++;
    }

    return count;
}

/* ========================================
 * ФУНКЦИЯ ЧТЕНИЯ ИЗ /proc ФАЙЛА
 * ======================================== */
static int proc_read(struct seq_file *m, void *v)
{
    int process_count;
    struct sysinfo info;
    unsigned long uptime_ms;

    /* Получаем количество процессов */
    process_count = count_processes();

    /* Получаем информацию о памяти */
    si_meminfo(&info);

    /* Получаем uptime в миллисекундах */
    uptime_ms = jiffies_to_msecs(get_jiffies_64());

    /* Выводим информацию в читаемом формате */
    seq_printf(m, "Processes: %d\n", process_count);
    seq_printf(m, "Memory Used: %lu MB\n", (info.totalram - info.freeram) >> 8);
    seq_printf(m, "System Uptime: %lu seconds\n", uptime_ms / 1000);

    /* Альтернативный формат (более информативный) */
    seq_printf(m, "\n--- Detailed Info ---\n");
    seq_printf(m, "Total Memory: %lu MB\n", info.totalram >> 8);
    seq_printf(m, "Free Memory: %lu MB\n", info.freeram >> 8);
    seq_printf(m, "Uptime (ms): %lu\n", uptime_ms);

    return 0;
}

/* ========================================
 * ФУНКЦИЯ ОТКРЫТИЯ /proc ФАЙЛА
 * ======================================== */
static int proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_read, NULL);
}

/* ========================================
 * СТРУКТУРА ОПЕРАЦИЙ ДЛЯ /proc ФАЙЛА
 * ======================================== */
static const struct proc_ops proc_file_ops = {
    .proc_open  = proc_open,
    .proc_read  = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

/* ========================================
 * ФУНКЦИЯ ИНИЦИАЛИЗАЦИИ МОДУЛЯ
 * ======================================== */
static int __init proc_module_init(void)
{
    struct proc_dir_entry *pde;

    printk(KERN_INFO "proc_module: Initializing sys_stats module...\n");

    /* Создаём /proc файл с правами только на чтение (0444)
     * Параметры:
     *   - PROC_NAME: имя файла
     *   - 0444: права доступа (r--r--r--, только чтение)
     *   - NULL: родительская директория (корень /proc)
     *   - &proc_file_ops: структура с функциями открытия/чтения
     */
    pde = proc_create(PROC_NAME, 0444, NULL, &proc_file_ops);

    if (!pde) {
        printk(KERN_ERR "proc_module: Failed to create /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }

    printk(KERN_INFO "proc_module: Created /proc/%s\n", PROC_NAME);
    printk(KERN_INFO "proc_module: Use: cat /proc/%s\n", PROC_NAME);

    return 0;
}

/* ========================================
 * ФУНКЦИЯ ВЫГРУЗКИ МОДУЛЯ
 * ======================================== */
static void __exit proc_module_exit(void)
{
    remove_proc_entry(PROC_NAME, NULL);
    printk(KERN_INFO "proc_module: Removed /proc/%s\n", PROC_NAME);
    printk(KERN_INFO "proc_module: Exiting sys_stats module...\n");
}

/* ========================================
 * МАКРОСЫ МОДУЛЯ
 * ======================================== */
module_init(proc_module_init);
module_exit(proc_module_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ариана Хоменко");
MODULE_DESCRIPTION("Proc filesystem module with system statistics");
MODULE_VERSION("1.0");

/*
 * ПРИМЕРЫ ИСПОЛЬЗОВАНИЯ:
 *
 * 1. Загрузить модуль:
 *    $ sudo insmod proc_module.ko
 *
 * 2. Прочитать статистику:
 *    $ cat /proc/sys_stats
 *    Processes: 234
 *    Memory Used: 2048 MB
 *    System Uptime: 3600 seconds
 *
 *    --- Detailed Info ---
 *    Total Memory: 8192 MB
 *    Free Memory: 2048 MB
 *    Uptime (ms): 3600000
 *
 * 3. Проверить логи:
 *    $ dmesg | tail -5
 *
 * 4. Выгрузить модуль:
 *    $ sudo rmmod proc_module
 *
 * РЕАЛИЗОВАННЫЕ ТРЕБОВАНИЯ:
 * ✓ Используется for_each_process() для подсчёта процессов
 * ✓ Используется si_meminfo() для получения информации о памяти
 * ✓ Используется jiffies_to_msecs() для преобразования uptime
 * ✓ seq_printf() используется для форматированного вывода
 * ✓ Читаемый формат вывода
 *
 * КЛЮЧЕВЫЕ ФУНКЦИИ:
 * - for_each_process(p): итерирует по всем процессам
 * - si_meminfo(&info): получает информацию о памяти
 * - jiffies_to_msecs(): преобразует jiffies в миллисекунды
 * - get_jiffies_64(): получает текущее время в jiffies
 * - seq_printf(m, fmt, ...): выводит данные в seq_file
 * - single_open(): открывает seq_file для одного вывода
 */

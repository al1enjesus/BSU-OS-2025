/*
 * proc_module.c - Модуль с /proc файлом с поддержкой записи
 *
 * Создаёт файл /proc/my_config с возможностью чтения и записи:
 * - По умолчанию содержит "default"
 * - При записи (echo "text" > /proc/my_config) сохраняет новое значение
 * - При чтении (cat /proc/my_config) возвращает текущее значение
 * - Максимальная длина: 256 символов
 *
 * Компиляция: make
 * Использование:
 *   sudo insmod proc_module.ko
 *   cat /proc/my_config          # Читает значение
 *   echo "new value" > /proc/my_config  # Записывает значение
 *   sudo rmmod proc_module
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define PROC_NAME "my_config"
#define MAX_SIZE 256

/* ========================================
 * ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ
 * ======================================== */

static struct proc_dir_entry *proc_file = NULL;
static char config_buffer[MAX_SIZE] = "default";

/* ========================================
 * ФУНКЦИЯ ЧТЕНИЯ ИЗ /proc ФАЙЛА
 * ======================================== */
static ssize_t proc_read(struct file *file, char __user *ubuf,
                         size_t count, loff_t *ppos)
{
    char buf[MAX_SIZE];
    int len;

    /* Если уже читали всё (повторный вызов), возвращаем 0 (EOF) */
    if (*ppos > 0)
        return 0;

    /* Копируем текущее значение конфига */
    len = snprintf(buf, sizeof(buf), "%s\n", config_buffer);

    /* Копируем из kernel space в user space */
    if (copy_to_user(ubuf, buf, len))
        return -EFAULT;

    /* Обновляем позицию чтения */
    *ppos = len;

    printk(KERN_INFO "proc_module: Read %d bytes from /proc/%s\n", len, PROC_NAME);

    return len;
}

/* ========================================
 * ФУНКЦИЯ ЗАПИСИ В /proc ФАЙЛ
 * ======================================== */
static ssize_t proc_write(struct file *file, const char __user *ubuf,
                          size_t count, loff_t *ppos)
{
    char buf[MAX_SIZE];
    size_t write_size;

    printk(KERN_INFO "proc_module: Write request, count=%zu\n", count);

    /* Ограничиваем размер записи максимальной длиной буфера */
    write_size = (count < MAX_SIZE) ? count : MAX_SIZE - 1;

    /* Копируем из user space в kernel space */
    if (copy_from_user(buf, ubuf, write_size))
        return -EFAULT;

    /* Удаляем перевод строки в конце (если есть) */
    if (write_size > 0 && buf[write_size - 1] == '\n')
        write_size--;

    /* Обнуляем буфер и копируем новое значение */
    memset(config_buffer, 0, sizeof(config_buffer));
    memcpy(config_buffer, buf, write_size);

    printk(KERN_INFO "proc_module: Written to /proc/%s: '%s' (len=%zu)\n",
           PROC_NAME, config_buffer, write_size);

    return count;  /* Возвращаем количество записанных байт */
}

/* ========================================
 * СТРУКТУРА ОПЕРАЦИЙ ДЛЯ /proc ФАЙЛА
 * ======================================== */
static const struct proc_ops proc_file_ops = {
    .proc_read  = proc_read,
    .proc_write = proc_write,
};

/* ========================================
 * ФУНКЦИЯ ИНИЦИАЛИЗАЦИИ МОДУЛЯ
 * ======================================== */
static int __init proc_module_init(void)
{
    printk(KERN_INFO "proc_module: Initializing...\n");

    /* Создаём /proc файл
     * Параметры:
     *   - PROC_NAME: имя файла
     *   - 0666: права доступа (rw-rw-rw-)
     *   - NULL: родительская директория (корень /proc)
     *   - &proc_file_ops: структура с функциями read/write
     */
    proc_file = proc_create(PROC_NAME, 0666, NULL, &proc_file_ops);

    if (!proc_file) {
        printk(KERN_ERR "proc_module: Failed to create /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }

    printk(KERN_INFO "proc_module: Created /proc/%s\n", PROC_NAME);
    printk(KERN_INFO "proc_module: Initial value: '%s'\n", config_buffer);
    printk(KERN_INFO "proc_module: Use:\n");
    printk(KERN_INFO "  cat /proc/%s          # Read value\n", PROC_NAME);
    printk(KERN_INFO "  echo 'text' > /proc/%s  # Write value\n", PROC_NAME);
    return 0;
}

/* ========================================
 * ФУНКЦИЯ ВЫГРУЗКИ МОДУЛЯ
 * ======================================== */
static void __exit proc_module_exit(void)
{
    if (proc_file) {
        proc_remove(proc_file);
        printk(KERN_INFO "proc_module: Removed /proc/%s\n", PROC_NAME);
    }

    printk(KERN_INFO "proc_module: Exiting...\n");
}

/* ========================================
 * МАКРОСЫ МОДУЛЯ
 * ======================================== */
module_init(proc_module_init);
module_exit(proc_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ариана Хоменко");
MODULE_DESCRIPTION("Proc filesystem module with read/write support");
MODULE_VERSION("1.0");

/*
 * ПРИМЕРЫ ИСПОЛЬЗОВАНИЯ:
 *
 * 1. Загрузить модуль:
 *    $ sudo insmod proc_module.ko
 *
 * 2. Прочитать текущее значение:
 *    $ cat /proc/my_config
 *    default
 *
 * 3. Записать новое значение:
 *    $ echo "new value" > /proc/my_config
 *    $ cat /proc/my_config
 *    new value
 *
 * 4. Проверить логи:
 *    $ dmesg | tail -10
 *
 * 5. Выгрузить модуль:
 *    $ sudo rmmod proc_module
 *
 * ОЖИДАЕМЫЙ РЕЗУЛЬТАТ:
 *
 * $ cat /proc/my_config
 * default
 *
 * $ echo "hew value" > /proc/my_config
 * $ cat /proc/my_config
 * hew value
 *
 * $ echo "New data" > /proc/my_config
 * $ cat /proc/my_config
 * New data
 *
 * ЛОГИ (dmesg):
 * [12345.678] proc_module: Created /proc/my_config
 * [12345.679] proc_module: Read 8 bytes from /proc/my_config
 * [12346.123] proc_module: Write request, count=10
 * [12346.124] proc_module: Written to /proc/my_config: 'new value' (len=9)
 * [12346.125] proc_module: Read 10 bytes from /proc/my_config
 */

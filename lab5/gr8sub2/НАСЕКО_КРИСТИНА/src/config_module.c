/*
 * config_module.c - Задание B (Вариант 2)
 * Создает /proc/my_config с возможностью чтения и записи.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/mutex.h> // Для синхронизации

#define PROC_NAME "my_config"
#define MAX_CONFIG_SIZE 256

static struct proc_dir_entry *proc_file;
static char config_buffer[MAX_CONFIG_SIZE];
static int current_len = 0;

// Мьютекс для защиты буфера от гонки данных (Bonus requirement)
static DEFINE_MUTEX(config_mutex);

// Функция чтения: cat /proc/my_config
static ssize_t proc_read(struct file *file, char __user *ubuf,
                         size_t count, loff_t *ppos)
{
    int ret;
    
    // Блокируем доступ к буферу
    mutex_lock(&config_mutex);

    if (*ppos > 0 || current_len == 0) {
        mutex_unlock(&config_mutex);
        return 0;
    }

    // Копируем данные пользователю
    if (copy_to_user(ubuf, config_buffer, current_len)) {
        mutex_unlock(&config_mutex);
        return -EFAULT;
    }

    ret = current_len;
    *ppos = current_len;
    
    mutex_unlock(&config_mutex);
    return ret;
}

// Функция записи: echo "text" > /proc/my_config
static ssize_t proc_write(struct file *file, const char __user *ubuf,
                          size_t count, loff_t *ppos)
{
    size_t copy_len;

    mutex_lock(&config_mutex);

    // Ограничиваем размер записи
    if (count >= MAX_CONFIG_SIZE)
        copy_len = MAX_CONFIG_SIZE - 1;
    else
        copy_len = count;

    // Очищаем буфер перед записью
    memset(config_buffer, 0, MAX_CONFIG_SIZE);

    // Копируем данные от пользователя в ядро
    if (copy_from_user(config_buffer, ubuf, copy_len)) {
        mutex_unlock(&config_mutex);
        return -EFAULT;
    }

    // Удаляем символ новой строки в конце, если есть (для красоты)
    if (config_buffer[copy_len - 1] == '\n')
        config_buffer[copy_len - 1] = '\0';
    else
        config_buffer[copy_len] = '\0';

    current_len = strlen(config_buffer) + 1; // +1 для \n при чтении, если нужно, или просто \0
    // Добавим перенос строки для красивого вывода cat
    if (current_len < MAX_CONFIG_SIZE) {
        config_buffer[current_len-1] = '\n';
        config_buffer[current_len] = '\0';
        current_len++;
    }

    printk(KERN_INFO "config_module: Config updated to '%s'\n", config_buffer);
    
    mutex_unlock(&config_mutex);
    return count;
}

static const struct proc_ops proc_file_ops = {
    .proc_read = proc_read,
    .proc_write = proc_write,
};

static int __init config_init(void)
{
    // Инициализация значения по умолчанию
    strncpy(config_buffer, "default\n", MAX_CONFIG_SIZE);
    current_len = strlen(config_buffer);

    // Создаем файл с правами 0666 (чтение/запись для всех)
    proc_file = proc_create(PROC_NAME, 0666, NULL, &proc_file_ops);
    if (!proc_file) {
        printk(KERN_ERR "config_module: Failed to create /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }

    printk(KERN_INFO "config_module: Loaded. Try 'echo new > /proc/%s'\n", PROC_NAME);
    return 0;
}

static void __exit config_exit(void)
{
    if (proc_file) {
        proc_remove(proc_file);
        printk(KERN_INFO "config_module: Removed /proc/%s\n", PROC_NAME);
    }
}

module_init(config_init);
module_exit(config_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chris");
MODULE_DESCRIPTION("Lab 5 Variant 2 Task B: Writable Proc File");

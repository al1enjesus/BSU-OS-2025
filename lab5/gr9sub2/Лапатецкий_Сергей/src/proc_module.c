#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>

#define PROC_NAME "my_config"
#define MAX_SIZE 256

static struct proc_dir_entry *proc_file = NULL;
static char *config_data = NULL;
static size_t config_size = 0;
static DEFINE_MUTEX(config_mutex);

static ssize_t proc_read(struct file *file, char __user *ubuf,
                         size_t count, loff_t *ppos)
{
    char buf[MAX_SIZE];
    int len = 0;

    // Если уже читали (повторный вызов), возвращаем 0 (EOF)
    if (*ppos > 0)
        return 0;

    // Защищаем доступ к данным мьютексом
    mutex_lock(&config_mutex);
    
    // Формируем строку для вывода
    len = snprintf(buf, sizeof(buf), "%s\n", config_data);
    
    mutex_unlock(&config_mutex);

    if (copy_to_user(ubuf, buf, len))
         return -EFAULT;

    // Обновляем позицию чтения
    *ppos = len;

    return len;
}

static ssize_t proc_write(struct file *file, const char __user *ubuf,
                          size_t count, loff_t *ppos)
{
    char *temp_buf;
    size_t data_size;
    
    // Проверяем размер данных
    if (count == 0)
        return 0;
        
    if (count > MAX_SIZE - 1)
        count = MAX_SIZE - 1;

    // Выделяем временный буфер
    temp_buf = kmalloc(count + 1, GFP_KERNEL);
    if (!temp_buf)
        return -ENOMEM;

    // Копируем данные из user space в kernel space
    if (copy_from_user(temp_buf, ubuf, count)) {
        kfree(temp_buf);
        return -EFAULT;
    }
    
    // Добавляем нулевой терминатор
    temp_buf[count] = '\0';
    
    // Убираем символ новой строки если есть
    if (temp_buf[count - 1] == '\n')
        temp_buf[count - 1] = '\0';

    // Защищаем доступ к данным мьютексом
    mutex_lock(&config_mutex);
    
    // Освобождаем старые данные если есть
    if (config_data)
        kfree(config_data);
    
    // Выделяем память для новых данных
    data_size = strlen(temp_buf) + 1;
    config_data = kmalloc(data_size, GFP_KERNEL);
    if (!config_data) {
        mutex_unlock(&config_mutex);
        kfree(temp_buf);
        return -ENOMEM;
    }
    
    // Копируем данные
    strncpy(config_data, temp_buf, data_size);
    config_size = data_size;
    
    mutex_unlock(&config_mutex);
    
    // Освобождаем временный буфер
    kfree(temp_buf);
    
    printk(KERN_INFO "proc_module: Config updated to: '%s'\n", config_data);
    
    return count;
}

// Структура операций для proc файла
static const struct proc_ops proc_file_ops = {
    .proc_read = proc_read,
    .proc_write = proc_write,
};

static int __init proc_module_init(void)
{
    printk(KERN_INFO "proc_module: Initializing\n");

    // Инициализируем данные по умолчанию
    config_data = kmalloc(8, GFP_KERNEL); // "default" + null terminator
    if (!config_data) {
        printk(KERN_ERR "proc_module: Failed to allocate memory\n");
        return -ENOMEM;
    }
    
    strcpy(config_data, "default");
    config_size = 8;

    // Создаём proc файл
    proc_file = proc_create(PROC_NAME, 0666, NULL, &proc_file_ops);
    if (!proc_file) {
        printk(KERN_ERR "proc_module: Failed to create /proc/%s\n", PROC_NAME);
        kfree(config_data);
        config_data = NULL;
        return -ENOMEM;
    }

    printk(KERN_INFO "proc_module: Created /proc/%s\n", PROC_NAME);
    printk(KERN_INFO "proc_module: Default value: '%s'\n", config_data);

    return 0;
}

static void __exit proc_module_exit(void)
{
    // Удаляем proc файл
    if (proc_file) {
        proc_remove(proc_file);
        printk(KERN_INFO "proc_module: Removed /proc/%s\n", PROC_NAME);
    }
    
    // Освобождаем память
    if (config_data) {
        kfree(config_data);
        config_data = NULL;
    }

    printk(KERN_INFO "proc_module: Exiting\n");
}

module_init(proc_module_init);
module_exit(proc_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("killerser");
MODULE_DESCRIPTION("Proc filesystem example");
MODULE_VERSION("1.0");

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>   // copy_to_user, copy_from_user
#include <linux/slab.h>

#define PROC_NAME "my_config"
#define MAX_LEN   256

static struct proc_dir_entry *proc_file;

/* Буфер для хранения текущей строки конфигурации */
static char config_buf[MAX_LEN] = "default";
static size_t config_len = 7;   // strlen("default")

/*
 * Функция чтения: вызывается при cat /proc/my_config
 */
static ssize_t proc_read(struct file *file, char __user *ubuf,
                         size_t count, loff_t *ppos)
{
    int ret;

    /* Если уже прочитали всё — вернуть 0 (EOF) */
    if (*ppos > 0)
        return 0;

    /* Ограничиваем количество байт, которые реально отдаём */
    if (count < config_len)
        return -EINVAL; /* можно и помягче, но так проще для начала */

    ret = copy_to_user(ubuf, config_buf, config_len);
    if (ret != 0)
        return -EFAULT;

    *ppos = config_len;
    return config_len;
}

/*
 * Функция записи: echo "text" > /proc/my_config
 */
static ssize_t proc_write(struct file *file, const char __user *ubuf,
                          size_t count, loff_t *ppos)
{
    size_t to_copy;

    if (count == 0)
        return 0;

    /* Не даём записать больше чем MAX_LEN-1 (оставляем место для '\0') */
    to_copy = min(count, (size_t)(MAX_LEN - 1));

    /* Обнуляем буфер перед записью */
    memset(config_buf, 0, MAX_LEN);

    if (copy_from_user(config_buf, ubuf, to_copy))
        return -EFAULT;

    /* Убираем возможный '\n' в конце (echo добавляет перенос строки) */
    if (config_buf[to_copy - 1] == '\n') {
        config_buf[to_copy - 1] = '\0';
        config_len = to_copy - 1;
    } else {
        config_buf[to_copy] = '\0';
        config_len = to_copy;
    }

    printk(KERN_INFO "proc_config: new value set: '%s'\n", config_buf);

    return count;  // возвращаем количество принятых байт
}

/* Описания операций с файлом в procfs (ядра 5.x – struct proc_ops) */
static const struct proc_ops proc_file_ops = {
    .proc_read  = proc_read,
    .proc_write = proc_write,
};

static int __init proc_config_init(void)
{
    proc_file = proc_create(PROC_NAME, 0666, NULL, &proc_file_ops);
    if (!proc_file) {
        printk(KERN_ERR "proc_config: failed to create /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }

    printk(KERN_INFO "proc_config: /proc/%s created, default='%s'\n",
           PROC_NAME, config_buf);
    return 0;
}

static void __exit proc_config_exit(void)
{
    if (proc_file) {
        proc_remove(proc_file);
        printk(KERN_INFO "proc_config: /proc/%s removed\n", PROC_NAME);
    }
}

module_init(proc_config_init);
module_exit(proc_config_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Anya");
MODULE_DESCRIPTION("/proc/my_config with read/write");
MODULE_VERSION("1.0");
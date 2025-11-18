#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>
#include <linux/seq_file.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ШеметАА");
MODULE_DESCRIPTION("/proc/student_info module");
MODULE_VERSION("1.0");

static struct proc_dir_entry *proc_file;
static unsigned long load_time;
static unsigned int read_count = 0;

// Функция чтения для /proc/student_info
static int student_info_show(struct seq_file *m, void *v)
{
    read_count++;
    
    seq_printf(m, "Name: ШеметАА\n");
    seq_printf(m, "Group: 6, Subgroup: 1\n");
    seq_printf(m, "Module loaded at: %lu jiffies\n", load_time);
    seq_printf(m, "Read count: %u\n", read_count);
    
    return 0;
}

// Функция открытия для seq_file
static int student_info_open(struct inode *inode, struct file *file)
{
    return single_open(file, student_info_show, NULL);
}

// Операции с файлом
static const struct proc_ops student_info_fops = {
    .proc_open = student_info_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

static int __init proc_module_init(void)
{
    // Запоминаем время загрузки модуля
    load_time = jiffies;
    
    // Создаём файл в /proc
    proc_file = proc_create("student_info", 0444, NULL, &student_info_fops);
    if (!proc_file) {
        printk(KERN_ERR "Failed to create /proc/student_info\n");
        return -ENOMEM;
    }
    
    printk(KERN_INFO "Proc module loaded. Jiffies: %lu\n", load_time);
    return 0;
}

static void __exit proc_module_exit(void)
{
    // Удаляем файл из /proc
    proc_remove(proc_file);
    printk(KERN_INFO "Proc module unloaded\n");
}

module_init(proc_module_init);
module_exit(proc_module_exit);

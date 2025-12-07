#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/jiffies.h>

#define PROC_FILENAME "student_info"

static struct proc_dir_entry *proc_file;

/* Глобальные переменные */
static unsigned long load_time = 0;
static int read_count = 0;

/* Функция чтения proc-файла */
static int proc_show(struct seq_file *m, void *v)
{
    read_count++;

    seq_printf(m, "Name: Litvinovich Mariya\n");
    seq_printf(m, "Group: 8, Subgroup: 1\n");
    seq_printf(m, "Module loaded at: %lu jiffies\n", load_time);
    seq_printf(m, "Read count: %d\n", read_count);

    return 0;
}

static int proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_show, NULL);
}

static const struct proc_ops proc_fops = {
    .proc_open = proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

/* Загрузка модуля */
static int __init my_init(void)
{
    load_time = jiffies;

    proc_file = proc_create(PROC_FILENAME, 0666, NULL, &proc_fops);
    if (!proc_file) {
        printk(KERN_ERR "Failed to create /proc/%s\n", PROC_FILENAME);
        return -ENOMEM;
    }

    printk(KERN_INFO "/proc/%s created\n", PROC_FILENAME);
    return 0;
}

/* Выгрузка */
static void __exit my_exit(void)
{
    proc_remove(proc_file);
    printk(KERN_INFO "/proc/%s removed\n", PROC_FILENAME);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mariya Litvinovich");
MODULE_DESCRIPTION("Student info proc file module");

module_init(my_init);
module_exit(my_exit);

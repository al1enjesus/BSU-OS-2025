#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/jiffies.h>

static unsigned long load_jiffies;
static unsigned int read_count = 0;

/* Функция чтения для /proc/student_info */
static int student_info_show(struct seq_file *m, void *v)
{
    read_count++;
    seq_printf(m,
               "Name: Vladimir Stepkin\n"
               "Group: 8, Subgroup: 1\n"
               "Module loaded at: %lu jiffies\n"
               "Read count: %u\n",
               load_jiffies,
               read_count);
    return 0;
}

/* Функция открытия файла /proc */
static int student_info_open(struct inode *inode, struct file *file)
{
    return single_open(file, student_info_show, NULL);
}

/* Определение proc_ops для современных ядер */
static const struct proc_ops student_info_fops = {
    .proc_open    = student_info_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

/* Функция инициализации модуля */
static int __init student_info_init(void)
{
    load_jiffies = jiffies;
    proc_create("student_info", 0, NULL, &student_info_fops);
    printk(KERN_INFO "student_info module loaded\n");
    return 0;
}

/* Функция выгрузки модуля */
static void __exit student_info_exit(void)
{
    remove_proc_entry("student_info", NULL);
    printk(KERN_INFO "student_info module unloaded\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vladimir Stepkin");
MODULE_DESCRIPTION("Module that creates /proc/student_info");

module_init(student_info_init);
module_exit(student_info_exit);

/*
 * hello_module.c - Задание A (Вариант 2)
 * Принимает параметр message, выводит приветствие/прощание.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/moduleparam.h>

// Глобальная переменная для параметра
static char *message = NULL;

// Регистрация параметра (права 0644 позволяют читать параметр из sysfs)
module_param(message, charp, 0644);
MODULE_PARM_DESC(message, "Custom greeting message");

static int __init hello_init(void)
{
    if (message) {
        printk(KERN_INFO "hello_module: Custom message: %s\n", message);
    } else {
        printk(KERN_INFO "hello_module: Hello from Chris module!\n");
    }
    return 0;
}

static void __exit hello_exit(void)
{
    // Прощальное сообщение, которое зависит от того, был ли задан параметр message при insmod.
    // ЭТА ФУНКЦИЯ НЕ МОЖЕТ ПОЛУЧИТЬ ПАРАМЕТР ИЗ КОМАНДЫ RMMOD.
    if (message) {
        printk(KERN_INFO "hello_module: Goodbye! Module was running in custom mode.\n");
    } else {
        printk(KERN_INFO "hello_module: Goodbye from Chris module!\n");
    }
}

module_init(hello_init);
module_exit(hello_exit);

// Обязательные метаданные
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chris");
MODULE_DESCRIPTION("Lab 5 Variant 2 Task A: Hello World");
MODULE_VERSION("1.0");

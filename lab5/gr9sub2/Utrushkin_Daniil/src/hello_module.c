/*
 * hello_module.c - Простой Hello World модуль
 * 
 * При загрузке выводит приветствие
 * При выгрузке выводит прощание
 * Поддерживает параметр message
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Utrushkin Daniil");
MODULE_DESCRIPTION("Simple Hello World module");
MODULE_VERSION("1.0");

// Параметр модуля
static char *message = "Hello from Utrushkin Daniil module!";
module_param(message, charp, 0644);
MODULE_PARM_DESC(message, "Message to display on load");

// Функция инициализации модуля
static int __init hello_init(void)
{
    printk(KERN_INFO "HELLO_MODULE: %s\n", message);
    return 0; // 0 = успех
}

// Функция выгрузки модуля
static void __exit hello_exit(void)
{
    printk(KERN_INFO "HELLO_MODULE: Goodbye from Utrushkin Daniil module!\n");
}

// Регистрация функций
module_init(hello_init);
module_exit(hello_exit);

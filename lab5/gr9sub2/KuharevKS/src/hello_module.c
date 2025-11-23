/*
 * hello_module.c - Простейший модуль ядра "Hello World"
 *
 * Компиляция: make
 * Загрузка: sudo insmod hello_module.ko
 * Выгрузка: sudo rmmod hello_module
 * Логи: dmesg | tail
 *
 * Это СКЕЛЕТ - реализуйте TODO самостоятельно!
 */

#include <linux/module.h>    
#include <linux/kernel.h>    
#include <linux/init.h>      
#include <linux/moduleparam.h> 

static char *message = NULL;
module_param(message,charp,0644);
MODULE_PARM_DESC(message,"Hello message");

static int __init hello_init(void)
{
     if (message) {
         printk(KERN_INFO "hello_module: %s\n", message);
     } else {
         printk(KERN_INFO "hello_module: Hello from Kirill module!\n");
     }

    printk(KERN_INFO "hello_module: Module loaded\n");

    return 0;  
}

static void __exit hello_exit(void)
{
    printk(KERN_INFO "hello_module: Goodbye from Kirill module!\n");
    printk(KERN_INFO "hello_module: Module unloaded\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");                   
MODULE_AUTHOR("Kuharev Kirill <kuharevk1@gmail.com>");  
MODULE_DESCRIPTION("Simple Hello World kernel module");
MODULE_VERSION("1.0");


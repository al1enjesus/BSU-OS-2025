#include <linux/module.h>    // Обязательно для всех модулей
#include <linux/kernel.h>    // Для printk, KERN_*
#include <linux/init.h>      // Для __init, __exit
#include <linux/moduleparam.h>  // Для module_param

static char *message = NULL;
module_param(message, charp, 0644);
MODULE_PARM_DESC(message, "Custom greeting message");

// Функция инициализации модуля
// Вызывается при insmod
static int __init hello_init(void)
{

    if (message) {
        printk(KERN_INFO "hello_module: %s\n", message);
    } else {
        printk(KERN_INFO "hello_module: Hello from Vladislav module!\n");
    }

    return 0;
}

// Функция выгрузки модуля
// Вызывается при rmmod
static void __exit hello_exit(void)
{
    printk(KERN_INFO "hello_module: Goodbye from Vladislav module!\n");
}

// Регистрация функций init/exit
module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL"); 
MODULE_AUTHOR("Vladislav <mrm70512@gmail.com>");
MODULE_DESCRIPTION("Simple Hello World kernel module");
MODULE_VERSION("1.0");

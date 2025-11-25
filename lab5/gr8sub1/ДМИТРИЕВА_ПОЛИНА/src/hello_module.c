#include <linux/module.h>       // Обязательно для всех модулей
#include <linux/kernel.h>       // printk, KERN_*
#include <linux/init.h>         // __init, __exit
#include <linux/moduleparam.h>  // module_param

// -------- Параметр модуля --------

// строковый параметр, по умолчанию NULL
static char *message = NULL;
module_param(message, charp, 0644);
MODULE_PARM_DESC(message, "Custom greeting message");

// Инициализация модуля 
static int __init hello_init(void)
{
    if (message) {
        printk(KERN_INFO "hello_module: %s\n", message);
    } else {
        printk(KERN_INFO "hello_module: Hello from Dmitrieva Polina module!\n");
    }

    return 0;  // 0 = успех
}

//Выгрузка модуля

static void __exit hello_exit(void)
{
    printk(KERN_INFO "hello_module: Goodbye from Polina Dmitrieva module!\n");
}

// Регистрация функций init/exit
module_init(hello_init);
module_exit(hello_exit);

// Метаданные
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dmitrieva Polina");
MODULE_DESCRIPTION("Simple Hello World kernel module");
MODULE_VERSION("1.0");

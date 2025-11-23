#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/moduleparam.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Anya");
MODULE_DESCRIPTION("Hello World Kernel Module");
MODULE_VERSION("1.0");

/* Параметр message: по умолчанию NULL => не задан */
static char *message = NULL;
module_param(message, charp, 0444);
MODULE_PARM_DESC(message, "Custom greeting message");

static int __init hello_init(void)
{
    if (message == NULL) {
        /* Параметр НЕ задан — дефолтное сообщение (для теста 1) */
        printk(KERN_INFO "Hello from Anya module!\n");
    } else {
        /* Параметр задан — выводим его вместо дефолтного (для теста 4) */
        printk(KERN_INFO "Hello Module: %s\n", message);
    }

    return 0;
}

static void __exit hello_exit(void)
{
    printk(KERN_INFO "Goodbye from Anya module!\n");
}

module_init(hello_init);
module_exit(hello_exit);
#include <linux/module.h>       // Для всех модулей
#include <linux/kernel.h>       // Для KERN_INFO
#include <linux/init.h>         // Макросы init и exit
#include <linux/moduleparam.h>  // Для module_param

static char *message = NULL;
module_param(message, charp, 0644);
MODULE_PARM_DESC(message, "Custom greeting message");

static int __init helloinit(void)
{
    if (message)
        printk(KERN_INFO "hellomodule: %s\n", message);
    else
        printk(KERN_INFO "hellomodule: Hello from ILYA module!\n");
    return 0;
}

static void __exit helloexit(void)
{
    printk(KERN_INFO "hellomodule: Goodbye from ILYA module!\n");
}

module_init(helloinit);
module_exit(helloexit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ILYA");
MODULE_DESCRIPTION("Simple Hello World kernel module");
MODULE_VERSION("1.0");

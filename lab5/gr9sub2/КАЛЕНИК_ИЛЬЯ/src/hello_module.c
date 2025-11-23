#include <linux/module.h>       // Для всех модулей
#include <linux/kernel.h>       
#include <linux/init.h>         
#include <linux/moduleparam.h>  

static char *message = NULL;
module_param(message, charp, 0644);
MODULE_PARM_DESC(message, "Custom greeting message (NULL for default)");


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

// hello_module.c — простой Hello World модуль ядра с параметром
// Вариант 1 (нечётные номера): задание A

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/moduleparam.h>

static char *message = "Hello from Kazak Artur module!";
module_param(message, charp, 0444);
MODULE_PARM_DESC(message, "Custom hello message");

static int __init hello_init(void)
{
	pr_info("hello_module: %s\n", message);
	return 0;
}

static void __exit hello_exit(void)
{
	pr_info("hello_module: Goodbye from Kazak Artur module!\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kazak Artur");
MODULE_DESCRIPTION("Simple Hello World kernel module with parameter");
MODULE_VERSION("1.0");

module_init(hello_init);
module_exit(hello_exit);

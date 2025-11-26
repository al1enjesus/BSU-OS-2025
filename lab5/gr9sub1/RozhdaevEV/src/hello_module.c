#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/string.h>

static char *message = NULL;
module_param(message, charp, 0644);
MODULE_PARM_DESC(message, "Custom greeting message");

#define STUDENT_NAME "RozhdaevEV"
#define GROUP_NUM 9
#define SUBGROUP_NUM 1

static int __init hello_init(void)
{
    if (message && strlen(message) > 0) {
        printk(KERN_INFO "hello_module: %s\n", message);
    } else {
        printk(KERN_INFO "hello_module: Hello from %s module! (Group: %d Subgroup: %d)\n",
               STUDENT_NAME, GROUP_NUM, SUBGROUP_NUM);
    }
    return 0;
}

static void __exit hello_exit(void)
{
    printk(KERN_INFO "hello_module: Goodbye from %s module!\n", STUDENT_NAME);
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("RozhdaevEV <student@example.com>");
MODULE_DESCRIPTION("Simple Hello World kernel module (implemented)");
MODULE_VERSION("1.0");

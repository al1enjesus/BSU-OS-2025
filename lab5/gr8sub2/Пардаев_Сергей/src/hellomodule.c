#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

static char *message = NULL;
module_param(message, charp, 0644);
MODULE_PARM_DESC(message, "Custom greeting message");

static int __init helloinit(void) {
  if (message)
    printk(KERN_INFO "hellomodule: Hello from Pardaev Sergey! Message: %s\n",
           message);
  else
    printk(KERN_INFO "hellomodule: Hello from Pardaev Sergey!\n");
  printk(KERN_INFO "hellomodule: Module loaded\n");
  return 0;
}

static void __exit helloexit(void) {
  printk(KERN_INFO "hellomodule: Goodbye from Pardaev Sergey!\n");
  printk(KERN_INFO "hellomodule: Module unloaded\n");
}

module_init(helloinit);
module_exit(helloexit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pardaev Sergey");
MODULE_DESCRIPTION("Simple Hello World kernel module");
MODULE_VERSION("1.0");

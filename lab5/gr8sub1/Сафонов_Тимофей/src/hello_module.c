#include <linux/module.h> 
#include <linux/kernel.h> 
#include <linux/init.h>
#include <linux/moduleparam.h>

static char *message = NULL;
module_param(message, charp, 0644);
MODULE_PARM_DESC(message, "Custom greeting message");

static int __init hello_init(void)
{
    if (message) {
         printk(KERN_INFO "hello_module: %s\n", message);
    } else {
         printk(KERN_INFO "hello_module: Hello from Timofey Safonov module!\n");
    }

    return 0;
}

static void __exit hello_exit(void)
{

    printk(KERN_INFO "hello_module: Goodbye from Timofey Safonov module!\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SafonovTV <timasnok@gmail.com>");
MODULE_DESCRIPTION("Simple Hello World kernel module");
MODULE_VERSION("1.0");

/*
 * ЗАДАНИЯ для студента:
 *
 * 1. Заполните TODO в коде выше
 *
 * 2. Добавьте параметр модуля "message"
 *
 * 3. В hello_init():
 *    - Если message != NULL, вывести его
 *    - Иначе вывести дефолтное приветствие с вашим именем
 *
 * 4. В hello_exit():
 *    - Вывести прощание с вашим именем
 *
 * 5. Заполните метаданные (автор, описание)
 *
 * 6. Скомпилируйте и протестируйте:
 *    $ make
 *    $ sudo insmod hello_module.ko
 *    $ dmesg | tail -5
 *    $ sudo rmmod hello_module
 *    $ dmesg | tail -5
 *
 * 7. Протестируйте с параметром:
 *    $ sudo insmod hello_module.ko message="Custom greeting"
 *    $ dmesg | tail -5
 *    $ sudo rmmod hello_module
 *
 * 8. Проверьте параметр в sysfs:
 *    $ cat /sys/module/hello_module/parameters/message
 *
 * ОЖИДАЕМЫЙ РЕЗУЛЬТАТ:
 *
 * $ sudo insmod hello_module.ko
 * $ dmesg | tail -1
 * [12345.678] hello_module: Hello from Ivan Ivanov module!
 *
 * $ sudo insmod hello_module.ko message="Test"
 * $ dmesg | tail -1
 * [12346.789] hello_module: Test
 *
 * $ sudo rmmod hello_module
 * $ dmesg | tail -1
 * [12347.890] hello_module: Goodbye from Ivan Ivanov module!
 */

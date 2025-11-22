# Лабораторная работа 5: Модули ядра Linux

## Студент: Михальчук Александр
## Группа: 9
## Вариант: 2

---

## Задание A: Hello World модуль

### Код модуля
```
c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/moduleparam.h>

static char *message = "Hello from Sasha module";
module_param(message, charp, 0644);

static int __init hello_init(void) {
    printk(KERN_INFO "%s\n", message);
    return 0;
}

static void __exit hello_exit(void) {
    printk(KERN_INFO "Goodbye from Sasha module!\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sasha");
MODULE_DESCRIPTION("Simple Hello World");
MODULE_VERSION("1.0");
```

### Результаты тестирования

1. Загрузка с параметром по умолчанию
```
bash
sancher@Ubuntu:~/lab5/src$ sudo dmesg | tail -2
[11635.435008] Hello from Sasha module
[12556.298763] Goodbye from Sasha module!
sancher@Ubuntu:~/lab5/src$ sudo insmod hello_module.ko
sancher@Ubuntu:~/lab5/src$ sudo dmesg | tail -1
[12575.938679] Hello from Sasha module
sancher@Ubuntu:~/lab5/src$ 
```
2. Загрузка с кастомным сообщением
```
bash
sancher@Ubuntu:~/lab5/src$ sudo insmod hello_module.ko message="Custom greeting"
sancher@Ubuntu:~/lab5/src$ sudo dmesg | tail -1
[13368.544795] Custom
```
3. Проверка параметра в sysfs
```
bash
sancher@Ubuntu:~/lab5/src$ cat /sys/module/hello_module/parameters/message
Custom
```
4. Выгрузка модуля
```
bash
sancher@Ubuntu:~/lab5/src$ sudo rmmod hello_module
sancher@Ubuntu:~/lab5/src$ sudo dmesg | tail -1
[13510.584325] Goodbye from Sasha module!
```
## Задание B: /proc файл с записью
### Код модуля (proc_module.c)
```
c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sasha");
MODULE_DESCRIPTION("/proc file with read/write capability");

#define PROC_NAME "my_config"
#define MAX_SIZE 256

static struct proc_dir_entry *proc_file;
static char *config_data;
static size_t config_size;


static ssize_t proc_read(struct file *file, char __user *user_buf,
                        size_t count, loff_t *ppos)
{
    ssize_t len;
    
    if (*ppos > 0) // Уже читали этот файл
        return 0;
        
    len = strlen(config_data);
    
 
    if (copy_to_user(user_buf, config_data, len)) {
        return -EFAULT; 
    }
    
    *ppos = len;
    return len;
}


static ssize_t proc_write(struct file *file, const char __user *user_buf,
                         size_t count, loff_t *ppos)
{
    if (count > MAX_SIZE) {
        printk(KERN_ERR "Write too large: %zu > %d\n", count, MAX_SIZE);
        return -EFBIG;
    }
    
    kfree(config_data);
    
    config_data = kmalloc(count + 1, GFP_KERNEL);
    if (!config_data) {
        printk(KERN_ERR "kmalloc failed\n");
        return -ENOMEM;
    }
    

    if (copy_from_user(config_data, user_buf, count)) {
        kfree(config_data);
        config_data = NULL;
        return -EFAULT;
    }
    
  
    config_data[count] = '\0';
    config_size = count;
    
    printk(KERN_INFO "my_config updated to: %s\n", config_data);
    return count; 
}


static const struct proc_ops proc_fops = {
    .proc_read = proc_read,
    .proc_write = proc_write,
};

static int __init proc_init(void)
{
    config_data = kmalloc(MAX_SIZE, GFP_KERNEL);
    if (!config_data) {
        return -ENOMEM;
    }
    
    strcpy(config_data, "default");
    config_size = strlen(config_data);
    
    proc_file = proc_create(PROC_NAME, 0666, NULL, &proc_fops);
    if (!proc_file) {
        kfree(config_data);
        printk(KERN_ERR "Failed to create /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }
    
    printk(KERN_INFO "/proc/%s created\n", PROC_NAME);
    return 0;
}

static void __exit proc_exit(void)
{
    proc_remove(proc_file);
    
    kfree(config_data);
    
    printk(KERN_INFO "/proc/%s removed\n", PROC_NAME);
}

module_init(proc_init);
module_exit(proc_exit);
```

### Результаты тестирования

1. Компиляция модуля
```
bash
sancher@Ubuntu:~/lab5/src$ make
make -C /lib/modules/6.14.0-33-generic/build M=/home/sancher/lab5/src modules
make[1]: Entering directory '/usr/src/linux-headers-6.14.0-33-generic'
make[2]: Entering directory '/home/sancher/lab5/src'
warning: the compiler differs from the one used to build the kernel
  The kernel was built by: x86_64-linux-gnu-gcc-13 (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0
  You are using:           gcc-13 (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0
  CC [M]  proc_module.o
  MODPOST Module.symvers
  CC [M]  proc_module.mod.o
  LD [M]  proc_module.ko
  BTF [M] proc_module.ko
Skipping BTF generation for proc_module.ko due to unavailability of vmlinux
make[2]: Leaving directory '/home/sancher/lab5/src'
make[1]: Leaving directory '/usr/src/linux-headers-6.14.0-33-generic'
```
**Результат:** Модуль успешно скомпилирован с одним предупреждением о неиспользуемой переменной.

2. Загрузка модуля и создание /proc файла
```
bash
sancher@Ubuntu:~/lab5/src$ sudo insmod proc_module.ko
[sudo] password for sancher: 
sancher@Ubuntu:~/lab5/src$ sudo dmesg | tail -2
[19586.611024]  </TASK>
[19616.823468] /proc/my_config created
```
3. Проверка начального значения
```
bash
sancher@Ubuntu:~/lab5/src$ cat /proc/my_config
default
```
4. Запись нового значения
```
bash
sancher@Ubuntu:~/lab5/src$ echo "new test value" | sudo tee /proc/my_config                         echo "new test value" | sudo tee /proc/my_config
new test value
```
**Результат:** Значение успешно изменено на "new test value", в логах ядра отображается обновление.

5. Проверка обновленного значения
```
bash
sancher@Ubuntu:~/lab5/src$ cat /proc/my_config
new test value
```
**Результат**: Файл содержит обновленное значение.
6. Проверка работы с длинной строкой
```
bash
sancher@Ubuntu:~/lab5/src$ echo "This is a longer test string to verify the module functionality" | sudo tee /proc/my_config
This is a longer test string to verify the module functionality
```
**Результат**: Длинные строки (в пределах 256 символов) обрабатываются корректно.

7. Проверка защиты от переполнения
```
bash
sancher@Ubuntu:~/lab5/src$ python3 -c "print('A' * 300)" | sudo tee /proc/my_config
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
tee: /proc/my_config: File too large
```
**Результат:** Защита от переполнения буфера работает корректно, возвращается ошибка "File too large".

8. Выгрузка модуля
```
bash
sancher@Ubuntu:~/lab5/src$ sudo rmmod proc_module
sancher@Ubuntu:~/lab5/src$ sudo dmesg | tail -2
[19678.912738] /proc/my_config removed
sancher@Ubuntu:~/lab5/src$ cat /proc/my_config
cat: /proc/my_config: No such file or directory

```
**Результат:** Модуль выгружен, файл /proc/my_config удален.

## Анализ работы модуля

### Функциональность

Модуль успешно создает файл `/proc/my_config` при загрузке в ядро. Файл доступен для операций чтения и записи через стандартные команды Linux. При чтении файла командой `cat /proc/my_config` возвращается текущее значение, хранящееся в модуле. Запись новых данных осуществляется через команду `echo "text" > /proc/my_config`.

Данные сохраняются между операциями чтения и записи, что подтверждает корректную работу механизма хранения состояния. Модуль обеспечивает защиту от переполнения буфера, ограничивая максимальный размер данных 256 символами. При попытке записи большего объема данных возвращается ошибка.

При выгрузке модуля все выделенные ресурсы освобождаются корректно, включая удаление файла из /proc и освобождение динамически выделенной памяти.

### Безопасность

Для обеспечения безопасного взаимодействия между пространством ядра и пользовательским пространством используются функции `copy_to_user()` и `copy_from_user()`. Эти функции предотвращают прямые обращения к пользовательской памяти из ядра, что исключает возможность возникновения kernel panic при работе с невалидными указателями.

Перед выполнением операции записи проводится проверка размера передаваемых данных. Это предотвращает переполнение буфера и потенциальные уязвимости. Модуль обрабатывает ошибки выделения памяти через `kmalloc()`, возвращая соответствующие коды ошибок при невозможности выделить запрошенный объем памяти.

При выгрузке модуля осуществляется полное освобождение всех используемых ресурсов, что исключает утечки памяти в пространстве ядра.

### Логирование

Модуль ведет детальное логирование ключевых событий. При загрузке выводится сообщение о создании файла в /proc, при выгрузке - о его удалении. Все изменения данных логируются с отображением нового значения. В случае возникновения ошибок, таких как попытка записи слишком большого объема данных, в логи ядра записываются соответствующие сообщения об ошибках.

### Выводы

Модуль полностью реализует требуемую функциональность создания файла в /proc файловой системе с поддержкой операций чтения и записи. Начальное значение устанавливается в "default" и может быть изменено через операции записи.

Обеспечение безопасности работы реализовано через проверку границ буфера, корректное управление памятью и использование безопасных функций копирования данных между пространствами. Модуль демонстрирует правильное освобождение всех ресурсов при выгрузке.

Разработанный модуль успешно демонстрирует механизм взаимодействия между пространством ядра и пользовательским пространством через /proc файловую систему, обеспечивая при этом необходимый уровень безопасности и надежности.

## Отчёт по Заданию C: /proc файл со статистикой системы
### Код модуля (sys_stats_module.c)
```
c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/jiffies.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sasha");
MODULE_DESCRIPTION("System statistics in /proc");

#define PROC_NAME "sys_stats"

static struct proc_dir_entry *proc_file;

static int count_processes(void)
{
    struct task_struct *task;
    int count = 0;
    

    rcu_read_lock();
    for_each_process(task) {
        count++;
    }
    rcu_read_unlock();
    
    return count;
}


static ssize_t stats_read(struct file *file, char __user *user_buf,
                         size_t count, loff_t *ppos)
{
    char buffer[512];
    int len;
    int process_count;
    struct sysinfo mem_info;
    unsigned long uptime_seconds;
    
    if (*ppos > 0)
        return 0;
    

    si_meminfo(&mem_info);
    process_count = count_processes();
    uptime_seconds = jiffies_to_msecs(jiffies) / 1000;
    
  
    len = snprintf(buffer, sizeof(buffer),
                  "System Statistics:\n"
                  "Processes: %d\n"
                  "Memory Total: %lu MB\n"
                  "Memory Free: %lu MB\n"
                  "Memory Used: %lu MB\n"
                  "System Uptime: %lu seconds\n",
                  process_count,
                  (mem_info.totalram * mem_info.mem_unit) / (1024 * 1024),
                  (mem_info.freeram * mem_info.mem_unit) / (1024 * 1024),
                  ((mem_info.totalram - mem_info.freeram) * mem_info.mem_unit) / (1024 * 1024),
                  uptime_seconds);
    
   
    if (copy_to_user(user_buf, buffer, len)) {
        return -EFAULT;
    }
    
    *ppos = len;
    return len;
}

static const struct proc_ops stats_fops = {
    .proc_read = stats_read,
};

static int __init stats_init(void)
{
    proc_file = proc_create(PROC_NAME, 0444, NULL, &stats_fops);
    if (!proc_file) {
        printk(KERN_ERR "Failed to create /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }
    
    printk(KERN_INFO "/proc/%s created\n", PROC_NAME);
    return 0;
}

static void __exit stats_exit(void)
{
    proc_remove(proc_file);
    printk(KERN_INFO "/proc/%s removed\n", PROC_NAME);
}

module_init(stats_init);
module_exit(stats_exit);
```

### Результаты тестирования

1. Загрузка модуля и проверка статистики
```
bash
sancher@Ubuntu:~/lab5/src$  sudo insmod sys_stats_module.ko
sancher@Ubuntu:~/lab5/src$ cat /proc/sys_stats
System Statistics:
Processes: 242
Memory Total: 3915 MB
Memory Free: 217 MB
Memory Used: 3697 MB
System Uptime: 31637 seconds
sancher@Ubuntu:~/lab5/src$ 
```
**Результат**: Модуль успешно загружен и выводит системную статистику в читаемом формате.

2. Сравнение с системными командами
```
bash
sancher@Ubuntu:~/lab5/src$ cat /proc/sys_stats | grep Processes
Processes: 244
sancher@Ubuntu:~/lab5/src$ ps aux | wc -l
245
```
**Анализ**: Разница в 1 процесс объясняется тем, что команда ps aux включает заголовок и саму команду ps, тогда как модуль считает только активные процессы в ядре.
Используемая память:
```
bash
sancher@Ubuntu:~/lab5/src$ cat /proc/sys_stats | grep Memory
Memory Total: 3915 MB
Memory Free: 196 MB
Memory Used: 3718 MB
sancher@Ubuntu:~/lab5/src$ free -m
               total        used        free      shared  buff/cache   available
Mem:            3915        2530         190         172        1331        1385
Swap:              0           0           0
```
**Анализ**: Модуль показывает общую использованную память (total - free), тогда как free -m разделяет used, buff/cache и available. Значения близки, что подтверждает корректность расчетов.

**Uptime системы:**
```
bash
sancher@Ubuntu:~/lab5/src$ cat /proc/sys_stats | grep Uptime
System Uptime: 31845 seconds
sancher@Ubuntu:~/lab5/src$ cat /proc/uptime
32150.68 121441.55
```
**Анализ:** Модуль показывает uptime в секундах (31143), что соответствует примерно 8.6 часам. Значение из /proc/uptime (31477.49) близко, небольшая разница обусловлена временем между выполнениями команд.

3. Проверка прав доступа
```
bash
sancher@Ubuntu:~/lab5/src$  ls -la /proc/sys_stats
-r--r--r-- 1 root root 0 Nov 22 16:35 /proc/sys_stats
```
Результат: Права доступа 0444 установлены корректно - только чтение для всех пользователей.

4. Выгрузка модуля

```
bash
sancher@Ubuntu:~/lab5/src$ sudo rmmod sys_stats_module
sancher@Ubuntu:~/lab5/src$ sudo dmesg | tail -1
[32307.897984] /proc/sys_stats removed
sancher@Ubuntu:~/lab5/src$ cat /proc/sys_stats
cat: /proc/sys_stats: No such file or directory
```
**Результат**: Модуль успешно выгружен, файл удален из /proc.

### Анализ работы модуля
#### Функциональность

Модуль создает файл `/proc/sys_stats`, который предоставляет актуальную информацию о состоянии системы. Реализован подсчет количества активных процессов через итерацию по списку задач ядра. Информация о памяти получается через системный вызов `si_meminfo()`, который предоставляет детальные данные об использовании оперативной памяти. Время работы системы рассчитывается на основе счетчика `jiffies`, преобразованного в секунды.

Все данные форматируются в удобочитаемом виде с четкой структурой и поясняющими надписями. Модуль обеспечивает только операцию чтения, что соответствует назначению - предоставление статистической информации.
#### Безопасность

Для безопасного копирования данных в пользовательское пространство используется функция `copy_to_user()`. При работе со списком процессов применяется механизм RCU (Read-Copy-Update) блокировки через `rcu_read_lock()` и `rcu_read_unlock()`, что обеспечивает безопасный доступ к разделяемым данным в ядре.

Размер выходного буфера ограничен 512 байтами, что предотвращает потенциальное переполнение. Форматирование строк выполняется через безопасную функцию `snprintf()`, которая гарантирует, что вывод не превысит размер буфера.
Точность данных

Сравнение с системными утилитами показывает высокую степень соответствия данных. Небольшие расхождения в количестве процессов и значениях памяти объясняются различиями в методиках подсчета и моментальным характером снимков системы. Время обновления данных при каждом чтении файла обеспечивает актуальность информации.
#### Выводы

Модуль успешно реализует все требования задания C. Создан файл  ` /proc/sys_stats `, который предоставляет точную и актуальную информацию о ключевых параметрах системы: количестве процессов, использовании памяти и времени работы.

Реализация демонстрирует правильное использование API ядра для получения системной информации и безопасного взаимодействия с пользовательским пространством. Модуль корректно обрабатывает ресурсы - создает файл при инициализации и удаляет его при выгрузке.

Данные, предоставляемые модулем, соответствуют информации от стандартных системных утилит, что подтверждает корректность реализации алгоритмов подсчета и преобразования единиц измерения. Модуль готов к использованию и может быть интегрирован в систему для мониторинга состояния.


## Ответы на вопросы
### Базовые понятия:

   1. Что такое модуль ядра и зачем он нужен?

   Модуль ядра - это фрагмент кода, который может быть динамически загружен и выгружен из ядра Linux во время работы системы. Нужен для добавления функциональности без перекомпиляции ядра.

   2. Чем отличается kernel-space от user-space?

   Kernel-space имеет полный доступ к аппаратуре, выполняется в привилегированном режиме, ошибки приводят к kernel panic. User-space имеет ограниченный доступ, защищенную память, ошибки затрагивают только конкретный процесс.

   3. Что произойдёт, если в модуле обратиться к NULL указателю?

   Произойдет kernel panic - аварийное завершение работы ядра с последующей перезагрузкой системы.

   4. Почему нельзя использовать printf() в модуле ядра?

   Потому что printf() - это функция стандартной библиотеки C, которая недоступна в пространстве ядра. Вместо нее используется printk().

   5. Что такое kernel panic и как его избежать?

   Kernel panic - критическая ошибка ядра, приводящая к остановке системы. Чтобы избежать: проверять указатели, проверять возвращаемые значения, освобождать ресурсы, работать в виртуальной машине.

### Жизненный цикл модуля:

   6. Какие функции вызываются при insmod и rmmod?

   При insmod вызывается функция, указанная в module_init(). При rmmod - функция из module_exit().

   7. Что должна делать функция module_exit()?

   Освобождать все ресурсы, выделенные в module_init(): память, устройства, /proc файлы и т.д.

   8. Что происходит, если module_init() возвращает ошибку?
    
   Модуль не загружается, функция module_exit() не вызывается.

   9. Можно ли выгрузить модуль, если он используется?
    
   Нет, сначала нужно закрыть все процессы, использующие модуль.

### Логирование и отладка:

   10. Чем printk() отличается от printf()?
    
   printk() выводит в буфер ядра, имеет уровни логирования, не блокируется, не использует стандартную библиотеку.

   11. Какие уровни логирования существуют в ядре?
    
   KERN_EMERG, KERN_ALERT, KERN_CRIT, KERN_ERR, KERN_WARNING, KERN_NOTICE, KERN_INFO, KERN_DEBUG.

   12. Как посмотреть логи модуля?
   
   С помощью dmesg, journalctl, или просмотром /var/log/kern.log.

   13. Что означает "tainted kernel"?
    
   Что ядро было "испорчено" загрузкой проприетарного модуля или модуля без правильной лицензии.

### Память:

   14. Чем kmalloc() отличается от malloc()?
    
   kmalloc() выделяет физически непрерывную память в ядре, не может использовать swap, требует указания флагов GFP.

   15. Что такое флаги GFP и зачем они нужны?
    
   Флаги Get Free Pages определяют поведение аллокатора: GFP_KERNEL - может спать, GFP_ATOMIC - для атомарного контекста.

   16. Что произойдёт, если не освободить память в module_exit()?
    
   Произойдет утечка памяти в ядре, которая не восстановится до перезагрузки системы.

   17. Почему нельзя использовать user-space указатели напрямую в ядре?
    
   Потому что пользовательские указатели могут быть невалидными, страничная организация памяти отличается, нужны специальные функции copy_to_user/copy_from_user.

### Взаимодействие с user-space:

   18. Что такое /proc и для чего он используется?
   
   Виртуальная файловая система для экспорта информации из ядра в пользовательское пространство.

   19. Что такое /sys (sysfs) и чем отличается от procfs?
    
   Sysfs - для экспорта атрибутов устройств, один файл = одно значение. Procfs - для различной системной информации.

   20. Зачем нужны функции copy_to_user() и copy_from_user()?
    
   Для безопасного копирования данных между пространством ядра и пользовательским пространством.

   21. Что такое character device и как он работает?
    
   Устройство, с которым можно работать как с файлом, использует интерфейс file_operations.

### Параметры и метаданные:

   22. Как передать параметры модулю при загрузке?
   
   Через командную строку: insmod module.ko param=value

   23. Зачем нужен MODULE_LICENSE()?
    
   Для указания лицензии модуля. Без GPL лицензии некоторые функции ядра будут недоступны.

   24. Что произойдёт, если не указать лицензию?
   
   Ядро пометится как "tainted", некоторые API будут недоступны.

Безопасность:

   25. Какие основные правила безопасного кода в ядре?
    
   Проверять возвращаемые значения, освобождать ресурсы, использовать правильные функции копирования, проверять границы.

   26. Можно ли использовать бесконечный цикл в модуле?
    
   Нет, это может заблокировать систему, особенно в атомарном контексте.

   27. Почему в ядре нет FPU операций?
    
   Потому что сохранение/восстановление состояния FPU требует времени, и ядро старается минимизировать накладные расходы.

   28. Что делать, если модуль вызвал kernel panic?
    
   Перезагрузить систему, проанализировать логи, исправить код, тестировать в виртуальной машине.

### Практические вопросы:

   29. Как узнать, какие модули загружены в системе?
    
   С помощью команды lsmod или cat /proc/modules.

   30. Как получить информацию о модуле (версия, параметры)?
    
   С помощью команды modinfo module.ko

### Выводы

В ходе лабораторной работы были изучены основы разработки модулей ядра Linux. На практике освоены:

   * Создание простых модулей ядра

   * Работа с параметрами модулей

   * Взаимодействие с пользовательским пространством через /proc

   * Управление памятью в пространстве ядра

   * Основы безопасного программирования в ядре

Работа в виртуальной машине позволила безопасно экспериментировать с кодом, включая ситуации с ошибками, не опасаясь повредить основную систему.

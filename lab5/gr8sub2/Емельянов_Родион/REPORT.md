# Лабораторная работа 5 - Модули ядра Linux
## Вариант 2

**Студент:** Емельянов Родион

**Группа:** 8, Подгруппа: 2

---

## Задание A: Hello World модуль

### Код реализации
```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/moduleparam.h>

static char *message = "Hello from Rodion module!";

module_param(message, charp, 0644);
MODULE_PARM_DESC(message, "Message to display instead of default");

static int __init hello_init(void) {
    printk(KERN_INFO "%s\n", message);
    return 0;
}

static void __exit hello_exit(void) {
    printk(KERN_INFO "Goodbye from Rodion module!\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
```

### Тестирование

#### Команды тестирования:
```bash
make
sudo insmod hello_module.ko
sudo dmesg | tail -5
sudo rmmod hello_module
sudo insmod hello_module.ko message="hihi"
sudo dmesg | tail -5
```

#### Результаты:
```
daberauoy@ubuntu-vm:~/labs/lr5$ sudo insmod hello_module.ko
daberauoy@ubuntu-vm:~/labs/lr5$ sudo rmmod hello_module
daberauoy@ubuntu-vm:~/labs/lr5$ sudo insmod hello_module.ko message="hihi"
daberauoy@ubuntu-vm:~/labs/lr5$ sudo dmesg | tail -5
[  259.293553] audit: type=1400 audit(1764015163.834:174): apparmor="STATUS" operation="profile_replace" info="same as current profile, skipping" profile="unconfined" name="snap.firefox.hook.configure" pid=3957 comm="apparmor_parser"
[  259.296374] audit: type=1400 audit(1764015163.837:175): apparmor="STATUS" operation="profile_replace" info="same as current profile, skipping" profile="unconfined" name="snap.firefox.hook.install" pid=3959 comm="apparmor_parser"
[ 1608.789738] Hello from Rodion module!
[ 1745.652537] Goodbye from Rodion module!
[ 1747.840089] hihi
daberauoy@ubuntu-vm:~/labs/lr5$ lsmod | grep hello_module
hello_module           12288  0
```

#### Выводы:
- Модуль успешно загружается и выгружается
- Параметр message корректно обрабатывается
- Сообщения выводятся в kernel log

---

## Задание B: /proc файл с записью

### Код реализации proc_module.c - одновременно реализация B и C заданий
```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>
#include <linux/mm.h>
#include <linux/sched/signal.h>
#include <linux/seq_file.h>

#define MAX_LEN 256

static char my_config_buf[MAX_LEN] = "default";

static struct proc_dir_entry *proc_my_config;
static struct proc_dir_entry *proc_sys_stats;

// Чтение из /proc/my_config
static ssize_t my_config_read(struct file *file, char __user *buf, size_t count, loff_t *ppos) {
    int len = strlen(my_config_buf);

    if (*ppos >= len)
        return 0;

    if (count > len - *ppos)
        count = len - *ppos;

    if (copy_to_user(buf, my_config_buf + *ppos, count))
        return -EFAULT;

    *ppos += count;
    return count;
}

// Запись в /proc/my_config
static ssize_t my_config_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos) {
    if (count > MAX_LEN - 1)
        return -EINVAL;

    if (copy_from_user(my_config_buf, buf, count))
        return -EFAULT;

    my_config_buf[count] = '\0';

    // Удаляем возможный \n в конце
    if (count > 0 && my_config_buf[count - 1] == '\n')
        my_config_buf[count - 1] = '\0';

    return count;
}

static const struct proc_ops my_config_fops = {
    .proc_read = my_config_read,
    .proc_write = my_config_write,
};

// Чтение из /proc/sys_stats
static int sys_stats_show(struct seq_file *m, void *v) {
    struct sysinfo info;
    unsigned long uptime_secs;
    int process_count = 0;
    struct task_struct *task;

    // Кол-во процессов
    for_each_process(task) {
        process_count++;
    }

    // Инфо о памяти
    si_meminfo(&info);

    // Uptime (jiffies to seconds)
    uptime_secs = jiffies_to_msecs(get_jiffies_64()) / 1000;

    seq_printf(m, "Processes: %d\n", process_count);
    seq_printf(m, "Memory Used: %lu MB\n", (info.totalram - info.freeram) * info.mem_unit / (1024 * 1024));
    seq_printf(m, "System Uptime: %lu seconds\n", uptime_secs);

    return 0;
}

static int sys_stats_open(struct inode *inode, struct file *file) {
    return single_open(file, sys_stats_show, NULL);
}

static const struct proc_ops sys_stats_fops = {
    .proc_open = sys_stats_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

static int __init proc_module_init(void) {
    proc_my_config = proc_create("my_config", 0666, NULL, &my_config_fops);
    if (!proc_my_config) {
        printk(KERN_ERR "Failed to create /proc/my_config\n");
        return -ENOMEM;
    }

    proc_sys_stats = proc_create("sys_stats", 0444, NULL, &sys_stats_fops);
    if (!proc_sys_stats) {
        proc_remove(proc_my_config);
        printk(KERN_ERR "Failed to create /proc/sys_stats\n");
        return -ENOMEM;
    }

    printk(KERN_INFO "proc_module loaded\n");
    return 0;
}

static void __exit proc_module_exit(void) {
    proc_remove(proc_my_config);
    proc_remove(proc_sys_stats);
    printk(KERN_INFO "proc_module unloaded\n");
}

module_init(proc_module_init);
module_exit(proc_module_exit);

MODULE_LICENSE("GPL");
```

#### Результаты:
```
daberauoy@ubuntu-vm:~/labs/lr5$ cat /proc/my_config
default
daberauoy@ubuntu-vm:~/labs/lr5$ echo "new value" > /proc/my_config
daberauoy@ubuntu-vm:~/labs/lr5$ cat /proc/my_config
new value
```

#### Выводы:
- Файл /proc/my_config создаётся успешно
- Чтение и запись работают корректно
- Данные сохраняются между операциями чтения

---

## Задание C: /proc файл со статистикой системы

#### Результаты:

```
daberauoy@ubuntu-vm:~/labs/lr5$ cat /proc/sys_stats
Processes: 237
Memory Used: 3230 MB
System Uptime: 1810 seconds
```

#### Выводы:
- Статистика системы корректно собирается и отображается
- Количество процессов, используемая память и uptime отображаются правильно

---

## Ответы на вопросы

### Базовые понятия

1. **Что такое модуль ядра и зачем он нужен?**  
Модуль ядра — фрагмент кода, который можно динамически загружать в ядро Linux для расширения его функциональности без перезагрузки. Он используется для добавления поддержки устройств, файловых систем, сетевых протоколов и др.

2. **Чем отличается kernel-space от user-space?**  
Kernel-space — привилегированное пространство работы ядра с полным доступом к оборудованию и памяти. User-space — пространство обычных программ с ограниченными правами, где работает большинство приложений.

3. **Что произойдёт, если в модуле обратиться к NULL указателю?**  
Будет попытка доступа к невалидной памяти, что вызовет kernel panic — аварийный крах ядра.

4. **Почему нельзя использовать `printf()` в модуле ядра?**  
`printf()` — функция user-space, она выводит в стандартный поток. В ядре есть своя функция `printk()`, работающая с системным логом, и `printf()` недоступна.

5. **Что такое kernel panic и как его избежать?**  
Kernel panic — критическая ошибка ядра, приводящая к зависанию или перезагрузке системы. Избегают его через проверку указателей, корректное управление памятью и осторожное программирование.

### Жизненный цикл модуля

6. **Какие функции вызываются при `insmod` и `rmmod`?**  
При `insmod` вызывается функция инициализации (обычно с атрибутом `__init`), при `rmmod` — функция выгрузки (с атрибутом `__exit`).

7. **Что должна делать функция `module_exit()`?**  
Освобождать все ресурсы, отменять регистрации, освобождать память, чтобы не было утечек и ошибок.

8. **Что происходит, если `module_init()` возвращает ошибку?**  
Модуль не загружается, и `module_exit()` не вызывается.

9. **Можно ли выгрузить модуль, если он используется?**  
Нет, ядро не позволит выгрузить модуль, если он используется (например, если открыт файл устройства).

### Логирование и отладка

10. **Чем `printk()` отличается от `printf()`?**  
`printk()` выводит сообщения в системный журнал ядра с различными уровнями важности, доступный через `dmesg`. `printf()` работает в user-space.

11. **Какие уровни логирования существуют в ядре?**  
Уровни от 0 (EMERG, аварийный) до 7 (DEBUG, отладка), например, KERN_INFO, KERN_ERR.

12. **Как посмотреть логи модуля?**  
Командой `dmesg` или через `journalctl -k`.

13. **Что означает "tainted kernel"?**  
Это состояние ядра, помеченное как "испорченное", возникает, когда загружены проприетарные или неподписанные модули, или когда произошли ошибки.

### Память

14. **Чем `kmalloc()` отличается от `malloc()`?**  
`kmalloc()` используется в kernel-space, выделяет физическую память без свопа. `malloc()` — в user-space, выделяет виртуальную память с возможностью свопа.

15. **Что такое флаги GFP и зачем они нужны?**  
Флаги GFP (Get Free Pages) задают условия выделения памяти (`GFP_KERNEL`, `GFP_ATOMIC`), влияя на поведение аллокатора.

16. **Что произойдёт, если не освободить память в `module_exit()`?**  
Произойдет утечка памяти в ядре, которая не будет освобождена до перезагрузки.

17. **Почему нельзя использовать user-space указатели напрямую в ядре?**  
Потому что адреса user-space не валидны в kernel-space; используются функции `copy_to_user()` и `copy_from_user()` для безопасного копирования данных.

### Взаимодействие с user-space

18. **Что такое `/proc` и для чего он используется?**  
Виртуальная файловая система, предоставляющая интерфейс для передачи информации между ядром и user-space.

19. **Что такое `/sys` (sysfs) и чем отличается от procfs?**  
Sysfs — современная виртуальная файловая система для отображения атрибутов устройств и драйверов. В отличие от procfs более структурирована и предназначена для управления устройствами.

20. **Зачем нужны функции `copy_to_user()` и `copy_from_user()`?**  
Для безопасного копирования данных между kernel-space и user-space, предотвращая обращение к невалидным адресам.

21. **Что такое character device и как он работает?**  
Это устройство, обрабатывающее данные посимвольно, доступное через файл в `/dev`. Для него реализуются операции `open`, `read`, `write`, `release`.

### Параметры и метаданные

22. **Как передать параметры модулю при загрузке?**  
Через `module_param()` в коде и параметры командной строки `insmod module.ko param=value`.

23. **Зачем нужен `MODULE_LICENSE()`?**  
Указывает лицензию модуля для ядра, влияет на доступность внутренних функций и статус ядра (tainted или нет).

24. **Что произойдёт, если не указать лицензию?**  
Ядро пометит модуль и ядро как "испорченное" (tainted), ограничит доступ к некоторым функциям.

### Безопасность

25. **Какие основные правила безопасного кода в ядре?**  
Проверять возвращаемые значения, освобождать ресурсы, использовать `copy_to_user()`/`copy_from_user()`, избегать бесконечных циклов и обращения к NULL.

26. **Можно ли использовать бесконечный цикл в модуле?**  
Нельзя, это приведет к блокировке системы и kernel panic.

27. **Почему в ядре нет FPU операций?**  
Потому что ядро не использует процессорный FPU для работы, чтобы избежать сложностей переключения состояний и снизить накладные расходы.

28. **Что делать, если модуль вызвал kernel panic?**  
Перезагрузить систему, восстановить из snapshot (если в VM), найти и исправить ошибку, добавить printk для отладки.

### Практические вопросы

29. **Как узнать, какие модули загружены в системе?**  
Команда `lsmod` или просмотр `/proc/modules`.

30. **Как получить информацию о модуле (версия, параметры)?**  
Команда `modinfo module_name.ko`.

---

## Использование AI

При выполнении лабораторной работы использовал AI для:
- Проверки синтаксиса kernel API
- Поиска оптимальных способов получения системной статистики
- Генерации шаблона отчёта










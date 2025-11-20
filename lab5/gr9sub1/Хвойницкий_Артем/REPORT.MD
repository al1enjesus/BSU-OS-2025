# Лабораторная работа №5 (Вариант 2) 
## Модули ядра Linux

**Студент:** Хвойницкий Артём  
**Группа:** группа 9, подгруппа 1  
**Каталог работы:** `lab5/gr9sub1/Хвойницкий_Артем`  

---

## Цель работы

Изучить механизм создания, сборки, загрузки и выгрузки модулей ядра Linux, а также взаимодействие с файловой системой `/proc` для обмена данными между ядром и пользовательским пространством.

---

## Оборудование и программное обеспечение

- Операционная система: GNU/Linux (Ubuntu-подобный дистрибутив, запуск в VirtualBox)
- Версия ядра:
    uname -r
    6.14.0-29-generic
- Среда разработки:
  - компилятор: gcc-14
  - система сборки: make
  - заголовки ядра: linux-headers-6.14.0-29-generic
- Рабочий каталог для исходников:
    ~/lab5/gr9sub1/Хвойницкий_Артем/src

---

## Структура проекта

    lab5/
      gr9sub1/
        Хвойницкий_Артем/
          REPORT.MD
          Makefile
          src/
            hello_module.c
            my_config_module.c
            sys_stats_module.c
            char_device.c
          screenshots/
            a1_hello_insmod_default.png
            a2_hello_rmmod_default.png
            a3_hello_insmod_custom.png
            a4_hello_rmmod_custom.png
            b1_my_config_insmod_and_default.png
            b2_my_config_write_and_read.png
            b3_my_config_dmesg.png
            b4_my_config_rmmod.png
            c1_sys_stats_insmod_and_ls.png
            c2_sys_stats_cat.png
            c3_sys_stats_rmmod.png
          logs/
            ... текстовые логи (dmesg, cat /proc/...) ...

Сборка модулей:

    cd ~/lab5/gr9sub1/Хвойницкий_Артем/src
    make -C /lib/modules/$(uname -r)/build M=$(pwd) modules

Сборка завершалась успешно, были только информационные предупреждения (настройка компилятора, BTF).

---

## Makefile

    obj-m += hello_module.o
    obj-m += my_config_module.o
    obj-m += sys_stats_module.o
    obj-m += char_device.o

    KDIR := /lib/modules/$(shell uname -r)/build
    PWD  := $(shell pwd)

    all:
        $(MAKE) -C $(KDIR) M=$(PWD) modules

    clean:
        $(MAKE) -C $(KDIR) M=$(PWD) clean

---

## Задание A — Hello World модуль (`hello_module`)

### Постановка задачи

Создать модуль ядра, который:

1. При загрузке (`insmod`) выводит:  
   `Hello from [ИМЯ] module!`
2. При выгрузке (`rmmod`) выводит:  
   `Goodbye from [ИМЯ] module!`
3. Принимает строковый параметр `message`.
4. Если параметр задан, выводит его вместо сообщения по умолчанию.

Требования:

- использовать `printk` с уровнем `KERN_INFO`;
- указать метаданные `MODULE_LICENSE`, `MODULE_AUTHOR`;
- использовать `module_param` для параметра.

### Исходный код `hello_module.c`

    #include <linux/module.h>
    #include <linux/kernel.h>
    #include <linux/init.h>
    #include <linux/moduleparam.h>

    static char *message = "Hello from Artsem Khvainitski module!";
    module_param(message, charp, 0444);
    MODULE_PARM_DESC(message, "Custom greeting message");

    static int __init hello_init(void)
    {
        printk(KERN_INFO "hello_module: %s\n", message);
        return 0;
    }

    static void __exit hello_exit(void)
    {
        printk(KERN_INFO "hello_module: Goodbye from Artsem Khvainitski module!\n");
    }

    module_init(hello_init);
    module_exit(hello_exit);

    MODULE_LICENSE("GPL");
    MODULE_AUTHOR("Artsem Khvainitski");
    MODULE_DESCRIPTION("Simple Hello World kernel module with parameter");

### Ход выполнения

#### A1. Загрузка модуля без параметров

Команды:

    cd ~/lab5/gr9sub1/Хвойницкий_Артем/src
    sudo insmod hello_module.ko
    sudo dmesg | tail -20

Фрагмент `dmesg`:

    hello_module: Hello from Artsem Khvainitski module!

Скриншот: `screenshots/a1_hello_insmod_default.png`.

#### A2. Выгрузка модуля без параметров

    sudo rmmod hello_module
    sudo dmesg | tail -20

Фрагмент `dmesg`:

    hello_module: Goodbye from Artsem Khvainitski module!

Скриншот: `screenshots/a2_hello_rmmod_default.png`.

#### A3. Загрузка модуля с параметром `message`

Корректный вызов:

    sudo insmod hello_module.ko message="Custom greeting from Artem"
    sudo dmesg | tail -20

При корректной передаче параметра ожидается строка вида:

    hello_module: Custom greeting from Artem

В ходе эксперимента были получены и ошибочные варианты без кавычек, которые приводили к сообщениям:

    hello_module: unknown parameter 'greeting' ignored
    hello_module: Custom

Это связано с тем, что ядро воспринимает каждое слово как отдельный параметр, если строка не заключена в кавычки.

Скриншот: `screenshots/a3_hello_insmod_custom.png`.

#### A4. Выгрузка модуля после работы с параметром

    sudo rmmod hello_module
    sudo dmesg | tail -20

Финальный фрагмент лога по модулю:

    hello_module: Goodbye from Artsem Khvainitski module!

Скриншот: `screenshots/a4_hello_rmmod_custom.png`.

### Выводы по заданию A

- Реализован простой модуль ядра с функциями `module_init`/`module_exit`.
- Сообщения выводятся через `printk(KERN_INFO, ...)` при загрузке и выгрузке.
- Строковый параметр `message` объявлен через `module_param` и может задаваться при `insmod`.
- Ошибки при передаче параметров без кавычек приводят к сообщениям `unknown parameter`, что демонстрирует важность корректного синтаксиса вызова `insmod`.

---

## Задание B — `/proc` файл с записью (`my_config_module`)

### Постановка задачи

Создать модуль ядра, который создаёт файл `/proc/my_config`:

- по умолчанию содержит строку `"default"`;
- при записи (`echo "text" > /proc/my_config`) сохраняет новое значение;
- при чтении (`cat /proc/my_config`) возвращает текущее значение;
- максимальная длина строки — 256 символов.

Требования:

- использовать `proc_create()` с правами `0666`;
- реализовать функции чтения и записи (`proc_read` и `proc_write`);
- использовать `copy_from_user()` и `copy_to_user()`.

### Исходный код `my_config_module.c`

    #include <linux/module.h>
    #include <linux/kernel.h>
    #include <linux/init.h>
    #include <linux/proc_fs.h>
    #include <linux/uaccess.h>

    #define PROC_NAME "my_config"
    #define BUF_LEN   256

    static struct proc_dir_entry *proc_entry;
    static char config_value[BUF_LEN] = "default";

    static ssize_t my_config_read(struct file *file, char __user *buffer,
                                  size_t length, loff_t *offset)
    {
        int len;

        if (*offset > 0)
            return 0; // EOF

        len = strlen(config_value);

        if (copy_to_user(buffer, config_value, len))
            return -EFAULT;

        *offset = len;
        return len;
    }

    static ssize_t my_config_write(struct file *file, const char __user *buffer,
                                   size_t length, loff_t *offset)
    {
        size_t to_copy = min(length, (size_t)(BUF_LEN - 1));

        if (copy_from_user(config_value, buffer, to_copy))
            return -EFAULT;

        config_value[to_copy] = '\0';

        if (to_copy > 0 && config_value[to_copy - 1] == '\n')
            config_value[to_copy - 1] = '\0';

        printk(KERN_INFO "my_config_module: new value = '%s'\n", config_value);

        return length;
    }

    static const struct proc_ops my_config_fops = {
        .proc_read  = my_config_read,
        .proc_write = my_config_write,
    };

    static int __init my_config_init(void)
    {
        proc_entry = proc_create(PROC_NAME, 0666, NULL, &my_config_fops);
        if (!proc_entry) {
            printk(KERN_ERR "my_config_module: failed to create /proc/%s\n", PROC_NAME);
            return -ENOMEM;
        }

        printk(KERN_INFO "my_config_module: /proc/%s created, default='%s'\n",
               PROC_NAME, config_value);
        return 0;
    }

    static void __exit my_config_exit(void)
    {
        proc_remove(proc_entry);
        printk(KERN_INFO "my_config_module: /proc/%s removed\n", PROC_NAME);
    }

    module_init(my_config_init);
    module_exit(my_config_exit);

    MODULE_LICENSE("GPL");
    MODULE_AUTHOR("Artsem Khvainitski");
    MODULE_DESCRIPTION("/proc/my_config with read/write support");

### Ход выполнения

#### B1. Загрузка модуля и проверка `/proc/my_config`

Команды:

    sudo insmod my_config_module.ko
    ls -l /proc/my_config
    cat /proc/my_config

Вывод:

    -rw-rw-rw- 1 root root 0 ноя 20 18:33 /proc/my_config
    default

Скриншот: `screenshots/b1_my_config_insmod_and_default.png`.

Фрагмент `dmesg`:

    my_config_module: /proc/my_config created, default='default'

#### B2. Запись нового значения и повторное чтение

    echo "new value" | sudo tee /proc/my_config
    cat /proc/my_config

Вывод:

    new value
    new value

Скриншот: `screenshots/b2_my_config_write_and_read.png`.

#### B3. Проверка логов ядра

    sudo dmesg | tail -20

Фрагмент:

    my_config_module: /proc/my_config created, default='default'
    my_config_module: new value = 'new value'

Скриншот: `screenshots/b3_my_config_dmesg.png`.

#### B4. Выгрузка модуля

    sudo rmmod my_config_module
    sudo dmesg | tail -20

Фрагмент:

    my_config_module: /proc/my_config removed

Скриншот: `screenshots/b4_my_config_rmmod.png`.

### Выводы по заданию B

- Реализован файл `/proc/my_config` с правами `0666` (чтение/запись для всех).
- Значение хранится в буфере ядра `config_value` длиной 256 байт.
- Чтение и запись реализованы через `proc_ops` и безопасные функции `copy_to_user`/`copy_from_user`.
- При записи через `echo` новое значение сохраняется в ядре и корректно отображается при чтении и в логах `dmesg`.

---

## Задание C — `/proc` файл со статистикой системы (`sys_stats_module`)

### Постановка задачи

Создать модуль, который создаёт файл `/proc/sys_stats` с информацией:

- количество запущенных процессов;
- использованная память (примерная, через `si_meminfo`);
- uptime системы (через `jiffies_to_msecs(jiffies)`).

Требования:

- реализовать функцию чтения;
- форматировать вывод в человеко-читаемом виде.

### Исходный код `sys_stats_module.c`

    #include <linux/module.h>
    #include <linux/kernel.h>
    #include <linux/init.h>
    #include <linux/proc_fs.h>
    #include <linux/seq_file.h>
    #include <linux/mm.h>
    #include <linux/jiffies.h>
    #include <linux/sched/signal.h>

    #define PROC_NAME_STATS "sys_stats"

    static struct proc_dir_entry *proc_stats_entry;

    static int sys_stats_show(struct seq_file *m, void *v)
    {
        struct sysinfo info;
        unsigned long used_mem_mb;
        unsigned long uptime_sec;
        int proc_count = 0;
        struct task_struct *task;

        for_each_process(task) {
            proc_count++;
        }

        si_meminfo(&info);
        used_mem_mb =
            (info.totalram - info.freeram) * info.mem_unit / (1024 * 1024);

        uptime_sec = jiffies_to_msecs(jiffies) / 1000;

        seq_printf(m, "Processes: %d\n", proc_count);
        seq_printf(m, "Memory Used: %lu MB\n", used_mem_mb);
        seq_printf(m, "System Uptime: %lu seconds\n", uptime_sec);

        return 0;
    }

    static int sys_stats_open(struct inode *inode, struct file *file)
    {
        return single_open(file, sys_stats_show, NULL);
    }

    static const struct proc_ops sys_stats_fops = {
        .proc_open    = sys_stats_open,
        .proc_read    = seq_read,
        .proc_lseek   = seq_lseek,
        .proc_release = single_release,
    };

    static int __init sys_stats_init(void)
    {
        proc_stats_entry =
            proc_create(PROC_NAME_STATS, 0444, NULL, &sys_stats_fops);
        if (!proc_stats_entry) {
            printk(KERN_ERR "sys_stats_module: failed to create /proc/%s\n",
                   PROC_NAME_STATS);
            return -ENOMEM;
        }

        printk(KERN_INFO "sys_stats_module: /proc/%s created\n", PROC_NAME_STATS);
        return 0;
    }

    static void __exit sys_stats_exit(void)
    {
        proc_remove(proc_stats_entry);
        printk(KERN_INFO "sys_stats_module: /proc/%s removed\n", PROC_NAME_STATS);
    }

    module_init(sys_stats_init);
    module_exit(sys_stats_exit);

    MODULE_LICENSE("GPL");
    MODULE_AUTHOR("Artsem Khvainitski");
    MODULE_DESCRIPTION("/proc/sys_stats system statistics module");

### Ход выполнения

#### C1. Загрузка модуля и проверка `/proc/sys_stats`

    sudo insmod sys_stats_module.ko
    ls -l /proc/sys_stats

Вывод:

    -r--r--r-- 1 root root 0 ноя 20 18:43 /proc/sys_stats

Скриншот: `screenshots/c1_sys_stats_insmod_and_ls.png`.

В `dmesg`:

    sys_stats_module: /proc/sys_stats created

#### C2. Чтение статистики

    cat /proc/sys_stats

Фактический вывод:

    Processes: 237
    Memory Used: 3172 MB
    System Uptime: 2122 seconds

Скриншот: `screenshots/c2_sys_stats_cat.png`.

#### C3. Выгрузка модуля

    sudo rmmod sys_stats_module
    sudo dmesg | tail -20

Фрагмент:

    sys_stats_module: /proc/sys_stats removed

Скриншот: `screenshots/c3_sys_stats_rmmod.png`.

### Выводы по заданию C

- Реализован модуль, формирующий читаемый отчёт о состоянии системы через `/proc/sys_stats`.
- Подсчёт процессов выполняется через `for_each_process`.
- Использован `si_meminfo` для оценки использованной памяти и `jiffies_to_msecs` для вычисления uptime.
- Для вывода использован интерфейс `seq_file` (`single_open`, `seq_read`, `seq_printf`), что упрощает реализацию `/proc`-файлов.

---

## Дополнительный модуль `char_device.c` (заглушка)

Для соответствия структуре (`char_device.c`) реализован простой модуль-заглушка:

    #include <linux/module.h>
    #include <linux/kernel.h>
    #include <linux/init.h>

    static int __init chardev_init(void)
    {
        printk(KERN_INFO "char_device: stub module loaded\n");
        return 0;
    }

    static void __exit chardev_exit(void)
    {
        printk(KERN_INFO "char_device: stub module unloaded\n");
    }

    module_init(chardev_init);
    module_exit(chardev_exit);

    MODULE_LICENSE("GPL");
    MODULE_AUTHOR("Artsem Khvainitski");
    MODULE_DESCRIPTION("Stub char device module for lab structure");

Модуль успешно компилируется и может быть загружен/выгружен, но полноценное символьное устройство в данной лабораторной не требуется.

---

## Вопросы для отчёта (1–30)

### Базовые понятия

**1. Что такое модуль ядра и зачем он нужен?**  
Модуль ядра — это загружаемый объектный файл, который расширяет функциональность ядра без его пересборки и перезагрузки. Примеры: драйверы устройств, сетевые протоколы, файловые системы. Модуль можно загрузить и выгрузить по необходимости.

**2. Чем отличается kernel-space от user-space?**  
- kernel-space — код ядра, выполняется с максимальными привилегиями, имеет доступ ко всей физической памяти и устройствам.  
- user-space — обычные процессы пользователя, работают с ограниченными правами, обращаются к ядру только через системные вызовы.  
Разделение необходимо для безопасности и стабильности системы.

**3. Что произойдёт, если в модуле обратиться к NULL-указателю?**  
Произойдёт обращение к недопустимому адресу в контексте ядра. Это почти всегда приводит к аварии (`Oops`) и может вызвать `kernel panic` или нестабильность системы, вплоть до полной зависания.

**4. Почему нельзя использовать `printf()` в модуле ядра?**  
`printf()` — функция пользовательского уровня (glibc), недоступная в пространстве ядра. В ядре используется своя функция логирования — `printk()`, которая пишет в буфер сообщений ядра (`dmesg`, `journalctl -k`).

**5. Что такое `kernel panic` и как его избежать?**  
`kernel panic` — критическая ошибка ядра, при которой оно не может продолжать работу. Система обычно «замораживается» или перезагружается.  
Избежать помогают:
- проверки указателей и границ массивов;
- корректная работа с памятью и блокировками;
- обработка всех кодов ошибок;
- отсутствие опасных бесконечных циклов и некорректного доступа к железу.

---

### Жизненный цикл модуля

**6. Какие функции вызываются при `insmod` и `rmmod`?**  
- При `insmod` вызывается функция, указанная в `module_init(...)` (например, `hello_init`).  
- При `rmmod` — функция, указанная в `module_exit(...)` (например, `hello_exit`).

**7. Что должна делать функция `module_exit()`?**  
Она должна корректно освободить все ресурсы, выделенные в `module_init()`:

- удалить файлы из `/proc`, `/sys`;
- отрегестрированные устройства и таймеры;
- освободить выделенную память и прочие ресурсы.  

После `module_exit()` не должно оставаться утечек и висящих объектов.

**8. Что происходит, если `module_init()` возвращает ошибку?**  
Модуль считается не загруженным. Ядро не будет вызывать `module_exit()`. Код `module_init()` должен сам аккуратно освободить все уже выделенные ресурсы перед возвратом ошибки.

**9. Можно ли выгрузить модуль, если он используется?**  
Нет. У модуля есть счётчик ссылок (usage count). Если кто-то использует модуль (например, открыто устройство, зарегистрирован интерфейс и т.д.), `rmmod` вернёт ошибку «module in use». Нужно сначала освободить все ресурсы/закрыть дескрипторы.

---

### Логирование и отладка

**10. Чем `printk()` отличается от `printf()`?**  
- `printf()` — пользовательский вывод в stdout, работает в user-space.  
- `printk()` — функция ядра, пишет в буфер сообщений ядра, откуда их читают `dmesg` и `journalctl`. У `printk()` есть уровни важности (например, `KERN_INFO`, `KERN_ERR`).

**11. Какие уровни логирования существуют в ядре?**  
Основные уровни:

- `KERN_EMERG` — авария, ядро не может работать;
- `KERN_ALERT`;
- `KERN_CRIT`;
- `KERN_ERR`;
- `KERN_WARNING`;
- `KERN_NOTICE`;
- `KERN_INFO`;
- `KERN_DEBUG`.

В лабораторной работе активно использовался уровень `KERN_INFO`.

**12. Как посмотреть логи модуля?**  

- `dmesg | tail`  
- `dmesg | grep <имя_модуля>`  
- `journalctl -k` (логи ядра через systemd-journald)  

В работе использовалась команда:

    sudo dmesg | tail -20

**13. Что означает "tainted kernel"?**  
Это флаг, который ядро устанавливает, когда его состояние «загрязнено»:

- загружен проприетарный или не-GPL модуль;
- произошли определённые серьёзные ошибки.  

При «tainted kernel» разработчики ядра могут отказать в поддержке, так как состояние системы нестандартное.

---

### Память

**14. Чем `kmalloc()` отличается от `malloc()`?**  

- `malloc()` — функция пользовательского пространства (glibc), выделяет память в адресном пространстве процесса, недоступна в ядре.  
- `kmalloc()` — функция ядра, выделяет память в адресном пространстве ядра; принимает размер и флаги (`GFP_KERNEL`, `GFP_ATOMIC` и т.д.).

**15. Что такое флаги GFP и зачем они нужны?**  
GFP (Get Free Page) — флаги, задающие поведение выделения памяти в ядре:

- можно ли «заснуть» (`GFP_KERNEL` — можно блокироваться; `GFP_ATOMIC` — нельзя);
- из каких зон памяти можно выделять;
- контекст вызова (прерывание, обычный процесс и т.д.).  

Они помогают ядру решить, как безопасно удовлетворить запрос на выделение памяти.

**16. Что произойдёт, если не освободить память в `module_exit()`?**  
Память останется занята даже после выгрузки модуля, то есть произойдёт утечка памяти в ядре. Со временем это может привести к нехватке RAM и нестабильности системы.

**17. Почему нельзя использовать указатели user-space напрямую в ядре?**  
Адресное пространство ядра и процесса различаются. Указатель user-space может указывать на недоступный или изменившийся участок при переключении процессов. Для безопасного доступа используются `copy_to_user()` и `copy_from_user()`, которые проверяют корректность и доступность адресов.

---

### Взаимодействие с user-space

**18. Что такое `/proc` и для чего он используется?**  
`/proc` — псевдо-файловая система, позволяющая получать и изменять состояние ядра и процессов через файловый интерфейс.  
Она используется для:

- просмотра информации о процессах;
- чтения параметров ядра;
- иногда — записи настроек (через `echo > /proc/...`).

В лабораторной `/proc` использовался для обмена строками и статистикой между модулем и user-space.

**19. Что такое `/sys` (sysfs) и чем он отличается от procfs?**  
`/sys` (sysfs) — псевдо-файловая система для представления объектов ядра (устройства, драйверы, шины) и их атрибутов.  
Отличия:

- `/proc` исторически для процессов и параметров ядра;
- `/sys` — более строгая иерархия объектов ядра и устройств.  

Часто драйверы создают атрибуты именно в `/sys`.

**20. Зачем нужны `copy_to_user()` и `copy_from_user()`?**  
Они выполняют безопасное копирование данных между ядром и user-space:

- `copy_to_user()` — из буфера ядра в адресное пространство процесса;
- `copy_from_user()` — из user-space в буфер ядра.  

При этом происходит проверка указателей и обработка ошибок доступа.

**21. Что такое character device и как он работает?**  
Символьное устройство — тип устройства, с которым взаимодействуют как с потоком байтов (например, терминал, последовательный порт).  
Для него в `/dev` создаётся файл-устройство, а драйвер реализует операции `open`, `read`, `write`, `release` и т.д. В лабораторной `char_device.c` был реализован как заглушка.

---

### Параметры и метаданные

**22. Как передать параметры модулю при загрузке?**  
Через строку команды `insmod` или `modprobe`, например:

    sudo insmod hello_module.ko message="Custom greeting"

В коде параметр объявляется через `module_param(name, type, perm)` и, при необходимости, `MODULE_PARM_DESC(...)`.

**23. Зачем нужен `MODULE_LICENSE()`?**  
Этот макрос указывает тип лицензии модуля (например, `"GPL"`). Он важен, чтобы ядро знало, относится ли модуль к свободному ПО, и может ли он использовать некоторые экспортируемые символы (зависящие от GPL).

**24. Что произойдёт, если не указать лицензию?**  
Ядро считает модуль проприетарным, состояние ядра может стать «tainted». Некоторые GPL-символы ядра будут недоступны для такого модуля.

---

### Безопасность

**25. Какие основные правила безопасного кода в ядре?**

- Проверять указатели на `NULL` и корректность границ массивов.
- Проверять коды возврата всех функций.
- Правильно использовать блокировки (во избежание гонок и deadlock’ов).
- Всегда освобождать ресурсы и память в `module_exit()`.
- Не выполнять долгие или бесконечные циклы в контексте, где нельзя блокироваться.
- Не использовать неподготовленные указатели user-space напрямую.

**26. Можно ли использовать бесконечный цикл в модуле?**  
В «обычном» виде — нельзя. Бесконечный цикл в контексте ядра может «заморозить» систему. Допускаются только циклы, которые корректно отдают управление (через `schedule()`, таймеры и т.п.), и только при строгой необходимости.

**27. Почему в ядре обычно нет операций с FPU (плавающей точкой)?**  
Использование FPU требует сохранения/восстановления её состояния при переключении контекста, что дорого по производительности и усложняет код. Поэтому ядро старается не использовать плавающую точку, кроме специально выделенных мест, где состояние FPU аккуратно сохраняется.

**28. Что делать, если модуль вызвал `kernel panic`?**  

- Перезагрузить систему.
- Просмотреть логи ядра (`dmesg`, `journalctl -k`) и анализировать stack trace.
- Загрузиться без проблемного модуля (убрать из автозагрузки, не загружать руками).
- Найти и исправить ошибку в коде модуля (память, указатели, блокировки и т.п.).  

В продакшене такой модуль использовать нельзя до устранения проблемы.

---

### Практические вопросы

**29. Как узнать, какие модули загружены в системе?**  

- `lsmod` — показывает список всех загруженных модулей.  
- Можно отфильтровать по имени:

    lsmod | grep hello_module

**30. Как получить информацию о модуле (версия, параметры)?**  

С помощью `modinfo`:

    modinfo hello_module.ko

Показываются: имя, версия, автор, лицензия, описание, параметры (`parm:`), путь к модулю и другие метаданные.

---

## Итоговые выводы по лабораторной

1. Были реализованы три модуля ядра:  
   - `hello_module` — демонстрация жизненного цикла модуля и параметров загрузки;  
   - `my_config_module` — обмен строковыми данными с user-space через `/proc`;  
   - `sys_stats_module` — пример получения системной статистики из ядра.
2. Отработаны команды управления модулями: `insmod`, `rmmod`, `lsmod`, просмотр логов через `dmesg`.
3. Освоены базовые принципы безопасной работы с памятью ядра и указателями user-space (`copy_to_user`, `copy_from_user`).
4. Закреплены теоретические знания о разделении kernel-space/user-space, параметрах модулей, лицензировании, механизмах `/proc` и базовых правилах безопасного кода в ядре.

---

## Использование ИИ

ИИ использовался для помощи в оформлении отчета и ответов на вопросы.



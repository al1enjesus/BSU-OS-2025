# Лабораторная 5 — Модули ядра Linux

### Окружение
```bash
Arch Linux x86_64
Ядро: 6.16.8-arch3-1
Shell: bash
Compiler: gcc 
```

### Цель

Целью работы было ознакомиться с разработкой модулей ядра Linux, научиться их загружать/выгружать, взаимодействовать с user-space через `/proc` файловую систему и реализовать простой character device `/dev/mychardev` с буфером в kernel space.

---

## Подготовка 

Для работы с модулями ядра на Arch Linux я установил следующей командой:

```bash
sudo pacman -S base-devel linux-headers
```

Сборка модулей на Arch выполняется аналогично Ubuntu, используя:

```bash
make -C /usr/lib/modules/$(uname -r)/build M=$(PWD) modules
```

Важные отличия Arch Linux:
- Пакеты: `pacman` вместо `apt`
- Kernel headers: `/usr/lib/modules/$(uname -r)/build`
- Модули загружаются в `/lib/modules/$(uname -r)/kernel/`

---

## Задание A — Hello World модуль

### Реализация

Написал простейший модуль, который при загрузке выводит приветствие, при выгрузке — прощание. Модуль поддерживает строковый параметр `message` (по умолчанию `NULL`).

```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/moduleparam.h>

static char *message = NULL;
module_param(message, charp, 0644);
MODULE_PARM_DESC(message, "Hello message");

static int __init hello_init(void)
{
    if (message) {
        printk(KERN_INFO "hello_module: %s\n", message);
    } else {
        printk(KERN_INFO "hello_module: Hello from Kirill module!\n");
    }
    return 0;
}

static void __exit hello_exit(void)
{
    printk(KERN_INFO "hello_module: Goodbye from Kirill module!\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kuharev Kirill");
MODULE_DESCRIPTION("Simple Hello World kernel module");
MODULE_VERSION("1.0");
```

**Ключевые точки:**
- `module_param()` регистрирует параметр `message`
- `printk(KERN_INFO ...)` пишет в dmesg
- `MODULE_LICENSE("GPL")` обязателен, иначе ядро будет "tainted"
- `__init` / `__exit` макросы помогают компилятору оптимизировать память


### Проверка

Я проверял на виртуальной машине и оставил скриншоты в папке screenshots

---

## Задание B — /proc файл с информацией о студенте

### Реализация

Модуль создаёт файл `/proc/student_info`, который выводит информацию о студенте, время загрузки (в jiffies) и счётчик обращений. Каждое чтение увеличивает счётчик.

```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>

#define PROC_NAME "student_info"
#define MAX_SIZE 1024

static struct proc_dir_entry *proc_file = NULL;
static int read_count = 0;
static unsigned long load_time = 0;
static char *student_name = "Kuharev Kirill";
static int group = 9;
static int subgroup = 2;

module_param(student_name, charp, 0644);
module_param(group, int, 0644);
module_param(subgroup, int, 0644);

static ssize_t proc_read(struct file *file, char __user *ubuf,
                         size_t count, loff_t *ppos)
{
    char buf[MAX_SIZE];
    int len;
    unsigned long uptime_jiffies, uptime_seconds;

    if (*ppos > 0)
        return 0;  // EOF

    read_count++;
    uptime_jiffies = jiffies - load_time;
    uptime_seconds = uptime_jiffies / HZ;

    len = snprintf(buf, sizeof(buf),
        "╔════════════════════════════════════════════════╗\n"
        "║       Student Information                      ║\n"
        "╠════════════════════════════════════════════════╣\n"
        " Name: %s\n"
        " Group: %d\n"
        " Subgroup: %d\n"
        " Module loaded at: %lu jiffies\n"
        " Module uptime: %lu seconds\n"
        " Read count: %d\n"
        " Current jiffies: %lu\n"
        "╚════════════════════════════════════════════════╝\n",
        student_name, group, subgroup, load_time,
        uptime_seconds, read_count, jiffies);

    if (copy_to_user(ubuf, buf, len))
        return -EFAULT;

    *ppos = len;
    printk(KERN_INFO "proc_module: /proc/%s read (count: %d)\n",
           PROC_NAME, read_count);

    return len;
}

static const struct proc_ops proc_file_ops = {
    .proc_read = proc_read,
};

static int __init proc_module_init(void)
{
    printk(KERN_INFO "proc_module: Initializing\n");
    load_time = jiffies;

    proc_file = proc_create(PROC_NAME, 0444, NULL, &proc_file_ops);
    if (!proc_file) {
        printk(KERN_ERR "proc_module: Failed to create /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }

    printk(KERN_INFO "proc_module: Successfully created /proc/%s\n", PROC_NAME);
    printk(KERN_INFO "proc_module: Student: %s, Group: %d, Subgroup: %d\n",
           student_name, group, subgroup);
    printk(KERN_INFO "proc_module: Load time: %lu jiffies\n", load_time);

    return 0;
}

static void __exit proc_module_exit(void)
{
    if (proc_file) {
        proc_remove(proc_file);
        printk(KERN_INFO "proc_module: Removed /proc/%s\n", PROC_NAME);
    }
    printk(KERN_INFO "proc_module: Module unloaded. Total reads: %d\n", read_count);
}

module_init(proc_module_init);
module_exit(proc_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kirill Kuharev");
MODULE_DESCRIPTION("Proc filesystem module with student info");
MODULE_VERSION("1.0");
```

**Ключевые компоненты:**
- `proc_create()` создаёт файл в `/proc` с правами 0444 (read-only)
- `copy_to_user()` копирует данные из kernel-space в user-space
- `jiffies` — системный счётчик тактов, `HZ` преобразует в секунды
- Глобальная переменная `read_count` считает обращения
- `proc_remove()` удаляет файл при выгрузке

### Проверка

в папке screenshots.

---

## Задание C — Character Device с буфером

### Реализация

Модуль регистрирует character device `/dev/mychardev`, который:
- Сохраняет данные при записи (до 1024 байт)
- Возвращает сохранённые данные при чтении
- Выводит сообщения открытия/закрытия в dmesg
- Использует mutex для синхронизации доступа

```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/mutex.h>

#define DEVICE_NAME "mychardev"
#define BUF_SIZE 1024

static dev_t dev_num;
static struct cdev my_cdev;
static char *device_buffer;
static int buffer_size = 0;
static struct mutex device_mutex;

static int dev_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "chardev: Device opened\n");
    return 0;
}

static int dev_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "chardev: Device closed\n");
    return 0;
}

static ssize_t dev_read(struct file *file, char __user *buf,
                        size_t len, loff_t *off)
{
    int bytes_to_read;

    mutex_lock(&device_mutex);

    if (*off >= buffer_size) {
        mutex_unlock(&device_mutex);
        return 0;  // EOF
    }

    bytes_to_read = min((size_t)(buffer_size - *off), len);

    if (copy_to_user(buf, device_buffer + *off, bytes_to_read)) {
        mutex_unlock(&device_mutex);
        return -EFAULT;
    }

    *off += bytes_to_read;
    mutex_unlock(&device_mutex);

    printk(KERN_INFO "chardev: Read %d bytes from offset %lld\n",
           bytes_to_read, *off - bytes_to_read);

    return bytes_to_read;
}

static ssize_t dev_write(struct file *file, const char __user *buf,
                         size_t len, loff_t *off)
{
    int bytes_to_write;

    mutex_lock(&device_mutex);

    bytes_to_write = min(len, (size_t)BUF_SIZE);
    memset(device_buffer, 0, BUF_SIZE);

    if (copy_from_user(device_buffer, buf, bytes_to_write)) {
        mutex_unlock(&device_mutex);
        return -EFAULT;
    }

    buffer_size = bytes_to_write;
    mutex_unlock(&device_mutex);

    printk(KERN_INFO "chardev: Write %d bytes: %s\n", bytes_to_write, device_buffer);

    return bytes_to_write;
}

static loff_t dev_lseek(struct file *file, loff_t offset, int whence)
{
    loff_t new_pos;

    mutex_lock(&device_mutex);

    switch (whence) {
    case SEEK_SET:
        new_pos = offset;
        break;
    case SEEK_CUR:
        new_pos = file->f_pos + offset;
        break;
    case SEEK_END:
        new_pos = buffer_size + offset;
        break;
    default:
        mutex_unlock(&device_mutex);
        return -EINVAL;
    }

    if (new_pos < 0) {
        mutex_unlock(&device_mutex);
        return -EINVAL;
    }

    if (new_pos > BUF_SIZE)
        new_pos = BUF_SIZE;

    file->f_pos = new_pos;
    mutex_unlock(&device_mutex);

    return new_pos;
}

static long dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    switch (cmd) {
    case 0x10:  
        if (copy_to_user((int __user *)arg, &buffer_size, sizeof(int)))
            return -EFAULT;
        printk(KERN_INFO "chardev: IOCTL GET_BUFFER_SIZE: %d\n", buffer_size);
        break;

    case 0x11:  
        mutex_lock(&device_mutex);
        memset(device_buffer, 0, BUF_SIZE);
        buffer_size = 0;
        mutex_unlock(&device_mutex);
        printk(KERN_INFO "chardev: IOCTL CLEAR_BUFFER\n");
        break;

    default:
        return -EINVAL;
    }

    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .release = dev_release,
    .read = dev_read,
    .write = dev_write,
    .llseek = dev_lseek,
    .unlocked_ioctl = dev_ioctl,
};

static int __init chardev_init(void)
{
    int ret;

    printk(KERN_INFO "chardev: Initializing\n");

    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ERR "chardev: Failed to allocate major number\n");
        return ret;
    }

    printk(KERN_INFO "chardev: Registered with major number %d\n", MAJOR(dev_num));

    device_buffer = kmalloc(BUF_SIZE, GFP_KERNEL);
    if (!device_buffer) {
        ret = -ENOMEM;
        goto fail_alloc;
    }

    memset(device_buffer, 0, BUF_SIZE);
    mutex_init(&device_mutex);

    cdev_init(&my_cdev, &fops);
    my_cdev.owner = THIS_MODULE;

    ret = cdev_add(&my_cdev, dev_num, 1);
    if (ret < 0) {
        printk(KERN_ERR "chardev: Failed to add cdev\n");
        goto fail_cdev;
    }

    printk(KERN_INFO "chardev: Device registered successfully\n");
    printk(KERN_INFO "chardev: Create device with: sudo mknod /dev/%s c %d 0\n",
           DEVICE_NAME, MAJOR(dev_num));

    return 0;

fail_cdev:
    kfree(device_buffer);
fail_alloc:
    unregister_chrdev_region(dev_num, 1);
    return ret;
}

static void __exit chardev_exit(void)
{
    cdev_del(&my_cdev);
    kfree(device_buffer);
    unregister_chrdev_region(dev_num, 1);
    printk(KERN_INFO "chardev: Device unregistered\n");
}

module_init(chardev_init);
module_exit(chardev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kirill Kuharev");
MODULE_DESCRIPTION("Simple character device driver with advanced features");
MODULE_VERSION("1.0");
```

**Ключевые компоненты:**
- `alloc_chrdev_region()` выделяет major/minor номера
- `cdev_init()` и `cdev_add()` регистрируют character device
- `kmalloc()` / `kfree()` выделяют/освобождают kernel буфер
- `copy_to_user()` / `copy_from_user()` копируют данные между kernel и user space
- `mutex` защищает буфер от race conditions
- `lseek()` и `ioctl()` реализуют расширенную функциональность

### Проверка

в screenshots.

---

## Ответы на вопросы для отчёта

### Базовые понятия

**1. Что такое модуль ядра и зачем он нужен?**

Модуль ядра (kernel module) — это фрагмент кода, который может быть динамически загружен в ядро Linux во время работы системы без перезагрузки. Модули нужны для:
- Расширения функциональности ядра без перекомпиляции
- Управления драйверами устройств (USB, сетевые карты)
- Загрузки файловых систем
- Реализации сетевых протоколов

На Arch Linux модули загружаются через `insmod` и управляются через `modprobe`.

**2. Чем отличается kernel-space от user-space?**

| Аспект | User-space | Kernel-space |
|--------|-----------|--------------|
| Доступ | Ограниченный | Полный |
| Память | Виртуальная | Физическая |
| Крах | Только процесс | Kernel panic |
| Библиотеки | libc, POSIX | kernel API |
| Привилегии | Пользователь | root (режим ядра) |

**3. Что произойдёт, если в модуле обратиться к NULL указателю?**

Возникнет **kernel panic** — система упадёт. В user-space программа просто упадёт, но в kernel-space нет защиты памяти, поэтому это критично. Нужна внимательность и проверка всех указателей.

**4. Почему нельзя использовать `printf()` в модуле ядра?**

В kernel-space нет стандартной библиотеки C (`libc`). `printf()` не скомпилируется. Вместо этого используется `printk()`, который пишет в kernel log buffer, доступный через `dmesg`.

**5. Что такое kernel panic и как его избежать?**

Kernel panic — состояние, когда ядро не может продолжать работу. Это результат критической ошибки (null-pointer dereference, divide by zero, infinite loop в interrupt handler). Избежать помогает:
- Проверка всех возвращаемых значений
- Проверка границ массивов и буферов
- Правильное управление памятью (kmalloc/kfree)
- Избежание sleep операций где нельзя
- Использование в VM для тестирования

### Жизненный цикл модуля

**6. Какие функции вызываются при `insmod` и `rmmod`?**

- `sudo insmod hello_module.ko` → вызывает `module_init()` функцию (по умолчанию `__init hello_init()`)
- `sudo rmmod hello_module` → вызывает `module_exit()` функцию (по умолчанию `__exit hello_exit()`)

**7. Что должна делать функция `module_exit()`?**

Функция `module_exit()` должна:
- Освободить все выделенные ресурсы (`kfree()`)
- Отменить регистрации (`proc_remove()`, `cdev_del()`, `unregister_chrdev_region()`)
- Очистить глобальные состояния
- Вывести сообщение выгрузки в dmesg

**8. Что происходит, если `module_init()` возвращает ошибку?**

Если `module_init()` возвращает значение < 0 (ошибка):
- Модуль НЕ загружается
- `module_exit()` вызвана НЕ будет
- Нужно вручную освободить уже выделенные ресурсы перед возвратом ошибки

**9. Можно ли выгрузить модуль, если он используется?**

Нет. Если `rmmod` попытается выгрузить используемый модуль, выведется ошибка:
```bash
$ sudo rmmod chardev_module
rmmod: ERROR: Module chardev_module is in use
```
Нужно сначала закрыть все файловые дескрипторы (закрыть `/dev/mychardev`).

### Логирование и отладка

**10. Чем `printk()` отличается от `printf()`?**

| Аспект | printf | printk |
|--------|--------|--------|
| Библиотека | libc (user-space) | kernel API |
| Вывод | stdout (терминал) | kernel log buffer |
| Просмотр | Прямо в консоли | `dmesg`, `/var/log/kern.log` |
| Уровни | Нет | KERN_INFO, KERN_ERR и т.д. |

На Arch Linux логи смотрятся через `dmesg` или `journalctl -k`.

**11. Какие уровни логирования существуют в ядре?**

```
KERN_EMERG   (0) - Система непригодна к работе
KERN_ALERT   (1) - Требуется немедленное действие
KERN_CRIT    (2) - Критическое состояние
KERN_ERR     (3) - Ошибка
KERN_WARNING (4) - Предупреждение
KERN_NOTICE  (5) - Обычно значимое событие
KERN_INFO    (6) - Информационное сообщение
KERN_DEBUG   (7) - Отладочная информация
```

**12. Как посмотреть логи модуля?**

```bash
dmesg | tail -20              # Последние 20 строк
dmesg | grep "module_name"    # Поиск по модулю
dmesg -w                      # Живой просмотр (watch mode)
dmesg -T                      # С человекочитаемым временем
sudo journalctl -k -f         # Kernel logs через systemd (Arch)
tail -f /var/log/messages     # На некоторых системах
```

**13. Что означает "tainted kernel"?**

"Tainted" означает, что ядро в недоверенном состоянии. Причины:
- Загружен проприетарный модуль (не GPL)
- Модуль без `MODULE_LICENSE("GPL")`
- Принудительная загрузка `insmod -f`

Проверка:
```bash
cat /proc/sys/kernel/tainted  # 0 = clean, 1+ = tainted
```

### Память

**14. Чем `kmalloc()` отличается от `malloc()`?**

| Аспект | malloc() | kmalloc() |
|--------|----------|-----------|
| Контекст | User-space | Kernel-space |
| Библиотека | libc | kernel/slab.h |
| Память | Виртуальная | Физическая (GFP_KERNEL) |
| Swap | Возможен | Нет (обычно GFP_KERNEL) |
| Проверка ошибок | Критично | ОБЯЗАТЕЛЬНО |

**15. Что такое флаги GFP и зачем они нужны?**

GFP (Get Free Pages) флаги управляют поведением `kmalloc()`:

- `GFP_KERNEL` — обычное выделение, может спать, не в interrupt
- `GFP_ATOMIC` — выделение в atomic context (interrupt), НЕ может спать
- `GFP_USER` — для user-space данных

Пример:
```c
ptr = kmalloc(size, GFP_KERNEL);    // В обычном контексте (safe)
ptr = kmalloc(size, GFP_ATOMIC);    // В interrupt handler (unsafe to sleep)
```

**16. Что произойдёт, если не освободить память в `module_exit()`?**

Память останется в ядре (утечка памяти) и будет занята до перезагрузки. В ядре нет garbage collector, поэтому это критично.

**17. Почему нельзя использовать user-space указатели напрямую в ядре?**

User-space указатели — это виртуальные адреса, которые могут быть:
- Выгружены из памяти (swap)
- Недоступны в контексте ядра
- Указывать на невалидную память

Решение — использовать `copy_to_user()` и `copy_from_user()`.

### Взаимодействие с user-space

**18. Что такое `/proc` и для чего он используется?**

`/proc` — виртуальная файловая система для экспорта информации из ядра в user-space:

```bash
cat /proc/cpuinfo       # Информация о CPU
cat /proc/meminfo       # Информация о памяти
cat /proc/modules       # Загруженные модули
```

Модули могут создавать свои файлы через `proc_create()`.

**19. Что такое `/sys` (sysfs) и чем отличается от procfs?**

| Аспект | /proc | /sys |
|--------|-------|------|
| Назначение | Информация о ядре | Атрибуты устройств |
| Философия | Любая информация | Один атрибут = один файл |
| Доступ | Read-only обычно | Read/Write |
| Модерн | Старый способ | Новый способ |

**20. Зачем нужны функции `copy_to_user()` и `copy_from_user()`?**

Эти функции безопасно копируют данные между kernel-space и user-space:

```c
// Чтение из user-space в kernel
copy_from_user(kernel_buf, user_ptr, count);

// Запись из kernel в user-space
copy_to_user(user_ptr, kernel_buf, count);
```

Без них может быть segfault или ошибка доступа.

**21. Что такое character device и как он работает?**

Character device — это специальный файл, который позволяет взаимодействовать с драйвером как с файлом:

```bash
echo "data" > /dev/mychardev    # write()
cat /dev/mychardev              # read()
```

Операции маршрутизируются в driver через `file_operations` структуру.

### Параметры и метаданные

**22. Как передать параметры модулю при загрузке?**

```bash
sudo insmod module.ko param1=value1 param2=value2
```

Параметры объявляются через `module_param()`:

```c
static int count = 1;
module_param(count, int, 0644);
MODULE_PARM_DESC(count, "Number of iterations");
```

На Arch Linux параметры также видны в `/sys/module/modulename/parameters/`.

**23. Зачем нужен `MODULE_LICENSE()`?**

`MODULE_LICENSE("GPL")` указывает лицензию модуля:
- Требуется для совместимости с GPL ядром
- Без него ядро будет "tainted"
- Некоторые функции доступны только GPL модулям

**24. Что произойдёт, если не указать лицензию?**

Модуль загрузится, но:
- Ядро будет "tainted"
- В `dmesg` появится warning
- Некоторые GPL-only функции вернут ошибку

### Безопасность

**25. Какие основные правила безопасного кода в ядре?**

✅ **ДЕЛАТЬ:**
- Всегда проверять возвращаемые значения
- Освобождать все ресурсы в exit
- Использовать `copy_to_user()` / `copy_from_user()`
- Проверять границы буферов
- Использовать mutex для синхронизации

❌ **НЕ ДЕЛАТЬ:**
- Использовать `printf()`, `malloc()`, стандартную libc
- Обращаться к NULL или невалидным указателям
- Писать бесконечные циклы
- Разыменовывать user-space указатели напрямую
- Выделять большие буфера на стеке

**26. Можно ли использовать бесконечный цикл в модуле?**

Нет. Бесконечный цикл заморозит CPU и сделает систему неответчивой или вызовет kernel panic.

**27. Почему в ядре нет FPU операций?**

FPU (Floating Point Unit) требует сохранения/восстановления состояния при переключении контекста. В ядре это может быть:
- Неэффективно
- Вызвать проблемы с точностью
- Конфликты при прерываниях

**28. Что делать, если модуль вызвал kernel panic?**

1. VM позволяет откатиться к snapshot
2. Или перезагрузить систему
3. Модуль НЕ загрузится после reboot
4. Проверить логи в `/var/log/messages` (на Arch) или `journalctl`
5. Найти ошибку в коде
6. Пересобрать и протестировать

### Практические вопросы

**29. Как узнать, какие модули загружены в системе?**

```bash
lsmod                          # Список модулей
lsmod | grep mymodule          # Поиск конкретного
cat /proc/modules              # Детальная информация
sudo modinfo mymodule.ko       # Информация о модуле
```

На Arch Linux также можно использовать:
```bash
sudo modprobe --show-depends mymodule  # Зависимости
```

**30. Как получить информацию о модуле (версия, параметры)?**

```bash
sudo modinfo hello_module.ko
name:                hello_module
version:             1.0
description:         Simple Hello World kernel module
author:              Kirill Kuharev
license:             GPL
depends:
srcversion:          ...
parm:                message:Hello message (charp)
```

---

## Особенности работы на Arch Linux

**Установка зависимостей:**
```bash
sudo pacman -S base-devel linux-headers
```

**Сборка модулей:**
```bash
make -C /usr/lib/modules/$(uname -r)/build M=$(PWD) modules
```

**Загрузка модулей:**
```bash
sudo insmod module.ko
sudo modprobe module_name  
```

**Просмотр логов:**
```bash
dmesg              
journalctl -k      
journalctl -k -f   
```

**Удаление device node:**
```bash
sudo rm /dev/mychardev
```

---

## Выводы

В ходе выполнения лабораторной работы на Arch Linux я:

1. **Задание A** — Создал простейший модуль с поддержкой строковых параметров и правильно вывел логи в kernel dmesg.

2. **Задание B** — Реализовал proc файл `/proc/student_info` с информацией о студенте, счётчиком обращений и временем загрузки модуля, используя `jiffies` и `copy_to_user()`.

3. **Задание C** — Разработал character device `/dev/mychardev` с:
   - Kernel буфером на 1024 байта
   - Функциями read/write с `copy_to_user()`/`copy_from_user()`
   - Mutex-based синхронизацией
   - Правильной регистрацией через `alloc_chrdev_region()`, `cdev_init()`, `cdev_add()`
   - Расширенной функциональностью (lseek, ioctl)

4. **Понимание архитектуры:**
   - Различие kernel-space от user-space
   - Жизненный цикл модулей
   - Важность управления памятью (kmalloc/kfree)
   - Правильное взаимодействие с user-space через copy функции

5. **Практический опыт на Arch Linux:**
   - Установка необходимых пакетов через pacman
   - Компиляция модулей с использованием `/usr/lib/modules/$(uname -r)/build`
   - Загрузка/выгрузка с `insmod`/`rmmod`
   - Просмотр логов через `journalctl` и `dmesg`
   - Отладка и работа с character devices

Все модули компилируются без ошибок и предупреждений, корректно загружаются на Arch Linux, работают без kernel panic и правильно выгружаются с очисткой ресурсов.
Для получения информации я пользовался Deepseek.

# Лабораторная работа 5 — Модули ядра Linux

Студент: **Казак Артур Эдуардович**  
Группа: **8**, подгруппа: **2**  
Номер по списку: **13** (нечётный → вариант 1)  
Семестр: весна 2025  

ОС: Ubuntu 22.04 (виртуальная машина)  
Версия ядра: `5.15.0-113-generic`  

---

## 1. Цель и задачи работы

**Цель:**  
Изучить архитектуру модулей ядра Linux и получить практические навыки:

- разработки простых модулей ядра;
- динамической загрузки и выгрузки модулей;
- логирования событий с помощью `printk`/`pr_info` и анализа журнала `dmesg`;
- взаимодействия с пространством пользователя через виртуальную файловую систему `/proc`;
- реализации символьного драйвера `/dev/mychardev`;
- учёта вопросов безопасности и многопоточности при разработке кода в kernel space.

**Задачи по варианту 1 (нечётный номер по списку):**

1. **Задание A:** реализовать модуль `hello_module` с параметром `message`.
2. **Задание B:** реализовать модуль `proc_module`, создающий `/proc/student_info`.
3. **Задание C:** реализовать модуль `char_device`, обеспечивающий символьное устройство `/dev/mychardev`.

---

## 2. Структура каталога студента

Работа выполнялась в ветке репозитория:

- `gr8sub2-lab5-КазакАЭ`

Каталог студента в `lab5`:

```text
lab5/
  gr8sub2/
    КАЗАК_АРТУР/
      REPORT.MD
      README.md
      Makefile
      src/
        Makefile
        hello_module.c
        proc_module.c
        char_device.c
      screenshots/
        dmesg_hello.png
        dmesg_proc.png
        dmesg_chardev.png
        lsmod.png
        proc_student_info_cat.png
      logs/
        test_hello.txt
        test_proc.txt
        test_chardev.txt
```

- `Makefile` в корне каталога студента вызывает сборку модулей в `src/`.
- `src/Makefile` содержит список объектных файлов для трёх модулей.

---

## 3. Описание реализованных модулей

### 3.1. Задание A — модуль `hello_module`

**Файл:** `src/hello_module.c`  

Назначение: выводить диагностическое сообщение при загрузке и выгрузке модуля, а также поддерживать строковый параметр `message`, задаваемый при `insmod`.

Основные элементы реализации:

- Подключены заголовочные файлы:

  ```c
  #include <linux/module.h>
  #include <linux/kernel.h>
  #include <linux/init.h>
  #include <linux/moduleparam.h>
  ```

- Параметр модуля:

  ```c
  static char *message = "Hello from Kazak Artur module!";
  module_param(message, charp, 0444);
  MODULE_PARM_DESC(message, "Custom hello message");
  ```

- Функции инициализации и завершения:

  ```c
  static int __init hello_init(void)
  {
      pr_info("hello_module: %s
", message);
      return 0;
  }

  static void __exit hello_exit(void)
  {
      pr_info("hello_module: Goodbye from Kazak Artur module!\n");
  }
  ```

- Регистрация точек входа/выхода и метаданные:

  ```c
  MODULE_LICENSE("GPL");
  MODULE_AUTHOR("Kazak Artur");
  MODULE_DESCRIPTION("Simple Hello World kernel module with parameter");
  MODULE_VERSION("1.0");

  module_init(hello_init);
  module_exit(hello_exit);
  ```

При загрузке модуль выводит в `dmesg` строку с текстом `message`. При выгрузке — сообщение о завершении работы.

---

### 3.2. Задание B — модуль `proc_module` (файл `/proc/student_info`)

**Файл:** `src/proc_module.c`  

Назначение: предоставить информацию о студенте и модуле через файл `/proc/student_info`, а также вести счётчик обращений к этому файлу.

Функциональность:

- При загрузке модуля создаётся файл `/proc/student_info`.
- При чтении файла выводится информация:

  ```text
  Name: Kazak Artur
  Group: 8, Subgroup: 2
  Module loaded at: <jiffies> jiffies
  Read count: <n>
  ```

- `Read count` — количество чтений, реализован как атомарная переменная `atomic_t`.

Ключевые элементы реализации:

- Глобальные данные:

  ```c
  static struct proc_dir_entry *proc_entry;
  static unsigned long loaded_jiffies;
  static atomic_t read_count = ATOMIC_INIT(0);
  ```

- Функция отображения содержимого файла:

  ```c
  static int student_info_show(struct seq_file *m, void *v)
  {
      int count = atomic_inc_return(&read_count);

      seq_printf(m, "Name: Kazak Artur\n");
      seq_printf(m, "Group: 8, Subgroup: 2\n");
      seq_printf(m, "Module loaded at: %lu jiffies\n", loaded_jiffies);
      seq_printf(m, "Read count: %d\n", count);

      return 0;
  }
  ```

- Используется интерфейс `seq_file` и `proc_ops`:

  ```c
  static int student_info_open(struct inode *inode, struct file *file)
  {
      return single_open(file, student_info_show, NULL);
  }

  static const struct proc_ops student_info_fops = {
      .proc_open    = student_info_open,
      .proc_read    = seq_read,
      .proc_lseek   = seq_lseek,
      .proc_release = single_release,
  };
  ```

- Инициализация (с проверкой результата `proc_create`):

  ```c
  static int __init student_info_init(void)
  {
      loaded_jiffies = jiffies;
      atomic_set(&read_count, 0);

      proc_entry = proc_create("student_info", 0444, NULL, &student_info_fops);
      if (!proc_entry) {
          pr_err("proc_module: failed to create /proc/student_info\n");
          return -ENOMEM;
      }

      pr_info("proc_module: loaded, /proc/student_info created\n");
      return 0;
  }
  ```

- Завершение:

  ```c
  static void __exit student_info_exit(void)
  {
      if (proc_entry) {
          proc_remove(proc_entry);
          proc_entry = NULL;
      }

      pr_info("proc_module: unloaded, /proc/student_info removed\n");
  }
  ```

Таким образом, учтены замечания по многопоточности (`atomic_t`) и по необходимости проверки результата `proc_create` на `NULL`.

---

### 3.3. Задание C — модуль `char_device` (символьное устройство `/dev/mychardev`)

**Файл:** `src/char_device.c`  

Назначение: реализовать символьное устройство, предоставляющее буфер фиксированного размера (1024 байта) и поддерживающее операции чтения и записи.

Основные характеристики:

- Имя устройства: `"mychardev"`.
- Буфер в ядре: `device_buffer` размером 1024 байта.
- Поддерживаемые операции: `open`, `release`, `read`, `write`.
- Используется `mutex` для защиты буфера и переменной `data_size` от гонок.
- Номер устройства (major/minor) выделяется динамически через `alloc_chrdev_region`.

Ключевые элементы:

- Глобальные данные:

  ```c
  #define MYCHDEV_MAX_SIZE 1024

  static dev_t dev_number;
  static struct cdev my_cdev;

  static char *device_buffer;
  static size_t data_size;

  static DEFINE_MUTEX(mychdev_mutex);
  ```

- Обработчики операций:

  - `my_open` и `my_release` логируют факты открытия и закрытия устройства в `dmesg`.
  - `my_read`:
    - захватывает `mutex`,
    - вычисляет количество доступных для чтения байт,
    - копирует данные в user-space с помощью `copy_to_user`,
    - обновляет смещение `*ppos`,
    - освобождает `mutex`.
  - `my_write`:
    - захватывает `mutex`,
    - проверяет, есть ли свободное место в буфере,
    - ограничивает количество записываемых данных 1024 байтами,
    - копирует данные из user-space (`copy_from_user`),
    - обновляет `data_size` и `*ppos`,
    - освобождает `mutex`.

- Регистрация устройства в `module_init`:

  ```c
  ret = alloc_chrdev_region(&dev_number, 0, 1, MYCHDEV_NAME);
  ...
  cdev_init(&my_cdev, &my_fops);
  cdev_add(&my_cdev, dev_number, 1);
  device_buffer = kmalloc(MYCHDEV_MAX_SIZE, GFP_KERNEL);
  ```

- Освобождение ресурсов в `module_exit`:

  ```c
  if (device_buffer)
      kfree(device_buffer);

  cdev_del(&my_cdev);
  unregister_chrdev_region(dev_number, 1);
  ```

В сообщениях `dmesg` дополнительно выводится подсказка по созданию `device node`:

```text
char_device: create node with 'mknod /dev/mychardev c <MAJOR> 0'
```

---

## 4. Сценарий сборки и тестирования

### 4.1. Сборка модулей

Все команды выполнялись из каталога:

```bash
cd lab5/gr8sub2/КАЗАК_АРТУР
```

Сборка:

```bash
make
```

Ожидаемый результат после успешной сборки:

```bash
ls src
Makefile
hello_module.c
hello_module.ko
proc_module.c
proc_module.ko
char_device.c
char_device.ko
```

Очистка:

```bash
make clean
```

---

### 4.2. Тестирование `hello_module`

Краткий сценарий (подробная трассировка находится в `logs/test_hello.txt`):

1. Загрузка модуля без параметров:

   ```bash
   sudo insmod src/hello_module.ko
   dmesg | tail -n 10
   ```

   В `dmesg` ожидается строка:

   ```text
   hello_module: Hello from Kazak Artur module!
   ```

2. Выгрузка:

   ```bash
   sudo rmmod hello_module
   dmesg | tail -n 10
   ```

   Ожидаемое сообщение:

   ```text
   hello_module: Goodbye from Kazak Artur module!
   ```

3. Загрузка с параметром:

   ```bash
   sudo insmod src/hello_module.ko message="Custom message from report"
   dmesg | tail -n 10
   ```

   В журнале ядра:

   ```text
   hello_module: Custom message from report
   ```

4. Повторная выгрузка — сообщение о завершении работы модуля.

---

### 4.3. Тестирование `proc_module` и `/proc/student_info`

Сценарий тестирования (подробности в `logs/test_proc.txt`):

1. Загрузка модуля:

   ```bash
   sudo insmod src/proc_module.ko
   dmesg | tail -n 10
   ```

   Ожидаемое сообщение:

   ```text
   proc_module: loaded, /proc/student_info created
   ```

2. Проверка наличия файла:

   ```bash
   ls /proc | grep student_info
   ```

3. Многократное чтение файла:

   ```bash
   cat /proc/student_info
   cat /proc/student_info
   cat /proc/student_info
   ```

   При каждом чтении наблюдается увеличение `Read count` (1, 2, 3 и т.д.), при этом значения `Name`, `Group` и `Module loaded at` остаются неизменными.

4. Выгрузка:

   ```bash
   sudo rmmod proc_module
   dmesg | tail -n 10
   ```

   Ожидаемое сообщение:

   ```text
   proc_module: unloaded, /proc/student_info removed
   ```

5. Повторная проверка:

   ```bash
   ls /proc | grep student_info
   ```

   — файл отсутствует.

---

### 4.4. Тестирование `char_device` и `/dev/mychardev`

Сценарий тестирования (подробности в `logs/test_chardev.txt`):

1. Загрузка модуля:

   ```bash
   sudo insmod src/char_device.ko
   dmesg | tail -n 10
   ```

   В выводе `dmesg` появляется информация о выделенном major-номере и размере буфера.

2. Создание символьного устройства:

   ```bash
   # предположим, что major = 240
   sudo mknod /dev/mychardev c 240 0

   # более строгие права: только root может писать, остальные могут читать
   sudo chmod 644 /dev/mychardev
   ```

3. Тест записи и чтения:

   ```bash
   echo "Hello from user space" > /dev/mychardev
   cat /dev/mychardev
   ```

   Ожидаемый вывод:

   ```text
   Hello from user space
   ```

   В `dmesg` фиксируются операции `open`, `write`, `read`, `close` с указанием количества байт и текущего смещения.

4. Проверка ограничения размера буфера:

   ```bash
   python3 - << 'EOF'
print("A" * 2000, end="")
EOF > /dev/mychardev
   ```

   Ожидается запись не более 1024 байт; в журнале ядра отображается фактический объём записанных данных (1024).

5. Очистка:

   ```bash
   sudo rm /dev/mychardev
   sudo rmmod char_device
   dmesg | tail -n 10
   ```

---

## 5. Риски и безопасность

В ходе работы были отдельно проанализированы следующие моменты:

1. **Права доступа к символьному устройству.**  
   Для `/dev/mychardev` использовались права `644`, при которых только владелец (root) может записывать в устройство, а остальные пользователи могут только читать. Такой вариант безопаснее, чем `666` или `660`, и уменьшает риск неконтролируемой записи в устройство со стороны обычных пользователей или групп.

2. **Многопоточность в символьном устройстве.**  
   В модуле `char_device` реализована защита общих данных (`device_buffer`, `data_size`) с помощью `mutex` (`DEFINE_MUTEX(mychdev_mutex)`). Все операции `read` и `write` оборачиваются в `mutex_lock_interruptible` / `mutex_unlock`, что предотвращает гонки при одновременном доступе из нескольких процессов.

3. **Многопоточность в работе `/proc/student_info`.**  
   Счётчик обращений `read_count` в `proc_module` реализован как атомарная переменная `atomic_t`, а увеличение осуществляется через `atomic_inc_return()`. Это гарантирует корректное изменение счётчика при одновременных чтениях.

4. **Обработка ошибок и освобождение ресурсов.**  
   Во всех модулях:

   - в `module_init` проверяются коды возврата системных функций (`alloc_chrdev_region`, `cdev_add`, `proc_create`, `kmalloc`);
   - в случае ошибки уже выделенные ресурсы корректно освобождаются;
   - в `module_exit` освобождаются все ресурсы: номера устройств, `cdev`, память, записи в `/proc`.

---

## 6. Ответы на теоретические вопросы

Ниже приведены краткие ответы на основные теоретические вопросы, связанные с работой.

### 6.1. Базовые понятия

**Что такое модуль ядра и зачем он нужен?**  
Модуль ядра — это отдельный бинарный файл, который можно динамически загрузить в уже работающее ядро Linux для расширения его функциональности (например, драйвер устройства) без пересборки и перезагрузки ядра.

**Чем отличается kernel-space от user-space?**  
В user-space работают обычные процессы с ограниченными правами доступа к памяти и устройствам; их ошибки приводят к падению только процесса. В kernel-space работает ядро и модули, которые имеют полный доступ к памяти и оборудованию; ошибка может привести к `kernel panic` и остановке всей системы.

**Почему нельзя использовать `printf()` в модуле ядра?**  
В ядре нет стандартного вывода и стандартной библиотеки C в том виде, как в user-space. Вместо `printf` используется `printk`/`pr_info`, которые пишут сообщения в буфер журнала ядра, просматриваемый через `dmesg` или `journalctl -k`.

**Что такое `kernel panic`?**  
`Kernel panic` — это критическая ошибка ядра, после которой его дальнейшая работа считается небезопасной. Обычно сопровождается сообщением в консоли и остановкой системы или перезагрузкой.

---

### 6.2. Жизненный цикл модуля

**Какие функции вызываются при `insmod` и `rmmod`?**  
При `insmod` вызывается функция, зарегистрированная через `module_init()`, а при `rmmod` — функция, зарегистрированная через `module_exit()`.

**Что должна делать функция `module_exit()`?**  
Освобождать все ресурсы, выделенные модулем: память, номера устройств, записи в `/proc` и другие структуры. После выполнения `module_exit` система должна находиться в таком состоянии, будто модуль никогда не загружался.

**Что происходит, если `module_init()` возвращает ошибку?**  
Модуль не загружается. Внутри `module_init()` перед возвратом ошибки необходимо вручную освободить все ресурсы, которые успели выделиться до возникновения ошибки.

---

### 6.3. Память и взаимодействие с user-space

**Чем `kmalloc()` отличается от `malloc()`?**  
`malloc` — функция user-space, выделяет память в адресном пространстве процесса. `kmalloc` — функция ядра, выделяет память в kernel-space и требует указания флагов GFP (например, `GFP_KERNEL` или `GFP_ATOMIC`).

**Что такое флаги GFP?**  
Флаги GFP (Get Free Pages) задают условия выделения памяти ядром: можно ли спать при выделении (`GFP_KERNEL`), нужно ли выделять память немедленно (`GFP_ATOMIC`) и из каких зон памяти её брать.

**Почему нельзя напрямую использовать указатели user-space в ядре?**  
Адресные пространства ядра и процесса различаются. Указатель из user-space может быть недопустимым или недоступным для ядра. Для безопасного обмена данными используются функции `copy_to_user()` и `copy_from_user()`.

---

### 6.4. /proc, /sys и символьные устройства

**Что такое `/proc`?**  
`/proc` — виртуальная файловая система, через которую ядро предоставляет информацию о себе и процессах. Модули могут создавать в ней свои файлы для выдачи статистики и настроек.

**Чем `/sys` отличается от `/proc`?**  
`/sys` (sysfs) представляет объектную модель устройств и драйверов; каждый файл обычно соответствует одному атрибуту. `/proc` изначально ориентирована на информацию о процессах и состоянии ядра.

**Что такое символьное устройство?**  
Символьное устройство — это объект, к которому обращаются как к потоку байт через системные вызовы `open`, `read`, `write`, `close`. Драйвер символьного устройства регистрирует `file_operations`, а ядро вызывает соответствующие функции при обращениях из user-space.

---

### 6.5. Параметры модулей и лицензирование

**Как передать параметр модулю при загрузке?**  
В коде объявляется переменная и регистрируется через `module_param`, например:

```c
static int count = 1;
module_param(count, int, 0444);
MODULE_PARM_DESC(count, "Example parameter");
```

При загрузке:

```bash
sudo insmod mymodule.ko count=10
```

**Зачем нужен `MODULE_LICENSE()`?**  
`MODULE_LICENSE()` сообщает ядру лицензию модуля. Для лицензии `"GPL"` разрешён доступ к GPL-only символам и ядро не помечает себя как tainted.

---

## 7. Использование AI-помощников

В процессе выполнения лабораторной работы использовались AI-инструменты как вспомогательный инструмент для:

- уточнения синтаксиса и типичных шаблонов реализации модулей ядра (использование `proc_create`, `seq_file`, `alloc_chrdev_region`, `cdev_add`, `mutex`, `atomic_t`);
- генерации черновиков текстов README и REPORT, которые затем были доработаны и адаптированы под фактическую реализацию;
- анализа замечаний проверяющего (права доступа к устройству, многопоточность, проверка `proc_create`) и разработки соответствующих исправлений.

Финальные решения по архитектуре модулей, структуре каталогов, сценариям тестирования и корректировкам кода принимались с учётом требований задания и замечаний рецензента.

---

## 8. Выводы

В ходе выполнения лабораторной работы:

- разработаны и собраны три модуля ядра Linux: `hello_module`, `proc_module`, `char_device`;
- реализовано взаимодействие ядра с user-space через `/proc` и через символьное устройство `/dev/mychardev`;
- на практике рассмотрены вопросы безопасности: выбор прав доступа, защита от гонок с помощью `mutex` и `atomic_t`, корректное освобождение ресурсов;
- оформлен отчёт, включающий описание структуры проекта, сценарии тестирования, анализ рисков и краткие ответы на теоретические вопросы.

Работа показала, что разработка модулей ядра требует особенно аккуратного отношения к деталям (управление ресурсами, синхронизация, права доступа), так как ошибка в kernel space может привести к выходу из строя всей системы, а не одного приложения.

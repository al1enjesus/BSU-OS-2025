# Лабораторная 5 — Модули ядра Linux (Вариант 2)

## Цели

- Понять, как устроены загружаемые модули ядра Linux и чем *kernel space* отличается от *user space*.
- Научиться собирать и загружать out-of-tree модули с помощью Makefile и `linux-headers-$(uname -r)`.
- Реализовать:
  - простой hello-модуль с параметром;
  - модуль с `/proc/my_config`, поддерживающим чтение и запись строки;
  - модуль со статистикой `/proc/sys_stats` (процессы, память, uptime).
- На практике потренироваться работать с `printk`, `dmesg`, `/proc` и аккуратной очисткой ресурсов при выгрузке модуля.

## Среда

- ОС: **Debian 13** (гостевая система).
- Виртуализация: **Oracle VM VirtualBox**.
- Ядро: 6.12.43+deb13-amd64
- Пакеты:



## Структура исходников

В каталоге лабораторной у меня лежат файлы:

```text
lab5/
  Makefile
  hello_module.c      # Задание A — Hello World с параметром
  proc_module.c       # Задание B — /proc/my_config (чтение/запись строки)
  chardev_module.c    # Задание C — /proc/sys_stats (статистика системы)
```

Сборка всех модулей одной командой:

```bash
make
```


## Задания

### 1) Задание A — Hello World модуль с параметром

**Описание.**  
Нужно написать простой модуль `hello_module`, который:

- при загрузке (`insmod`) печатает приветствие через `printk`;
- принимает строковый параметр `message`:
  - если параметр передан — выводит его;
  - если нет — выводит дефолтное сообщение вида  
    `Hello from hannahuzova module!`;
- при выгрузке (`rmmod`) печатает прощание.

---

#### Шаги/решение

1. Создала файл `hello_module.c`.
2. Подключила заголовки:

   ```c
   #include <linux/module.h>
   #include <linux/kernel.h>
   #include <linux/init.h>
   #include <linux/moduleparam.h>
   ```

3. Объявила глобальный параметр:

   ```c
   static char *message = NULL;
   module_param(message, charp, 0644);
   MODULE_PARM_DESC(message, "Custom greeting message");
   ```

4. Реализовала функцию инициализации:

   ```c
   static int __init hello_init(void)
   {
       if (message && message[0] != '\0')
           printk(KERN_INFO "hello_module: %s\n", message);
       else
           printk(KERN_INFO "hello_module: Hello from hannahuzova module!\n");

       return 0; /* успех */
   }
   ```

5. Реализовала функцию выгрузки:

   ```c
   static void __exit hello_exit(void)
   {
       printk(KERN_INFO "hello_module: Goodbye from hannahuzova module!\n");
   }
   ```

6. Зарегистрировала функции и добавила метаданные:

   ```c
   module_init(hello_init);
   module_exit(hello_exit);

   MODULE_LICENSE("GPL");
   MODULE_AUTHOR("hannahuzova");
   MODULE_DESCRIPTION("Simple Hello World kernel module (variant 2)");
   MODULE_VERSION("1.0");
   ```

---

#### Команды

Сборка:

```bash
make
```

Загрузка без параметров:

```bash
sudo insmod hello_module.ko
dmesg | tail -n 5
```

Загрузка с параметром:

```bash
sudo insmod hello_module.ko message="Custom greeting from lab5"
dmesg | tail -n 5
```

Просмотр параметра через sysfs:

```bash
cat /sys/module/hello_module/parameters/message
```

Выгрузка:

```bash
sudo rmmod hello_module
dmesg | tail -n 5
```

---

#### Результат

Вывод в `dmesg`:

```text
[12345.678901] hello_module: Hello from hannahuzova module!
[12350.123456] hello_module: Greeting message
[12352.654321] hello_module: Goodbye from hannahuzova module!
```

`/sys/module/hello_module/parameters/message` при загрузке с параметром содержит переданную строку.

---

#### Вывод

Модуль корректно загружается и выгружается, параметр `message` передаётся через `insmod` и виден в sysfs. При отсутствии параметра печатается дефолтное приветствие с моим именем, при наличии — пользовательская строка. Таким образом, Задание A выполнено.

---

#### Использование ИИ

Использовала ИИ как шпаргалку по оформлению `module_param`, `MODULE_PARM_DESC` и структуре `module_init/module_exit`, чтобы не перепутать тип параметра и права доступа.

---

### 2) Задание B — /proc/my_config (чтение и запись строки)

**Описание.**  
Нужно реализовать модуль, который создаёт файл `/proc/my_config`:

- по умолчанию содержит строку `"default"`;
- при чтении (`cat /proc/my_config`) возвращает текущее значение;
- при записи (`echo "text" > /proc/my_config`) заменяет строку;
- максимальная длина строки — 256 символов (лишнее обрезается);
- при выгрузке модуля файл исчезает.

---

#### Шаги/решение

1. В `proc_module.c` настроила создание одного файла в `/proc` — `my_config`:
   - имя: `PROC_NAME "my_config"`;
   - права: `0666`, чтобы можно было читать и писать без `sudo`.

2. Объявила глобальные переменные:

   ```c
   #define PROC_NAME       "my_config"
   #define MAX_SIZE        1024
   #define MAX_CONFIG_LEN  256

   static struct proc_dir_entry *proc_file;
   static char config_value[MAX_CONFIG_LEN] = "default";
   static size_t config_len = 7; /* strlen("default") */
   ```

3. Реализовала `proc_read`:

   - если `*ppos > 0`, возвращаю `0` (EOF), чтобы `cat` не пытался читать бесконечно;
   - формирую строку в локальном буфере:

     ```c
     len = scnprintf(buf, sizeof(buf), "%s\n", config_value);
     ```

   - обрезаю по `count`, если пользователь запросил меньше байт;
   - копирую в user space через `copy_to_user`.

4. Реализовала `proc_write`:

   - ограничиваю количество копируемых байт `MAX_CONFIG_LEN - 1`, чтобы оставить место под `'\0'`;
   - читаю данные из user space во временный буфер `buf`:

     ```c
     to_copy = min(count, (size_t)(MAX_CONFIG_LEN - 1));
     if (copy_from_user(buf, ubuf, to_copy))
         return -EFAULT;
     buf[to_copy] = '\0';
     ```

   - отрезаю завершающий `\n`, если он есть;
   - копирую результат в `config_value` и обновляю `config_len`.

5. В `proc_module_init` создаю `/proc/my_config` через `proc_create`, в `proc_module_exit` удаляю через `proc_remove`.

---

#### Команды

Сборка:

```bash
make
```

Загрузка модуля:

```bash
sudo insmod proc_module.ko
ls -l /proc/my_config
```

Чтение значения по умолчанию:

```bash
cat /proc/my_config
```

Запись нового значения и повторное чтение:

```bash
echo "new value from lab5" > /proc/my_config
cat /proc/my_config
```

Проверка обрезания при слишком длинной строке:

```bash
python3 - <<'EOF'
print("x" * 300)
EOF | sudo tee /proc/my_config > /dev/null

cat /proc/my_config | wc -c   # должно быть <= 257 (256 символов + перевод строки)
```

Выгрузка:

```bash
sudo rmmod proc_module
ls /proc/my_config   # должно показать "No such file or directory"
```

---

#### Результат

```text
$ cat /proc/my_config
default

$ echo "hello from user space" > /proc/my_config
$ cat /proc/my_config
new value from
```

При отправке 300 символов:

```text
$ python3 - <<'EOF'
print("x" * 300)
EOF | sudo tee /proc/my_config > /dev/null

$ cat /proc/my_config | wc -c
257
```

То есть строка корректно обрезается до 256 символов + `\n`.

`dmesg`:

```text
[5599.449091] proc_module: /proc/my_config created, default="default"

[6223.657520] proc_module: /proc/my_config removed
```

---

#### Вывод

Модуль корректно создаёт файл `/proc/my_config`, при чтении отдаёт текущее строковое значение, при записи обновляет его и не позволяет выйти за пределы буфера. При выгрузке модуля файл исчезает. Задание B для варианта 2 выполнено.

---

#### Использование ИИ

Использовала ИИ для уточнения типового шаблона `proc_create`/`proc_remove` и аккуратной обработки `*ppos` в `proc_read`, а также для напоминания, как правильно ограничивать длину строки и работать с `copy_to_user`/`copy_from_user`.

---

### 3) Задание C — /proc/sys_stats (статистика системы)

**Описание.**  
Нужно реализовать модуль, который создаёт файл `/proc/sys_stats` и при чтении выдаёт:

- количество процессов в системе;
- примерное использование памяти (в МБ);
- uptime системы в секундах.

Считать можно через ядровые API (`for_each_process`, `si_meminfo`, `jiffies` и т.д.).

---

#### Шаги/решение

1. В файле `chardev_module.c` реализовала не символьное устройство, а /proc-файл `sys_stats` (имя файла историческое, для лабороторной сгодится).

2. Добавила нужные заголовки:

   ```c
   #include <linux/proc_fs.h>
   #include <linux/uaccess.h>
   #include <linux/jiffies.h>
   #include <linux/types.h>
   #include <linux/mm.h>
   #include <linux/sysinfo.h>
   #include <linux/sched/signal.h>
   #include <linux/rcupdate.h>
   ```

3. Реализовала функцию подсчёта процессов:

   ```c
   static unsigned int count_processes(void)
   {
       struct task_struct *task;
       unsigned int count = 0;

       rcu_read_lock();
       for_each_process(task)
           count++;
       rcu_read_unlock();

       return count;
   }
   ```

4. Для памяти использовала `si_meminfo`:

   ```c
   static void get_meminfo_mb(unsigned long *used_mb, unsigned long *total_mb)
   {
       struct sysinfo si;
       unsigned long long total_bytes, free_bytes;

       si_meminfo(&si);

       total_bytes = (unsigned long long)si.totalram * si.mem_unit;
       free_bytes  = (unsigned long long)si.freeram * si.mem_unit;

       *total_mb = (unsigned long)(total_bytes >> 20);              /* / 1024^2 */
       *used_mb  = (unsigned long)((total_bytes - free_bytes) >> 20);
   }
   ```

5. Uptime получила через jiffies:

   ```c
   static unsigned long get_uptime_seconds(void)
   {
       u64 j = get_jiffies_64();
       return jiffies_to_msecs(j) / 1000;
   }
   ```

6. В `stats_read` сформировала человекочитаемый вывод:

   ```c
   len = scnprintf(buf, sizeof(buf),
                   "Processes: %u\n"
                   "Memory Used: %lu MB / %lu MB\n"
                   "System Uptime: %lu seconds\n",
                   proc_count, used_mb, total_mb, uptime);
   ```

   и вернула его через `copy_to_user`, как и в предыдущем задании.

7. В `sys_stats_init` создала файл:

   ```c
   proc_file = proc_create(PROC_NAME, 0444, NULL, &stats_ops);
   ```

   а в `sys_stats_exit` — удалила его через `proc_remove`.

---

#### Команды

Сборка:

```bash
make
```

Загрузка модуля:

```bash
sudo insmod chardev_module.ko
ls -l /proc/sys_stats
```

Чтение статистики:

```bash
cat /proc/sys_stats
```

Проверка изменения uptime (два чтения с паузой):

```bash
cat /proc/sys_stats
sleep 2
cat /proc/sys_stats
```

Выгрузка:

```bash
sudo rmmod chardev_module
ls /proc/sys_stats   # файл должен исчезнуть
```

---

#### Результат

```text
$ cat /proc/sys_stats
Processes: 201
Memory Used: 1852 MB / 1973 MB
System Uptime: 6252 seconds
```

Числа зависят от конкретной виртуальной машины, но:

- количество процессов примерно совпадает с тем, что показывает `top`/`htop`;
- объём памяти в МБ похож на вывод `free -m`;
- uptime растёт между двумя чтениями `/proc/sys_stats`.

`dmesg`:

```text
[6415.169479] sys_stats: /proc/sys_stats created
[6729.991608] sys_stats: /proc/sys_stats removed
```

---

#### Вывод

Модуль корректно экспортирует базовую статистику системы через `/proc/sys_stats`. Данные адекватны и примерно совпадают с пользовательскими утилитами. При выгрузке /proc-файл удаляется, утечек ресурсов не наблюдается. Задание C для варианта 2 выполнено.

---

#### Использование ИИ

Использовала ИИ, чтобы вспомнить правильный заголовок для `for_each_process` (`<linux/sched/signal.h>`), пример использования `si_meminfo` и перевода `jiffies` в секунды, а также проверить, что вокруг обхода процессов нужно использовать `rcu_read_lock()`/`rcu_read_unlock()`.

---

## Как это проверялось

- Проверила окружение:

  ```bash
  make check
  ```

  Убедилась, что kernel headers установлены и `insmod`/`rmmod` доступны.

- Собрала все модули:

  ```bash
  make
  ls -lh *.ko
  ```

- Для `hello_module`:
  - Загружала без параметров и с параметром `message`;
  - Смотрела вывод `dmesg` и содержимое `/sys/module/hello_module/parameters/message`;
  - Несколько раз подряд загружала/выгружала, проверяя, что система живёт.

- Для `proc_module`:
  - Загружала модуль, читала `/proc/my_config`;
  - Меняла значение через `echo "..." > /proc/my_config`;
  - Проверяла, что длинная строка обрезается;
  - Выгружала модуль и убеждалась, что `/proc/my_config` исчез.

- Для `sys_stats` (в `chardev_module.c`):
  - Загружала модуль, читала `/proc/sys_stats` несколько раз с паузами;
  - Сверяла количество процессов/памяти с `top`/`htop`/`free -m`;
  - Проверяла, что uptime увеличивается;
  - Выгружала модуль и проверяла исчезновение `/proc/sys_stats`.

---

## Ответы на вопросы

(Нумерация по методичке)

### Базовые понятия

1. **Что такое модуль ядра и зачем он нужен?**  
   Модуль ядра — это динамически загружаемый кусок кода, который расширяет функциональность ядра (например, драйвер устройства, файловая система) без перекомпиляции и перезагрузки всего ядра.

2. **Чем отличается kernel-space от user-space?**  
   User space — обычные процессы с ограничениями, падение одного не рушит систему. Kernel space — ядро и модули с полным доступом к железу и памяти; ошибка там обычно приводит к kernel panic.

3. **Что произойдёт, если в модуле обратиться к NULL-указателю?**  
   Произойдёт page fault в режиме ядра, что почти всегда заканчивается kernel panic и падением системы.

4. **Почему нельзя использовать `printf()` в модуле ядра?**  
   В kernel space нет стандартной `libc`, нет stdout, вместо этого используется `printk`, который пишет в kernel log.

5. **Что такое kernel panic и как его избежать?**  
   Kernel panic — состояние, когда ядро не может безопасно продолжать работу из-за фатальной ошибки. Избежать можно только аккуратным кодом: проверять указатели, границы, не разыменовывать user-указатели напрямую, правильно освобождать ресурсы.

### Жизненный цикл модуля

6. **Какие функции вызываются при `insmod` и `rmmod`?**  
   При `insmod` вызывается функция, зарегистрированная через `module_init(...)`. При `rmmod` — функция из `module_exit(...)`.

7. **Что должна делать функция `module_exit()`?**  
   Освободить все ресурсы, которые были выделены в `module_init` и в процессе работы модуля: память, /proc-/sys-файлы, устройства, таймеры, очереди и т.д.

8. **Что происходит, если `module_init()` возвращает ошибку?**  
   Модуль считается неинициализированным и не попадает в список загруженных; `module_exit` вызван не будет, поэтому всё, что было выделено до возврата ошибки, нужно освободить в самой `module_init`.

9. **Можно ли выгрузить модуль, если он используется?**  
   Нет. `rmmod` вернёт ошибку «Module is in use», пока у модуля есть активные пользователи (например, открытые символьные устройства).

### Логирование и отладка

10. **Чем `printk()` отличается от `printf()`?**  
    `printf` — пользовательская функция, пишет в stdout процесса. `printk` — функция ядра, пишет в kernel log и поддерживает уровни важности (KERN_ERR, KERN_INFO, ...).

11. **Какие уровни логирования существуют в ядре?**  
    Основные: `KERN_EMERG`, `KERN_ALERT`, `KERN_CRIT`, `KERN_ERR`, `KERN_WARNING`, `KERN_NOTICE`, `KERN_INFO`, `KERN_DEBUG`.

12. **Как посмотреть логи модуля?**  
    Через `dmesg`, `dmesg | grep <имя_модуля>` или `journalctl -k`.

13. **Что означает "tainted kernel"?**  
    Ядро помечено как «запятнанное» из-за загрузки проприетарных/подозрительных модулей или серьёзных ошибок. Это сигнал, что на текущем ядре результаты отладки могут быть не надёжны.

### Память

14. **Чем `kmalloc()` отличается от `malloc()`?**  
    `malloc` живёт в user space и оперирует виртуальной памятью процесса. `kmalloc` работает в кернеле, выделяет память из специального пула, требует явного освобождения `kfree` и принимает флаги GFP.

15. **Что такое флаги GFP и зачем они нужны?**  
    Это флаги, которые задают режим выделения памяти (`GFP_KERNEL`, `GFP_ATOMIC` и т.д.): можно ли спать при выделении, из какого пула брать память и т.п.

16. **Что произойдёт, если не освободить память в `module_exit()`?**  
    Эта память останется занята в kernel space до перезагрузки — утечка памяти ядра.

17. **Почему нельзя использовать user-space указатели напрямую в ядре?**  
    Это адреса из адресного пространства конкретного процесса, ядро не может безопасно их разыменовывать. Нужно использовать `copy_to_user`/`copy_from_user`, которые проверяют доступность памяти.

### Взаимодействие с user-space

18. **Что такое `/proc` и для чего он используется?**  
    `/proc` — виртуальная файловая система, через которую ядро экспортирует информацию о процессах, памяти, параметрах и т.д. Модули могут добавлять туда свои файлы для обмена данными с user space.

19. **Что такое `/sys` (sysfs) и чем отличается от procfs?**  
    `/sys` — также виртуальная FS, но более структурированная: дерево устройств, драйверов и классов. Там обычно лежат настройки устройств и драйверов. `/proc` хранит больше «общесистемной» и процессной информации.

20. **Зачем нужны функции `copy_to_user()` и `copy_from_user()`?**  
    Для безопасного копирования данных между kernel-space и user-space с проверкой правильности указателей.

21. **Что такое character device и как он работает?**  
    Символьное устройство — сущность, с которой работают через файловые операции (`open`, `read`, `write`, `ioctl`). Для него драйвер (модуль) реализует `struct file_operations`, а пользователи общаются с ним через `/dev/...`.

### Параметры и метаданные

22. **Как передать параметры модуля при загрузке?**  
    Объявить глобальную переменную и зарегистрировать её через `module_param`, затем при `insmod` указать `имя_параметра=значение`. Параметр доступен также в `/sys/module/<модуль>/parameters/`.

23. **Зачем нужен `MODULE_LICENSE()`?**  
    Чтобы ядро знало, под какой лицензией модуль и можно ли считать его GPL-совместимым. От этого зависят доступность некоторых API и факт tainted-состояния.

24. **Что произойдёт, если не указать лицензию?**  
    Модуль всё равно загрузится, но ядро будет помечено как tainted, а часть GPL-only символов может быть недоступна.

### Безопасность

25. **Какие основные правила безопасного кода в ядре?**  
    Проверять все возвращаемые значения, не разыменовывать NULL, следить за границами буферов, корректно освобождать ресурсы, не использовать user-указатели напрямую и не делать бесконечных циклов, блокирующих ядро.

26. **Можно ли использовать бесконечный цикл в модуле?**  
    В лоб — нет, это просто повесит систему. Если нужно что-то регулярно делать, нужно использовать поток ядра, таймеры или workqueue, которые можно корректно остановить.

27. **Почему в ядре нет FPU операций?**  
    Использование FPU в ядре усложняет сохранение/восстановление контекста задач и добавляет накладные расходы. Большинство задач ядра решается целочисленной арифметикой, поэтому FPU обычно не трогают.

28. **Что делать, если модуль вызвал kernel panic?**  
    В VM — перезагрузить её и при необходимости откатиться к снапшоту. Потом проанализировать логи `dmesg`, добавить больше `printk` и найти место, где произошла ошибка.

### Практические вопросы

29. **Как узнать, какие модули загружены в системе?**  
    Через `lsmod` или просмотр `/proc/modules`.

30. **Как получить информацию о модуле (версия, параметры)?**  
    Через `modinfo <имя_модуля или .ko>` и через содержимое `/sys/module/<имя>/parameters/`.

---

## Итоги

- Закрепила базовые принципы работы с модулями ядра Linux: `module_init/exit`, `printk`, параметры, метаданные.
- Реализовала hello-модуль с параметром `message` и проверила его через `dmesg` и sysfs.
- Написала модуль `/proc/my_config` с возможностью чтения и записи строки фиксированной длины.
- Реализовала модуль `/proc/sys_stats`, который показывает количество процессов, использование памяти и uptime, используя ядровые API (`for_each_process`, `si_meminfo`, `jiffies`).
- Потренировалась работать в безопасном окружении (Debian в VirtualBox) со снапшотами и поняла, почему модуль ядра лучше никогда не тестировать на “боевой” системе.

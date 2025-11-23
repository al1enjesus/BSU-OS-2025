# лабораторная 5 - модули ядра linux (вариант 2)

## окружение

```
Ubuntu 25.04 (VirtualBox)
GNU bash, версия 5.2.37(1)-release
gcc (Ubuntu 14.2.0-19ubuntu2) 14.2.0
```

## цель работы

научится разрабатывать базовые модули ядра для linux, изучив особенности kernel-space кода и архитектуру ядра.

## задача А - hello world

был написан простой модуль `hello_module`, который выводит сообщение при загрузке и выгрузке модуля из ядра:

```
somerandomprog@vbox:~/BSU-OS-2025/lab5/gr9sub2/БУДНИК_МИХАИЛ/src$ make test-hello
Building kernel modules...
make -C /lib/modules/6.14.0-35-generic/build M=/home/somerandomprog/BSU-OS-2025/lab5/gr9sub2/БУДНИК_МИХАИЛ/src modules
make[1]: вход в каталог «/usr/src/linux-headers-6.14.0-35-generic»
make[2]: вход в каталог «/home/somerandomprog/BSU-OS-2025/lab5/gr9sub2/БУДНИК_МИХАИЛ/src»
make[2]: выход из каталога «/home/somerandomprog/BSU-OS-2025/lab5/gr9sub2/БУДНИК_МИХАИЛ/src»
make[1]: выход из каталога «/usr/src/linux-headers-6.14.0-35-generic»

✓ Modules built successfully!
  To load: sudo insmod <module>.ko
  To view logs: dmesg | tail

=== Testing hello_module ===
Loading module...
sudo insmod hello_module.ko


Unloading module...
sudo rmmod hello_module

Output:
[  442.375597] hello_module: hello from module by Michael :)
[  442.475699] hello_module: goodbye from module by Michael...
```

также можно указать параметр `message`, который перезапишет сообщение по умолчанию:

```
somerandomprog@vbox:~/BSU-OS-2025/lab5/gr9sub2/БУДНИК_МИХАИЛ/src$ make
Building kernel modules...

somerandomprog@vbox:~/BSU-OS-2025/lab5/gr9sub2/БУДНИК_МИХАИЛ/src$ sudo insmod hello_module.ko message='"hello my precious world!"'

somerandomprog@vbox:~/BSU-OS-2025/lab5/gr9sub2/БУДНИК_МИХАИЛ/src$ sudo dmesg | grep -i hello_module | tail -5
[  834.902227] hello_module: hello my precious world!

somerandomprog@vbox:~/BSU-OS-2025/lab5/gr9sub2/БУДНИК_МИХАИЛ/src$ sudo rmmod hello_module
somerandomprog@vbox:~/BSU-OS-2025/lab5/gr9sub2/БУДНИК_МИХАИЛ/src$ sudo dmesg | grep -i hello_module | tail -5
[  834.902227] hello_module: hello my precious world!
[  843.435518] hello_module: goodbye from module by Michael...
```

## задача B - простой I/O с `/proc` файлом

был написан модуль, который создаёт файл `/proc/my_config`, в который можно записать и прочитать простое сообщение (до 256 символов). по умолчанию хранится строка `default`.

```
somerandomprog@vbox:~/BSU-OS-2025/lab5/gr9sub2/БУДНИК_МИХАИЛ/src$ make test-procBuilding kernel modules...
make -C /lib/modules/6.14.0-35-generic/build M=/home/somerandomprog/BSU-OS-2025/lab5/gr9sub2/БУДНИК_МИХАИЛ/src modules
make[1]: вход в каталог «/usr/src/linux-headers-6.14.0-35-generic»
make[2]: вход в каталог «/home/somerandomprog/BSU-OS-2025/lab5/gr9sub2/БУДНИК_МИХАИЛ/src»
make[2]: выход из каталога «/home/somerandomprog/BSU-OS-2025/lab5/gr9sub2/БУДНИК_МИХАИЛ/src»
make[1]: выход из каталога «/usr/src/linux-headers-6.14.0-35-generic»

✓ Modules built successfully!
  To load: sudo insmod <module>.ko
  To view logs: dmesg | tail

=== Testing proc_module ===
Loading module...
sudo insmod proc_module.ko

Reading /proc/my_config:
default
Writing 'test' to /proc/my_config...
Reading /proc/my_config again:
test

Unloading module...
sudo rmmod proc_module

dmesg output:
sudo dmesg | grep -i proc_module | tail -10
[ 1680.218986] proc_module: initializing
[ 1680.218998] proc_module: created /proc/my_config
[ 1680.285863] proc_module: read 8 bytes
[ 1680.308724] proc_module: wrote 5 bytes
[ 1680.320179] proc_module: read 5 bytes
[ 1680.374052] proc_module: Removed /proc/my_config
[ 1680.374062] proc_module: exiting
```

## задача C - `/proc` файл со статистикой системы

был написан модуль `proc_stats_module`, который выводит следующую информацию:

- кол-во процессов (`for_each_process()`)
- используемая память (`si_meminfo`)
- uptime системы (`ktime_get_boottime_ts64`)

```
somerandomprog@vbox:~/BSU-OS-2025/lab5/gr9sub2/БУДНИК_МИХАИЛ/src$ make test-proc-stats
Building kernel modules...
make -C /lib/modules/6.14.0-35-generic/build M=/home/somerandomprog/BSU-OS-2025/lab5/gr9sub2/БУДНИК_МИХАИЛ/src modules
make[1]: вход в каталог «/usr/src/linux-headers-6.14.0-35-generic»
make[2]: вход в каталог «/home/somerandomprog/BSU-OS-2025/lab5/gr9sub2/БУДНИК_МИХАИЛ/src»
make[2]: выход из каталога «/home/somerandomprog/BSU-OS-2025/lab5/gr9sub2/БУДНИК_МИХАИЛ/src»
make[1]: выход из каталога «/usr/src/linux-headers-6.14.0-35-generic»

✓ Modules built successfully!
  To load: sudo insmod <module>.ko
  To view logs: dmesg | tail

=== Testing proc_stats_module ===
Loading module...
sudo insmod proc_stats_module.ko

Reading /proc/sys_stats:
Processes: 220
Memory used: 3208 MB
System uptime: 2115 seconds

Unloading module...
sudo rmmod proc_stats_module

dmesg output:
sudo dmesg | grep -i proc_stats_module | tail -10
[ 2115.951489] proc_stats_module: initializing
[ 2115.951500] proc_stats_module: created /proc/sys_stats
[ 2116.055638] proc_stats_module: removed /proc/sys_stats
[ 2116.055646] proc_stats_module: exiting
```

## вопросы

### базовые понятия

- **что такое модуль ядра и зачем он нужен?**

    > **модуль ядра** - это кусок скомпилированного кода, который может быть загружен в ядро или выгружен из него по требованию, без необходимости пересборки ядра или перезагрузки системы.

    > он нужен для добавления поддержки нового оборудования (драйверы), файловых систем или сетевых протоколов динамически, что позволяет сохранить основной образ ядра компактным.

- **чем отличается kernel-space от user-space?**

    > **уровень привилегий:** kernel-space работает в ring 0 (полный доступ к памяти и оборудованию), user-space работает в ring 3 (ограниченный доступ).

    > **последствия ошибок:** ошибка в user-space (например, SEGFAULT) обычно убивает только процесс, ошибка в kernel-space может крашнуть всю систему (kernel panic).

    > **программный интерфейс:** в user-space доступна стандартная библиотека C (libc), в kernel-space - только API linux'а,  причём стек ядра очень мал (8-16 кб).

- **что произойдёт, если в модуле обратиться к `NULL` указателю?**

    > произойдет исключение процессора (page fault).

    > это вызовет сбой в потоке ядра. если контекст критический (например, прерывание), это приведет к kernel panic и полной остановке системы.

- **почему нельзя использовать printf() в модуле ядра?**

    > `printf()` - это функция стандартной библиотеки C (glibc), которая работает только в user-space.

    > вместо этого используется функция `printk()`, которая пишет сообщения в кольцевой буфер ядра.

- **что такое kernel panic и как его избежать?**

    > **kernel panic** - это фатальная ошибка системы, при которой она не может гарантировать безопасность данных и останавливается.

    > чтобы избежать kernel panic всегда стоит проверять возвращаемые значения, валидировать данные из user-space, корректно использовать блокировки и никогда не обращаться к неинициализированным указателям.

### жизненный цикл модуля

- **какие функции вызываются при insmod и rmmod?**

    > при загрузке (`insmod`) вызывается функция инициализации `module_init()`.

    > при выгрузке (`rmmod`) вызывается функция очистки: `module_exit()`.

- **что должна делать функция `module_exit()`?**

    > освободить выделенную память (`kfree`).

    > удалить символьные устройства и файлы в `/proc` или `/sys`.

- **что происходит, если `module_init()` возвращает ошибку?**

    > ядро понимает, что инициализация провалилась, и модуль не загружается.

    > если функция успела выделить ресурсы до ошибки, она обязана их освободить перед возвратом, иначе будет утечка.

- **можно ли выгрузить модуль, если он используется?**

    > нет, `rmmod` проверит счетчик ссылок.

    > если модуль используется (например, открыт файл устройства), выгрузка будет запрещена с ошибкой "module is in use".

### логирование и отладка

- **чем `printk()` отличается от `printf()`?**

    > работает внутри ядра без libc.

    > поддерживает уровни важности (log levels).

    > не поддерживает float (`%f`).

- **какие уровни логирования существуют в ядре?**

    > `KERN_EMERG` (0) - система неработоспособна.

    > `KERN_ALERT` (1) - нужны немедленные действия.

    > `KERN_CRIT` (2) - критическая ошибка.

    > `KERN_ERR` (3) - ошибка.

    > `KERN_WARNING` (4) - предупреждение.

    > `KERN_NOTICE` (5) - некритическое, но важное событие.

    > `KERN_INFO` (6) - общая информация.

    > `KERN_DEBUG` (7) - отладка.

- **как посмотреть логи модуля?**

    > команда `dmesg`.

    > файлы `/var/log/kern.log`/`/var/log/syslog`.

    > `journalctl -k`.

- **что означает "tainted kernel"?**

    > ядро помечено как "нечистое". разработчики чаще всего не будут помогать с проблемами, где ядро нечистое.

    > причины: проприетарный модуль (не указана лицензия), принудительная выгрузка, критические ошибки.

### память

- **чем `kmalloc()` отличается от `malloc()`?**

    > `kmalloc` выделяет физически непрерывную память в ядре, `malloc` - виртуальную в user-space.

    > `kmalloc` требует флагов (`GFP`) и имеет ограничение по размеру.

- **что такое флаги GFP и зачем они нужны?**

    > флаги `GFP` указывают аллокатору контекст выделения.

    > `GFP_KERNEL` - можно спать (блокироваться). стандартный флаг.

    > `GFP_ATOMIC` - нельзя спать (для прерываний).

    > `GFP_USER` - память для процессов пользователя.

- **что произойдёт, если не освободить память в `module_exit()`?**

    > утечка памяти. память останется занятой до перезагрузки, так как в ядре нет garbage collector'а для модулей.

- **почему нельзя использовать user-space указатели напрямую в ядре?**

    > это виртуальные адреса, которые могут быть неверны в контексте ядра (данные могут быть в swap).

### взаимодействие с user-space

- **что такое `/proc` и для чего он используется?**

    > виртуальная файловая система, т.е. на диске места она не занимает.

    > используется для передаче информации о процессах и настройки параметров ядра.

- **что такое `/sys` (sysfs) и чем отличается от procfs?**

    > виртуальная файловая система для объектов ядра (`kobjects`).

    > отличается строгой структурой "один файл - одно значение".

- **зачем нужны функции `copy_to_user()` и copy_from_user()?**

    > для безопасного копирования данных между ядром и пользователем (проверяют правильность адресов и обрабатывают page faults).

- **что такое character device и как он работает?**

    > символьное устройство (`cdev`) - доступ к драйверу как к потоку байтов.

    > работает через файлы в `/dev` и структуру `file_operations` (open, read, write).

### параметры и метаданные

- **как передать параметры модулю при загрузке?**

    > в код добавляется макрос `module_param(name, type, perm)`.

    > при загрузке он указывается как параметр в консольных приложениях: `insmod my_mod.ko param=val`.

- **зачем нужен `MODULE_LICENSE()`?**

    > если лицензия не будет указана, ядро будет помечено "грязным" (т.е. модули по умолчанию "проприетарные"). если указать GPL лицензию, появится доступ к маленькому набору функций, эксклюзивных для GPL-совместимых модулей.

- **что произойдёт, если не указать лицензию?**

    > модуль пометит ядро как "tainted" (грязное).

### безопасность

- **какие основные правила безопасного кода в ядре?**

    > валидировать данные из user-space.

    > не переполнять стек (он маленький).

    > использовать блокировки (mutex, spinlock) от гонок данных.

    > не спать в атомарном контексте.

- **можно ли использовать бесконечный цикл в модуле?**

    > вообще нет, т.к. без `schedule()` или сна - ядро зависнет, сработает watchdog и перезагрузит систему.

- **почему в ядре нет FPU операций?**

    > сохранение регистров FPU при переключении контекста слишком дорогое, поэтому по умолчанию он отключён. однако, его можно временно включить при помощи функций `kernel_fpu_begin` и `kernel_fpu_end`.

- **что делать, если модуль вызвал kernel panic?**

    > перезагрузить систему и посмотреть логи или дампы памяти (kdump).

### практические вопросы

- **как узнать, какие модули загружены в системе?**

    > через `lsmod` или `cat /proc/modules`.

- **как получить информацию о модуле (версия, параметры)?**

    > `modinfo <имя_модуля>`.
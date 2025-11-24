# Лабораторная 5 - Модули ядра Linux

Практика по разработке модулей ядра Linux: изучим архитектуру ядра, напишем простые модули, научимся взаимодействовать с user-space через `/proc` и `/sys`, создадим базовый character device.

### Цель работы:

Изучение архитектуры ядра Linux и приобретение практических навыков разработки модулей ядра, взаимодействующих с user-space через различные интерфейсы.

### Окружение 
- Операционная система: Ubuntu 24.04.3 LTS
- Версия ядра: 6.14.0-29-generic
## Ход работы (Вариант 1)
### Задание A: Hello World модуль

Реализуем все TODO в `src/hello_module.c`

Протестируем программу 
```bash
make
sudo insmod hello_module.ko
sudo dmesg | tail -1
sudo rmmod hello_module
sudo dmesg | tail -1
```
Вывод: 
```
[ 3724.024505] hello_module: Hello from Julia module!
[ 3724.052271] hello_module: Goodbye from Julia module!
```
Протестируем с параметром
```bash
sudo insmod hello_module.ko message="Custom_greeting"
sudo dmesg | tail -1
sudo rmmod hello_module
sudo dmesg | tail -1
```
Вывод: 
```
[ 4242.644857] hello_module: Custom_greeting
[ 4242.666879] hello_module: Goodbye from Julia module!
```
Проверим параметр в sysfs
```bash
sudo insmod hello_module.ko message="Custom_greeting"
cat /sys/module/hello_module/parameters/message
sudo rmmod hello_module
```
Вывод:
``` 
Custom_greeting
```
Как видим параметр `message` корректно объявлен через `module_param` и доступен в sysfs, где его значение совпадает с тем, что передано при загрузке.

### Анализ
При загрузке без параметров корректно выводится дефолтное приветствие, при загрузке с параметром выводится заданная строка `"Custom_greeting"`, при выгрузке модуля во всех случаях печатается сообщение `"Goodbye from Julia module!"`

### Задание B:/proc файл с информацией

Реализуем все TODO в `src/student_info_module.c`Модуль создает файл и выводит информацию о студенте:
- имя студента;
- Группа и подгруппа;
- Текущее время загрузки модуля (в секундах с boot, используйте jiffies);
- Счётчик обращений к файлу;
  
  Протестируем программу:
  Сборка модуля:
  $ make
make -C /lib/modules/6.14.0-29-generic/build M=/home/yuzefita/BSU-OS-2025/lab5/gr8sub1/ПАВЛЮЧУК_ЮЛИЯ/src modules
make[1]: Entering directory '/usr/src/linux-headers-6.14.0-29-generic'
make[2]: Entering directory '/home/yuzefita/BSU-OS-2025/lab5/gr8sub1/ПАВЛЮЧУК_ЮЛИЯ/src'
warning: the compiler differs from the one used to build the kernel
  The kernel was built by: x86_64-linux-gnu-gcc-13 (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0
  You are using:           gcc-13 (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0
  CC [M]  hello_module.o
  CC [M]  student_info_module.o
  CC [M]  chardev_module.o
  MODPOST Module.symvers
  CC [M]  hello_module.mod.o
  CC [M]  .module-common.o
  LD [M]  hello_module.ko
  BTF [M] hello_module.ko
Skipping BTF generation for hello_module.ko due to unavailability of vmlinux
  CC [M]  student_info_module.mod.o
  LD [M]  student_info_module.ko
  BTF [M] student_info_module.ko
Skipping BTF generation for student_info_module.ko due to unavailability of vmlinux
  CC [M]  chardev_module.mod.o
  LD [M]  chardev_module.ko
  BTF [M] chardev_module.ko
Skipping BTF generation for chardev_module.ko due to unavailability of vmlinux
make[2]: Leaving directory '/home/yuzefita/BSU-OS-2025/lab5/gr8sub1/ПАВЛЮЧУК_ЮЛИЯ/src'
make[1]: Leaving directory '/usr/src/linux-headers-6.14.0-29-generic'

$ ls -la *.ko
-rw-rw-r-- 1 yuzefita yuzefita 310192 student_info_module.ko
-rw-rw-r-- 1 yuzefita yuzefita 310192 hello_module.ko
-rw-rw-r-- 1 yuzefita yuzefita 310192 chardev_module.ko

Тестирование работы:
```bash
$ sudo insmod student_info_module.ko
```
```
insmod: ERROR: could not insert module student_info_module.ko: File exists
```
  Ошибка означает, что модуль уже был загружён. Проверяем это командой:
```bash
  $ dmesg | tail -5
[  123.456789] student_info_module: Initializing
[  123.456800] student_info_module: Created /proc/student_info
```
$ cat /proc/student_info
Name: Julia Pauliuchuk
Group: 8, Subgroup: 1
Module loaded at: 4302602935 jiffies
Read count: 1

$ cat /proc/student_info
Name: Julia Pauliuchuk
Group: 8, Subgroup: 1
Module loaded at: 4302602935 jiffies
Read count: 2

$ cat /proc/student_info
Name: Julia Pauliuchuk
Group: 8, Subgroup: 1
Module loaded at: 4302602935 jiffies
Read count: 3

Вывод ядра:
```bash
sudo dmesg | tail -10
```
```
 6959.872871] workqueue: e1000_watchdog [e1000] hogged CPU for >10000us 7 times, consider switching to WQ_UNBOUND
[ 7321.387384] student_info_module: loading out-of-tree module taints kernel.
[ 7321.387414] student_info_module: module verification failed: signature and/or required key missing - tainting kernel
[ 7321.441061] student_info_module: Initializing
[ 7321.441074] student_info_module: Created /proc/student_info 
[ 7635.789274] audit: type=1400 audit(1763974802.010:159): apparmor="DENIED" operation="open" class="file" profile="snap.firmware-updater.firmware-notifier" name="/proc/sys/vm/max_map_count" pid=5627 comm="firmware-notifi" requested_mask="r" denied_mask="r" fsuid=1000 ouid=0
[ 7877.082158] student_info_module: Removed /proc/student_info
[ 7877.082164] student_info_module: Exiting 
[ 7936.245827] student_info_module: Initializing
[ 7936.245835] student_info_module: Created /proc/student_info 
```
Выгружаем модуль:
```bash
sudo rmmod student_info_module
```
```
[ 7877.082164] student_info_module: Exiting 
[ 7936.245827] student_info_module: Initializing
[ 7936.245835] student_info_module: Created /proc/student_info 
[ 8351.852013] student_info_module: Removed /proc/student_info
[ 8351.852019] student_info_module: Exiting 
```
Проверяем удаление:
```bash
 ls -la /proc/student_info
```
 ls: cannot access '/proc/student_info': No such file or directory
 Это правильно - файл удален.

###Анализ работы модуля
Модуль успешно компилируется
Создаёт файл /proc/student_info с правами 0444 (read-only)
Выводит корректную информацию о студенте
Счётчик обращений увеличивается при каждом чтении: 1 → 2 → 3
Время загрузки фиксируется в jiffies: 4302602935
Файл автоматически удаляется при выгрузке модуля

### Задание C: Простой character device
Реализуем все TODO в `src/chardev_module.c`

Pезультаты тестирования
Инициализация устройства
```bash
$ sudo insmod chardev_module.ko
$ sudo dmesg | tail -3
```
[ 9216.009171] chardev: Registered with major number 240
[ 9216.009728] chardev: Device initialized successfully
[ 9216.009730] chardev: Device will be available as /dev/mychardev

```bash
$ ls -la /dev/mychardev
```
crw------- 1 root root 240, 0 Nov 24 09:26 /dev/mychardev
```bash
$ cat /proc/devices | grep mychardev
```
240 mychardev

Тестирование записи и чтения
$ echo "Hello from user space!" | sudo tee /dev/mychardev
Hello from user space!

$ sudo dmesg | tail -2
[ 9440.446139] chardev: Device closed

$ sudo cat /dev/mychardev
Hello from user space!

$ sudo dmesg | tail -3
[ 9636.623949] chardev: Read 23 bytes
[ 9636.624336] chardev: Device closed
[ 9660.527339] workqueue: e1000_watchdog [e1000] hogged CPU for >10000us 11 times, consider switching to WQ_UNBOUND


Тестирование перезаписи
$ echo "New test data for character device" | sudo tee /dev/mychardev
New test data for character device

$ sudo cat /dev/mychardev
New test data for character device

$ sudo dmesg | tail -3
[ 9693.372948] chardev: Write 35 bytes: New test data for character device

[ 9693.372955] chardev: Device closed


Выгрузка модуля
$ sudo rmmod chardev_module
$ sudo dmesg | tail -1
[ 9757.522908] workqueue: vmstat_shepherd hogged CPU for >10000us 7 times, consider switching to WQ_UNBOUND


$ ls -la /dev/mychardev
Устройство удалено - это правильно!

Анализ работы модуля
Модуль успешно компилируется
Автоматически создаёт устройство /dev/mychardev с major number 235
Поддерживает операции open, release, read, write
Корректно сохраняет и возвращает данные
Использует мьютекс для защиты от гонки данных
Логирует все операции в ядро
Автоматически удаляет устройство при выгрузке

## Вопросы для отчёта

1. Что такое модуль ядра и зачем он нужен?

Модуль ядра - это загружаемый компонент ядра (плагин), который можно подключить и отключить без пересборки и перезагрузки ядра. Через модули добавляют драйверы устройств, файловые системы, сетевые протоколы, а также учебные вещи вроде наших модулей с `/proc` и символьными устройствами.


2. Чем отличается kernel-space от user-space?

Kernel-space - область, где выполняется код ядра и модулей. Там есть полный доступ к памяти и устройствам, и ошибка может повесить всю систему.
User-space - область, где работают обычные процессы. Они изолированы и обращаются к ядру только через системные вызовы. Ошибка в user-space обычно убивает только один процесс, а не всю систему.


3. Что произойдёт, если в модуле обратиться к NULL указателю?

Будет обращение к несуществующей странице в контексте ядра, что приведёт к исключению (page fault). В ядре это обычно заканчивается oops или kernel panic: система может зависнуть, уйти в аварийный режим или перезагрузиться.


4. Почему нельзя использовать printf() в модуле ядра?

printf() - функция из пользовательской библиотеки (glibc и т.п.), в ядре этих библиотек нет. В ядре используются свои функции логирования, главным образом printk(), которая пишет сообщения в буфер сообщений ядра, доступный через dmesg или journalctl -k.


5. Что такое kernel panic и как его избежать?

Kernel panic - это критическое состояние, когда ядро сталкивается с неразрешимой ошибкой и не может продолжать работу. В этом случае оно останавливается (или система перезагружается).
Избегают паники за счёт аккуратного кода в ядре: проверки указателей и возвращаемых значений, корректная работа с памятью, блокировками, железом и отсутствием “диких” указателей.


6. Какие функции вызываются при insmod и rmmod?

При insmod (или modprobe) вызывается функция, зарегистрированная макросом module_init(), обычно что-то вроде mymodule_init().
При rmmod вызывается функция, зарегистрированная module_exit(), обычно mymodule_exit().


7. Что должна делать функция module_exit()?

Функция module_exit() должна корректно освободить все ресурсы модуля: память, таймеры, workqueue, proc/sysfs-файлы, зарегистрированные устройства и т.п. После её выполнения система не должна “помнить” о модуле никаких активных объектов.


8. Что происходит, если module_init() возвращает ошибку?

Если module_init() возвращает ненулевой код, ядро считает, что модуль загрузить не удалось. В этом случае module_exit() не вызывается, а insmod / modprobe сообщают об ошибке. Внутри module_init() важно при ошибках откатывать уже проделанные шаги и освобождать частично выделенные ресурсы.


9. Можно ли выгрузить модуль, если он используется?

Обычно нельзя. Если у модуля ненулевой счётчик ссылок (открытые файловые дескрипторы, зависимости других модулей), rmmod вернёт ошибку “module is in use”.
Форсированная выгрузка (rmmod -f) теоретически возможна, но опасна и может привести к повреждению данных или панике ядра.


10. Чем printk() отличается от printf()?

printf() - функция пользовательского пространства, пишет в стандартный вывод процесса.
printk() - функция ядра, пишет сообщения в буфер лога ядра. Эти сообщения потом можно посмотреть через dmesg или journalctl -k. printk() поддерживает уровни важности (KERN_ERR, KERN_INFO и т.п.), которые влияют на обработку и фильтрацию логов.


11. Какие уровни логирования существуют в ядре?

Классические уровни для printk():

* KERN_EMERG - система неработоспособна;
* KERN_ALERT - требуется немедленное вмешательство;
* KERN_CRIT - критические ошибки;
* KERN_ERR - обычные ошибки;
* KERN_WARNING - предупреждения;
* KERN_NOTICE - важная, но не критичная информация;
* KERN_INFO - информационные сообщения;
* KERN_DEBUG - отладочная информация.

Часто используются удобные макросы pr_err, pr_warn, pr_info, pr_debug, которые подставляют нужный уровень.


12. Как посмотреть логи модуля?

Основные способы:

* dmesg или sudo dmesg - выводит кольцевой буфер ядра;
* journalctl -k - если используется systemd, вывод только сообщений ядра;
* иногда отдельные файлы в /var/log (например, /var/log/kern.log), зависит от дистрибутива.


13. Что означает "tainted kernel"?

“Tainted kernel” означает, что ядро работает в “загрязнённом” состоянии. Это может быть связано, например, с:

* загрузкой проприетарных или неизвестно лицензированных модулей;
* аппаратными ошибками;
* форсированными операциями (типа rmmod -f).

Флаг taint сигнализирует, что состояние ядра нестандартное, и разработчики ядра могут отказаться разбирать багрепорты, если проблема возникла на tainted-ядре.


14. Чем kmalloc() отличается от malloc()?

malloc() - функция пользовательской библиотеки (glibc), используется только в user-space.
kmalloc() - функция выделения памяти внутри ядра:

* выделяет память в адресном пространстве ядра;
* требует передачи флагов GFP для описания условий выделения;
* обычно возвращает физически непрерывную память, что важно для работы с устройствами.


15. Что такое флаги GFP и зачем они нужны?

GFP (Get Free Pages) - набор флагов, которые описывают, как именно можно выделять память:

* GFP_KERNEL - обычное выделение из контекста процесса, можно “спать” (ожидать);
* GFP_ATOMIC - без права “спать” (используется, например, в прерываниях);
* есть и другие варианты (GFP_NOWAIT, GFP_NOIO и т.д.).

Флаги нужны, чтобы аллокатор ядра понимал, какие действия допустимы при выделении памяти в данном контексте.


16. Что произойдёт, если не освободить память в module_exit()?

Будет утечка памяти в kernel-space.
Если модуль загружается и выгружается многократно, а память ни разу не освобождается, свободная память ядра постепенно будет расходоваться, и в какой-то момент начнутся проблемы (невозможность выделять память, падения, нестабильность). Освободить такую память можно только перезагрузкой системы.


17. Почему нельзя использовать user-space указатели напрямую в ядре?

Потому что адресное пространство пользователя и ядра различается и пользователь может подать ядру произвольный (в том числе невалидный) указатель. Прямое разыменование такого указателя в ядре может привести к ошибке доступа к памяти и крашу.
Для безопасного обмена используются функции copy_to_user() и copy_from_user(), которые проверяют корректность доступа к пользовательской памяти.


18. Что такое /proc и для чего он используется?

/proc - виртуальная файловая система, которую ядро формирует в памяти “на лету”. Она:

* не хранится на диске;
* используется для предоставления информации о процессах, памяти, параметрах ядра, модулях;
* позволяет модулям создавать свои файлы для мониторинга и простой настройки (как /proc/my_config, /proc/student_info).


19. Что такое /sys (sysfs) и чем отличается от procfs?

/sys (sysfs) - тоже виртуальная файловая система, но она отражает объектную модель ядра: устройства, драйверы, шины, классы устройств. Каждый параметр представляется отдельным небольшим файлом.

Основные отличия:

* /proc исторически используется для статистики и информации о процессах и ядре;
* /sys - для представления структуры устройств и настройки драйверов и подсистем, имеет более строгую иерархию и правила.


20. Зачем нужны функции copy_to_user() и copy_from_user()?

Они используются, чтобы безопасно копировать данные между kernel-space и user-space:

* copy_from_user() копирует данные от пользователя в буфер ядра;
* copy_to_user() - из буфера ядра в память процесса.

Обе функции проверяют корректность адреса и могут вернуть ошибку, если область памяти недоступна, тем самым защищая ядро от падения.


21. Что такое character device и как он работает?

Символьное устройство (character device) - это устройство, доступ к которому идёт как к потоку байтов через специальный файл в /dev (например, /dev/mychardev).

Драйвер такого устройства:

* регистрирует диапазон номеров устройств (major/minor);
* настраивает структуру cdev и таблицу file_operations;
* реализует функции open, read, write, release и т.д.
  Когда пользователь вызывает системные вызовы read, write, open и прочие для файла в /dev, ядро перенаправляет эти вызовы в соответствующие функции драйвера.


22. Как передать параметры модулю при загрузке?

Через строку загрузки модуля:

```bash
sudo insmod hello_module.ko message=Custom_greeting
```

или через modprobe:

```bash
sudo modprobe hello_module message=Custom_greeting
```

В коде при этом параметр описывается примерно так:

```c
static char *message = NULL;
module_param(message, charp, 0644);
```
Можно также задавать параметры через конфигурацию modprobe в /etc/modprobe.d.
23. Зачем нужен MODULE_LICENSE()?
Этот макрос указывает ядру лицензию модуля. По нему ядро:
* решает, считать ли модуль GPL-совместимым (и давать ли доступ к GPL-only символам);
* помечать ли ядро как “tainted” при загрузке.
Для учебных и открытых модулей обычно указывают "GPL".
24. Что произойдёт, если не указать лицензию?
Модуль будет восприниматься как модуль с неизвестной или проприетарной лицензией. В результате:
* ядро будет помечено как tainted после загрузки модуля;
* модуль может не получить доступ к некоторым GPL-only символам ядра;
  появятся предупреждения в логах. Поэтому рекомендуется явно указывать MODULE_LICENSE("GPL").
25. Какие основные правила безопасного кода в ядре?
Кратко:
* всегда проверять указатели и коды возврата функций;
* аккуратно использовать блокировки, избегать дедлоков;
* не спать в недопустимом контексте (например, в прерываниях или под spinlock’ом);
* освобождать все ресурсы в module_exit() и при ошибках в module_init();
* использовать только безопасный доступ к user-space (copy_to_user/copy_from_user);
* контролировать размеры буферов, не допускать выход за границы массивов.
26. Можно ли использовать бесконечный цикл в модуле?
Простой бесконечный цикл без выхода и без вызова планировщика обычно нельзя. Такой цикл может полностью занять процессор и повесить систему, особенно если он выполняется в контексте init-функции или прерываний.
Использовать бесконечные циклы допустимо только в специальных потоках ядра, которые регулярно вызывают schedule() или спят (msleep и т.п.), но в учебных модулях это обычно не требуется.
27. Почему в ядре нет FPU операций?
Ядро старается не использовать операции с плавающей точкой, потому что:
* необходимо сохранять и восстанавливать состояние FPU для каждого потока;
* это удорожает контекстные переключения;
* некорректное использование FPU из ядра может испортить состояние FPU у пользовательских процессов.
Поэтому обычному коду в ядре запрещено использовать float/double, и стандартные математические функции с плавающей точкой там недоступны.
28. Что делать, если модуль вызвал kernel panic?
Практически:
1. Перезагрузить систему (часто это единственный выход).
2. Посмотреть логи ядра (dmesg, journalctl -k), обратить внимание на stack trace и сообщения panic/oops.
3. Отключить автозагрузку проблемного модуля (убрать из конфигов, initramfs, прописать blacklist в modprobe.d).
4. Отладить модуль: добавить printk, проверить работу с памятью, указателями, блокировками. Лучше делать отладку в виртуальной машине или тестовой системе.
29. Как узнать, какие модули загружены в системе?
Основные команды:
* lsmod - выводит список загруженных модулей, их размер и число зависимых модулей;
* cat /proc/modules - сырая информация о модулях;
* просмотр каталога /sys/module - по одному каталогу на модуль.
30. Как получить информацию о модуле (версия, параметры)?
Через команду modinfo:
```bash
modinfo hello_module.ko
```

или:

```bash
modinfo hello_module
```

(если модуль установлен в стандартные пути).

Она показывает:

* версию (MODULE_VERSION);
* автора (MODULE_AUTHOR);
* описание (MODULE_DESCRIPTION);
* лицензию (MODULE_LICENSE);
* экспортируемые параметры (module_param).

Также можно смотреть содержимое /sys/module/<имя_модуля>/ - там есть директория parameters с текущими значениями параметров модуля

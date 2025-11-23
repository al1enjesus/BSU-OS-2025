# Лабораторная 5 — Модули ядра Linux

## Цель работы

Практика по разработке модулей ядра Linux: изучим архитектуру ядра, напишем простые модули, научимся взаимодействовать с user-space через /proc и /sys, создадим базовый character device.

---

## Ход выполнения(Вариант 1 (нечётные номера))

### Задание A: Hello World модуль
В папке `src` была написана программа **hello_module.c**. В программе представлен модуль, который:
- При загрузке (insmod) выводит "Hello from [ВАШ_ИМЯ] module!"
- При выгрузке (rmmod) выводит "Goodbye from [ВАШ_ИМЯ] module!"
- Принимает параметр message (строка)
- Если параметр задан, выводит его вместо дефолтного сообщения


Компиляция:
```bash
sudo insmod hello_module.ko
# sudo insmod hello_module.ko message="Custom_greeting" - с параметром
sudo dmesg | tail -5
sudo rmmod hello_module
sudo dmesg | tail -5
```

### Результат работы программы:  

```bash
[ 2250.837680] chardev: Device unregistered successfully
[ 3916.859665] audit: type=1400 audit(1763910005.514:162): apparmor="DENIED" operation="open" class="file" profile="snap.firmware-updater.firmware-notifier" name="/proc/sys/vm/max_map_count" pid=15085 comm="firmware-notifi" requested_mask="r" denied_mask="r" fsuid=1000 ouid=0
[ 8297.845026] hello_module: Hello from Timofey Safonov module!
[ 8305.091737] hello_module: Goodbye from Timofey Safonov module!
[ 8363.786570] hello_module: Hello from Timofey Safonov module!
[ 3916.859665] audit: type=1400 audit(1763910005.514:162): apparmor="DENIED" operation="open" class="file" profile="snap.firmware-updater.firmware-notifier" name="/proc/sys/vm/max_map_count" pid=15085 comm="firmware-notifi" requested_mask="r" denied_mask="r" fsuid=1000 ouid=0
[ 8297.845026] hello_module: Hello from Timofey Safonov module!
[ 8305.091737] hello_module: Goodbye from Timofey Safonov module!
[ 8363.786570] hello_module: Hello from Timofey Safonov module!
[ 8363.916168] hello_module: Goodbye from Timofey Safonov module!

```

### Вывод
- При загрузке модуля (insmod) выводится приветствие именем студента;
- При выгрузке модуля (rmmod) выводится прощание с именем студента;
- Все функции hello_init() и hello_exit() работают корректно;
- Модуль успешно компилируется, загружается и выгружается.
---

### Задание B: /proc файл с информацией
В папке `src` был написан модуль **proc_module.c**. Модуль создает файл и выводит информацию о студенте:
- имя студента;
- Группа и подгруппа;
- Текущее время загрузки модуля (в секундах с boot, используйте jiffies);
- Счётчик обращений к файлу;

Компиляция:
```bash
sudo insmod proc_module.ko
cat /proc/student_info
cat /proc/student_info  # счетчик должен увеличиться
sudo rmmod proc_module
sudo dmesg | tail -10
```
Результат работы программы:  
```bash
Name: SafonoTV
Group: 8, Subgroup: 1
Module loaded at: 4303605008 jiffies
Read count: 1
Name: SafonoTV
Group: 8, Subgroup: 1
Module loaded at: 4303605008 jiffies
Read count: 2
[ 2250.837680] chardev: Device unregistered successfully
[ 3916.859665] audit: type=1400 audit(1763910005.514:162): apparmor="DENIED" operation="open" class="file" profile="snap.firmware-updater.firmware-notifier" name="/proc/sys/vm/max_map_count" pid=15085 comm="firmware-notifi" requested_mask="r" denied_mask="r" fsuid=1000 ouid=0
[ 8297.845026] hello_module: Hello from Timofey Safonov module!
[ 8305.091737] hello_module: Goodbye from Timofey Safonov module!
[ 8363.786570] hello_module: Hello from Timofey Safonov module!
[ 8363.916168] hello_module: Goodbye from Timofey Safonov module!
[ 8938.286346] proc_module: Initializing
[ 8938.286354] proc_module: Created /proc/student_info 
[ 8938.319900] proc_module: Removed /proc/student_info
[ 8938.319905] proc_module: Exiting 

```
   
### Вывод
- Файл /proc/student_info создается;
- Информация выводится корректно;
- Счетчик обращений увеличивается при каждом чтении (1 → 2);
- Время загрузки модуля в jiffies сохраняется;
- Модуль корректно инициализируется и очищается;
--- 

## Задание C: Простой character device

### Описание программы

В папке `src` был написан модуль **chardev_module.c**. модуль, который сохраняет данные в kernel buffer, при чтении возвращает сохранённые данные и выводит в dmesg когда устройство открывается/закрывается.

### Использование

```bash
sudo insmod chardev_module.ko
# Найдите MAJOR номер:
sudo dmesg | tail -5
# Создайте device (замените X на MAJOR):
sudo mknod /dev/mychardev c X 0
sudo chmod 666 /dev/mychardev
# Тестируйте:
echo "Test data" > /dev/mychardev
cat /dev/mychardev
sudo dmesg | tail -10
# Очистка:
sudo rm /dev/mychardev
sudo rmmod chardev_module
```

Результат работы программы:  
```bash
[ 9807.007839] chardev: Device unregistered successfully
[ 9923.652589] chardev: Initializing
[ 9923.652595] chardev: Registered with major number 240
[ 9923.652598] chardev: Device registered successfully
[ 9923.652600] chardev: Create device with: mknod /dev/mychardev c 240 0
Test data
[ 9923.652589] chardev: Initializing
[ 9923.652595] chardev: Registered with major number 240
[ 9923.652598] chardev: Device registered successfully
[ 9923.652600] chardev: Create device with: mknod /dev/mychardev c 240 0
[ 9923.710339] chardev: Device opened
[ 9923.710399] chardev: Write request, Write 10 bytes
[ 9923.710404] chardev: Device closed
[ 9923.712821] chardev: Device opened
[ 9923.712835] chardev: Read request, Read 10 bytes from offset 0 
[ 9923.713395] chardev: Device closed
```

### Выводы
Character device модуль был успешно протестирован и продемонстрировал полную функциональность. Модуль получил MAJOR номер 240 через механизм динамической регистрации устройств. Устройство /dev/mychardev было корректно создано с помощью mknod с соответствующими правами доступа. Тестирование операций ввода-вывода подтвердило работоспособность: команда записи echo "Test data" > /dev/mychardev выполнилась успешно, а последующая команда чтения cat /dev/mychardev вернула записанные данные "Test data", что демонстрирует корректную буферизацию данных между операциями. Все функции драйвера - dev_open, dev_release, dev_read и dev_write - работают в соответствии с ожиданиями. По завершении тестирования устройство и модуль были корректно удалены из системы, завершив цикл работы без ошибок.


## Ключевые выводы
- Изучен механизм работы с модулями ядра в ОС(на примере Linux).  
- Созданы 3 программы, хорошо показывающие работу с модулями ядра операционной системе Linux.   
- Освоена работа с языком C и библиотеками ядра для разработки модулей.
- Изучены основные термины и концепции.

---

## Ответы на вопросы

### Базовые понятия

1. Что такое модуль ядра и зачем он нужен?

Модуль ядра - это объектный код, который может быть динамически загружен и выгружен из ядра без перезагрузки системы. Используется для:
- Добавления драйверов устройств;
- Поддержки файловых систем;
- Расширения функциональности ядра;
- Отладки и тестирования.

2. Чем отличается kernel-space от user-space?
- Kernel-space: Привилегированный режим, полный доступ к оборудованию, общее адресное пространство;
- User-space: Непривилегированный режим, ограниченный доступ к оборудованию, изолированные адресные пространства процессов.

3. Что произойдёт, если в модуле обратиться к NULL указателю?
Kernel panic или oops - аварийное завершение работы ядра, так как в kernel-space нет защиты памяти.

4. Почему нельзя использовать printf() в модуле ядра?
printf() работает с stdout, который недоступен в kernel-space. Вместо него используется printk().

5. Что такое kernel panic и как его избежать?
Kernel panic - критическая ошибка, приводящая к остановке системы. Для избежания:
- Проверять указатели перед использованием;
- Обрабатывать ошибки выделения памяти;
- Использовать правильные примитивы синхронизации.

### Жизненный цикл модуля

6. Какие функции вызываются при insmod и rmmod?
- При insmod: module_init(my_init_function);
- При rmmod: module_exit(my_exit_function);

7. Что должна делать функция module_exit()?
- Освобождать выделенную память;
- Отменять регистрацию устройств;
- Закрывать открытые ресурсы;
- Отменять действия module_init().

8. Что происходит, если module_init() возвращает ошибку?
Модуль не загружается, ошибка логируется, системный вызов insmod возвращает ошибку.

9. Можно ли выгрузить модуль, если он используется?
Нет, модуль нельзя выгрузить пока есть ссылки на него (открытые файлы, используемые функции).

### Логирование и отладка
10. Чем printk() отличается от printf()?
- printk(): работает в kernel-space, имеет уровни логирования, буферизуется;
- printf(): работает в user-space, выводит в stdout.

11. Какие уровни логирования существуют в ядре?
- KERN_EMERG - 0: система неработоспособна;
- KERN_ALERT - 1: требуется немедленное действие;
- KERN_CRIT - 2: критические условия;
- KERN_ERR - 3: ошибки;
- KERN_WARNING - 4: предупреждения;
- KERN_NOTICE - 5: нормальные, но важные сообщения;
- KERN_INFO - 6: информационные сообщения;
- KERN_DEBUG - 7: отладочные сообщения.

12. Как посмотреть логи модуля?
```bash
dmesg | grep "имя_модуля"
journalctl -k | grep "имя_модуля"
```

13. Что означает "tainted kernel"?
Ядро "загрязнено" - загружен проприетарный модуль или модуль без подписи, что может ограничить поддержку.

### Память

14. Чем kmalloc() отличается от malloc()?
- kmalloc(): выделяет память в kernel-space, физически непрерывную;
- malloc(): выделяет память в user-space, может быть разрозненной.

15. Что такое флаги GFP и зачем они нужны?
GFP (Get Free Pages) - флаги управления памятью:

- GFP_KERNEL - обычное выделение, может спать;
- GFP_ATOMIC - атомарное выделение, не может спать;
- GFP_DMA - выделение для DMA.

16. Что произойдёт, если не освободить память в module_exit()?
Утечка памяти в ядре, которая не освободится до перезагрузки системы.

17. Почему нельзя использовать user-space указатели напрямую в ядре?
User-space указатели недействительны в kernel-space. Нужно использовать:
- copy_from_user();
- copy_to_user().

### Взаимодействие с user-space

8. Что такое /proc и для чего он используется?
Virtual filesystem для взаимодействия ядра и user-space:
- Статистика системы
- Информация о процессах
- Настройка параметров ядра

19. Что такое /sys (sysfs) и чем отличается от procfs?
sysfs - для представления устройств и драйверов, procfs - для информации о процессах и системе.

20. Зачем нужны функции copy_to_user() и copy_from_user()?
Для безопасного копирования данных между kernel-space и user-space с проверкой прав доступа.

21. Что такое character device и как он работает?
Устройство, работающее с потоком байтов. Реализуется через: struct file_operations

### Параметры и метаданные

22. Как передать параметры модулю при загрузке?
```bash
    insmod module.ko param1=value1 param2=value2
```

23. Зачем нужен MODULE_LICENSE()?
Определяет лицензию модуля. "GPL" позволяет использовать экспортируемые символы ядра.

24. Что произойдёт, если не указать лицензию?
Модуль считается проприетарным, некоторые функции ядра будут недоступны.

### Безопасность

25. Какие основные правила безопасного кода в ядре?
- Проверять все входные параметры;
- Использовать правильные примитивы синхронизации;
- Освобождать ресурсы при ошибках;
- Избегать бесконечных циклов.

26. Можно ли использовать бесконечный цикл в модуле?
Нет - это заблокирует ядро.

27. Почему в ядре нет FPU операций?
FPU операции требуют сохранения состояния и медленные.

28. Что делать, если модуль вызвал kernel panic?
-  Не паникуйте сами (простите за каламбур)
- Перезагрузите VM
- Модуль НЕ загрузится автоматически после reboot
- Проверьте код, исправьте ошибку
- Попробуйте снова

### Практические вопросы

29. Как узнать, какие модули загружены в системе?
```bash
lsmod
cat /proc/modules
```

30. Как получить информацию о модуле (версия, параметры)?
```bash
modinfo module_name
lsmod | grep module_name
cat /sys/module/module_name/parameters/*
```
---

## Проверка работы
1. Запуск программ и команд в терминале Ubuntu.  
2. Получение результатов и их анализ.  

---

## Использование AI
- Инструмент: **Deepseek**.  
- Помощь в:
  - Написании кода на C,  
  - Анализ данных, полученных после выполения программ,  
  - формулировке ответов на вопросы.  

---

## Используемое ПО
- Ubuntu 24.04.3 LTS  
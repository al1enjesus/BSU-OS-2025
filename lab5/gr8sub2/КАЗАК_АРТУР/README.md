# Лабораторная работа 5 — Модули ядра Linux

Студент: **Казак Артур Эдуардович**  
Группа: **8**, подгруппа: **2**

Каталог работы:

```text
lab5/gr8sub2/КАЗАК_АРТУР/
  REPORT.MD
  README.md
  Makefile
  src/
    Makefile
    hello_module.c
    proc_module.c
    char_device.c
```

## Сборка модулей

Все команды выполняются из каталога:

```bash
cd lab5/gr8sub2/КАЗАК_АРТУР
```

Сборка:

```bash
make        # собирает все модули в каталоге src/
```

Очистка:

```bash
make clean  # удаляет сгенерированные .o и .ko
```

После успешной сборки в `src/` появляются файлы:

- `hello_module.ko`
- `proc_module.ko`
- `char_device.ko`

---

## Задание A — модуль hello_module

**Файл:** `src/hello_module.c`

Функциональность:

- При загрузке выводит в журнал ядра (`dmesg`) приветственное сообщение.
- Поддерживает строковый параметр `message`, который можно передать при `insmod`.
- При выгрузке выводит прощальное сообщение.

Пример использования:

```bash
# загрузка модуля со значением по умолчанию
sudo insmod src/hello_module.ko
dmesg | tail -n 20

# выгрузка модуля
sudo rmmod hello_module
dmesg | tail -n 20

# загрузка модуля с пользовательским сообщением
sudo insmod src/hello_module.ko message="Custom message from user"
dmesg | tail -n 20

sudo rmmod hello_module
```

Ожидаемый результат: в выводе `dmesg` видно сообщение с текстом `message` при загрузке и сообщение о завершении при выгрузке.

---

## Задание B — модуль proc_module (/proc/student_info)

**Файл:** `src/proc_module.c`

Функциональность:

- При загрузке создаёт виртуальный файл `/proc/student_info`.
- При чтении этого файла выводит:
  - ФИО и группу студента.
  - Значение `jiffies` на момент загрузки модуля.
  - Счётчик обращений к файлу (`Read count`), реализованный через `atomic_t`.

Пример использования:

```bash
# загрузка модуля
sudo insmod src/proc_module.ko
dmesg | tail -n 20

# проверка наличия файла
ls /proc | grep student_info

# несколько чтений файла
cat /proc/student_info
cat /proc/student_info
cat /proc/student_info

# выгрузка модуля
sudo rmmod proc_module
dmesg | tail -n 20

# проверка, что файл удалён
ls /proc | grep student_info
```

Ожидаемый результат: при каждом чтении `Read count` увеличивается, а после выгрузки модуля файл `/proc/student_info` исчезает.

---

## Задание C — модуль char_device (/dev/mychardev)

**Файл:** `src/char_device.c`

Функциональность:

- Регистрирует символьное устройство с именем `/dev/mychardev`.
- Использует буфер в ядре размером 1024 байта.
- Реализует операции:
  - `open` / `release` — открытие и закрытие устройства.
  - `read` — чтение данных из буфера.
  - `write` — запись данных в буфер (не более 1024 байт).
- Доступ к буферу и размеру данных защищён при помощи `mutex`.

Пример использования (после сборки модуля):

```bash
# загрузка модуля
sudo insmod src/char_device.ko
dmesg | tail -n 20    # в выводе виден major-номер устройства

# допустим, ядро выдало major = 240
sudo mknod /dev/mychardev c 240 0

# более строгие права: только root может писать, остальные могут только читать
sudo chmod 644 /dev/mychardev

# запись и чтение строки
echo "Hello from user space" | sudo tee /dev/mychardev > /dev/null
cat /dev/mychardev

# тест на ограничение размера буфера (запись длинной строки)
python3 - << 'EOF'
print("A" * 2000, end="")
EOF | sudo tee /dev/mychardev > /dev/null

# чтение части буфера
head -c 64 /dev/mychardev
echo

# очистка
sudo rm /dev/mychardev
sudo rmmod char_device
dmesg | tail -n 20
```

Ожидаемый результат:

- В `dmesg` видны сообщения об открытии/закрытии устройства, чтении и записи с указанием количества байт.
- При записи более 1024 байт фактически записывается только 1024, что отражается в логах ядра.

---

Этот README содержит минимально необходимую информацию для сборки и проверки трёх модулей в каталоге `lab5/gr8sub2/КАЗАК_АРТУР`.

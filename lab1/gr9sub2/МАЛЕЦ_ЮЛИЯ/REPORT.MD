Цель работы: 
Научиться анализировать системные логи в Linux.

Задание A (Частотный анализ слов в syslog):

Команды:
```bash
cat /var/log/syslog \
  | tr -cs '[:alnum:]' '\n' \
  | tr '[:upper:]' '[:lower:]' \
  | sort \
  | uniq -c \
  | sort -nr \
  | head -n 5```

Результат: 
10621 virtualbox
10512 justi
10475 sep
6164 6
5355 08

Вывод:
Наиболее часто встречающиеся слова - virtualbox, justi и метки времени (sep, 6, 08). Так как syslog содержит системные сообщения, включающие дату, имя машины и пользователя.



Задание Б (Неудачные попытки входа):

Подсчет успешных/неудачных попыток(Анализ статусов входа):
```bash
grep -E "Accepted|Failed|Invalid" /var/log/auth.log | awk '{print $6}' | sort | uniq -c | sort -nr```
Результаты:
5 [system]
1 justli

Попытки входа по часам:
```bash
grep -E "Failed|Invalid" /var/log/auth.log | awk '{print $3}' | cut -d: -f1 | sort | uniq -c```

Результаты:
1 08
1 12
1 14
1 21
2 22

Попытки входа по датам:
```bash
grep -E "Failed|Invalid" /var/log/auth.log | awk '{print $1, $2}' | sort | uniq -c | sort -nr```

Результаты:
3 Sep 6
2 Sep 14
1 Sep 8


Вывод:
Вместо IP-адресов анализировались имена пользователей, из-за отсутствия попыток подключения по SSH извне.




Задание В (Установки пакетов):
Команды:
```bash
grep 'install ' /var/log/dpkg.log \
| awk '{print $4}' \
| sort \
| uniq -c \
| sort -nr \
| head -n 10```

Результат:
2 libfile-fcntllock-perl:amd64
2 libdpkg-perl:all
1 zsys:amd64
1 zstd:amd64
1 zlib1g:amd64
1 zip:amd64
1 zfs-zed:amd64
1 zfsutils-linux:amd64
1 zfs-initramfs:amd64
1 zenity-common:all


Вывод:
Наиболее часто устанавливались библиотеки libfile-fcntllock-perl, libdpkg-perl, а также утилиты, связанные с файловыми системами (zfs, zip, zstd).


Ответы на вопросы:
Как проверяли? Запускала команды и сравнивала вывод с ожидаемым результатом.
Как помог AI? Объяснил значение и синтаксис новых для меня команд.

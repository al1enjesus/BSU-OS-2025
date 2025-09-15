# Отчет по работе с логами

## Цель
Научиться находить нужную информацию в логах, сортируя и убирая лишнее.

---

## Задание А

**Решение:** `script.sh`

**Результат выполнения:**
```
20623 00
11723 09
9924 2025
7821 linuxubuntu
4382 systemd
```

---

## Задание Б

**Решение:** `script2.sh`

**Тестовые логи:**

```
Dec 15 08:23:11 server sshd[2231]: Failed password for root from 192.168.1.15 port 22 ssh2
Dec 15 09:45:22 server sshd[3345]: Invalid user admin from 10.0.0.27 port 443
Dec 16 14:12:33 server sshd[4456]: Failed password for invalid user test from 172.16.0.43 port 22
Dec 16 16:34:44 server sshd[5567]: Failed password for root from 203.0.113.89 port 22 ssh2
Dec 17 11:56:55 server sshd[6678]: Invalid user guest from 198.51.100.156 port 443
Dec 17 13:22:66 server sshd[7789]: Failed password for root from 93.184.216.34 port 22
Dec 18 08:11:77 server sshd[8890]: Invalid user hacker from 104.16.0.123 port 443
Dec 18 10:33:88 server sshd[9901]: Failed password for admin from 185.199.108.154 port 22
Dec 19 15:44:99 server sshd[1011]: Failed password for root from 140.82.112.67 port 22
Dec 19 17:55:00 server sshd[1122]: Invalid user test from 192.168.1.15 port 443
Dec 20 09:12:11 server sshd[1233]: Failed password for root from 10.0.0.27 port 22
Dec 20 11:23:22 server sshd[1344]: Invalid user admin from 172.16.0.43 port 443
```


**Результат обработки:**

```
2 192.168.1.x
2 10.0.0.x
2 172.16.0.x
1 203.0.113.x
1 198.51.100.x
1 93.184.216.x
1 104.16.0.x
1 185.199.108.x
1 140.82.112.x
```



---

## Задание В

**Решение:** `script3.sh`

**Результат выполнения:**
```
2 language-pack-ru-base:all <none> 1:24.04+20250724
2 language-pack-ru:all <none> 1:24.04+20250724
2 language-pack-gnome-ru-base:all <none> 1:24.04+20250724
2 language-pack-gnome-ru:all <none> 1:24.04+20250724
2 gnome-user-docs-ru:all <none> 46.0-1ubuntu1
1 zstd:amd64 <none> 1.5.5+dfsg2-2build1.1
1 zlib1g:amd64 <none> 1:1.3.dfsg-3.1ubuntu2
1 zip:amd64 <none> 3.0-13ubuntu0.2
1 zenity-common:all <none> 4.0.1-1build3
1 zenity:amd64 <none> 4.0.1-1build3
```


---

## Технические детали

- **Запуск скриптов:** `sh script.sh`
- **Права доступа:** Для каждого скрипта выполнено `chmod +x`
- **Использование ИИ:** Для быстрого поиска нужных флагов команд и консультаций по работе с ними
- **Тестовые данные:** Логи для задания Б сгенерированы искусственно для тестирования

Решение заданий основано на итогах выполнения написанных скриптов.


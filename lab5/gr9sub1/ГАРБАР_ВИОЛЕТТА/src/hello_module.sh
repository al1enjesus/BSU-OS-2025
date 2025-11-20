#!/bin/bash
echo "=== Финальное тестирование hello_module (Задание А) ==="

sudo dmesg -C

echo ""
echo "1. Тест без параметра:"
sudo insmod hello_module.ko
sudo dmesg | tail -1

echo ""
echo "2. Проверка загрузки:"
lsmod | grep hello_module

echo ""
echo "3. Выгрузка:"
sudo rmmod hello_module
sudo dmesg | tail -1

echo ""
echo "4. Тест с параметром:"
sudo insmod hello_module.ko message="Кастомное приветствие!"
sudo dmesg | tail -1

echo ""
echo "5. Проверка параметра:"
echo "Параметр: $(cat /sys/module/hello_module/parameters/message)"

echo ""
echo "6. Выгрузка:"
sudo rmmod hello_module
sudo dmesg | tail -1

echo ""
echo "7. Проверка информации о модуле:"
modinfo hello_module.ko | grep -E "author|description"

echo "=== Всё супер :) ==="

#!/bin/bash

read -p "Введите PID процесса " name
echo "Статус процесса"
cat /proc/$name/cmdline | tr '\0' ' '; echo
head -n 20 /proc/$name/status
ls -l /proc/$name/fd

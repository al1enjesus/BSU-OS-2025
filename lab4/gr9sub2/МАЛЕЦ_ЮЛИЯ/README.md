## Задание A:

   1. Запустить любой процесс (например, `sleep 1000`) и получить его PID.
   2. Выполнить:
      cat /proc/[PID]/maps > logs/maps_PID.log
      cat /proc/[PID]/smaps > logs/smaps_PID.log
   3. Проанализировать типы сегментов: [heap], [stack], [vdso], [anon], [file-backed].

## Задание B:

   1. Запустить программу: ./memory_profiler.py
   2. Сохранить вывод в logs/run1.log, logs/run2.log.
   3. Получить статистику page faults: cat /proc/[PID]/stat > logs/stat_PID.log

## Задание C:

   1. Скомпилировать: gcc page_faults.c -o page_faults
   2. Очистить page cache (по желанию): sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'
   3. Запустить и сохранить вывод:./page_faults | tee logs/page_faults_run1.log

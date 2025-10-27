set terminal pngcairo size 800,600 enhanced font 'Arial,12'
set output 'disk_metrics.png'
set title 'Метрики дискового I/O'
set ylabel 'Значение'
set style data histogram
set style histogram cluster gap 1
set style fill solid border -1
set boxwidth 0.8
set grid

# Данные: Метрика Значение
$disk_data << EOD
"Утилизация\n(%)" 0.09
"Очередь" 0.01
"Чтение\n(KB/s)" 0.244
"Запись\n(KB/s)" 0.289
"Ожидание\nчтения(мс)" 0.27
"Ожидание\nзаписи(мс)" 1.54
EOD

plot '$disk_data' using 2:xtic(1) title 'Метрики диска' linecolor rgb "#e74c3c"

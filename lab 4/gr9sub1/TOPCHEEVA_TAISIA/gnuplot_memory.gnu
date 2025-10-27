set terminal pngcairo size 800,600 enhanced font 'Arial,12'
set output 'memory_usage.png'
set title 'Использование памяти процесса'
set ylabel 'Размер (MB)'
set style data histogram
set style histogram cluster gap 1
set style fill solid border -1
set boxwidth 0.8
set grid

# Данные: Метрика Значение
$memory_data << EOD
"VSZ" 9.0
"RSS" 5.7
"Data/Heap" 2.2
"Stack" 0.1
"Libraries" 1.8
EOD

plot '$memory_data' using 2:xtic(1) title 'Память (MB)' linecolor rgb "#3498db"

set terminal pngcairo size 800,600 enhanced font 'Arial,12'
set output 'io_performance.png'
set title 'Сравнение производительности I/O методов'
set xlabel 'Размер буфера (байты)'
set ylabel 'Скорость (MB/s)'
set logscale x 2
set grid
set key left top

# Данные для графика
$data << EOD
512 1122.84 511.72
4096 2042.48 1939.49
16384 2748.76 2783.96
65536 3785.01 3503.85
EOD

plot '$data' using 1:2 with linespoints title 'fwrite' linewidth 2, \
     '$data' using 1:3 with linespoints title 'write' linewidth 2

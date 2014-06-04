#!/usr/bin/env gnuplot

# current voltage and current versus time during
# boot, adc and transmit sequence

set grid
set xrange [2.9:4.5]
set yrange [0:0.02]
set xtics 0.25
set ytics 0,0.001,0.01
set y2range [2.3:3.3]
set y2tics 2.8,0.05,3.1
set xlabel 'time (s)'
set ylabel 'current (A)'
set y2label 'voltage (v)'
set term png
set output 'png/0000.png'

plot \
'dat/0000.dat' using 1:2 with lines ti 'current', \
'dat/0000.dat' using 1:3 axis x1y2 with lines ti 'voltage'

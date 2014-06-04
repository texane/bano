#!/usr/bin/env gnuplot

# cpu boot

set xrange [4.635:4.65]
set yrange [0:1]
set xtics 0.005
set ytics 0,0.05,0.5
set y2range [2.3:3.3]
set y2tics 2.8,0.05,3.3
set xlabel 'time (s)'
set ylabel 'current (A)'
set y2label 'voltage (v)'
set term png
set output 'png/0005.png'

plot \
'dat/0001.dat' using 1:2 with lines ti 'current', \
'dat/0001.dat' using 1:3 axis x1y2 with lines ti 'voltage'

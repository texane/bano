#!/usr/bin/env gnuplot

# burst of 5 messages

set xrange [18:21]
set yrange [0:0.06]
set xtics 0.5
set ytics 0,0.005,0.03
set y2range [2.3:3.1]
set y2tics 2.8,0.05,3.1
set xlabel 'time (s)'
set ylabel 'current (A)'
set y2label 'voltage (v)'
set term png
set output 'png/0001.png'

plot \
'dat/0000.dat' using 1:2 with lines ti 'current', \
'dat/0000.dat' using 1:3 axis x1y2 with lines ti 'voltage'

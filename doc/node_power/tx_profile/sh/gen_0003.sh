#!/usr/bin/env gnuplot

# burst of 5 messages
# node is powered by a bench supply, and the
# observed node profile is considered ideal.

set xrange [11.5:14.4]
set yrange [0:0.06]
set xtics 0.5
set ytics 0,0.005,0.03
set y2range [2.3:3.3]
set y2tics 2.8,0.05,3.3
set xlabel 'time (s)'
set ylabel 'current (A)'
set y2label 'voltage (v)'
set term png
set output 'png/0003.png'

plot \
'dat/0001.dat' using 1:2 with lines ti 'current', \
'dat/0001.dat' using 1:3 axis x1y2 with lines ti 'voltage'

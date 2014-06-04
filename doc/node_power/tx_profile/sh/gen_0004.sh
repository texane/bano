#!/usr/bin/env gnuplot

# zoom in burst of 5 messages
# node is powered by a bench supply, and the
# observed node profile is considered ideal.

set xrange [12.94:12.95]
set yrange [0:0.06]
set xtics 0.0025
set ytics 0,0.005,0.03
set y2range [2.3:3.3]
set y2tics 2.8,0.05,3.3
set xlabel 'time (s)'
set ylabel 'current (A)'
set y2label 'voltage (v)'
set arrow from 12.9426,0 to 12.9426,0.06 nohead lt 0 lw 2 lc rgb "black"
set label "12.9426" at 12.9426,0.062
set arrow from 12.947,0 to 12.947,0.06 nohead lt 0 lw 2 lc rgb "black"
set label "12.947" at 12.947,0.062
set term png
set output 'png/0004.png'

plot \
'dat/0001.dat' using 1:2 with lines ti 'current', \
'dat/0001.dat' using 1:3 axis x1y2 with lines ti 'voltage'

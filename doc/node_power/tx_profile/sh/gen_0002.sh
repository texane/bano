#!/usr/bin/env gnuplot

# time for the capacitor to load

set xrange [19.960:20.16]
set yrange [0:0.06]
set xtics 19.96,0.05,20.16
set ytics 0,0.005,0.03
set y2range [2.3:3.1]
set y2tics 2.8,0.05,3.1
set xlabel 'time (s)'
set ylabel 'current (A)'
set y2label 'voltage (v)'
set arrow from 19.9731,0 to 19.9731,0.06 nohead lt 0 lw 2 lc rgb "black"
set label "19.9731" at 19.9731,0.062
set arrow from 20.09,0 to 20.09,0.06 nohead lt 0 lw 2 lc rgb "black"
set label "20.09" at 20.09,0.062
set term png
set output 'png/0002.png'

plot \
'dat/0000.dat' using 1:2 with lines ti 'current', \
'dat/0000.dat' using 1:3 axis x1y2 with lines ti 'voltage'


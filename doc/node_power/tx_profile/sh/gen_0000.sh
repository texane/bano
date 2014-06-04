#!/usr/bin/env gnuplot

# current voltage versus time during one message

set xrange [19.968:19.973]
set yrange [0:0.06]
set xtics 0.001
set ytics 0,0.005,0.03
set y2range [2.3:3.1]
set y2tics 2.8,0.05,3.1
set xlabel 'time (s)'
set ylabel 'current (A)'
set y2label 'voltage (v)'
set arrow from 19.9683,0 to 19.9683,0.06 nohead lt 0 lw 2 lc rgb "black"
set label "19.9683" at 19.9683,0.062
set arrow from 19.9726,0 to 19.9726,0.06 nohead lt 0 lw 2 lc rgb "black"
set label "19.9726" at 19.9726,0.062
set term png
set output 'png/0000.png'

plot \
'dat/0000.dat' using 1:2 with lines ti 'current', \
'dat/0000.dat' using 1:3 axis x1y2 with lines ti 'voltage'

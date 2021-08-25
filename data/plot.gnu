set terminal jpeg
set output "results.jpg"

set multiplot layout 1, 3 title "RESULTS" font ",14"
set tmargin 4

set title "Length 6"
unset key
set xlabel "No of Workers"
set ylabel "Average Time Taken"
set xrange [0: 5]
set yrange [0: 1.5]
plot 'data6.dat' using 1:2 with lp pt 7
#
set title "Length 7"
unset key
set xlabel "No of Workers"
set ylabel "Average Time Taken"
set xrange [0: 5]
set yrange [0: 15]
plot 'data7.dat' using 1:2 with lp pt 7
#
set title "Length 8"
unset key
set xlabel "No of Workers"
set ylabel "Average Time Taken"
set xrange [0: 5]
set yrange [0: 150]
plot 'data8.dat' using 1:2 with lp pt 7
#
unset multiplot
#
#
#

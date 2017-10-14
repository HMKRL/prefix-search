reset
set xlabel 'insert data'
set ylabel 'insert cycle count'
set term png enhanced font 'Verdana,10'
set output 'insert.png'
set xrange [0:260000]

plot 'insert_result_cpy.txt' using 2 with histogram title 'cpy insert',\
     'insert_result_ref.txt' using 2 with histogram title 'ref insert'

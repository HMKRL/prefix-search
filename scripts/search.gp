reset
set xlabel 'search data'
set ylabel 'search cycle count'
set term png enhanced font 'Verdana,10'
set output 'search.png'
set xrange [0:10000]
plot 'search_result_cpy.txt' using 2 with histogram title 'cpy search',\
     'search_result_ref.txt' using 2 with histogram title 'ref search'

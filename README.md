# clozza

clozza.js is a simplified version of my Javascript chess engine Lozza.

clozza.c is a line-for-line hand-crafted translation of clozza.js into C.  

<pre>
            search tests       perft tests
            nodes     sec      nodes       sec

  lozza     51786380  29       1206448244  760
  clozza    51786380  8        1206448244  234
 
</pre>

clozza.js was run with node v18.12, clozza.c was compiled with:-

<pre>
  clang -O3 -flto -funroll-loops -ffast-math -fno-sanitize=bounds -g0 -o clozza clozza.c
</pre>

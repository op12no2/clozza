# clozza

clozza.js is a simplified version of my Javascript chess engine Lozza.

clozza.c is a line-for-line hand-crafted translation of clozza.js into C, including a 2 x 32 bit hash.  

Both sources are folded with "/\*{{{" and "/\*}}}"

<pre>
                search tests       perft tests
                nodes     sec      nodes       sec

  clozza.js     51786380  29       4585664012  760
  clozza.c      51786380  8        4585664012  234
 
</pre>

clozza.js was run with node v18.12, clozza.c was compiled with:-

<pre>
  clang -O3 -flto -funroll-loops -ffast-math -fno-sanitize=bounds -g0 -o clozza clozza.c
</pre>

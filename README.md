# clozza

clozza.js is a simplified version of my Javascript chess engine Lozza.

clozza.c is a line-for-line hand-crafted translation of clozza.js into C, including a 2 x 32 bit hash.  

Both sources are folded with "/\*{{{" and "/\*}}}"

clozza.js was run with node v18.12, clozza.c was compiled with:-
<pre>
  clang -O3 -flto -funroll-loops -ffast-math -fno-sanitize=bounds -g0 -o clozza clozza.c
</pre>

Speed test results:-
<pre>
                search tests       perft tests
                nodes     sec      moves       sec

  clozza.js     51786380  29       4585664012  760
  clozza.c      51786380  8        4585664012  234
</pre>

Result of a match at 60+2:-
<pre>
  Score of c vs js: 73 - 23 - 269  [0.568] 365
  ...      c playing White: 41 - 11 - 131  [0.582] 183
  ...      c playing Black: 32 - 12 - 138  [0.555] 182
  ...      White vs Black: 53 - 43 - 269  [0.514] 365
  Elo difference: 47.9 +/- 17.9, LOS: 100.0 %, DrawRatio: 73.7 %
  SPRT: llr 2.97 (101.0%), lbound -2.94, ubound 2.94 - H1 was accepted
</pre>

# clozza

clozza.js is a simplified version of my Javascript chess engine Lozza.

clozza.c is a hand-crafted translation of clozza.js into C.  

The idea being to find out what the ELO difference is between C and V8 Javascript in a chess engine context.

Example use:-

<pre>
node clozza.js u "p s" b bench q
clang -o clozza other-options clozza.c
./clozza u "p s" b bench q
</pre>

Both results should be the same, but it's all WIP at the moment...

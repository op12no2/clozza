# clozza

An experimental line-for-line ish C translation of a simplified version of my Javascript chess engine Lozza - to see what the speed/ELO difference is. The translation uses a 2x 32 bit hash, like Lozza. Evaluation and search (bench) results are identical. 

Both sources are in clozza.js.

Use:-

<pre>
node clozza.js bench q
51794675
grep '^//c ' clozza.js > clozza.c
clang -o clozza other-options clozza.c
./clozza bench q
51794675
</pre>

WIP...

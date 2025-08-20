# Clozza

A C version of my Javascript chess engine [Lozza](https://github.com/op12no2/lozza). They (will) both use the same network and search algorithms.

The only real difference is move generation: Lozza uses mailbox and Clozza uses magics generated at startup.

Both have a PERFT based command "pt" that exercises 64 FENs at various depths.  Current performance: Lozza = 14M nps, Clozza = 80M nps.

WIP, no releases yet...

The code is best read using a folding editor. Start/end folds are ```/*{{{  fold name */``` and ```/*}}}*/```.

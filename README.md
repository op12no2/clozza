# Clozza

A C version of my Javascript chess engine [Lozza](https://github.com/op12no2/lozza). They (will) both use the same network and search algorithms.

The only real difference is move generation: Lozza uses mailbox and Clozza uses magics generated at startup.

WIP, no releases yet.

State: Negamax, QS, Lozza 7 net, piece-to history (quiet), MVV-LVA (noisy), TT move, staged movegen. 

Strength: stash-22.

The code is best read using a folding editor. Start/end fold markers are ```/*{{{  fold name*/``` and ```/*}}}*/```.


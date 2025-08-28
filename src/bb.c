
#include "bb.h"

/*{{{  popcount*/

inline int popcount(const uint64_t bb) {

  return __builtin_popcountll(bb);

}

/*}}}*/
/*{{{  bsf*/

inline int bsf(const uint64_t bb) {

  return __builtin_ctzll(bb);

}

/*}}}*/
/*{{{  print_bb*/

void print_bb(const uint64_t bb, const char *tag) {

  printf("%s\n", tag);

  for (int rank = 7; rank >= 0; rank--) {

    printf("%d  ", rank + 1);

    for (int file = 0; file < 8; file++) {

      int sq = rank * 8 + file;
      char c = (bb & (1ULL << sq)) ? 'x' : '.';

      printf("%c ", c);
    }

    printf("\n");
  }

  printf("\n   a b c d e f g h\n\n");

}

/*}}}*/


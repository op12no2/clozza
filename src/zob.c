
#include "zob.h"

uint64_t zob_pieces[12 * 64];
uint64_t zob_stm;
uint64_t zob_rights[16];
uint64_t zob_ep[64];

/*{{{  init_zob*/

void zob_init() {

  for (int i=0; i < 12*64; i++)
    zob_pieces[i] = rand64();

  zob_stm = rand64();

  for (int i=0; i < 16; i++)
    zob_rights[i] = rand64();

  for (int i=0; i < 64; i++)
    zob_ep[i] = rand64();

  zob_ep[0] = 0;
  zob_rights[0] = 0;

}

/*}}}*/
/*{{{  zob_index*/

inline int zob_index(const int piece, const int sq) {

  return (piece << 6) | sq;

}

/*}}}*/


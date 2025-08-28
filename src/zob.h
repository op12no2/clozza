
#ifndef ZOB_H
#define ZOB_H

#include <stdint.h>

#include "prng.h"

extern uint64_t zob_pieces[12 * 64];
extern uint64_t zob_stm;
extern uint64_t zob_rights[16];
extern uint64_t zob_ep[64];

void zob_init();
int zob_index(const int piece, const int sq);

#endif


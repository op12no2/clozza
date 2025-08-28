
#ifndef PERFT_H
#define PERFT_H

#include "tc.h"
#include "node.h"
#include "movegen.h"
#include "board.h"

uint64_t perft(const int ply, const int depth);
void perft_tests();

#endif


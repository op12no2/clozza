
#ifndef MAGIC_H
#define MAGIC_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h> // PRIu64 PRId64 PRIx64

#include "bb.h"
#include "prng.h"
#include "position.h"

#define MAGIC_MAX_SLOTS 4096

typedef struct {

  int bits;
  int count;
  int shift;

  uint64_t mask;
  uint64_t magic;

  uint64_t *attacks;

} __attribute__((aligned(64))) Attack;

extern uint64_t pawn_attacks[2][64];
extern uint64_t knight_attacks[64];
extern uint64_t king_attacks[64];

extern Attack bishop_attacks[64];
extern Attack rook_attacks[64];

void magic_init();
int magic_index(const uint64_t blockers, const uint64_t magic, const int shift);
int is_attacked(const Position *const pos, const int sq, const int opp);

#endif


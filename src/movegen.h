
#ifndef MOVEGEN_H
#define MOVEGEN_H

#include <stdint.h>

#include "bb.h"
#include "magic.h"
#include "node.h"

#define FLAG_PAWN_PUSH  (1 << 12)
#define FLAG_EP_CAPTURE (1 << 13)
#define FLAG_CASTLE     (1 << 14)
#define FLAG_PROMO      (1 << 15)

#define PROMO_SHIFT 17

#define MASK_N_PROMO (FLAG_PROMO | ((KNIGHT-1) << PROMO_SHIFT))
#define MASK_B_PROMO (FLAG_PROMO | ((BISHOP-1) << PROMO_SHIFT))
#define MASK_R_PROMO (FLAG_PROMO | ((ROOK-1)   << PROMO_SHIFT))
#define MASK_Q_PROMO (FLAG_PROMO | ((QUEEN-1)  << PROMO_SHIFT))

#define MASK_SPECIAL (FLAG_PAWN_PUSH | FLAG_EP_CAPTURE | FLAG_CASTLE | FLAG_PROMO)

void init_next_search_move(Node *const node, const uint8_t in_check);
uint32_t get_next_search_move(Node *const node);

#endif


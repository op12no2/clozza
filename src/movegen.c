
#include "movegen.h"

/*{{{  encode_move*/

static inline uint32_t encode_move(const int from, const int to, const uint32_t flags) {

  return (from << 6) | to | flags;

}

/*}}}*/

/*{{{  gen_pawns*/

/*{{{  gen_pawns_white_quiet*/

static void gen_pawns_white_quiet(Node *const node) {

  const Position *const pos = &node->pos;
  const uint64_t pawns    = pos->all[WPAWN];
  const uint64_t occupied = pos->occupied;
  uint32_t *const m = node->moves + node->num_moves;
  int n = 0;

  /*{{{  push 1*/
  
  const uint64_t one = (pawns << 8) & ~occupied;
  uint64_t quiet = one & ~RANK_8;
  uint64_t promo = one &  RANK_8;
  
  while (quiet) {
    const int to = bsf(quiet); quiet &= quiet - 1;
    m[n++] = encode_move(to - 8, to, 0);
  }
  
  while (promo) {
    const int to = bsf(promo); promo &= promo - 1;
    const int from = to - 8;
    m[n++] = encode_move(from, to, MASK_Q_PROMO);
    m[n++] = encode_move(from, to, MASK_R_PROMO);
    m[n++] = encode_move(from, to, MASK_B_PROMO);
    m[n++] = encode_move(from, to, MASK_N_PROMO);
  }
  
  /*}}}*/
  /*{{{  push 2*/
  
  uint64_t two = ((one & RANK_3) << 8) & ~occupied;  // RANK_3 == from squares after first step
  
  while (two) {
    int to = bsf(two); two &= two - 1;
    m[n++] = encode_move(to - 16, to, FLAG_PAWN_PUSH);
  }
  
  /*}}}*/

  node->num_moves += n;

}

/*}}}*/
/*{{{  gen_pawns_white_noisy*/

static void gen_pawns_white_noisy(Node *const node) {

  const Position *const pos = &node->pos;
  const uint64_t pawns    = pos->all[WPAWN];
  const uint64_t enemies  = pos->colour[BLACK];
  uint32_t *const m = node->moves + node->num_moves;
  int n = 0;

  /*{{{  captures*/
  
  const uint64_t left  = ((pawns << 7) & NOT_H_FILE) & enemies;
  const uint64_t right = ((pawns << 9) & NOT_A_FILE) & enemies;
  
  uint64_t cap   = left & ~RANK_8;
  uint64_t promo = left &  RANK_8;
  
  while (cap) {
    int to = bsf(cap); cap &= cap - 1;
    m[n++] = encode_move(to - 7, to, 0);
  }
  
  while (promo) {
    int to = bsf(promo); promo &= promo - 1;
    const int from = to - 7;
    m[n++] = encode_move(from, to, MASK_Q_PROMO);
    m[n++] = encode_move(from, to, MASK_R_PROMO);
    m[n++] = encode_move(from, to, MASK_B_PROMO);
    m[n++] = encode_move(from, to, MASK_N_PROMO);
  }
  
  cap   = right & ~RANK_8;
  promo = right &  RANK_8;
  
  while (cap) {
    const int to = bsf(cap); cap &= cap - 1;
    m[n++] = encode_move(to - 9, to, 0);
  }
  
  while (promo) {
    const int to = bsf(promo); promo &= promo - 1;
    const int from = to - 9;
    m[n++] = encode_move(from, to, MASK_Q_PROMO);
    m[n++] = encode_move(from, to, MASK_R_PROMO);
    m[n++] = encode_move(from, to, MASK_B_PROMO);
    m[n++] = encode_move(from, to, MASK_N_PROMO);
  }
  
  /*}}}*/
  /*{{{  ep*/
  
  if (pos->ep) {
  
    uint64_t ep_from = pawn_attacks[WHITE][pos->ep] & pawns;
  
    while (ep_from) {
      const int from = bsf(ep_from); ep_from &= ep_from - 1;
      m[n++] = encode_move(from, pos->ep, FLAG_EP_CAPTURE);
    }
  
  }
  
  /*}}}*/

  node->num_moves += n;

}

/*}}}*/

/*{{{  gen_pawns_black_quiet*/

static void gen_pawns_black_quiet(Node *const node) {

  const Position *const pos = &node->pos;
  const uint64_t pawns    = pos->all[BPAWN];
  const uint64_t occupied = pos->occupied;
  uint32_t *const m = node->moves + node->num_moves;
  int n = 0;

  /*{{{  push 1*/
  
  const uint64_t one = (pawns >> 8) & ~occupied;
  uint64_t quiet = one & ~RANK_1;
  uint64_t promo = one &  RANK_1;
  
  while (quiet) {
    const int to = bsf(quiet); quiet &= quiet - 1;
    m[n++] = encode_move(to + 8, to, 0);
  }
  
  while (promo) {
    const int to = bsf(promo); promo &= promo - 1;
    const int from = to + 8;
    m[n++] = encode_move(from, to, MASK_Q_PROMO);
    m[n++] = encode_move(from, to, MASK_R_PROMO);
    m[n++] = encode_move(from, to, MASK_B_PROMO);
    m[n++] = encode_move(from, to, MASK_N_PROMO);
  }
  
  /*}}}*/
  /*{{{  push 2*/
  
  uint64_t two = ((one & RANK_6) >> 8) & ~occupied;
  
  while (two) {
    int to = bsf(two); two &= two - 1;
    m[n++] = encode_move(to + 16, to, FLAG_PAWN_PUSH);
  }
  
  /*}}}*/

  node->num_moves += n;

}

/*}}}*/
/*{{{  gen_pawns_black_noisy*/

static void gen_pawns_black_noisy(Node *const node) {

  const Position *const pos = &node->pos;
  const uint64_t pawns    = pos->all[BPAWN];
  const uint64_t enemies  = pos->colour[WHITE];
  uint32_t *const m = node->moves + node->num_moves;
  int n = 0;

  /*{{{  captures*/
  
  const uint64_t left  = ((pawns >> 9) & NOT_H_FILE) & enemies;
  const uint64_t right = ((pawns >> 7) & NOT_A_FILE) & enemies;
  
  uint64_t cap   = left & ~RANK_1;
  uint64_t promo = left &  RANK_1;
  
  while (cap) {
    const int to = bsf(cap); cap &= cap - 1;
    m[n++] = encode_move(to + 9, to, 0);
  }
  
  while (promo) {
    const int to = bsf(promo); promo &= promo - 1;
    const int from = to + 9;
    m[n++] = encode_move(from, to, MASK_Q_PROMO);
    m[n++] = encode_move(from, to, MASK_R_PROMO);
    m[n++] = encode_move(from, to, MASK_B_PROMO);
    m[n++] = encode_move(from, to, MASK_N_PROMO);
  }
  
  cap   = right & ~RANK_1;
  promo = right &  RANK_1;
  
  while (cap) {
    const int to = bsf(cap); cap &= cap - 1;
    m[n++] = encode_move(to + 7, to, 0);
  }
  
  while (promo) {
    const int to = bsf(promo); promo &= promo - 1;
    const int from = to + 7;
    m[n++] = encode_move(from, to, MASK_Q_PROMO);
    m[n++] = encode_move(from, to, MASK_R_PROMO);
    m[n++] = encode_move(from, to, MASK_B_PROMO);
    m[n++] = encode_move(from, to, MASK_N_PROMO);
  }
  
  /*}}}*/
  /*{{{  ep*/
  
  if (pos->ep) {
  
    uint64_t ep_from = pawn_attacks[BLACK][pos->ep] & pawns;
  
    while (ep_from) {
      const int from = bsf(ep_from); ep_from &= ep_from - 1;
      m[n++] = encode_move(from, pos->ep, FLAG_EP_CAPTURE);
    }
  
  }
  
  /*}}}*/

  node->num_moves += n;

}

/*}}}*/

static void gen_pawns_quiet(Node *const node) {
  if (node->pos.stm == WHITE)
    gen_pawns_white_quiet(node);
  else
    gen_pawns_black_quiet(node);
}

static void gen_pawns_noisy(Node *const node) {
  if (node->pos.stm == WHITE)
    gen_pawns_white_noisy(node);
  else
    gen_pawns_black_noisy(node);
}

/*}}}*/
/*{{{  gen_jumpers*/

static void gen_jumpers(Node *const node, const uint64_t *const attack_table, const int piece, const uint64_t targets) {

  const Position *const pos = &node->pos;
  const int stm = pos->stm;

  uint32_t *const m = node->moves + node->num_moves;
  int n = 0;

  uint64_t bb = pos->all[piece_index(piece, stm)];

  while (bb) {

    const int from = bsf(bb); bb &= bb - 1;
    uint64_t attacks = attack_table[from] & targets;

    while (attacks) {
      const int to = bsf(attacks); attacks &= attacks - 1;
      m[n++] = encode_move(from, to, 0);
    }
  }

  node->num_moves += n;

}

/*}}}*/
/*{{{  gen_sliders*/

static void gen_sliders(Node *const node, Attack *const attack_table, const int piece, const uint64_t targets) {

  const Position *const pos = &node->pos;
  const int stm = pos->stm;
  const uint64_t occ = pos->occupied;

  uint32_t *const m = node->moves + node->num_moves;
  int n = 0;

  uint64_t bb = pos->all[piece_index(piece, stm)];

  while (bb) {

    const int from = bsf(bb); bb &= bb - 1;
    const Attack *const a = &attack_table[from];
    const uint64_t blockers = occ & a->mask;
    const int index = magic_index(blockers, a->magic, a->shift);

    uint64_t attacks = a->attacks[index] & targets;

    while (attacks) {
      const int to = bsf(attacks); attacks &= attacks - 1;
      m[n++] = encode_move(from, to, 0);
    }
  }

  node->num_moves += n;

}

/*}}}*/
/*{{{  gen_castling*/

static void gen_castling(Node *const node) {

  const Position *const pos = &node->pos;
  const int stm = pos->stm;
  const int opp = stm ^ 1;
  const uint8_t rights = pos->rights;
  const uint64_t occupied = pos->occupied;

  uint32_t *const m = node->moves + node->num_moves;
  int n = 0;

  if (stm == WHITE) {
    if ((rights & WHITE_RIGHTS_KING) &&
        !(occupied & 0x0000000000000060ULL) &&
        !is_attacked(pos, F1, opp) &&
        !is_attacked(pos, G1, opp)) {
      m[n++] = encode_move(E1, G1, FLAG_CASTLE);
    }
    if ((rights & WHITE_RIGHTS_QUEEN) &&
        !(occupied & 0x000000000000000EULL) &&
        !is_attacked(pos, D1, opp) &&
        !is_attacked(pos, C1, opp)) {
      m[n++] = encode_move(E1, C1, FLAG_CASTLE);
    }
  }
  else {
    if ((rights & BLACK_RIGHTS_KING) &&
        !(occupied & 0x6000000000000000ULL) &&
        !is_attacked(pos, F8, opp) &&
        !is_attacked(pos, G8, opp)) {
      m[n++] = encode_move(E8, G8, FLAG_CASTLE);
    }
    if ((rights & BLACK_RIGHTS_QUEEN) &&
        !(occupied & 0x0E00000000000000ULL) &&
        !is_attacked(pos, D8, opp) &&
        !is_attacked(pos, C8, opp)) {
      m[n++] = encode_move(E8, C8, FLAG_CASTLE);
    }
  }

  node->num_moves += n;

}

/*}}}*/

/*{{{  init_next_search_move*/

void init_next_search_move(Node *const node, const uint8_t in_check) {

  node->stage = 0;
  node->in_check = in_check;
  node->num_moves = 0;
  node->next_move = 0;

  const Position *const pos = &node->pos;
  const int stm = pos->stm;
  const int opp = stm ^ 1;
  const uint64_t opp_king = pos->all[piece_index(KING, opp)];
  const uint64_t opp_king_near = king_attacks[bsf(opp_king)];
  const uint64_t enemies = pos->colour[opp] & ~opp_king;

  gen_pawns_noisy(node);

  gen_jumpers(node, knight_attacks, KNIGHT, enemies);
  gen_sliders(node, bishop_attacks, BISHOP, enemies);
  gen_sliders(node, rook_attacks,   ROOK,   enemies);
  gen_sliders(node, bishop_attacks, QUEEN,  enemies);
  gen_sliders(node, rook_attacks,   QUEEN,  enemies);
  gen_jumpers(node, king_attacks,   KING,   enemies & ~opp_king_near);

}

/*}}}*/
/*{{{  get_next_search_move*/

uint32_t get_next_search_move(Node *const node) {

  switch (node->stage) {

    case 0: {
      /*{{{  next noisy*/
      
      if (node->next_move < node->num_moves)
        return node->moves[node->next_move++];
      
      node->stage++;
      node->num_moves = 0;
      node->next_move = 0;
      
      const Position *const pos = &node->pos;
      const int stm = pos->stm;
      const int opp = stm ^ 1;
      const uint64_t occ = pos->occupied;
      const uint64_t opp_king = pos->all[piece_index(KING, opp)];
      const uint64_t opp_king_near = king_attacks[bsf(opp_king)];
      
      gen_pawns_quiet(node);
      
      gen_jumpers(node, knight_attacks, KNIGHT, ~occ);
      gen_sliders(node, bishop_attacks, BISHOP, ~occ);
      gen_sliders(node, rook_attacks,   ROOK,   ~occ);
      gen_sliders(node, bishop_attacks, QUEEN,  ~occ);
      gen_sliders(node, rook_attacks,   QUEEN,  ~occ);
      gen_jumpers(node, king_attacks,   KING,   ~occ & ~opp_king_near);
      
      if (node->pos.rights && !node->in_check)
        gen_castling(node);
      
      /*}}}*/
    }

    case 1: {
      /*{{{  next quiet*/
      
      if (node->next_move < node->num_moves)
        return node->moves[node->next_move++];
      
      return 0;
      
      /*}}}*/
    }

    default:
      fprintf(stderr, "oops\n");
      return 0;
  }
}

/*}}}*/

/*{{{  init_next_qsearch_move*/

void init_next_qsearch_move(Node *const node) {

  node->stage = 0;     // unused
  node->in_check = 0;  // unused
  node->num_moves = 0;
  node->next_move = 0;

  const Position *const pos = &node->pos;
  const int stm = pos->stm;
  const int opp = stm ^ 1;
  const uint64_t opp_king = pos->all[piece_index(KING, opp)];
  const uint64_t opp_king_near = king_attacks[bsf(opp_king)];
  const uint64_t enemies = pos->colour[opp] & ~opp_king;

  gen_pawns_noisy(node);

  gen_jumpers(node, knight_attacks, KNIGHT, enemies);
  gen_sliders(node, bishop_attacks, BISHOP, enemies);
  gen_sliders(node, rook_attacks,   ROOK,   enemies);
  gen_sliders(node, bishop_attacks, QUEEN,  enemies);
  gen_sliders(node, rook_attacks,   QUEEN,  enemies);
  gen_jumpers(node, king_attacks,   KING,   enemies & ~opp_king_near);

}

/*}}}*/
/*{{{  get_next_qsearch_move*/

uint32_t get_next_qsearch_move(Node *const node) {

  if (node->next_move == node->num_moves)
    return 0;

  return node->moves[node->next_move++];

}

/*}}}*/



#include "magic.h"

static uint64_t raw_attacks[107648] __attribute__((aligned(64))); // 102400 (R) + 5248 (B)

uint64_t pawn_attacks[2][64] __attribute__((aligned(64)));
uint64_t knight_attacks[64]  __attribute__((aligned(64)));
uint64_t king_attacks[64]    __attribute__((aligned(64)));

Attack bishop_attacks[64]  __attribute__((aligned(64)));
Attack rook_attacks[64]    __attribute__((aligned(64)));

/*{{{  magic_index*/

inline int magic_index(const uint64_t blockers, const uint64_t magic, const int shift) {

  return (int)((blockers * magic) >> shift);

}

/*}}}*/
/*{{{  get_blockers*/

static void get_blockers(Attack *a, uint64_t *blockers) {

  int bits[64];
  int num_bits = 0;

  for (int b = 0; b < 64; b++) {
    if (a->mask & (1ULL << b)) {
      bits[num_bits++] = b;
    }
  }

  for (int i = 0; i < a->count; i++) {

    uint64_t blocker = 0;

    for (int j = 0; j < a->bits; j++) {
      if (i & (1 << j)) {
        blocker |= 1ULL << bits[j];
      }
    }

    blockers[i] = blocker;

  }
}

/*}}}*/
/*{{{  find_magics*/

static void find_magics(Attack attacks[64], const char *label) {

  uint64_t tbl[MAGIC_MAX_SLOTS];
  uint32_t used[MAGIC_MAX_SLOTS];

  static uint32_t stamp = 1;

  const int verbose = 1;

  int total_tries = 0;
  int total_slots = 0;

  for (int sq = 0; sq < 64; ++sq) {

    Attack *a = &attacks[sq];
    const int N = a->count;

    uint64_t blockers[MAGIC_MAX_SLOTS];
    get_blockers(a, blockers);

    memset(used, 0, (size_t)N * sizeof used[0]);

    int tries = 0;

    for (;;) {

      ++tries;

      if (++stamp == 0) {
        memset(used, 0, (size_t)N * sizeof used[0]);
        stamp = 1;
      }

      const uint64_t magic = rand64() & rand64() & rand64();

      if (popcount((a->mask * magic) >> (64 - a->bits)) < a->bits - 3)
        continue;

      int fail = 0;

      for (int i = 0; i < N; ++i) {

        const int idx = magic_index(blockers[i], magic, a->shift);
        const uint64_t att = a->attacks[i];

        if (used[idx] != stamp) {
          used[idx] = stamp;
          tbl[idx]  = att;
        }
        else if (tbl[idx] != att) {
          fail = 1;
          break;
        }
      }

      if (!fail) {

        a->magic = magic;

        for (int i = 0; i < N; ++i) {
          const int idx = magic_index(blockers[i], magic, a->shift);
          a->attacks[idx] = tbl[idx];
        }

        if (verbose) {
          printf("%s sq %2d tries %d %8d magic %" PRIx64 "\n",
                 label, sq, tries, N, a->magic);
        }

        total_tries += tries;
        total_slots += N;

        break;

      }
    }
  }

  if (verbose)
    printf("%s total_tries %d total_slots %d\n", label, total_tries, total_slots);

  (void)label;

}

/*}}}*/
/*{{{  is_attacked*/

int is_attacked(const Position *const pos, const int sq, const int opp) {

  const int base = colour_index(opp);

  const uint64_t opp_pawns   = pos->all[base+PAWN];
  const uint64_t opp_kings   = pos->all[base+KING];
  const uint64_t opp_knights = pos->all[base+KNIGHT];
  const uint64_t opp_bq      = pos->all[base+BISHOP] | pos->all[base+QUEEN];
  const uint64_t opp_rq      = pos->all[base+ROOK]   | pos->all[base+QUEEN];

  if (opp_knights & knight_attacks[sq])    return 1;
  if (opp_pawns   & pawn_attacks[opp][sq]) return 1;
  if (opp_kings   & king_attacks[sq])      return 1;

  {
    const Attack *const a = &bishop_attacks[sq];
    const uint64_t blockers = pos->occupied & a->mask;
    const uint64_t attacks  = a->attacks[magic_index(blockers, a->magic, a->shift)];
    if (attacks & opp_bq) return 1;
  }

  {
    const Attack *const a = &rook_attacks[sq];
    const uint64_t blockers = pos->occupied & a->mask;
    const uint64_t attacks  = a->attacks[magic_index(blockers, a->magic, a->shift)];
    if (attacks & opp_rq) return 1;
  }

  return 0;

}

/*}}}*/

/*{{{  init_pawn_attacks*/

// these are attacks *to* the sq and used in pawn_gen (ep) and is_attacked

static void init_pawn_attacks(void) {

  for (int sq = 0; sq < 64; sq++) {

    const uint64_t bb = 1ULL << sq;

    pawn_attacks[WHITE][sq] =
      ((bb >> 7) & NOT_A_FILE) | ((bb >> 9) & NOT_H_FILE);

    pawn_attacks[BLACK][sq] =
      ((bb << 7) & NOT_H_FILE) | ((bb << 9) & NOT_A_FILE);

  }
}

/*}}}*/
/*{{{  init_knight_attacks*/

static void init_knight_attacks(void) {

  for (int sq = 0; sq < 64; sq++) {

    int r = sq / 8, f = sq % 8;
    uint64_t bb = 0;

    int dr[8] = {-2, -1, 1, 2, 2, 1, -1, -2};
    int df[8] = {1, 2, 2, 1, -1, -2, -2, -1};

    for (int i = 0; i < 8; i++) {
      int nr = r + dr[i], nf = f + df[i];
      if (nr >= 0 && nr < 8 && nf >= 0 && nf < 8)
        bb |= 1ULL << (nr * 8 + nf);
    }

    knight_attacks[sq] = bb;

  }
}

/*}}}*/
/*{{{  init_bishop_attacks*/

static void init_bishop_attacks(void) {

  int next_attack_index = 0;

  for (int sq = 0; sq < 64; sq++) {

    Attack *a = &bishop_attacks[sq];

    /*{{{  build mask*/
    
    int rank = sq / 8;
    int file = sq % 8;
    
    a->mask = 0;
    
    for (int r = rank + 1, f = file + 1; r <= 6 && f <= 6; r++, f++)
      a->mask |= 1ULL << (r * 8 + f);
    
    for (int r = rank + 1, f = file - 1; r <= 6 && f >= 1; r++, f--)
      a->mask |= 1ULL << (r * 8 + f);
    
    for (int r = rank - 1, f = file + 1; r >= 1 && f <= 6; r--, f++)
      a->mask |= 1ULL << (r * 8 + f);
    
    for (int r = rank - 1, f = file - 1; r >= 1 && f >= 1; r--, f--)
      a->mask |= 1ULL << (r * 8 + f);
    
    /*}}}*/

    a->bits = popcount(a->mask);
    a->shift = 64 - a->bits;
    a->count = 1 << a->bits;
    a->attacks = &raw_attacks[next_attack_index];

    uint64_t blockers[a->count];
    get_blockers(a, blockers);

    for (int i = 0; i < a->count; i++) {
      /*{{{  build attacks[next_attack_index]*/
      
      uint64_t blocker = blockers[i];
      uint64_t attack = 0;
      
      for (int r = rank + 1, f = file + 1; r <= 7 && f <= 7; r++, f++) {
        int s = r * 8 + f;
        attack |= 1ULL << s;
        if (blocker & (1ULL << s))
          break;
      }
      
      for (int r = rank + 1, f = file - 1; r <= 7 && f >= 0; r++, f--) {
        int s = r * 8 + f;
        attack |= 1ULL << s;
        if (blocker & (1ULL << s))
          break;
      }
      
      for (int r = rank - 1, f = file + 1; r >= 0 && f <= 7; r--, f++) {
        int s = r * 8 + f;
        attack |= 1ULL << s;
        if (blocker & (1ULL << s))
          break;
      }
      
      for (int r = rank - 1, f = file - 1; r >= 0 && f >= 0; r--, f--) {
        int s = r * 8 + f;
        attack |= 1ULL << s;
        if (blocker & (1ULL << s))
          break;
      }
      
      raw_attacks[next_attack_index++] = attack;
      
      /*}}}*/
    }
  }

  find_magics(bishop_attacks, "B");

}

/*}}}*/
/*{{{  init_rook_attacks*/

static void init_rook_attacks(void) {

  int next_attack_index = 5248;

  for (int sq = 0; sq < 64; sq++) {

    Attack *a = &rook_attacks[sq];

    /*{{{  build mask*/
    
    int rank = sq / 8;
    int file = sq % 8;
    
    a->mask = 0;
    
    for (int f = file + 1; f <= 6; f++)
      a->mask |= 1ULL << (rank * 8 + f);
    
    for (int f = file - 1; f >= 1; f--)
      a->mask |= 1ULL << (rank * 8 + f);
    
    for (int r = rank + 1; r <= 6; r++)
      a->mask |= 1ULL << (r * 8 + file);
    
    for (int r = rank - 1; r >= 1; r--)
      a->mask |= 1ULL << (r * 8 + file);
    
    /*}}}*/

    a->bits = popcount(a->mask);
    a->shift = 64 - a->bits;
    a->count = 1 << a->bits;
    a->attacks = &raw_attacks[next_attack_index];

    uint64_t blockers[a->count];
    get_blockers(a, blockers);

    for (int i = 0; i < a->count; i++) {
      /*{{{  build attacks[next_attack_index]*/
      
      uint64_t blocker = blockers[i];
      uint64_t attack = 0;
      
      for (int r = rank + 1; r <= 7; r++) {
        int s = r * 8 + file;
        attack |= 1ULL << s;
        if (blocker & (1ULL << s)) {
          break;
        }
      }
      
      for (int r = rank - 1; r >= 0; r--) {
        int s = r * 8 + file;
        attack |= 1ULL << s;
        if (blocker & (1ULL << s)) {
          break;
        }
      }
      
      for (int f = file + 1; f <= 7; f++) {
        int s = rank * 8 + f;
        attack |= 1ULL << s;
        if (blocker & (1ULL << s)) {
          break;
        }
      }
      
      for (int f = file - 1; f >= 0; f--) {
        int s = rank * 8 + f;
        attack |= 1ULL << s;
        if (blocker & (1ULL << s)) {
          break;
        }
      }
      
      raw_attacks[next_attack_index++] = attack;
      
      /*}}}*/
    }
  }

  find_magics(rook_attacks, "R");

}

/*}}}*/
/*{{{  init_king_attacks*/

static void init_king_attacks(void) {

  for (int sq = 0; sq < 64; sq++) {

    int r = sq / 8, f = sq % 8;
    uint64_t bb = 0;

    for (int dr = -1; dr <= 1; dr++) {
      for (int df = -1; df <= 1; df++) {
        if (dr == 0 && df == 0) continue;
        int nr = r + dr, nf = f + df;
        if (nr >= 0 && nr < 8 && nf >= 0 && nf < 8)
          bb |= 1ULL << (nr * 8 + nf);
      }
    }

    king_attacks[sq] = bb;

  }
}

/*}}}*/
/*{{{  magic_init*/

void magic_init () {

  init_pawn_attacks();
  init_knight_attacks();
  init_bishop_attacks();
  init_rook_attacks();
  init_king_attacks();

}

/*}}}*/


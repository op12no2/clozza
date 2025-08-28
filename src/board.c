
#include "board.h"

/*{{{  position*/

void position(Node *const node, const char *board_fen, const char *stm_str, const char *rights_str, const char *ep_str) {

  Position *const pos = &node->pos;

  static const int char_to_piece[128] = {
    ['p'] = 0, ['n'] = 1, ['b'] = 2, ['r'] = 3, ['q'] = 4, ['k'] = 5,
  };

  memset(pos, 0, sizeof(Position));

  /*{{{  board*/
  
  for (int i=0; i < 64; i++)
    pos->board[i] = EMPTY;
  
  int sq = 56;
  
  for (const char *p = board_fen; *p; ++p) {
  
    if (*p == '/') {
      sq -= 16;
    }
  
    else if (isdigit(*p)) {
      sq += *p - '0';
    }
  
    else {
  
      int colour = !!islower(*p);
      int piece = char_to_piece[tolower(*p)];  // 0-5
      int index = piece_index(piece, colour);  // 0-11
  
      uint64_t bb = 1ULL << sq;
  
      pos->all[index]     |= bb;
      pos->occupied       |= bb;
      pos->colour[colour] |= bb;
  
      pos->board[sq] = index;
  
      pos->hash ^= zob_pieces[zob_index(index, sq)];
  
      sq++;
  
    }
  }
  
  /*}}}*/
  /*{{{  stm*/
  
  pos->stm = (stm_str[0] == 'w') ? WHITE : BLACK;
  
  if (pos->stm == BLACK)
    pos->hash ^= zob_stm;
  
  /*}}}*/
  /*{{{  rights*/
  
  pos->rights = 0;
  
  for (const char *p = rights_str; *p; ++p) {
    switch (*p) {
      case 'K': pos->rights |= WHITE_RIGHTS_KING;  break;
      case 'Q': pos->rights |= WHITE_RIGHTS_QUEEN; break;
      case 'k': pos->rights |= BLACK_RIGHTS_KING;  break;
      case 'q': pos->rights |= BLACK_RIGHTS_QUEEN; break;
    }
  }
  
  pos->hash ^= zob_rights[pos->rights];
  
  /*}}}*/
  /*{{{  ep*/
  
  if (ep_str[0] == '-') {
    pos->ep = 0;  // 0 is not a legal ep square so this is ok
  }
  
  else {
  
    int file = ep_str[0] - 'a';
    int rank = ep_str[1] - '1';
  
    pos->ep = rank * 8 + file;
  
  }
  
  pos->hash ^= zob_ep[pos->ep];
  
  /*}}}*/

  net_rebuild_accs(node);

}

/*}}}*/
/*{{{  rebuild_debug*/

void rebuild_debug(Node *const n1) {

  Node n;
  Node *n2 = &n;

  Position *const pos = &n1->pos;

  /*{{{  check ue*/
  
  net_copy(n1, n2);                      // copy the ue accumulators into n2
  net_rebuild_accs(n1);                  // rebuild the n1 accumulators from scratch
  
  for (int i=0; i < NET_H1_SIZE; i++) {  // compare them
    if (n1->acc1[i] != n2->acc1[i]) {
      fprintf(stderr, "a1 %d\n", i);
    }
    if (n1->acc2[i] != n2->acc2[i]) {
      fprintf(stderr, "a2 %d\n", i);
    }
  }
  
  /*}}}*/
  /*{{{  check bb == board*/
  
  uint64_t all[12] = {0};
  uint64_t colour[2] = {0};
  uint64_t occupied = 0;
  
  for (int sq=0; sq < 64; sq++) {
    int piece = pos->board[sq];
    if (piece == EMPTY) continue;
    all[piece]                         |= (1ULL << sq);
    colour[piece >= 6 ? BLACK : WHITE] |= (1ULL << sq);
    occupied                           |= (1ULL << sq);
  }
  
  for (int p = 0; p < 12; p++) {
    if (all[p] != pos->all[p]) {
      fprintf(stderr, "verify_from_board: mismatch in all[%d]\n", p);
    }
  }
  
  if (colour[WHITE] != pos->colour[WHITE]) {
    fprintf(stderr, "verify_from_board: mismatch in colour[WHITE]\n");
  }
  
  if (colour[BLACK] != pos->colour[BLACK]) {
    fprintf(stderr, "verify_from_board: mismatch in colour[BLACK]\n");
  }
  
  if (occupied != pos->occupied) {
    fprintf(stderr, "verify_from_board: mismatch in occupied\n");
  }
  
  /*}}}*/
  /*{{{  check board == bb*/
  
  uint8_t board[64];
  
  for (int sq = 0; sq < 64; sq++)
    board[sq] = EMPTY;
  
  for (int p=0; p < 12; p++) {
    uint64_t bb = pos->all[p];
    while (bb) {
      int sq = __builtin_ctzll(bb);
      board[sq] = p;
      bb &= bb - 1;
    }
  }
  
  for (int sq = 0; sq < 64; sq++) {
    if (board[sq] != pos->board[sq]) {
      fprintf(stderr, "verify_from_bitboards: mismatch at square %d (expected %d got %d)\n",
                      sq, pos->board[sq], board[sq]);
    }
  }
  
  /*}}}*/
  /*{{{  check hash*/
  
  uint64_t hash = 0;
  
  for (int sq=0; sq < 64; sq++) {
    int piece = pos->board[sq];
    if (piece == EMPTY) continue;
    hash ^= zob_pieces[zob_index(piece, sq)];
  }
  
  hash ^= zob_rights[n1->pos.rights];
  hash ^= zob_ep[n1->pos.ep];
  
  if (n1->pos.stm == BLACK)
    hash ^= zob_stm;
  
  if (hash != n1->pos.hash)
    fprintf(stderr, "inc %" PRIx64 " rebuilt %" PRIx64 "\n", n1->pos.hash, hash);
  
  /*}}}*/

}

/*}}}*/
/*{{{  print_board*/

void print_board(Node *const node) {

  const Position *const pos = &node->pos;

  const char piece_chars[12] = {
    'P', 'N', 'B', 'R', 'Q', 'K',
    'p', 'n', 'b', 'r', 'q', 'k'
  };

  for (int rank=7; rank >= 0; rank--) {

    printf("%d  ", rank + 1);

    for (int file=0; file < 8; file++) {

      int sq = rank * 8 + file;
      uint64_t bb = 1ULL << sq;
      char c = '.';

      for (int i = 0; i < 12; i++) {
        if (pos->all[i] & bb) {
          c = piece_chars[i];
          break;
        }
      }

      printf("%c ", c);
    }

    printf("\n");
  }

  printf("   a b c d e f g h\n");
  printf("stm=%d\n", pos->stm);
  printf("rights=%d\n", pos->rights);
  printf("ep=%d\n", pos->ep);
  printf("e=%d\n", net_eval(node));
  printf("h=%" PRIx64 "\n", pos->hash);

  rebuild_debug(node);

}

/*}}}*/
/*{{{  make_move*/

/*{{{  helper tables*/

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winitializer-overrides"

static const int orth_offset[2] = {8, -8};

static const int rook_to[64] = {
  [G1] = F1,
  [C1] = D1,
  [G8] = F8,
  [C8] = D8,
};

static const int rook_from[64] = {
  [G1] = H1,
  [C1] = A1,
  [G8] = H8,
  [C8] = A8,
};

static const uint8_t rights_mask[64] = {
  [0 ... 63] = ALL_RIGHTS,
  [A1] = ALL_RIGHTS & ~WHITE_RIGHTS_QUEEN,
  [H1] = ALL_RIGHTS & ~WHITE_RIGHTS_KING,
  [E1] = ALL_RIGHTS & ~(WHITE_RIGHTS_KING | WHITE_RIGHTS_QUEEN),

  [A8] = ALL_RIGHTS & ~BLACK_RIGHTS_QUEEN,
  [H8] = ALL_RIGHTS & ~BLACK_RIGHTS_KING,
  [E8] = ALL_RIGHTS & ~(BLACK_RIGHTS_KING | BLACK_RIGHTS_QUEEN)
};

/*}}}*/

void make_move(Position *const pos, const uint32_t move) {

  uint64_t *const all = pos->all;
  uint64_t *const colour = pos->colour;
  uint8_t *const board = pos->board;

  int rights = pos->rights;
  int ep = pos->ep;
  uint64_t hash = pos->hash;

  const int from = (move >> 6) & 0x3F;
  const int to   = move & 0x3F;

  const uint64_t from_bb = 1ULL << from;
  const uint64_t to_bb   = 1ULL << to;

  const int stm = pos->stm;
  const int opp = stm ^ 1;

  const int from_piece = board[from];  // 0-11
  const int to_piece   = board[to];    // 0-11

  /*{{{  remove from piece*/
  
  all[from_piece] &= ~from_bb;
  colour[stm]     &= ~from_bb;
  
  board[from] = EMPTY;
  
  hash ^= zob_pieces[zob_index(from_piece, from)];
  
  /*}}}*/

  if (to_piece != EMPTY) {
    /*{{{  remove to piece*/
    
    all[to_piece] &= ~to_bb;
    colour[opp]   &= ~to_bb;
    
    hash ^= zob_pieces[zob_index(to_piece, to)];
    
    net_prepare_capture(from_piece, from, to, to_piece);
    
    /*}}}*/
  }
  else {
    net_prepare_move(from_piece, from, to);
  }

  /*{{{  put from piece on to*/
  
  all[from_piece] |= to_bb;
  colour[stm]     |= to_bb;
  
  board[to] = from_piece;
  
  hash ^= zob_pieces[zob_index(from_piece, to)];
  
  /*}}}*/
  /*{{{  clear ep and update rights*/
  
  hash ^= zob_ep[ep];
  ep = 0;
  hash ^= zob_ep[ep];
  
  hash ^= zob_rights[rights];
  rights &= rights_mask[from] & rights_mask[to];
  hash ^= zob_rights[rights];
  
  /*}}}*/

  if (move & MASK_SPECIAL) {
    /*{{{  do the extra work*/
    
    if (move & FLAG_PROMO) {
      /*{{{  promo*/
      
      const int pro = piece_index(((move >> PROMO_SHIFT) & 3) + 1, stm);
      
      all[from_piece] &= ~to_bb;
      all[pro]        |= to_bb;
      
      board[to] = pro;
      
      hash ^= zob_pieces[zob_index(from_piece, to)];
      hash ^= zob_pieces[zob_index(pro, to)];
      
      net_prepare_promote(from_piece, from, to, to_piece, pro);
      
      /*}}}*/
    }
    
    else if (move & FLAG_EP_CAPTURE) {
      /*{{{  ep*/
      
      const int pawn_sq = to + orth_offset[opp];
      const uint64_t pawn_bb = 1ULL << pawn_sq;
      const int opp_pawn = piece_index(PAWN, opp);
      
      all[opp_pawn] &= ~pawn_bb;
      colour[opp] &= ~pawn_bb;
      board[pawn_sq] = EMPTY;
      
      hash ^= zob_pieces[zob_index(opp_pawn, pawn_sq)];
      
      net_prepare_ep_capture(from_piece, from, to, opp_pawn, pawn_sq);
      
      /*}}}*/
    }
    
    else if (move & FLAG_PAWN_PUSH) {
      /*{{{  set ep*/
      
      ep = from + orth_offset[stm];
      
      hash ^= zob_ep[ep];
      
      /*}}}*/
    }
    
    else if (move & FLAG_CASTLE) {
      /*{{{  castle*/
      
      const int rook = piece_index(ROOK, stm);
      
      const uint64_t rook_from_bb = 1ULL << rook_from[to];
      const uint64_t rook_to_bb   = 1ULL << rook_to[to];
      
      const int rook_from_sq = rook_from[to];
      const int rook_to_sq   = rook_to[to];
      
      all[rook]   &= ~rook_from_bb;
      colour[stm] &= ~rook_from_bb;
      board[rook_from_sq] = EMPTY;
      
      hash ^= zob_pieces[zob_index(rook, rook_from_sq)];
      
      all[rook]   |= rook_to_bb;
      colour[stm] |= rook_to_bb;
      board[rook_to_sq] = rook;
      
      hash ^= zob_pieces[zob_index(rook, rook_to_sq)];
      
      net_prepare_castle(from_piece, from, to, rook, rook_from_sq, rook_to_sq);
      
      /*}}}*/
    }
    
    /*}}}*/
  }

  pos->occupied = colour[WHITE] | colour[BLACK];

  hash ^= zob_stm;

  pos->hash = hash;
  pos->stm = (uint8_t)opp;
  pos->rights = (uint8_t)rights;
  pos->ep = (uint8_t)ep;

}

/*}}}*/


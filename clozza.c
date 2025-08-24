
const int __update_accs_in_perft = 0;
const int __check_everything_in_perft = 0;
const int __check_everything_in_search = 0;
const int __check_everything_in_qsearch = 0;

/*{{{  includes*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h> // PRIu64 PRId64 PRIx64
#include <ctype.h>
#include <time.h>
#include <sys/time.h>

/*}}}*/
/*{{{  constants*/

#define MAX_PLY 128
#define MAX_MOVES 256

#define INF  30000
#define MATE 29000

#define NET_H1_SIZE 256
#define NET_H1_SHIFT 8

#define UCI_LINE_LENGTH 8192
#define UCI_TOKENS      8192

#define WHITE_RIGHTS_KING  1
#define WHITE_RIGHTS_QUEEN 2
#define BLACK_RIGHTS_KING  4
#define BLACK_RIGHTS_QUEEN 8
#define ALL_RIGHTS         15

#define EMPTY 255

#define RANK_1 0x00000000000000FFULL
#define RANK_2 0x000000000000FF00ULL
#define RANK_3 0x0000000000FF0000ULL
#define RANK_6 0x0000FF0000000000ULL
#define RANK_7 0x00FF000000000000ULL
#define RANK_8 0xFF00000000000000ULL
#define RANK_PROMO (RANK_1 | RANK_8)

#define NOT_A_FILE 0xfefefefefefefefeULL
#define NOT_H_FILE 0x7f7f7f7f7f7f7f7fULL

enum {WHITE, BLACK};
enum {PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING};
enum {WPAWN, WKNIGHT, WBISHOP, WROOK, WQUEEN, WKING, BPAWN, BKNIGHT, BBISHOP, BROOK, BQUEEN, BKING};

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

enum {
  A1, B1, C1, D1, E1, F1, G1, H1,
  A2, B2, C2, D2, E2, F2, G2, H2,
  A3, B3, C3, D3, E3, F3, G3, H3,
  A4, B4, C4, D4, E4, F4, G4, H4,
  A5, B5, C5, D5, E5, F5, G5, H5,
  A6, B6, C6, D6, E6, F6, G6, H6,
  A7, B7, C7, D7, E7, F7, G7, H7,
  A8, B8, C8, D8, E8, F8, G8, H8
};

/*}}}*/
/*{{{  macros*/

#define MIN(a,b) \
  ({ __typeof__ (a) _a = (a); \
     __typeof__ (b) _b = (b); \
    _a < _b ? _a : _b; })

#define MAX(a,b) \
  ({ __typeof__ (a) _a = (a); \
     __typeof__ (b) _b = (b); \
    _a > _b ? _a : _b; })

/*}}}*/
/*{{{  structs*/

/*{{{  Position struct*/

typedef struct {

  uint64_t all[12];
  uint64_t colour[2];
  uint64_t occupied;

  uint8_t board[64];

  uint8_t stm;
  uint8_t rights;
  uint8_t ep;
  uint8_t flags;

  uint64_t hash;

} __attribute__((aligned(64))) Position;

/*}}}*/
/*{{{  Node struct*/

typedef struct {

  Position pos;

  uint32_t moves[MAX_MOVES];
  int num_moves;
  int next_move;
  uint8_t in_check;
  uint8_t stage;

  int32_t acc1[NET_H1_SIZE] __attribute__((aligned(64)));  // us acc
  int32_t acc2[NET_H1_SIZE] __attribute__((aligned(64)));  // them acc

} __attribute__((aligned(64))) Node;

/*}}}*/
/*{{{  Attack struct*/

typedef struct {

  int bits;
  int count;
  int shift;

  uint64_t mask;
  uint64_t magic;

  uint64_t *attacks;

} __attribute__((aligned(64))) Attack;

/*}}}*/
/*{{{  TimeControl struct*/

typedef struct {

  uint64_t start_time;
  uint64_t finish_time;
  uint64_t max_depth;
  uint64_t max_nodes;
  uint8_t finished;
  uint64_t nodes;
  uint32_t bm;
  int bs;
  uint8_t measure;
  uint8_t quiet;

} TimeControl;

/*}}}*/
/*{{{  Perft*/

typedef struct {

  const char *fen;
  int depth;
  uint64_t expected;
  const char *label;

} Perft;

/*}}}*/

/*}}}*/
/*{{{  globals*/

static uint64_t pawn_attacks[2][64] __attribute__((aligned(64)));
static uint64_t knight_attacks[64]  __attribute__((aligned(64)));
static Attack   bishop_attacks[64]  __attribute__((aligned(64)));
static Attack   rook_attacks[64]    __attribute__((aligned(64)));
static uint64_t king_attacks[64]    __attribute__((aligned(64)));

static Node ss[MAX_PLY] __attribute__((aligned(64)));

static TimeControl tc;

static uint64_t zob_pieces[12 * 64];
static uint64_t zob_stm;
static uint64_t zob_rights[16];
static uint64_t zob_ep[64];

/*}}}*/

/*{{{  utility*/

/*{{{  mylog*/

void mylog(const char *const str) {

  FILE *f = fopen("clozza.log", "a");
  if (!f)
    return;
  fprintf(f, "%s\n", str);
  fclose(f);

}

/*}}}*/
/*{{{  now_ms*/

uint64_t now_ms(void) {

  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000u + (uint64_t)(ts.tv_nsec / 1000000u);

}

/*}}}*/
/*{{{  check_tc*/

int check_tc() {

  if (tc.measure == 0)
    return tc.finished;

  tc.measure = 0;

  if (tc.bm == 0)
    return 0;

  if (tc.finish_time) {
    if (now_ms() >= tc.finish_time)
      return (tc.finished = 1);
  }

  else if (tc.max_nodes) {
    if (tc.nodes >= tc.max_nodes)
      return (tc.finished = 1);
  }

  return 0;

}

/*}}}*/
/*{{{  bump_nodes*/

#define MEASURE_MASK ((1 << 10) - 1)

void bump_nodes() {

  tc.nodes++;

  if ((tc.nodes & MEASURE_MASK) == 0)
    tc.measure = 1;

}

/*}}}*/
/*{{{  popcount*/

int popcount(const uint64_t bb) {

  return __builtin_popcountll(bb);

}

/*}}}*/
/*{{{  encode_move*/

uint32_t encode_move(const int from, const int to, const uint32_t flags) {

  return (from << 6) | to | flags;

}

/*}}}*/
/*{{{  bsf*/

int bsf(const uint64_t bb) {

  return __builtin_ctzll(bb);

}

/*}}}*/
/*{{{  rand64*/

uint64_t rand64_seed = 0xDEADBEEFCAFEBABEULL;

uint64_t rand64(void) {

  rand64_seed ^= rand64_seed >> 12;
  rand64_seed ^= rand64_seed << 25;
  rand64_seed ^= rand64_seed >> 27;

  return rand64_seed * 2685821657736338717ULL;

}

/*}}}*/
/*{{{  cleanup*/

void cleanup() {

  for (int sq=0; sq < 64; sq++) {

    free(rook_attacks[sq].attacks);
    free(bishop_attacks[sq].attacks);

  }

}

/*}}}*/
/*{{{  piece_index*/

int piece_index(const int piece, const int colour) {

  return piece + colour * 6;

}

/*}}}*/
/*{{{  colour_index*/

int colour_index(const int colour) {

  return colour * 6;

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
/*{{{  format_move*/

char *format_move(const uint32_t move, char *const buf) {

  static const char files[] = "abcdefgh";
  static const char ranks[] = "12345678";
  static const char promo[] = "nbrq";

  const int from = (move >> 6) & 0x3F;
  const int to   = move & 0x3F;

  buf[0] = files[from % 8];
  buf[1] = ranks[from / 8];
  buf[2] = files[to   % 8];
  buf[3] = ranks[to   / 8];
  buf[4] = '\0';
  buf[5] = '\0';

  if (move & FLAG_PROMO) {
    buf[4] = promo[(move >> PROMO_SHIFT) & 3];
  }

  return buf;

}

/*}}}*/
/*{{{  print_board*/

int32_t net_eval (Node *const node);

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

}

/*}}}*/
/*{{{  find_token*/

int find_token(char *token, int n, char **tokens) {

  for (int i=0; i < n; i++) {
    if (!strcmp(token, tokens[i]))
      return i;
  }

  return -1;

}

/*}}}*/
/*{{{  zob_index*/

int zob_index(const int piece, const int sq) {

  return (piece << 6) | sq;

}

/*}}}*/

/*}}}*/
/*{{{  net*/

#define NET_FILE "/home/xyzzy/lozza/nets/farm1/lozza-500/quantised.bin" // hack
#define NET_I_SIZE 768
#define NET_QA 255
#define NET_QB 64
#define NET_QAB (NET_QA * NET_QB)
#define NET_SCALE 400

static int32_t net_h1_w[NET_I_SIZE * NET_H1_SIZE] __attribute__((aligned(64)));
static int32_t net_h2_w[NET_I_SIZE * NET_H1_SIZE] __attribute__((aligned(64)));  // flipped
static int32_t net_h1_b[NET_H1_SIZE]              __attribute__((aligned(64)));
static int32_t net_o_w [NET_H1_SIZE * 2]          __attribute__((aligned(64)));
static int32_t net_o_b;

static void (*ue_func)(Node *const node);
static int ue_arg0;
static int ue_arg1;
static int ue_arg2;
static int ue_arg3;
static int ue_arg4;
static int ue_arg5;

/*{{{  base*/

int base(const int piece, const int sq) {

#ifdef DEBUG

  if (piece == EMPTY) {
    fprintf(stderr, "piece_index error: piece is EMPTY (%d), square=%d\n", piece, sq);
    abort();
  }
  if (piece < 0 || piece > 11) {
    fprintf(stderr, "piece_index error: piece out of range (%d), square=%d\n", piece, sq);
    abort();
  }
  if (sq < 0 || sq > 63) {
    fprintf(stderr, "piece_index error: square out of range (%d), piece=%d\n", sq, piece);
    abort();
  }

#endif

  return (((piece << 6) | sq) << NET_H1_SHIFT);

}

/*}}}*/
/*{{{  init_weights*/

/*{{{  flip_index*/
//
// As defined by bullet.
// https://github.com/jw1912/bullet/blob/main/docs/1-basics.md
// See also https://github.com/op12no2/clozza/wiki/bullet-notes
//

int flip_index(const int index) {

  int piece  = index / (64 * NET_H1_SIZE);
  int square = (index / NET_H1_SIZE) % 64;
  int h      = index % NET_H1_SIZE;

  square ^= 56;
  piece = (piece + 6) % 12;

  return ((piece * 64) + square) * NET_H1_SIZE + h;

}

/*}}}*/
/*{{{  read_file*/

uint8_t *read_file(const char *path, size_t *size_out) {

    *size_out = 0;
    FILE *f = fopen(path, "rb");

    if (!f)
      return NULL;

    if (fseek(f, 0, SEEK_END) != 0) {
      fclose(f);
      return NULL;
    }

    long sz = ftell(f);
    if (sz < 0) {
      fclose(f);
      return NULL;
    }
    rewind(f);

    uint8_t *buf = (uint8_t*)malloc((size_t)sz);
    if (!buf) {
      fclose(f);
      return NULL;
    }

    size_t got = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    if (got != (size_t)sz) {
      free(buf);
      return NULL;
    }

    *size_out = (size_t)sz;

    return buf;

}

/*}}}*/
/*{{{  get_weights*/

int get_weights(const char *path, int16_t **out, size_t *count_out) {

  size_t bytes = 0;

  uint8_t *raw = read_file(path, &bytes);
  if (!raw)
    return 0;

  if (bytes % sizeof(int16_t) != 0) {
    free(raw);
    return 0;
  }

  int16_t *w = (int16_t*)raw;
  size_t count = bytes / sizeof(int16_t);

  *out = w;           // caller must free(*out)
  *count_out = count; // number of int16 weights

  return 1;

}

/*}}}*/

int init_weights() {

  int16_t *weights = NULL;
  size_t n = 0;

  if (get_weights(NET_FILE, &weights, &n) == 0) {
    free(weights);
    fprintf(stderr, "cannot load weights file %s\n", NET_FILE);
    return 1;
  }

  size_t offset = 0;

  for (int i=0; i < NET_I_SIZE * NET_H1_SIZE; i++) {
    net_h1_w[i]             = (int32_t)weights[i];  // us
    net_h2_w[flip_index(i)] = (int32_t)weights[i];  // them
  }

  offset += NET_I_SIZE * NET_H1_SIZE;
  for (int i=0; i < NET_H1_SIZE; i++) {
    net_h1_b[i] = (int32_t)weights[offset+i];
  }

  offset += NET_H1_SIZE;
  for (int i=0; i < NET_H1_SIZE * 2; i++) {
    net_o_w[i] = (int32_t)weights[offset+i];
  }

  offset += NET_H1_SIZE * 2;
  net_o_b = (int32_t)weights[offset];

  free(weights);

  return 0;

}

/*}}}*/
/*{{{  net_copy*/

void net_copy(const Node *const from_node, Node *const to_node) {

  memcpy(to_node->acc1, from_node->acc1, sizeof from_node->acc1);
  memcpy(to_node->acc2, from_node->acc2, sizeof from_node->acc2);

}

/*}}}*/
/*{{{  net_rebuild_accs*/

void net_rebuild_accs(Node *const node) {

  memcpy(node->acc1, net_h1_b, sizeof net_h1_b);
  memcpy(node->acc2, net_h1_b, sizeof net_h1_b);

  for (int sq=0; sq < 64; sq++) {

    const int piece = node->pos.board[sq];

    if (piece == EMPTY)
      continue;

    for (int h=0; h < NET_H1_SIZE; h++) {
      const int idx1 = (piece * 64 + sq) * NET_H1_SIZE + h;
      node->acc1[h] += net_h1_w[idx1];
      node->acc2[h] += net_h2_w[idx1];
    }
  }

}

/*}}}*/
/*{{{  net_check*/

void net_check(Node *const n1) {

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
/*{{{  net_eval*/

int32_t sqrelu(const int32_t x) {
  const int32_t y = x & ~(x >> 31);
  return y * y;
}

int32_t net_eval(Node *const node) {

  const int stm = node->pos.stm;

  const int32_t *const a1 = (stm == 0 ? node->acc1 : node->acc2);
  const int32_t *const a2 = (stm == 0 ? node->acc2 : node->acc1);

  const int32_t *const w1 = &net_o_w[0];
  const int32_t *const w2 = &net_o_w[NET_H1_SIZE];

  int32_t acc = 0;

  for (int i=0; i < NET_H1_SIZE; i++) {  // vec eval
    acc += w1[i] * sqrelu(a1[i]) + w2[i] * sqrelu(a2[i]);
  }

  acc /= NET_QA;
  acc += net_o_b;
  acc *= NET_SCALE;
  acc /= NET_QAB;

  return acc;

}

/*}}}*/

/*{{{  net_move*/

void net_move(Node *const node) {

  const int fr_piece = ue_arg0;
  const int fr       = ue_arg1;
  const int to       = ue_arg2;

  int32_t *const a1 = node->acc1;
  int32_t *const a2 = node->acc2;

  const int b1 = base(fr_piece, fr);
  const int b2 = base(fr_piece, to);

  const int32_t *const w1_b1 = &net_h1_w[b1];
  const int32_t *const w1_b2 = &net_h1_w[b2];

  const int32_t *const w2_b1 = &net_h2_w[b1];
  const int32_t *const w2_b2 = &net_h2_w[b2];

  for (int i=0; i < NET_H1_SIZE; i++) {  // vec move
    a1[i] += w1_b2[i] - w1_b1[i];
    a2[i] += w2_b2[i] - w2_b1[i];
  }

}

/*}}}*/
/*{{{  net_capture*/

void net_capture(Node *const node) {

  const int fr_piece = ue_arg0;
  const int fr       = ue_arg1;
  const int to       = ue_arg2;
  const int to_piece = ue_arg3;

  int32_t *const a1 = node->acc1;
  int32_t *const a2 = node->acc2;

  const int b1 = base(fr_piece, fr);
  const int b2 = base(to_piece, to);
  const int b3 = base(fr_piece, to);

  const int32_t *const w1_b1 = &net_h1_w[b1];
  const int32_t *const w1_b2 = &net_h1_w[b2];
  const int32_t *const w1_b3 = &net_h1_w[b3];

  const int32_t *const w2_b1 = &net_h2_w[b1];
  const int32_t *const w2_b2 = &net_h2_w[b2];
  const int32_t *const w2_b3 = &net_h2_w[b3];

  for (int i=0; i < NET_H1_SIZE; i++) {  // vec cap
    a1[i] += w1_b3[i] - w1_b2[i] - w1_b1[i];
    a2[i] += w2_b3[i] - w2_b2[i] - w2_b1[i];
  }

}

/*}}}*/
/*{{{  net_promote*/

void net_promote (Node *const node) {

  const int pawn_piece    = ue_arg0;
  const int pawn_fr       = ue_arg1;
  const int pawn_to       = ue_arg2;
  const int capture_piece = ue_arg3;
  const int promote_piece = ue_arg4;

  int32_t *const a1 = node->acc1;
  int32_t *const a2 = node->acc2;

  const int b1 = base(pawn_piece, pawn_fr);
  const int b2 = base(promote_piece, pawn_to);

  const int32_t *const w1_b1 = &net_h1_w[b1];
  const int32_t *const w1_b2 = &net_h1_w[b2];

  const int32_t *const w2_b1 = &net_h2_w[b1];
  const int32_t *const w2_b2 = &net_h2_w[b2];

  if (capture_piece != EMPTY) {

    const int b3 = base(capture_piece, pawn_to);
    const int32_t *const w1_b3 = &net_h1_w[b3];
    const int32_t *const w2_b3 = &net_h2_w[b3];

    for (int i=0; i < NET_H1_SIZE; i++) {  // vec prom cap
      a1[i] += w1_b2[i] - w1_b1[i] - w1_b3[i];
      a2[i] += w2_b2[i] - w2_b1[i] - w2_b3[i];
    }

  }

  else {

    for (int i=0; i < NET_H1_SIZE; i++) {  // vec prom push
      a1[i] += w1_b2[i] - w1_b1[i];
      a2[i] += w2_b2[i] - w2_b1[i];
    }

  }
}

/*}}}*/
/*{{{  net_ep_capture*/

void net_ep_capture (Node *const node) {

  const int pawn_piece     = ue_arg0;
  const int pawn_fr        = ue_arg1;
  const int pawn_to        = ue_arg2;
  const int opp_pawn_piece = ue_arg3;
  const int opp_pawn_sq    = ue_arg4;

  int32_t *const a1 = node->acc1;
  int32_t *const a2 = node->acc2;

  const int b1 = base(pawn_piece,     pawn_fr);
  const int b2 = base(pawn_piece,     pawn_to);
  const int b3 = base(opp_pawn_piece, opp_pawn_sq);

  const int32_t *const w1_b1 = &net_h1_w[b1];
  const int32_t *const w1_b2 = &net_h1_w[b2];
  const int32_t *const w1_b3 = &net_h1_w[b3];

  const int32_t *const w2_b1 = &net_h2_w[b1];
  const int32_t *const w2_b2 = &net_h2_w[b2];
  const int32_t *const w2_b3 = &net_h2_w[b3];

  for (int i=0; i < NET_H1_SIZE; i++) {  // vec ep
    a1[i] += w1_b2[i] - w1_b1[i] - w1_b3[i];
    a2[i] += w2_b2[i] - w2_b1[i] - w2_b3[i];
  }

}


/*}}}*/
/*{{{  net_castle*/

void net_castle (Node *const node) {

  const int king_piece = ue_arg0;
  const int king_fr    = ue_arg1;
  const int king_to    = ue_arg2;
  const int rook_piece = ue_arg3;
  const int rook_fr    = ue_arg4;
  const int rook_to    = ue_arg5;

  int32_t *const a1 = node->acc1;
  int32_t *const a2 = node->acc2;

  const int b1 = base(king_piece, king_fr);
  const int b2 = base(king_piece, king_to);
  const int b3 = base(rook_piece, rook_fr);
  const int b4 = base(rook_piece, rook_to);

  const int32_t *const w1_b1 = &net_h1_w[b1];
  const int32_t *const w1_b2 = &net_h1_w[b2];
  const int32_t *const w1_b3 = &net_h1_w[b3];
  const int32_t *const w1_b4 = &net_h1_w[b4];

  const int32_t *const w2_b1 = &net_h2_w[b1];
  const int32_t *const w2_b2 = &net_h2_w[b2];
  const int32_t *const w2_b3 = &net_h2_w[b3];
  const int32_t *const w2_b4 = &net_h2_w[b4];

  for (int i=0; i < NET_H1_SIZE; i++) {  // vec_castle
    a1[i] += w1_b2[i] - w1_b1[i] + w1_b4[i] - w1_b3[i];
    a2[i] += w2_b2[i] - w2_b1[i] + w2_b4[i] - w2_b3[i];
  }

}

/*}}}*/

/*}}}*/
/*{{{  move gen*/

/*{{{  magic_index*/

int magic_index(const uint64_t blockers, const uint64_t magic, const int shift) {

  return (int)((blockers * magic) >> shift);

}

/*}}}*/
/*{{{  get_blockers*/

void get_blockers(Attack *a, uint64_t *blockers) {

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

void find_magics(Attack attacks[64], const char *label, int verbose) {

  int total_tries = 0;

  static uint64_t *tbl = NULL;
  static unsigned *used = NULL;
  static int cap = 0;
  static unsigned stamp = 1;

  for (int sq = 0; sq < 64; sq++) {

    Attack *a = &attacks[sq];

    uint64_t blockers[a->count];
    get_blockers(a, blockers);

    if (a->count > cap) {
      free(tbl);
      free(used);
      cap = a->count;
      tbl  = (uint64_t*)malloc((size_t)cap * sizeof(uint64_t));
      used = (unsigned*)malloc((size_t)cap * sizeof(unsigned));
      if (!tbl || !used) {
        fprintf(stderr, "cannot malloc tbp or used\n");
        cleanup();
        abort();
      }
      memset(used, 0, (size_t)cap * sizeof(unsigned));
    }

    int tries = 0;

    for (;;) {

      tries++;

      if (++stamp == 0) {
        memset(used, 0, (size_t)cap * sizeof(unsigned));
        stamp = 1;
      }

      uint64_t magic = rand64() & rand64() & rand64();

      if (popcount((a->mask * magic) >> (64 - a->bits)) < a->bits - 3) // 3 found by trial and error
        continue;

      int fail = 0;

      for (int i = 0; i < a->count; i++) {

        uint64_t blocker = blockers[i];
        uint64_t attack  = a->attacks[i];

        int index = magic_index(blocker, magic, a->shift);

        if (used[index] != stamp) {
          used[index] = stamp;
          tbl[index]  = attack;
        }
        else if (tbl[index] != attack) {
          fail = 1;
          break;
        }
      }

      if (!fail) {
        a->magic = magic;

        free(a->attacks);
        a->attacks = (uint64_t*)malloc((size_t)a->count * sizeof(uint64_t));

        for (int i = 0; i < a->count; i++) {
          a->attacks[i] = (used[i] == stamp) ? tbl[i] : 0ULL;
        }

        if (verbose) {
          printf("%s sq %2d tries %8d magic 0x%016llx\n",
                 label, sq, tries, (unsigned long long)a->magic);
        }

        total_tries += tries;
        break;
      }
    }
  }

  if (verbose) {
    printf("%s total_tries %d\n", label, total_tries);
  }

  free(tbl);  tbl = NULL;
  free(used); used = NULL;
  cap = 0;

  (void)label;

}

/*}}}*/

/*{{{  init_pawn_attacks*/

// these are attacks *to* the sq and used in pawn_gen (ep) and is_attacked

void init_pawn_attacks(void) {

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

void init_knight_attacks(void) {

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

void init_bishop_attacks(void) {

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

    a->attacks = malloc(a->count * sizeof(uint64_t));
    if (!a->attacks) {
      fprintf(stderr, "malloc failed for bishop_attacks[%d]\n", sq);
      cleanup();
      exit(1);
    }

    uint64_t blockers[a->count];
    get_blockers(a, blockers);

    for (int i = 0; i < a->count; i++) {
      /*{{{  build attacks[i]*/
      
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
      
      a->attacks[i] = attack;
      
      /*}}}*/
    }
  }

  find_magics(bishop_attacks, "B", 0);

}

/*}}}*/
/*{{{  init_rook_attacks*/

void init_rook_attacks(void) {

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

    a->attacks = malloc(a->count * sizeof(uint64_t));
    if (!a->attacks) {
      fprintf(stderr, "malloc failed for attacks[%d]\n", sq);
      cleanup();
      exit(1);
    }

    uint64_t blockers[a->count];
    get_blockers(a, blockers);

    for (int i = 0; i < a->count; i++) {
      /*{{{  build attacks[i]*/
      
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
      
      a->attacks[i] = attack;
      
      /*}}}*/
    }
  }

  find_magics(rook_attacks, "R", 0);

}

/*}}}*/
/*{{{  init_king_attacks*/

void init_king_attacks(void) {

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

/*{{{  gen_pawns*/

/*{{{  gen_pawns_white_quiet*/

void gen_pawns_white_quiet(Node *const node) {

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

void gen_pawns_white_noisy(Node *const node) {

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

void gen_pawns_black_quiet(Node *const node) {

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

void gen_pawns_black_noisy(Node *const node) {

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

void gen_pawns_quiet(Node *const node) {
  if (node->pos.stm == WHITE)
    gen_pawns_white_quiet(node);
  else
    gen_pawns_black_quiet(node);
}

void gen_pawns_noisy(Node *const node) {
  if (node->pos.stm == WHITE)
    gen_pawns_white_noisy(node);
  else
    gen_pawns_black_noisy(node);
}

/*}}}*/
/*{{{  gen_jumpers*/

void gen_jumpers(Node *const node, const uint64_t *const attack_table, const int piece, const uint64_t targets) {

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

void gen_sliders(Node *const node, Attack *const attack_table, const int piece, const uint64_t targets) {

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

void gen_castling(Node *const node) {

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

/*{{{  *_next_search_move*/

void init_next_search_move(Node *const node, const uint8_t in_check) {

  bump_nodes();

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
/*{{{  *_next_qsearch_move*/

void init_next_qsearch_move(Node *const node) {

  bump_nodes();

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

uint32_t get_next_qsearch_move(Node *const node) {

  if (node->next_move == node->num_moves)
    return 0;

  return node->moves[node->next_move++];

}

/*}}}*/

/*}}}*/
/*{{{  board*/

/*{{{  init_zob*/

void init_zob() {

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
/*{{{  eval*/

int eval(Node *const node) {

  const Position *const pos = &node->pos;
  const uint64_t *const a = pos->all;
  const int num_pieces = popcount(pos->occupied);

  if (num_pieces == 2)
    return 0;

  else if (num_pieces == 3 && (a[WKNIGHT] | a[BKNIGHT] | a[WBISHOP] | a[BBISHOP]))
    return 0;

  return net_eval(node);

}

/*}}}*/
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
/*{{{  make_move*/

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
  /*{{{  ue*/
  
  ue_func = net_move;  // assume a move - update as needed
  
  ue_arg0 = from_piece;
  ue_arg1 = from;
  ue_arg2 = to;
  
  /*}}}*/

  if (to_piece != EMPTY) {
    /*{{{  remove to piece*/
    
    all[to_piece] &= ~to_bb;
    colour[opp]   &= ~to_bb;
    
    hash ^= zob_pieces[zob_index(to_piece, to)];
    
    ue_func = net_capture;
    
    ue_arg3 = to_piece;
    
    /*}}}*/
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
      
      ue_func = net_promote;
      
      //ue_arg0 = from_piece;
      //ue_arg1 = from;
      //ue_arg2 = to;
      ue_arg3 = to_piece;
      ue_arg4 = pro;
      
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
      
      ue_func = net_ep_capture;
      
      //ue_arg0 = from_piece;
      //ue_arg1 = from;
      //ue_arg2 = to;
      ue_arg3 = opp_pawn;
      ue_arg4 = pawn_sq;
      
      /*}}}*/
    }
    
    else if (move & FLAG_PAWN_PUSH) {
      /*{{{  set ep*/
      
      ep = from + orth_offset[stm];
      
      hash ^= zob_ep[ep];
      
      //ue_func = net_move;
      
      //ue_arg0 = from_piece;
      //ue_arg1 = from;
      //ue_arg2 = to;
      
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
      
      ue_func = net_castle;
      
      //ue_arg0 = from_piece;
      //ue_arg1 = from;
      //ue_arg2 = to;
      ue_arg3 = rook;
      ue_arg4 = rook_from_sq;
      ue_arg5 = rook_to_sq;
      
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
/*{{{  play_move*/

void play_move(Node *const node, char *uci_move) {

  char buf[6];

  Position *const pos = &node->pos;
  const int stm = pos->stm;
  const int opp = stm ^ 1;
  const int stm_king_sq = piece_index(KING, stm);
  const int in_check = is_attacked(pos, bsf(pos->all[stm_king_sq]), opp);
  uint32_t move;

  init_next_search_move(node, in_check);

  while ((move = get_next_search_move(node))) {

    format_move(move, buf);
    if (!strcmp(uci_move, buf)) {
      make_move(pos, move);
      return;
    }
  }

  fprintf(stderr, "cannot find uci move %s\n", uci_move);

}

/*}}}*/

/*}}}*/
/*{{{  search*/

/*{{{  perft*/

uint64_t perft(const int ply, const int depth) {

  if (depth == 0)
    return 1;

  Node *const this_node = &ss[ply];
  Node *const next_node = &ss[ply+1];

  const Position *const this_pos = &this_node->pos;
  Position *const next_pos = &next_node->pos;

  const int stm = this_pos->stm;
  const int opp = stm ^ 1;
  const int stm_king_sq = piece_index(KING, stm);

  uint64_t tot_nodes = 0;
  uint32_t move;

  init_next_search_move(this_node, is_attacked(this_pos, bsf(this_pos->all[stm_king_sq]), opp));

  const uint64_t *const next_stm_king_ptr = &next_pos->all[stm_king_sq];

  while ((move = get_next_search_move(this_node))) {

    *next_pos = *this_pos;      // copy
    make_move(next_pos, move);  // make

    if (is_attacked(next_pos, bsf(*next_stm_king_ptr), opp))
      continue;

    if (__update_accs_in_perft || __check_everything_in_perft) {
      net_copy(this_node, next_node);
      ue_func(next_node);
    }
    if (__check_everything_in_perft) {
      net_check(next_node);
    }

    tot_nodes += perft(ply+1, depth-1);

  }

  return tot_nodes;

}

/*}}}*/
/*{{{  qsearch*/

int qsearch(const int ply, int alpha, const int beta) {

  if (ply == MAX_PLY) {
    tc.finished = 1;
    return 0;
  }

  if (check_tc())
    return 0;

  Node *const this_node = &ss[ply];
  const Position *const this_pos = &this_node->pos;

  const int stand_pat = eval(this_node);
  if (stand_pat >= beta)
    return beta;
  if (stand_pat > alpha)
    alpha = stand_pat;

  Node *const next_node = &ss[ply+1];
  Position *const next_pos = &next_node->pos;

  const int stm = this_pos->stm;
  const int opp = stm ^ 1;
  const int stm_king_sq = piece_index(KING, stm);

  uint32_t move;

  init_next_qsearch_move(this_node);

  const uint64_t *const next_stm_king_ptr = &next_pos->all[stm_king_sq];

  while ((move = get_next_qsearch_move(this_node))) {

    *next_pos = *this_pos;      // pos copy
    make_move(next_pos, move);  // pos update

    if (is_attacked(next_pos, bsf(*next_stm_king_ptr), opp))
      continue;

    net_copy(this_node, next_node);   // acc copy
    ue_func(next_node);               // acc update

    if (__check_everything_in_qsearch) {
      net_check(next_node);
    }

    const int score = -qsearch(ply+1, -beta, -alpha);

    if (score >= beta)
      return beta;

    if (score > alpha) {
      alpha = score;
    }
  }

  return alpha;

}

/*}}}*/
/*{{{  search*/

int search(const int ply, int depth, int alpha, const int beta) {

  if (ply == MAX_PLY) {
    tc.finished = 1;
    return 0;
  }

  if (check_tc())
    return 0;

  Node *const this_node = &ss[ply];
  Node *const next_node = &ss[ply + 1];

  const Position *const this_pos = &this_node->pos;
  Position *const next_pos = &next_node->pos;

  const int stm = this_pos->stm;
  const int opp = stm ^ 1;
  const int stm_king_sq = piece_index(KING, stm);
  const int in_check = is_attacked(this_pos, bsf(this_pos->all[stm_king_sq]), opp);

  if (depth <= 0 && in_check == 0) {
    const int qs = qsearch(ply, alpha, beta);
    return qs;
  }
  depth = MAX(depth, 0);

  uint32_t move;
  int score = -INF;
  int num_legal_moves = 0;

  init_next_search_move(this_node, in_check);

  const uint64_t *const next_stm_king_ptr = &next_pos->all[stm_king_sq];

  while ((move = get_next_search_move(this_node))) {

    *next_pos = *this_pos;            // pos copy
    make_move(next_pos, move);        // pos update

    if (is_attacked(next_pos, bsf(*next_stm_king_ptr), opp))
      continue;

    net_copy(this_node, next_node);   // acc copy
    ue_func(next_node);               // acc update

    if (__check_everything_in_search) {
      net_check(next_node);
    }

    num_legal_moves++;

    if (ply == 0 && tc.bm == 0)
      tc.bm = move;

    score = -search(ply+1, depth-1, -beta, -alpha);

    if (tc.finished)
      return 0;

    if (score > alpha) {
      alpha = score;
      if (ply == 0) {
        tc.bm = move;
        tc.bs = score;
      }
      if (score >= beta) {
        return score;
      }
    }
  }

  if (ply == 0 && num_legal_moves == 1) {
    tc.finished = 1;
  }

  if (num_legal_moves == 0) {
    return this_node->in_check ? (-MATE + ply) : 0;
  }

  return alpha;

}

/*}}}*/
/*{{{  go*/

void go(int max_ply) {

  int alpha = 0;
  int beta  = 0;
  int score = 0;
  int depth = 0;

  char bm_str[6];

  for (int ply=1; ply <= max_ply; ply++) {

    alpha = -INF;
    beta  = INF;

    depth = ply;

    while (1) {
      score = search(0, depth, alpha, beta);
      break;
    }

    format_move(tc.bm, bm_str);
    if (!tc.quiet)
      printf("info depth %d score %d nodes %" PRIu64 " pv %s\n", depth, tc.bs, tc.nodes, bm_str);

    if (tc.finished)
      break;

  }

  format_move(tc.bm, bm_str);
  if (!tc.quiet)
    printf("bestmove %s\n", bm_str);

}

/*}}}*/

/*}}}*/
/*{{{  uci*/

/*{{{  uci_tokens*/

/*{{{  perft fens*/

static const Perft perft_tests[] = {

  {"p f rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR             w KQkq -  0 1", 0, 1,         "cpw-pos1-0"},
  {"p f rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR             w KQkq -  0 1", 1, 20,        "cpw-pos1-1"},
  {"p f rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR             w KQkq -  0 1", 2, 400,       "cpw-pos1-2"},
  {"p f rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR             w KQkq -  0 1", 3, 8902,      "cpw-pos1-3"},
  {"p f rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR             w KQkq -  0 1", 4, 197281,    "cpw-pos1-4"},
  {"p f rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR             w KQkq -  0 1", 5, 4865609,   "cpw-pos1-5"},
  {"p f rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR             w KQkq -  0 1", 6, 119060324, "cpw-pos1-6"},
  {"p f rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR             w KQkq -  0 1", 7, 3195901860,"cpw-pos1-6"},
  {"p f 4k3/8/8/8/8/8/R7/R3K2R                                  w Q    -  0 1", 3, 4729,      "castling-2"},
  {"p f 4k3/8/8/8/8/8/R7/R3K2R                                  w K    -  0 1", 3, 4686,      "castling-3"},
  {"p f 4k3/8/8/8/8/8/R7/R3K2R                                  w -    -  0 1", 3, 4522,      "castling-4"},
  {"p f r3k2r/r7/8/8/8/8/8/4K3                                  b kq   -  0 1", 3, 4893,      "castling-5"},
  {"p f r3k2r/r7/8/8/8/8/8/4K3                                  b q    -  0 1", 3, 4729,      "castling-6"},
  {"p f r3k2r/r7/8/8/8/8/8/4K3                                  b k    -  0 1", 3, 4686,      "castling-7"},
  {"p f r3k2r/r7/8/8/8/8/8/4K3                                  b -    -  0 1", 3, 4522,      "castling-8"},
  {"p f rnbqkb1r/pp1p1ppp/2p5/4P3/2B5/8/PPP1NnPP/RNBQK2R        w KQkq -  0 1", 1, 42,        "cpw-pos5-1"},
  {"p f rnbqkb1r/pp1p1ppp/2p5/4P3/2B5/8/PPP1NnPP/RNBQK2R        w KQkq -  0 1", 2, 1352,      "cpw-pos5-2"},
  {"p f rnbqkb1r/pp1p1ppp/2p5/4P3/2B5/8/PPP1NnPP/RNBQK2R        w KQkq -  0 1", 3, 53392,     "cpw-pos5-3"},
  {"p f r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -  0 1", 1, 48,        "cpw-pos2-1"},
  {"p f r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -  0 1", 2, 2039,      "cpw-pos2-2"},
  {"p f r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -  0 1", 3, 97862,     "cpw-pos2-3"},
  {"p f 8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8                         w -    -  0 1", 5, 674624,    "cpw-pos3-5"},
  {"p f n1n5/PPPk4/8/8/8/8/4Kppp/5N1N                           b -    -  0 1", 1, 24,        "prom-1    "},
  {"p f 8/5bk1/8/2Pp4/8/1K6/8/8                                 w -    d6 0 1", 6, 824064,    "ccc-1     "},
  {"p f 8/8/1k6/8/2pP4/8/5BK1/8                                 b -    d3 0 1", 6, 824064,    "ccc-2     "},
  {"p f 8/8/1k6/2b5/2pP4/8/5K2/8                                b -    d3 0 1", 6, 1440467,   "ccc-3     "},
  {"p f 8/5k2/8/2Pp4/2B5/1K6/8/8                                w -    d6 0 1", 6, 1440467,   "ccc-4     "},
  {"p f 5k2/8/8/8/8/8/8/4K2R                                    w K    -  0 1", 6, 661072,    "ccc-5     "},
  {"p f 4k2r/8/8/8/8/8/8/5K2                                    b k    -  0 1", 6, 661072,    "ccc-6     "},
  {"p f 3k4/8/8/8/8/8/8/R3K3                                    w Q    -  0 1", 6, 803711,    "ccc-7     "},
  {"p f r3k3/8/8/8/8/8/8/3K4                                    b q    -  0 1", 6, 803711,    "ccc-8     "},
  {"p f r3k2r/1b4bq/8/8/8/8/7B/R3K2R                            w KQkq -  0 1", 4, 1274206,   "ccc-9     "},
  {"p f r3k2r/7b/8/8/8/8/1B4BQ/R3K2R                            b KQkq -  0 1", 4, 1274206,   "ccc-10    "},
  {"p f r3k2r/8/3Q4/8/8/5q2/8/R3K2R                             b KQkq -  0 1", 4, 1720476,   "ccc-11    "},
  {"p f r3k2r/8/5Q2/8/8/3q4/8/R3K2R                             w KQkq -  0 1", 4, 1720476,   "ccc-12    "},
  {"p f 2K2r2/4P3/8/8/8/8/8/3k4                                 w -    -  0 1", 6, 3821001,   "ccc-13    "},
  {"p f 3K4/8/8/8/8/8/4p3/2k2R2                                 b -    -  0 1", 6, 3821001,   "ccc-14    "},
  {"p f 8/8/1P2K3/8/2n5/1q6/8/5k2                               b -    -  0 1", 5, 1004658,   "ccc-15    "},
  {"p f 5K2/8/1Q6/2N5/8/1p2k3/8/8                               w -    -  0 1", 5, 1004658,   "ccc-16    "},
  {"p f 4k3/1P6/8/8/8/8/K7/8                                    w -    -  0 1", 6, 217342,    "ccc-17    "},
  {"p f 8/k7/8/8/8/8/1p6/4K3                                    b -    -  0 1", 6, 217342,    "ccc-18    "},
  {"p f 8/P1k5/K7/8/8/8/8/8                                     w -    -  0 1", 6, 92683,     "ccc-19    "},
  {"p f 8/8/8/8/8/k7/p1K5/8                                     b -    -  0 1", 6, 92683,     "ccc-20    "},
  {"p f K1k5/8/P7/8/8/8/8/8                                     w -    -  0 1", 6, 2217,      "ccc-21    "},
  {"p f 8/8/8/8/8/p7/8/k1K5                                     b -    -  0 1", 6, 2217,      "ccc-22    "},
  {"p f 8/k1P5/8/1K6/8/8/8/8                                    w -    -  0 1", 7, 567584,    "ccc-23    "},
  {"p f 8/8/8/8/1k6/8/K1p5/8                                    b -    -  0 1", 7, 567584,    "ccc-24    "},
  {"p f 8/8/2k5/5q2/5n2/8/5K2/8                                 b -    -  0 1", 4, 23527,     "ccc-25    "},
  {"p f 8/5k2/8/5N2/5Q2/2K5/8/8                                 w -    -  0 1", 4, 23527,     "ccc-26    "},
  {"p f 8/p7/8/1P6/K1k3p1/6P1/7P/8                              w -    -  0 1", 8, 8103790,   "jvm-7     "},
  {"p f n1n5/PPPk4/8/8/8/8/4Kppp/5N1N                           b -    -  0 1", 6, 71179139,  "jvm-8     "},
  {"p f r3k2r/p6p/8/B7/1pp1p3/3b4/P6P/R3K2R                     w KQkq -  0 1", 6, 77054993,  "jvm-9     "},
  {"p f 8/5p2/8/2k3P1/p3K3/8/1P6/8                              b -    -  0 1", 8, 64451405,  "jvm-11    "},
  {"p f r3k2r/pb3p2/5npp/n2p4/1p1PPB2/6P1/P2N1PBP/R3K2R         w KQkq -  0 1", 5, 29179893,  "jvm-12    "},
  {"p f 8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8                         w -    -  0 1", 7, 178633661, "jvm-10    "},
  {"p f r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -  0 1", 5, 193690690, "jvm-6     "},
  {"p f 8/2pkp3/8/RP3P1Q/6B1/8/2PPP3/rb1K1n1r                   w -    -  0 1", 6, 181153194, "ob1       "},
  {"p f rnbqkb1r/ppppp1pp/7n/4Pp2/8/8/PPPP1PPP/RNBQKBNR         w KQkq f6 0 1", 6, 244063299, "jvm-5     "},
  {"p f 8/2ppp3/8/RP1k1P1Q/8/8/2PPP3/rb1K1n1r                   w -    -  0 1", 6, 205552081, "ob2       "},
  {"p f 8/8/3q4/4r3/1b3n2/8/3PPP2/2k1K2R                        w K    -  0 1", 6, 207139531, "ob3       "},
  {"p f 4r2r/RP1kP1P1/3P1P2/8/8/3ppp2/1p4p1/4K2R                b K    -  0 1", 6, 314516438, "ob4       "},
  {"p f r3k2r/8/8/8/3pPp2/8/8/R3K1RR                            b KQkq e3 0 1", 6, 485647607, "jvm-1     "},
  {"p f 8/3K4/2p5/p2b2r1/5k2/8/8/1q6                            b -    -  0 1", 7, 493407574, "jvm-4     "},
  {"p f r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1   w kq   -  0 1", 6, 706045033, "jvm-2     "},
  {"p f r6r/1P4P1/2kPPP2/8/8/3ppp2/1p4p1/R3K2R                  w KQ   -  0 1", 6, 975944981, "ob5       "}

};

/*}}}*/
/*{{{  bench fens*/

static const char *const bench_fens[] = {

  "p fen r3k2r/2pb1ppp/2pp1q2/p7/1nP1B3/1P2P3/P2N1PPP/R2QK2R w KQkq a6 0 14",
  "p fen 4rrk1/2p1b1p1/p1p3q1/4p3/2P2n1p/1P1NR2P/PB3PP1/3R1QK1 b - - 2 24",
  "p fen r3qbrk/6p1/2b2pPp/p3pP1Q/PpPpP2P/3P1B2/2PB3K/R5R1 w - - 16 42",
  "p fen 6k1/1R3p2/6p1/2Bp3p/3P2q1/P7/1P2rQ1K/5R2 b - - 4 44",
  "p fen 8/8/1p2k1p1/3p3p/1p1P1P1P/1P2PK2/8/8 w - - 3 54",
  "p fen 7r/2p3k1/1p1p1qp1/1P1Bp3/p1P2r1P/P7/4R3/Q4RK1 w - - 0 36",
  "p fen r1bq1rk1/pp2b1pp/n1pp1n2/3P1p2/2P1p3/2N1P2N/PP2BPPP/R1BQ1RK1 b - - 2 10",
  "p fen 3r3k/2r4p/1p1b3q/p4P2/P2Pp3/1B2P3/3BQ1RP/6K1 w - - 3 87",
  "p fen 2r4r/1p4k1/1Pnp4/3Qb1pq/8/4BpPp/5P2/2RR1BK1 w - - 0 42",
  "p fen 4q1bk/6b1/7p/p1p4p/PNPpP2P/KN4P1/3Q4/4R3 b - - 0 37",
  "p fen 2q3r1/1r2pk2/pp3pp1/2pP3p/P1Pb1BbP/1P4Q1/R3NPP1/4R1K1 w - - 2 34",
  "p fen 1r2r2k/1b4q1/pp5p/2pPp1p1/P3Pn2/1P1B1Q1P/2R3P1/4BR1K b - - 1 37",
  "p fen r3kbbr/pp1n1p1P/3ppnp1/q5N1/1P1pP3/P1N1B3/2P1QP2/R3KB1R b KQkq b3 0 17",
  "p fen 8/6pk/2b1Rp2/3r4/1R1B2PP/P5K1/8/2r5 b - - 16 42",
  "p fen 1r4k1/4ppb1/2n1b1qp/pB4p1/1n1BP1P1/7P/2PNQPK1/3RN3 w - - 8 29",
  "p fen 8/p2B4/PkP5/4p1pK/4Pb1p/5P2/8/8 w - - 29 68",
  "p fen 3r4/ppq1ppkp/4bnp1/2pN4/2P1P3/1P4P1/PQ3PBP/R4K2 b - - 2 20",
  "p fen 5rr1/4n2k/4q2P/P1P2n2/3B1p2/4pP2/2N1P3/1RR1K2Q w - - 1 49",
  "p fen 1r5k/2pq2p1/3p3p/p1pP4/4QP2/PP1R3P/6PK/8 w - - 1 51",
  "p fen q5k1/5ppp/1r3bn1/1B6/P1N2P2/BQ2P1P1/5K1P/8 b - - 2 34",
  "p fen r1b2k1r/5n2/p4q2/1ppn1Pp1/3pp1p1/NP2P3/P1PPBK2/1RQN2R1 w - - 0 22",
  "p fen r1bqk2r/pppp1ppp/5n2/4b3/4P3/P1N5/1PP2PPP/R1BQKB1R w KQkq - 0 5",
  "p fen r1bqr1k1/pp1p1ppp/2p5/8/3N1Q2/P2BB3/1PP2PPP/R3K2n b Q - 1 12",
  "p fen r1bq2k1/p4r1p/1pp2pp1/3p4/1P1B3Q/P2B1N2/2P3PP/4R1K1 b - - 2 19",
  "p fen r4qk1/6r1/1p4p1/2ppBbN1/1p5Q/P7/2P3PP/5RK1 w - - 2 25",
  "p fen r7/6k1/1p6/2pp1p2/7Q/8/p1P2K1P/8 w - - 0 32",
  "p fen r3k2r/ppp1pp1p/2nqb1pn/3p4/4P3/2PP4/PP1NBPPP/R2QK1NR w KQkq - 1 5",
  "p fen 3r1rk1/1pp1pn1p/p1n1q1p1/3p4/Q3P3/2P5/PP1NBPPP/4RRK1 w - - 0 12",
  "p fen 5rk1/1pp1pn1p/p3Brp1/8/1n6/5N2/PP3PPP/2R2RK1 w - - 2 20",
  "p fen 8/1p2pk1p/p1p1r1p1/3n4/8/5R2/PP3PPP/4R1K1 b - - 3 27",
  "p fen 8/4pk2/1p1r2p1/p1p4p/Pn5P/3R4/1P3PP1/4RK2 w - - 1 33",
  "p fen 8/5k2/1pnrp1p1/p1p4p/P6P/4R1PK/1P3P2/4R3 b - - 1 38",
  "p fen 8/8/1p1kp1p1/p1pr1n1p/P6P/1R4P1/1P3PK1/1R6 b - - 15 45",
  "p fen 8/8/1p1k2p1/p1prp2p/P2n3P/6P1/1P1R1PK1/4R3 b - - 5 49",
  "p fen 8/8/1p4p1/p1p2k1p/P2npP1P/4K1P1/1P6/3R4 w - - 6 54",
  "p fen 8/8/1p4p1/p1p2k1p/P2n1P1P/4K1P1/1P6/6R1 b - - 6 59",
  "p fen 8/5k2/1p4p1/p1pK3p/P2n1P1P/6P1/1P6/4R3 b - - 14 63",
  "p fen 8/1R6/1p1K1kp1/p6p/P1p2P1P/6P1/1Pn5/8 w - - 0 67",
  "p fen 1rb1rn1k/p3q1bp/2p3p1/2p1p3/2P1P2N/PP1RQNP1/1B3P2/4R1K1 b - - 4 23",
  "p fen 4rrk1/pp1n1pp1/q5p1/P1pP4/2n3P1/7P/1P3PB1/R1BQ1RK1 w - - 3 22",
  "p fen r2qr1k1/pb1nbppp/1pn1p3/2ppP3/3P4/2PB1NN1/PP3PPP/R1BQR1K1 w - - 4 12",
  "p fen 2r2k2/8/4P1R1/1p6/8/P4K1N/7b/2B5 b - - 0 55",
  "p fen 6k1/5pp1/8/2bKP2P/2P5/p4PNb/B7/8 b - - 1 44",
  "p fen 2rqr1k1/1p3p1p/p2p2p1/P1nPb3/2B1P3/5P2/1PQ2NPP/R1R4K w - - 3 25",
  "p fen r1b2rk1/p1q1ppbp/6p1/2Q5/8/4BP2/PPP3PP/2KR1B1R b - - 2 14",
  "p fen 6r1/5k2/p1b1r2p/1pB1p1p1/1Pp3PP/2P1R1K1/2P2P2/3R4 w - - 1 36",
  "p fen rnbqkb1r/pppppppp/5n2/8/2PP4/8/PP2PPPP/RNBQKBNR b KQkq c3 0 2",
  "p fen 2rr2k1/1p4bp/p1q1p1p1/4Pp1n/2PB4/1PN3P1/P3Q2P/2RR2K1 w - f6 0 20",
  "p fen 3br1k1/p1pn3p/1p3n2/5pNq/2P1p3/1PN3PP/P2Q1PB1/4R1K1 w - - 0 23",
  "p fen 2r2b2/5p2/5k2/p1r1pP2/P2pB3/1P3P2/K1P3R1/7R w - - 23 93"

};

/*}}}*/

int uci_exec(char *line_in);

int uci_tokens(int num_tokens, char **tokens) {

  if (num_tokens == 0)
    return 0;

  if (strlen(tokens[0]) == 0)
    return 0;

  const char *cmd = tokens[0];
  const char *sub = tokens[num_tokens > 1 ? 1 : 0];

  if (!strcmp(cmd, "isready")) {
    /*{{{  isready*/
    
    printf("readyok\n");
    
    /*}}}*/
  }
  else if (!strcmp(cmd, "position") || !strcmp(cmd, "p")) {
    /*{{{  position*/
    
    if (!strcmp(sub, "startpos") || !strcmp(sub, "s"))
      position(&ss[0], "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR", "w", "KQkq", "-");
    
    else if (!strcmp(sub, "fen") || !strcmp(sub, "f") )
      position(&ss[0], tokens[2], tokens[3], tokens[4], tokens[5]);
    
    int n = find_token("moves", num_tokens, tokens);
    if (n > 0) {
      for (int i=n+1; i < num_tokens; i++)
        play_move(&ss[0], tokens[i]);
    }
    
    /*}}}*/
  }
  else if (!strcmp(cmd, "ucinewgame") || !strcmp(cmd, "u")) {
    /*{{{  ucinewgame*/
    
    ;
    
    /*}}}*/
  }
  else if (!strcmp(cmd, "go") || !strcmp(cmd, "g")) {
    /*{{{  go*/
    
    int stm = ss[0].pos.stm;
    
    int64_t wtime = 0;
    int64_t winc = 0;
    int64_t btime = 0;
    int64_t binc = 0;
    int64_t max_nodes = 0;
    int64_t move_time = 0;
    int max_depth = MAX_PLY;
    int moves_to_go = 30;
    
    /*{{{  get the go params*/
    
    int t = 1;
    
    while (t < num_tokens - 1) {
    
      const char *token = tokens[t];
    
      if (!strcmp(token, "wtime")) {
        t++;
        wtime = MAX(0, atoi(tokens[t]));
      }
      else if (!strcmp(token, "winc")) {
        t++;
        winc = MAX(0, atoi(tokens[t]));
      }
      else if (!strcmp(token, "btime")) {
        t++;
        btime = MAX(0, atoi(tokens[t]));
      }
      else if (!strcmp(token, "binc")) {
        t++;
        binc = MAX(0, atoi(tokens[t]));
      }
      else if (!strcmp(token, "depth") || !strcmp(token, "d")) {
        t++;
        max_depth = MAX(0, atoi(tokens[t]));
      }
      else if (!strcmp(token, "nodes")) {
        t++;
        max_nodes = MAX(0, atoi(tokens[t]));
      }
      else if (!strcmp(token, "movetime") || !strcmp(token, "m")) {
        t++;
        move_time = MAX(0, atoi(tokens[t]));
      }
      else if (!strcmp(token, "movestogo")) {
        t++;
        moves_to_go = MAX(0, atoi(tokens[t]));
      }
      else if (!strcmp(token, "infinite")) {
        ;
      }
    
      t++;
    
    }
    
    /*}}}*/
    
    if (!move_time && stm == WHITE && wtime)
      move_time = MAX(1, move_time / moves_to_go + winc / 2);
    else if (!move_time && stm == BLACK && btime)
      move_time = MAX(1, move_time / moves_to_go + binc / 2);
    
    if (move_time) {
      tc.start_time  = now_ms();
      tc.finish_time = tc.start_time + move_time;
    }
    else {
      tc.start_time  = 0;
      tc.finish_time = 0;
    }
    
    tc.max_nodes = max_nodes;
    tc.max_depth = max_depth;
    
    tc.nodes    = 0;
    tc.finished = 0;
    tc.bm       = 0;
    tc.bs       = 0;
    tc.measure  = 0;
    
    go(max_depth);
    
    /*}}}*/
  }
  else if (!strcmp(cmd, "uci")) {
    /*{{{  uci*/
    
    printf("id name Naddu8\n");
    printf("id author ColinJenkins\n");
    printf("uciok\n");
    
    /*}}}*/
  }
  else if (!strcmp(cmd, "b")) {
    /*{{{  board*/
    
    print_board(&ss[0]);
    
    /*}}}*/
  }
  else if (!strcmp(cmd, "e")) {
    /*{{{  eval*/
    
    const int e = eval(&ss[0]);
    
    printf("%d\n", e);
    
    /*}}}*/
  }
  else if (!strcmp(cmd, "perft") || !strcmp(cmd, "f")) {
    /*{{{  perft*/
    
    const int depth = atoi(sub);
    uint64_t start_ms = now_ms();
    uint64_t total_nodes = 0;
    
    uint64_t num_nodes = perft(0, depth);
    total_nodes += num_nodes;
    
    uint64_t end_ms     = now_ms();
    uint64_t elapsed_ms = MAX(1, end_ms - start_ms);
    uint64_t nps        = (total_nodes * 1000ULL) / elapsed_ms;
    
    printf("time %" PRIu64 " nodes %" PRIu64 " nps %" PRIu64 "\n", elapsed_ms, total_nodes, nps);
    
    /*}}}*/
  }
  else if (!strcmp(cmd, "bench")) {
    /*{{{  bench*/
    
    const int num_fens = 50;
    uint64_t start_ms = now_ms();
    uint64_t total_nodes = 0;
    
    tc.quiet = 1;
    
    for (int i=0; i < num_fens; i++) {
    
      const char *fen = bench_fens[i];
    
      char line[UCI_LINE_LENGTH];
    
      strncpy(line, fen, sizeof(line) - 1);
      line[sizeof(line) - 1] = '\0';
      uci_exec(line);
    
      strncpy(line, "go depth 4", sizeof(line) - 1);
      line[sizeof(line) - 1] = '\0';
      uci_exec(line);
    
      total_nodes += tc.nodes;
    
    }
    
    tc.quiet = 0;
    
    uint64_t end_ms     = now_ms();
    uint64_t elapsed_ms = MAX(1, end_ms - start_ms);
    uint64_t nps        = (total_nodes * 1000ULL) / elapsed_ms;
    
    printf("time %" PRIu64 " nodes %" PRIu64 " nps %" PRIu64 "\n", elapsed_ms, total_nodes, nps);
    
    /*}}}*/
  }
  else if (!strcmp(cmd, "pt")) {
    /*{{{  perft tests*/
    
    const int num_tests = 65;
    uint64_t start_ms = now_ms();
    uint64_t total_nodes = 0;
    int errors = 0;
    
    for (int i = 0; i < num_tests; i++) {
    
      const Perft *test = &perft_tests[i];
    
      char line[UCI_LINE_LENGTH];
      strncpy(line, test->fen, sizeof(line) - 1);
      line[sizeof(line) - 1] = '\0';
    
      uci_exec(line);
    
      uint64_t num_nodes = perft(0, test->depth);
      total_nodes += num_nodes;
      if (num_nodes != test->expected)
        errors = 1;
    
      printf("%s %s %d (%" PRIu64 ") %" PRIu64 " %" PRIu64 "\n",
      test->label,
      test->fen,
      test->depth,
      num_nodes - test->expected,
      num_nodes,
      test->expected);
    
    }
    
    uint64_t end_ms     = now_ms();
    uint64_t elapsed_ms = MAX(1, end_ms - start_ms);
    uint64_t nps        = (total_nodes * 1000ULL) / elapsed_ms;
    
    printf("time %" PRIu64 " nps %" PRIu64 "\n", elapsed_ms, nps);
    
    if (errors)
      printf("(errors)\n");
    
    /*}}}*/
  }
  else if (!strcmp(cmd, "q")) {
    /*{{{  quit*/
    
    return 1;
    
    /*}}}*/
  }
  else {
    printf("?|%s|\n", cmd);
  }

  return 0;

}

/*}}}*/
/*{{{  uci_exec*/

int uci_exec(char *line) {

  char *tokens[UCI_TOKENS];
  char *token;

  int num_tokens = 0;

  token = strtok(line, " \t\n");

  while (token != NULL && num_tokens < UCI_TOKENS) {

    tokens[num_tokens++] = token;
    token = strtok(NULL, " \r\t\n");

  }

  return uci_tokens(num_tokens, tokens);

}

/*}}}*/
/*{{{  uci_loop*/

void uci_loop(int argc, char **argv) {

  setvbuf(stdout, NULL, _IONBF, 0);

  char chunk[UCI_LINE_LENGTH];

  for (int i=1; i < argc; i++) {
    if (uci_exec(argv[i]))
      return;
  }

  while (fgets(chunk, sizeof(chunk), stdin) != NULL) {
    if (uci_exec(chunk))
      return;
  }

}

/*}}}*/

/*}}}*/
/*{{{  init_once*/

int init_once() {

  tc.quiet = 0;

  memset(ss, 0, sizeof(ss));

  uint64_t start_ms = now_ms();

  init_zob();

  init_pawn_attacks();
  init_knight_attacks();
  init_bishop_attacks();

  init_rook_attacks();
  init_king_attacks();

  if (init_weights())
    return 1;

  uint64_t elapsed_ms = now_ms() - start_ms;

  //printf("info init_once %" PRIu64 "ms\n", elapsed_ms);

  return 0;

}

/*}}}*/

/*{{{  main*/

int main(int argc, char **argv) {

  if (init_once()) {
    cleanup();
    return 1;
  }

  uci_loop(argc, argv);
  cleanup();

  return 0;

}

/*}}}*/


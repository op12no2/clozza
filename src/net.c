
#include "net.h"

/*{{{  fold marker*/

/*}}}*/

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

static inline int base(const int piece, const int sq) {

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
/*{{{  flip_index*/
//
// As defined by bullet.
// https://github.com/jw1912/bullet/blob/main/docs/1-basics.md
// See also https://github.com/op12no2/clozza/wiki/bullet-notes
//

static int flip_index(const int index) {

  int piece  = index / (64 * NET_H1_SIZE);
  int square = (index / NET_H1_SIZE) % 64;
  int h      = index % NET_H1_SIZE;

  square ^= 56;
  piece = (piece + 6) % 12;

  return ((piece * 64) + square) * NET_H1_SIZE + h;

}

/*}}}*/
/*{{{  read_file*/

static uint8_t *read_file(const char *path, size_t *size_out) {

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

static int get_weights(const char *path, int16_t **out, size_t *count_out) {

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
/*{{{  net_init*/

void net_init() {

  int16_t *weights = NULL;
  size_t n = 0;

  if (get_weights(NET_FILE, &weights, &n) == 0) {
    free(weights);
    fprintf(stderr, "cannot load weights file %s\n", NET_FILE);
    exit(1);
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

}

/*}}}*/

/*{{{  net_copy*/

inline void net_copy(const Node *const from_node, Node *const to_node) {

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
/*{{{  sqrrelu*/

static inline int32_t sqrelu(const int32_t x) {
  const int32_t y = x & ~(x >> 31);
  return y * y;
}

/*}}}*/
/*{{{  net_eval*/

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

static void net_move(Node *const node) {

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

static void net_capture(Node *const node) {

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

static void net_promote (Node *const node) {

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

static void net_ep_capture (Node *const node) {

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

static void net_castle (Node *const node) {

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

/*{{{  net_prepare_**/

inline void net_prepare_move(const int a0, const int a1, const int a2) {
  ue_func = net_move;
  ue_arg0 = a0;
  ue_arg1 = a1;
  ue_arg2 = a2;
}

inline void net_prepare_capture(const int a0, const int a1, const int a2, const int a3) {
  ue_func = net_capture;
  ue_arg0 = a0;
  ue_arg1 = a1;
  ue_arg2 = a2;
  ue_arg3 = a3;
}

inline void net_prepare_promote(const int a0, const int a1, const int a2, const int a3, const int a4) {
  ue_func = net_promote;
  ue_arg0 = a0;
  ue_arg1 = a1;
  ue_arg2 = a2;
  ue_arg3 = a3;
  ue_arg4 = a4;
}

inline void net_prepare_ep_capture(const int a0, const int a1, const int a2, const int a3, const int a4) {
  ue_func = net_ep_capture;
  ue_arg0 = a0;
  ue_arg1 = a1;
  ue_arg2 = a2;
  ue_arg3 = a3;
  ue_arg4 = a4;
}

inline void net_prepare_castle(const int a0, const int a1, const int a2, const int a3, const int a4, const int a5) {
  ue_func = net_castle;
  ue_arg0 = a0;
  ue_arg1 = a1;
  ue_arg2 = a2;
  ue_arg3 = a3;
  ue_arg4 = a4;
  ue_arg5 = a5;
}

/*}}}*/


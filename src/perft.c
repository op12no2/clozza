
#include "perft.h"

/*{{{  perft data*/

typedef struct {

  const char *fen;
  const char *stm;
  const char *rights;
  const char *ep;
  int depth;
  uint64_t expected;
  const char *label;

} Perft;

static const Perft perft_data[] = {

  {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR",             "w", "KQkq", "-",  0, 1,         "cpw-pos1-0"},
  {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR",             "w", "KQkq", "-",  1, 20,        "cpw-pos1-1"},
  {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR",             "w", "KQkq", "-",  2, 400,       "cpw-pos1-2"},
  {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR",             "w", "KQkq", "-",  3, 8902,      "cpw-pos1-3"},
  {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR",             "w", "KQkq", "-",  4, 197281,    "cpw-pos1-4"},
  {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR",             "w", "KQkq", "-",  5, 4865609,   "cpw-pos1-5"},
  {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR",             "w", "KQkq", "-",  6, 119060324, "cpw-pos1-6"},
  {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR",             "w", "KQkq", "-",  7, 3195901860,"cpw-pos1-6"},
  {"4k3/8/8/8/8/8/R7/R3K2R",                                  "w", "Q",    "-",  3, 4729,      "castling-2"},
  {"4k3/8/8/8/8/8/R7/R3K2R",                                  "w", "K",    "-",  3, 4686,      "castling-3"},
  {"4k3/8/8/8/8/8/R7/R3K2R",                                  "w", "-",    "-",  3, 4522,      "castling-4"},
  {"r3k2r/r7/8/8/8/8/8/4K3",                                  "b", "kq",   "-",  3, 4893,      "castling-5"},
  {"r3k2r/r7/8/8/8/8/8/4K3",                                  "b", "q",    "-",  3, 4729,      "castling-6"},
  {"r3k2r/r7/8/8/8/8/8/4K3",                                  "b", "k",    "-",  3, 4686,      "castling-7"},
  {"r3k2r/r7/8/8/8/8/8/4K3",                                  "b", "-",    "-",  3, 4522,      "castling-8"},
  {"rnbqkb1r/pp1p1ppp/2p5/4P3/2B5/8/PPP1NnPP/RNBQK2R",        "w", "KQkq", "-",  1, 42,        "cpw-pos5-1"},
  {"rnbqkb1r/pp1p1ppp/2p5/4P3/2B5/8/PPP1NnPP/RNBQK2R",        "w", "KQkq", "-",  2, 1352,      "cpw-pos5-2"},
  {"rnbqkb1r/pp1p1ppp/2p5/4P3/2B5/8/PPP1NnPP/RNBQK2R",        "w", "KQkq", "-",  3, 53392,     "cpw-pos5-3"},
  {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R", "w", "KQkq", "-",  1, 48,        "cpw-pos2-1"},
  {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R", "w", "KQkq", "-",  2, 2039,      "cpw-pos2-2"},
  {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R", "w", "KQkq", "-",  3, 97862,     "cpw-pos2-3"},
  {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8",                         "w", "-",    "-",  5, 674624,    "cpw-pos3-5"},
  {"n1n5/PPPk4/8/8/8/8/4Kppp/5N1N",                           "b", "-",    "-",  1, 24,        "prom-1"},
  {"8/5bk1/8/2Pp4/8/1K6/8/8",                                 "w", "-",    "d6", 6, 824064,    "ccc-1"},
  {"8/8/1k6/8/2pP4/8/5BK1/8",                                 "b", "-",    "d3", 6, 824064,    "ccc-2"},
  {"8/8/1k6/2b5/2pP4/8/5K2/8",                                "b", "-",    "d3", 6, 1440467,   "ccc-3"},
  {"8/5k2/8/2Pp4/2B5/1K6/8/8",                                "w", "-",    "d6", 6, 1440467,   "ccc-4"},
  {"5k2/8/8/8/8/8/8/4K2R",                                    "w", "K",    "-",  6, 661072,    "ccc-5"},
  {"4k2r/8/8/8/8/8/8/5K2",                                    "b", "k",    "-",  6, 661072,    "ccc-6"},
  {"3k4/8/8/8/8/8/8/R3K3",                                    "w", "Q",    "-",  6, 803711,    "ccc-7"},
  {"r3k3/8/8/8/8/8/8/3K4",                                    "b", "q",    "-",  6, 803711,    "ccc-8"},
  {"r3k2r/1b4bq/8/8/8/8/7B/R3K2R",                            "w", "KQkq", "-",  4, 1274206,   "ccc-9"},
  {"r3k2r/7b/8/8/8/8/1B4BQ/R3K2R",                            "b", "KQkq", "-",  4, 1274206,   "ccc-10"},
  {"r3k2r/8/3Q4/8/8/5q2/8/R3K2R",                             "b", "KQkq", "-",  4, 1720476,   "ccc-11"},
  {"r3k2r/8/5Q2/8/8/3q4/8/R3K2R",                             "w", "KQkq", "-",  4, 1720476,   "ccc-12"},
  {"2K2r2/4P3/8/8/8/8/8/3k4",                                 "w", "-",    "-",  6, 3821001,   "ccc-13"},
  {"3K4/8/8/8/8/8/4p3/2k2R2",                                 "b", "-",    "-",  6, 3821001,   "ccc-14"},
  {"8/8/1P2K3/8/2n5/1q6/8/5k2",                               "b", "-",    "-",  5, 1004658,   "ccc-15"},
  {"5K2/8/1Q6/2N5/8/1p2k3/8/8",                               "w", "-",    "-",  5, 1004658,   "ccc-16"},
  {"4k3/1P6/8/8/8/8/K7/8",                                    "w", "-",    "-",  6, 217342,    "ccc-17"},
  {"8/k7/8/8/8/8/1p6/4K3",                                    "b", "-",    "-",  6, 217342,    "ccc-18"},
  {"8/P1k5/K7/8/8/8/8/8",                                     "w", "-",    "-",  6, 92683,     "ccc-19"},
  {"8/8/8/8/8/k7/p1K5/8",                                     "b", "-",    "-",  6, 92683,     "ccc-20"},
  {"K1k5/8/P7/8/8/8/8/8",                                     "w", "-",    "-",  6, 2217,      "ccc-21"},
  {"8/8/8/8/8/p7/8/k1K5",                                     "b", "-",    "-",  6, 2217,      "ccc-22"},
  {"8/k1P5/8/1K6/8/8/8/8",                                    "w", "-",    "-",  7, 567584,    "ccc-23"},
  {"8/8/8/8/1k6/8/K1p5/8",                                    "b", "-",    "-",  7, 567584,    "ccc-24"},
  {"8/8/2k5/5q2/5n2/8/5K2/8",                                 "b", "-",    "-",  4, 23527,     "ccc-25"},
  {"8/5k2/8/5N2/5Q2/2K5/8/8",                                 "w", "-",    "-",  4, 23527,     "ccc-26"},
  {"8/p7/8/1P6/K1k3p1/6P1/7P/8",                              "w", "-",    "-",  8, 8103790,   "jvm-7"},
  {"n1n5/PPPk4/8/8/8/8/4Kppp/5N1N",                           "b", "-",    "-",  6, 71179139,  "jvm-8"},
  {"r3k2r/p6p/8/B7/1pp1p3/3b4/P6P/R3K2R",                     "w", "KQkq", "-",  6, 77054993,  "jvm-9"},
  {"8/5p2/8/2k3P1/p3K3/8/1P6/8",                              "b", "-",    "-",  8, 64451405,  "jvm-11"},
  {"r3k2r/pb3p2/5npp/n2p4/1p1PPB2/6P1/P2N1PBP/R3K2R",         "w", "KQkq", "-",  5, 29179893,  "jvm-12"},
  {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8",                         "w", "-",    "-",  7, 178633661, "jvm-10"},
  {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R", "w", "KQkq", "-",  5, 193690690, "jvm-6"},
  {"8/2pkp3/8/RP3P1Q/6B1/8/2PPP3/rb1K1n1r",                   "w", "-",    "-",  6, 181153194, "ob1"},
  {"rnbqkb1r/ppppp1pp/7n/4Pp2/8/8/PPPP1PPP/RNBQKBNR",         "w", "KQkq", "f6", 6, 244063299, "jvm-5"},
  {"8/2ppp3/8/RP1k1P1Q/8/8/2PPP3/rb1K1n1r",                   "w", "-",    "-",  6, 205552081, "ob2"},
  {"8/8/3q4/4r3/1b3n2/8/3PPP2/2k1K2R",                        "w", "K",    "-",  6, 207139531, "ob3"},
  {"4r2r/RP1kP1P1/3P1P2/8/8/3ppp2/1p4p1/4K2R",                "b", "K",    "-",  6, 314516438, "ob4"},
  {"r3k2r/8/8/8/3pPp2/8/8/R3K1RR",                            "b", "KQkq", "e3", 6, 485647607, "jvm-1"},
  {"8/3K4/2p5/p2b2r1/5k2/8/8/1q6",                            "b", "-",    "-",  7, 493407574, "jvm-4"},
  {"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1",   "w", "kq",   "-",  6, 706045033, "jvm-2"},
  {"r6r/1P4P1/2kPPP2/8/8/3ppp2/1p4p1/R3K2R",                  "w", "KQ",   "-",  6, 975944981, "ob5"}

};

/*}}}*/

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

    tot_nodes += perft(ply+1, depth-1);

  }

  return tot_nodes;

}

/*}}}*/
/*{{{  perft_tests*/

void perft_tests () {

  const int num_tests = 65;
  uint64_t start_ms = now_ms();
  uint64_t total_nodes = 0;
  int errors = 0;

  for (int i=0; i < num_tests; i++) {

    const Perft *p = &perft_data[i];

    position(&ss[0], p->fen, p->stm, p->rights, p->ep);

    uint64_t num_nodes = perft(0, p->depth);
    total_nodes += num_nodes;

    if (num_nodes != p->expected)
      errors += 1;

    printf("%s %s %d (%" PRIu64 ") %" PRIu64 " %" PRIu64 "\n",
    p->label,
    p->fen,
    p->depth,
    num_nodes - p->expected,
    num_nodes,
    p->expected);

  }

  uint64_t end_ms     = now_ms();
  uint64_t elapsed_ms = end_ms - start_ms;
  uint64_t nps        = (total_nodes * 1000ULL) / (elapsed_ms ? elapsed_ms : 1);

  printf("time %" PRIu64 " nps %" PRIu64 "\n", elapsed_ms, nps);
  printf("errors %d\n", errors);

}

/*}}}*/

